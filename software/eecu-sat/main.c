/*
 * This file is part of the eecu-sat project.
 *
 * Copyright (C) 2024 Petre Rodan <2b4eda@subdimension.ro>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// program that converts a number of raw analog capture files generated by saleae's logic into a srzip file recognized by sigrok

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <dirent.h>
#include <fnmatch.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "tlpi_hdr.h"
#include "proj.h"
#include "version.h"
#include "output.h"
#include "output_analog.h"
#include "output_srzip.h"
#include "transform.h"
#include "calib.h"
#include "parsers.h"
#include "trigger.h"
#include "session.h"

// program arguments
static char *opt_default_input_prefix = "analog_[0-9]*.bin";
struct cmdline_opt opt = { 0 };

static void show_usage(void)
{
    fprintf(stdout, "Usage: eecu-sat [-i PREFIX] [-o FILE]\n");
    fprintf(stdout, "\t-i, --input\n");
    fprintf(stdout, "\t\ta prefix that defines the input raw analog files. default is %s\n", opt_default_input_prefix);
    fprintf(stdout, "\t-o, --output=FILE\n");
    fprintf(stdout, "\t\toutput file to be generated\n");
    fprintf(stdout, "\t-O, --output-format OUTPUT\n");
    fprintf(stdout, "\t\toutput data format to use, see -L for list\n");
    fprintf(stdout, "\t-t, --triggers TRIGGERS\n");
    fprintf(stdout, "\t\ttrigger configuration\n");
    fprintf(stdout, "\t-T, --transform-module TRANSFORM\n");
    fprintf(stdout, "\t\tprocess data via a function, see -L for list\n");
    fprintf(stdout, "\t-L, --list\n");
    fprintf(stdout, "\t\tlist known output formats and transform modules\n");
    fprintf(stdout, "\t-h, --help\n");
    fprintf(stdout, "\t-v, --version\n");
}

static void show_version(void)
{
    fprintf(stdout, "eecu-sat version: %d.%d build %d commit %d\n", VER_MAJOR, VER_MINOR, BUILD, COMMIT);
}

static void show_capabilities(void)
{
    const struct sr_output_module **outputs;
    const struct sr_transform_module **transforms;
    int i;

    printf("Supported output formats:\n");
    outputs = sat_output_list();
    for (i = 0; outputs[i]; i++) {
        printf("  %-20s %s\n", sat_output_id_get(outputs[i]), sat_output_description_get(outputs[i]));
    }

    printf("Supported transform modules:\n");
    transforms = sat_transform_list();
    for (i = 0; transforms[i]; i++) {
        printf("  %-20s %s\n", sat_transform_id_get(transforms[i]), sat_transform_description_get(transforms[i]));
    }

}

static int parse_options(int argc, char **argv)
{
    int q, opt_idx;

    while (1) {
        opt_idx = 0;
        static struct option long_options[] = {
            {"input", 1, 0, 'i'},
            {"output", 1, 0, 'o'},
            {"output-format", 1, 0, 'O'},
            {"triggers", 1, 0, 't'},
            {"transform-module", 1, 0, 'T'},
            {"list", 0, 0, 'L'},
            {"help", 0, 0, 'h'},
            {"version", 0, 0, 'v'},
            {0, 0, 0, 0}
        };

        q = getopt_long(argc, argv, "i:o:O:t:T:Lhv", long_options, &opt_idx);
        if (q == -1) {
            break;
        }
        switch (q) {
        case 'i':
            opt.input_prefix = optarg;
            break;
        case 'o':
            opt.output_file = optarg;
            break;
        case 'O':
            opt.output_format = optarg;
            break;
        case 't':
            opt.triggers = optarg;
            break;
        case 'T':
            opt.transform_module = optarg;
            break;
        case 'L':
            show_capabilities();
            break;
        case 'h':
            show_usage();
            break;
        case 'v':
            show_version();
            break;
        default:
            break;
        }
    }

    if (!opt.input_prefix) {
        opt.input_prefix = opt_default_input_prefix;
    }

    return SR_OK;
}

int main(int argc, char **argv)
{
    int res;
    struct dirent **namelist;
    int i, ch_cnt;
    int channel_total = 0;
    char *_input_dirname;
    char *input_dirname;
    char *_input_basename;
    char *input_basename;
    ssize_t file_name_len;
    FILE *fp;
    ch_data_t *ch_data_ptr;
    ssize_t file_size_compare = -1;
    int ret = SR_OK;
    struct sr_dev_inst sdi = { 0 };
    struct dev_frame frame = { 0 };
    GSList *l, *channels = NULL;
    uint8_t buff[8];

    if (parse_options(argc, argv)) {
        return SR_ERR_ARG;
    }

    _input_dirname = strdup(opt.input_prefix);
    _input_basename = strdup(opt.input_prefix);

    input_dirname = dirname(_input_dirname);
    input_basename = basename(_input_basename);

    //printf("input files prefix is %s, dirname %s, basename %s\n", opt_input_prefix, input_dirname, input_basename);
    ch_cnt = scandir(input_dirname, &namelist, NULL, versionsort);

    sdi.priv = &frame;

    // create a linked list with all files that match the filter defined by the --input option
    if (ch_cnt < 0)
        perror("scandir");
    else {
        for (i = 0; i < ch_cnt; i++) {
            res = fnmatch(input_basename, namelist[i]->d_name, FNM_EXTMATCH);
            if (!res) {
                //printf("%s matches\n", namelist[i]->d_name);
                channel_total++;
                ch_data_ptr = calloc(1, sizeof(struct ch_data));
                if (!ch_data_ptr) {
                    errMsg("during calloc");
                    ret = SR_ERR_MALLOC;
                    goto cleanup;
                }

                file_name_len = strlen(input_dirname) + strlen(namelist[i]->d_name);
                ch_data_ptr->input_file_name = (char *)calloc(file_name_len + 3, sizeof(char));
                if (!ch_data_ptr->input_file_name) {
                    errMsg("during calloc");
                    ret = SR_ERR_MALLOC;
                    goto cleanup;
                }
                snprintf(ch_data_ptr->input_file_name, file_name_len + 2, "%s/%s", input_dirname, namelist[i]->d_name);
                ch_data_ptr->id = channel_total;
                ch_data_ptr->sample_size = sizeof(float);

                // get file size
                if ((fp = fopen(ch_data_ptr->input_file_name, "r")) == NULL) {
                    errMsg("opening input file");
                    ret = SR_ERR_ARG;
                    goto cleanup;
                }
                fseek(fp, 0L, SEEK_END);
                ch_data_ptr->input_file_size = ftell(fp);

                fseek(fp, 0L, SEEK_SET);
                if (fread(&buff, 1, 8, fp) != 8) {
                    errMsg("during read()");
                    fclose(fp);
                    ret = SR_ERR_IO;
                    goto cleanup;
                }

                if (saleae_magic_is_present(&buff[0])) {
                    ch_data_ptr->header_present = true;
                    ch_data_ptr->sample_count = (ch_data_ptr->input_file_size - SALEAE_HEADER_SZ) / ch_data_ptr->sample_size;
                } else {
                    ch_data_ptr->sample_count = ch_data_ptr->input_file_size / ch_data_ptr->sample_size;
                }
 
                fclose(fp);

                if (file_size_compare == -1)
                    file_size_compare = ch_data_ptr->input_file_size;
                if (file_size_compare != ch_data_ptr->input_file_size) {
                    fprintf(stderr, "error: input files do not have the exact same size\n");
                    fprintf(stderr, " %s has %ld bytes, but %ld bytes were expected\n", ch_data_ptr->input_file_name,
                            ch_data_ptr->input_file_size, file_size_compare);
                    ret = SR_ERR_ARG;
                    goto cleanup;
                }
                sdi.channels = g_slist_append(sdi.channels, ch_data_ptr);
            }
        }
    }

    if (!channel_total) {
        fprintf(stderr, "error: no valid input channels found\n");
        show_usage();
        return SR_ERR_ARG;
    }

    ret = run_session(&sdi, &opt);

#ifdef CONFIG_DEBUG
    channels = sr_dev_inst_channels_get(&sdi);
    for (l = channels; l; l = l->next) {
        ch_data_ptr = l->data;
        printf("file id %d in list %s\n", ch_data_ptr->id, ch_data_ptr->input_file_name);
    }
#endif

 cleanup:
    for (i = 0; i < ch_cnt; i++) {
        free(namelist[i]);
    }
    free(namelist);
    if (channels) {
        for (l = channels; l; l = l->next) {
            ch_data_ptr = l->data;
            if (ch_data_ptr->input_file_name)
                free(ch_data_ptr->input_file_name);
            if (ch_data_ptr->trigger)
                sat_trigger_free(ch_data_ptr->trigger);
            free(l->data);
        }
        g_slist_free(channels);
    }
    free(_input_dirname);
    free(_input_basename);

    return ret;
}

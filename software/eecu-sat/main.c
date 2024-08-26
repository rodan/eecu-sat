
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
#include "list.h"
#include "output.h"
#include "output_analog.h"
#include "output_srzip.h"
#include "transform.h"
#include "calib.h"
#include "parsers.h"

struct ch_data {
    struct ch_data *next;
    char *input_file_name;
    char *output_file_name;
    //char *channel_name;
    uint16_t id;
    ssize_t input_file_size;
};
typedef struct ch_data ch_data_t;

// program arguments
static char *opt_default_input_prefix = "analog_[0-9]*.bin";
static char *opt_input_prefix = NULL;
static char *opt_output_file = NULL;
static char *opt_output_format = NULL;
static char *opt_transform_module = NULL;
static char *opt_calibration_file = NULL;
static char *opt_metadata_file = NULL;
static bool opt_skip_header = false;
static uint32_t opt_action = ACTION_DO_CONVERT;
LIST(channels);

static void show_usage(void)
{
    fprintf(stdout, "Usage: eecu-sat [-i PREFIX] [-o FILE]\n");
    fprintf(stdout, "\t-i, --input\n");
    fprintf(stdout, "\t\ta prefix that defines the input raw analog files. default is %s\n", opt_default_input_prefix);
    fprintf(stdout, "\t-o, --output=FILE\n");
    fprintf(stdout, "\t\toutput file to be generated\n");
    fprintf(stdout, "\t-O, --output-format OUTPUT\n");
    fprintf(stdout, "\t\toutput data format to use, see -L for list\n");
    fprintf(stdout, "\t-s, --skip\n");
    fprintf(stdout, "\t\tskips the first 0x30 bytes (saleae header)\n");
    fprintf(stdout, "\t-T, --transform-module TRANSFORM\n");
    fprintf(stdout, "\t\tprocess data via a function, see -L for list\n");
    fprintf(stdout, "\t-c, --calibration\n");
    fprintf(stdout, "\t\tget per-channel slopes and offsets to be used for 3 point calibration\n");
    fprintf(stdout, "\t--initcal\n");
    fprintf(stdout, "\t-m, --metadata\n");
    fprintf(stdout, "\t\tuse custom metadata file\n");
    fprintf(stdout, "\t-l, --list\n");
    fprintf(stdout, "\t\tlist known output formats\n");
    fprintf(stdout, "\t-h, --help\n");
    fprintf(stdout, "\t-v, --version\n");
}

static void show_version(void)
{
    fprintf(stdout, "eecu-sat version: %d.%d build %d commit %d\n", VER_MAJOR, VER_MINOR, BUILD, COMMIT);
}

static void show_capabilities(void)
{
    const struct sat_output_module **outputs;
    const struct sat_transform_module **transforms;
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
            {"transform-module", 1, 0, 'T'},
            {"metadata", 1, 0, 'm'},
            {"skip", 0, 0, 's'},
            {"calibration", 1, 0, 'c'},
            {"initcal", 0, 0, 'x'},
            {"list", 0, 0, 'L'},
            {"help", 0, 0, 'h'},
            {"version", 0, 0, 'v'},
            {0, 0, 0, 0}
        };

        q = getopt_long(argc, argv, "i:o:O:c:T:xhLv", long_options, &opt_idx);
        if (q == -1) {
            break;
        }
        switch (q) {
        case 'i':
            opt_input_prefix = optarg;
            break;
        case 'o':
            opt_output_file = optarg;
            break;
        case 'O':
            opt_output_format = optarg;
            break;
        case 'T':
            opt_transform_module = optarg;
            break;
        case 'm':
            opt_metadata_file = optarg;
            break;
        case 'c':
            opt_calibration_file = optarg;
            opt_action |= ACTION_DO_CALIBRATION;
            opt_action &= ~ACTION_DO_CONVERT;
            break;
        case 'x':
            opt_action |= ACTION_DO_CALIB_INIT;
            opt_action &= ~ACTION_DO_CONVERT;
            break;
        case 's':
            opt_skip_header = true;
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

    if (!opt_input_prefix) {
        opt_input_prefix = opt_default_input_prefix;
    }

    return EXIT_SUCCESS;
}

#ifdef CONFIG_DEBUG
void ll_print(ch_data_t *head)
{
    ch_data_t *p = head;

    if (head == NULL) {
        printf("ll is empty\n");
        return;
    }

    while (NULL != p) {
        printf("n %p, [%s] sz %ld  next %p\n", (void *)p, p->input_file_name, p->input_file_size, (void *)p->next);
        if (p->next != NULL) {
            p = p->next;
        } else {
            return;
        }
    }
}
#endif

static void ll_free_all(ch_data_t **head)
{
    if (*head == NULL)
        return;
    ch_data_t *p = *head;
    ch_data_t *del;

    while (NULL != p) {
        //printf("remove node @%p\n", (void *)p);
        del = p;
        if (del->input_file_name)
            free(del->input_file_name);
        if (del->output_file_name)
            free(del->output_file_name);
        p = p->next;
        free(del);
    }

    *head = NULL;
}

static int do_calibration()
{
    int ret = EXIT_SUCCESS;
    calib_context_t *ctx = NULL;
    calib_globals_t globals;
    ch_data_t *ch_data_ptr;

    if ((calib_read_params_from_file(opt_calibration_file, &globals, CALIB_INI_GLOBALS)) != EXIT_SUCCESS) {
        fprintf(stderr, "error during calib_read_params_from_file()\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    ch_data_ptr = list_head(channels);

    while (NULL != ch_data_ptr) {
        ctx = (calib_context_t *) calloc(1, sizeof(struct calib_context));
        if (!ctx) {
            errMsg("during calloc");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        ctx->globals = &globals;
        ctx->channel_data.id = ch_data_ptr->id;

        if ((calib_read_params_from_file(opt_calibration_file, &ctx->channel_data, CALIB_INI_CHANNEL)) != EXIT_SUCCESS) {
            fprintf(stderr, "error during calib_read_params_from_file()\n");
            ret = EXIT_FAILURE;
            goto cleanup;
        }

        ctx->channel_data.calibration_type = CALIB_TYPE_3_POINT;
        //if ((calib_init_from_data_file(ch_data_ptr->file_name, opt_calibration_file, ctx)) != EXIT_SUCCESS) {
        //    fprintf(stderr, "error during calib_init_from_data_file()\n");
        //}
        printf("channel %d %s %s\n", ch_data_ptr->id, ch_data_ptr->input_file_name, ch_data_ptr->output_file_name);

        free(ctx);

        if (ch_data_ptr->next != NULL) {
            ch_data_ptr = ch_data_ptr->next;
        } else {
            break;
        }
    }

 cleanup:

    return ret;
}

static int do_calibration_init()
{
    int ret = EXIT_SUCCESS;
    calib_context_t *ctx = NULL;
    calib_globals_t globals;
    ch_data_t *ch_data_ptr;

    if ((calib_read_params_from_file(opt_calibration_file, &globals, CALIB_INI_GLOBALS)) != EXIT_SUCCESS) {
        fprintf(stderr, "error during calib_read_params_from_file()\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    ch_data_ptr = list_head(channels);

    while (NULL != ch_data_ptr) {
        ctx = (calib_context_t *) calloc(1, sizeof(struct calib_context));
        if (!ctx) {
            errMsg("during calloc");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        ctx->globals = &globals;
        ctx->channel_data.id = ch_data_ptr->id;
        ctx->channel_data.calibration_type = CALIB_TYPE_3_POINT;
        if ((calib_init_from_data_file(ch_data_ptr->input_file_name, opt_calibration_file, ctx)) != EXIT_SUCCESS) {
            fprintf(stderr, "error during calib_init_from_data_file()\n");
        }

        free(ctx);

        if (ch_data_ptr->next != NULL) {
            ch_data_ptr = ch_data_ptr->next;
        } else {
            break;
        }
    }

 cleanup:

    return ret;
}

static int do_conversion()
{
    int ret = EXIT_SUCCESS;
    struct sat_output *o = NULL;
    ch_data_t *ch_data_ptr;
    ssize_t read_len;
    int i, n;
    int fd;
    struct sr_datafeed_packet pkt = { 0 };
    struct sr_datafeed_analog analog = { 0 };
    struct sr_analog_encoding encoding = { 0 };
	struct sr_analog_meaning meaning = { 0 };
	struct sr_analog_spec spec = { 0 };
    struct sat_generic_pkt gpkt = { 0 };

    analog.encoding = &encoding;
    analog.meaning = &meaning;
    analog.spec = &spec;

    if (!opt_output_format) {
        fprintf(stderr, "output format not selected\n");
        return EXIT_FAILURE;
    }

    o = (struct sat_output *)calloc(1, sizeof(struct sat_output));
    if (!o) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    analog.data = (uint8_t *) calloc(CHUNK_SIZE, 1);
    if (!analog.data) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    gpkt.payload = (uint8_t *) calloc(CHUNK_SIZE, 1);
    if (!gpkt.payload) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    o->module = sat_output_find(opt_output_format);
    if (o->module == NULL) {
        fprintf(stderr, "invalid output module '%s'\n", opt_output_format);
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    o->filename = opt_output_file;
    if (o->module->init(o) == EXIT_FAILURE) {
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    ch_data_ptr = list_head(channels);
    i = 0;
    pkt.payload = &analog;

    while (NULL != ch_data_ptr) {
        i++;

        if ((fd = open(ch_data_ptr->input_file_name, O_RDONLY)) < 0) {
            errMsg("opening input file");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        if (opt_skip_header) {
            if (lseek(fd, 0x30, SEEK_SET) < 0) {
                errMsg("opening metadata file");
                close(fd);
                ret = EXIT_FAILURE;
                goto cleanup;
            }
        }

        n = 1;
        while ((read_len = read(fd, analog.data, CHUNK_SIZE)) > 0) {
            o->ch = i;
            o->chunk = n;
            pkt.type = SR_DF_ANALOG;
            analog.num_samples = read_len / sizeof(float);
            o->module->receive(o, &pkt);
            n++;
        }
        close(fd);

        if (ch_data_ptr->next != NULL) {
            ch_data_ptr = ch_data_ptr->next;
        } else {
            break;
        }
    }

    pkt.payload = &gpkt;
    if (opt_metadata_file) {
        if ((fd = open(opt_metadata_file, O_RDONLY)) < 0) {
            errMsg("opening metadata file");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        while ((read_len = read(fd, gpkt.payload, CHUNK_SIZE)) > 0) {
            pkt.type = SR_DF_META;
            gpkt.payload_sz = read_len;
            o->module->receive(o, &pkt);
        }
        close(fd);
    }

    printf("%d channels exported\n", i);

 cleanup:
    if (o) {
        if (o->module)
            o->module->cleanup(o);
        free(o);
    }
    if (analog.data)
        free(analog.data);
    if (gpkt.payload)
        free(gpkt.payload);

    return ret;
}

const struct sat_transform *setup_transform_module(void)
{
    const struct sat_transform_module *tmod;
    const struct sr_option **options;
    const struct sat_transform *t;
    GHashTable *fmtargs, *fmtopts;
    char *fmtspec;

    fmtargs = parse_generic_arg(opt_transform_module, TRUE, NULL);
    fmtspec = g_hash_table_lookup(fmtargs, "sigrok_key");
    if (!fmtspec) {
        fprintf(stderr, "Invalid transform module.\n");
        exit(EXIT_FAILURE);
    }
    printf("fmtspec %s\n", fmtspec);
    if (!(tmod = sat_transform_find(fmtspec))) {
        fprintf(stderr, "Unknown transform module '%s'.\n", fmtspec);
        exit(EXIT_FAILURE);
    }
    g_hash_table_remove(fmtargs, "sigrok_key");
    if ((options = sat_transform_options_get(tmod))) {
        fmtopts = generic_arg_to_opt(options, fmtargs);
        //(void)warn_unknown_keys(options, fmtargs, NULL);
        sat_transform_options_free(options);
    } else {
        fmtopts = NULL;
    }
    t = sat_transform_new(tmod, fmtopts);

    printf("transform set to %s\n", t->module->name);

    if (fmtopts)
        g_hash_table_destroy(fmtopts);
    g_hash_table_destroy(fmtargs);

    return t;
}

int main(int argc, char **argv)
{
    int res;
    struct dirent **namelist;
    int i, n;
    int channel_total = 0;
    char *_input_dirname;
    char *input_dirname;
    char *_input_basename;
    char *input_basename;
    ssize_t file_name_len;
    FILE *fp;
    ch_data_t *ch_data_ptr;
    ssize_t file_size_compare = -1;
    int ret = EXIT_SUCCESS;
    const struct sat_transform *t;

    if (parse_options(argc, argv)) {
        return EXIT_FAILURE;
    }

    _input_dirname = strdup(opt_input_prefix);
    _input_basename = strdup(opt_input_prefix);

    input_dirname = dirname(_input_dirname);
    input_basename = basename(_input_basename);

    //printf("input files prefix is %s, dirname %s, basename %s\n", opt_input_prefix, input_dirname, input_basename);
    n = scandir(input_dirname, &namelist, NULL, versionsort);
    list_init(channels);

    // create a linked list with all files that match the filter defined by the --input option
    if (n < 0)
        perror("scandir");
    else {
        for (i = 0; i < n; i++) {
            res = fnmatch(input_basename, namelist[i]->d_name, FNM_EXTMATCH);
            if (!res) {
                //printf("%s matches\n", namelist[i]->d_name);
                channel_total++;
                ch_data_ptr = calloc(1, sizeof(struct ch_data));
                if (!ch_data_ptr) {
                    errMsg("during calloc");
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }

                file_name_len = strlen(input_dirname) + strlen(namelist[i]->d_name);
                ch_data_ptr->input_file_name = (char *)calloc(file_name_len + 3, sizeof(char));
                if (!ch_data_ptr->input_file_name) {
                    errMsg("during calloc");
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }
                snprintf(ch_data_ptr->input_file_name, file_name_len + 2, "%s/%s", input_dirname, namelist[i]->d_name);

                if (opt_output_file && (opt_action & ACTION_DO_CALIBRATION)) {
                    file_name_len = strlen(opt_output_file) + strlen(namelist[i]->d_name);
                    ch_data_ptr->output_file_name = (char *)calloc(file_name_len + 3, sizeof(char));
                    if (!ch_data_ptr->output_file_name) {
                        errMsg("during calloc");
                        ret = EXIT_FAILURE;
                        goto cleanup;
                    }
                    snprintf(ch_data_ptr->output_file_name, file_name_len + 2, "%s/%s", opt_output_file,
                             namelist[i]->d_name);
                }

                ch_data_ptr->id = channel_total;

                // get file size
                if ((fp = fopen(ch_data_ptr->input_file_name, "r")) == NULL) {
                    errMsg("opening input file");
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }
                fseek(fp, 0L, SEEK_END);
                ch_data_ptr->input_file_size = ftell(fp);
                fclose(fp);
                if (file_size_compare == -1)
                    file_size_compare = ch_data_ptr->input_file_size;
                if (file_size_compare != ch_data_ptr->input_file_size) {
                    fprintf(stderr, "error: input files do not have the exact same size\n");
                    fprintf(stderr, " %s has %ld bytes, but %ld bytes were expected\n", ch_data_ptr->input_file_name,
                            ch_data_ptr->input_file_size, file_size_compare);
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }
                list_add(channels, ch_data_ptr);
            }
            free(namelist[n]);
        }
        free(namelist);
    }

    if (opt_transform_module) {
        if (!(t = setup_transform_module())) {
            fprintf(stderr, "Failed to initialize transform module.\n");
            return EXIT_FAILURE;
        }
    }

    if (!channel_total) {
        fprintf(stderr, "error: no valid input channels found\n");
        show_usage();
        return EXIT_FAILURE;
    }

    if (opt_action & ACTION_DO_CONVERT) {
        ret = do_conversion();
    } else if (opt_action & ACTION_DO_CALIB_INIT) {
        ret = do_calibration_init();
    } else if (opt_action & ACTION_DO_CALIBRATION) {
        ret = do_calibration();
    }
#ifdef CONFIG_DEBUG
    //ll_print(list_head(channels));
#endif

 cleanup:
    ch_data_ptr = list_head(channels);
    if (ch_data_ptr)
        ll_free_all(&ch_data_ptr);
    free(_input_dirname);
    free(_input_basename);

    return ret;
}

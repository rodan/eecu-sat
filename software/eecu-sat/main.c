
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

#include "tlpi_hdr.h"
#include "proj.h"
#include "version.h"
#include "list.h"
#include "output_srzip.h"
#include "calib.h"
#include "config.h"

struct ch_data {
    struct ch_data *next;
    char *file_name;
    char *channel_name;
    ssize_t file_size;
};
typedef struct ch_data ch_data_t;

// program arguments
static char *opt_default_input_prefix = "analog_[0-9]*.bin";
static char *opt_input_prefix = NULL;
static char *opt_output_file = NULL;
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
    fprintf(stdout, "\t\tsrzip output file to be generated\n");
    fprintf(stdout, "\t-s, --skip\n");
    fprintf(stdout, "\t\tskips the first 0x30 bytes (saleae header)\n");
    fprintf(stdout, "\t-c, --calibration\n");
    fprintf(stdout, "\t\tget per-channel slopes and offsets to be used for 3 point calibration\n");
    fprintf(stdout, "\t-m, --metadata\n");
    fprintf(stdout, "\t\tuse custom metadata file\n");
    fprintf(stdout, "\t-h, --help\n");
    fprintf(stdout, "\t-v, --version\n");
}

static void show_version(void)
{
    fprintf(stdout, "eecu-sat version: %d.%d build %d commit %d\n", VER_MAJOR, VER_MINOR, BUILD, COMMIT);
}

static int parse_options(int argc, char **argv)
{
    int q, opt_idx;

    while (1) {
        opt_idx = 0;
        static struct option long_options[] = {
            {"input", 1, 0, 'i'},
            {"output", 1, 0, 'o'},
            {"metadata", 1, 0, 'm'},
            {"skip", 0, 0, 's'},
            {"calibration", 1, 0, 'c'},
            {"help", 0, 0, 'h'},
            {"version", 0, 0, 'v'},
            {0, 0, 0, 0}
        };

        q = getopt_long(argc, argv, "i:o:c:hv", long_options, &opt_idx);
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
        case 'm':
            opt_metadata_file = optarg;
            break;
        case 'c':
            opt_calibration_file = optarg;
            opt_action = ACTION_DO_CALIB_INIT;
            break;
        case 's':
            opt_skip_header = true;
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
static void ll_print(ch_data_t *head)
{
    ch_data_t *p = head;

    if (head == NULL) {
        printf("ll is empty\n");
        return;
    }

    while (NULL != p) {
        printf("n %p, [%s] sz %ld  next %p\n", (void *)p, p->file_name, p->file_size, (void *)p->next);
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
        free(del->file_name);
        p = p->next;
        free(del);
    }

    *head = NULL;
}

static int do_calibration_init()
{
    int ret = EXIT_SUCCESS;
    calib_context_t *calib = NULL;
    ch_data_t *ch_data_ptr;

    calib = (calib_context_t *) calloc(1, sizeof(struct calib_context));
    if (!calib) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    if ((calib_read_params_from_file(opt_calibration_file, calib)) != EXIT_SUCCESS) {
        fprintf(stderr, "error during calibration extract procedure\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    ch_data_ptr = list_head(channels);

    while (NULL != ch_data_ptr) {

        if (ch_data_ptr->next != NULL) {
            ch_data_ptr = ch_data_ptr->next;
        } else {
            break;
        }
    }

cleanup:
    if (calib)
        free(calib);

    return ret;
}

static int do_conversion()
{
    int ret = EXIT_SUCCESS;
    srzip_context_t *srzip = NULL;
    ch_data_t *ch_data_ptr;
    ssize_t read_len;
    int i,n;
    int fd;

    srzip = (srzip_context_t *) calloc(1, sizeof(struct srzip_context));
    if (!srzip) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    srzip->archive_file_name = opt_output_file;
    if (output_srzip_init(srzip) == EXIT_FAILURE) {
        goto cleanup;
    }

    ch_data_ptr = list_head(channels);
    i = 0;

    while (NULL != ch_data_ptr) {
        i++;

        // skip saleae header
        sprintf(srzip->target_file_name, ch_data_ptr->file_name);
        if ((fd = open(ch_data_ptr->file_name, O_RDONLY)) < 0) {
            errMsg("opening input file");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        if (opt_skip_header) {
            if (lseek(fd, 0x30, SEEK_SET) < 0) {
                errMsg("opening metadata file");
                ret = EXIT_FAILURE;
                goto cleanup;
            }
        }

        n = 1;
        while ((read_len = read(fd, srzip->buffer, CHUNK_SIZE)) > 0) {
            snprintf(srzip->target_file_name, PATH_MAX - 1, "analog-1-%d-%d", i, n);
            srzip->buffer_len = read_len;
            printf("srzip\n");
            //output_srzip_append(srzip);
            n++;
        }
        close(fd);

        if (ch_data_ptr->next != NULL) {
            ch_data_ptr = ch_data_ptr->next;
        } else {
            break;
        }
    }

    if (opt_metadata_file) {
        if ((fd = open(opt_metadata_file, O_RDONLY)) < 0) {
            errMsg("opening metadata file");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        while ((read_len = read(fd, srzip->buffer, CHUNK_SIZE)) > 0) {
            snprintf(srzip->target_file_name, PATH_MAX - 1, "metadata");
            srzip->buffer_len = read_len;
            output_srzip_append(srzip);
        }
        close(fd);
    }

    printf("%d channels exported\n", i);

cleanup:
    if (srzip)
        output_srzip_free(&srzip);

    return ret;
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
                ch_data_ptr->file_name = (char *)calloc(file_name_len + 3, sizeof(char));
                if (!ch_data_ptr->file_name) {
                    errMsg("during calloc");
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }
                snprintf(ch_data_ptr->file_name, file_name_len + 2, "%s/%s", input_dirname, namelist[i]->d_name);

                // get file size
                if ((fp = fopen(ch_data_ptr->file_name, "r")) == NULL) {
                    errMsg("opening input file");
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }
                fseek(fp, 0L, SEEK_END);
                ch_data_ptr->file_size = ftell(fp);
                fclose(fp);
                if (file_size_compare == -1)
                    file_size_compare = ch_data_ptr->file_size;
                if (file_size_compare != ch_data_ptr->file_size) {
                    fprintf(stderr, "error: input files do not have the exact same size\n");
                    fprintf(stderr, " %s has %ld bytes, but %ld bytes were expected\n", ch_data_ptr->file_name,
                            ch_data_ptr->file_size, file_size_compare);
                    ret = EXIT_FAILURE;
                    goto cleanup;
                }
                list_add(channels, ch_data_ptr);
            }
            free(namelist[n]);
        }
        free(namelist);
    }

    if (!channel_total) {
        fprintf(stderr, "error: no valid input channels found\n");
        show_usage();
        return EXIT_FAILURE;
    }

    switch (opt_action) {
        case ACTION_DO_CONVERT:
            ret = do_conversion();
            break;
        case ACTION_DO_CALIB_INIT:
            ret = do_calibration_init();
            break;
        default:
            break;
    }

#ifdef CONFIG_DEBUG
    ll_print(list_head(channels));
#endif

 cleanup:
    ch_data_ptr = list_head(channels);
    if (ch_data_ptr)
        ll_free_all(&ch_data_ptr);
    free(_input_dirname);
    free(_input_basename);

    return ret;
}

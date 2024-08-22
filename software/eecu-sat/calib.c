
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include "proj.h"
#include "tlpi_hdr.h"
#include "ini.h"
#include "calib.h"

static int calib_inih_handler(void* data, const char* section, const char* name,
                   const char* value)
{
    calib_globals_t *globals = (calib_globals_t *)data;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("globals", "r_0")) {
        sscanf(value, "%lf", &globals->r_0);
    } else if (MATCH("globals", "r_1")) {
        sscanf(value, "%lf", &globals->r_1);
    } else if (MATCH("globals", "r_2")) {
        sscanf(value, "%lf", &globals->r_2);
    } else if (MATCH("globals", "r_acc")) {
        sscanf(value, "%lf", &globals->r_acc);
    } else if (MATCH("globals", "r_stab")) {
        sscanf(value, "%lf", &globals->r_stab);
    } else if (MATCH("globals", "r_stab_cnt")) {
        globals->r_stab_cnt = atoi(value);
    } else if (MATCH("globals", "r_oob_floor")) {
        sscanf(value, "%lf", &globals->r_oob_floor);
    } else if (MATCH("globals", "r_oob_ceil")) {
        sscanf(value, "%lf", &globals->r_oob_ceil);
    } else if (MATCH("globals", "t_0")) {
        sscanf(value, "%lf", &globals->t_0);
    } else if (MATCH("globals", "t_1")) {
        sscanf(value, "%lf", &globals->t_1);
    } else if (MATCH("globals", "t_2")) {
        sscanf(value, "%lf", &globals->t_2);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

int calib_read_params_from_file(char *file_name, calib_globals_t *ctx)
{
    int ret = EXIT_SUCCESS;

    if (ini_parse(file_name, calib_inih_handler, ctx) < 0) {
        fprintf(stderr, "error: can't load calibration file\n");
        ret = EXIT_FAILURE;
    }

    return ret;
}


static void calib_check_stab(calib_globals_t *g, calib_channel_t *c)
{
    if (c->stab_cnt > g->r_stab_cnt) {
        switch (c->state) {
            case CALIB_P0:
                c->mean_0 = c->stab_buff/(double)c->stab_cnt;
                c->checklist |= CALIB_P0_DONE;
                break;
            case CALIB_P1:
                c->mean_1 = c->stab_buff/(double)c->stab_cnt;
                c->checklist |= CALIB_P1_DONE;
                break;
            case CALIB_P2:
                c->mean_2 = c->stab_buff/(double)c->stab_cnt;
                c->checklist |= CALIB_P2_DONE;
                break;
            default:
                break;
        }
    }
}

#define MAX_LINE_SZ  64

int calib_init_from_data_file(char *data_file_name, char *ini_file_name, calib_context_t *ctx)
{
    int ret = EXIT_SUCCESS;
    int fd = -1;
    int ifd = -1;
    float *buffer = NULL;
    ssize_t read_len;
    ssize_t read_samples;
    ssize_t i;
    ssize_t total_samples = 0;
    calib_globals_t *g = ctx->globals;
    calib_channel_t *c = &ctx->channel_data;
    double r0_min, r0_max, r1_min, r1_max, r2_min, r2_max;
    char *cco = NULL;
    char ccol[MAX_LINE_SZ];

    buffer = (float *) calloc(CHUNK_SIZE / sizeof(float), sizeof(float));
    if (!buffer) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    if ((fd = open(data_file_name, O_RDONLY)) < 0) {
        errMsg("opening input file");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    if (lseek(fd, 0x30, SEEK_SET) < 0) {
        errMsg("opening metadata file");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    r0_min = g->r_0 - g->r_acc;
    r0_max = g->r_0 + g->r_acc;
    r1_min = g->r_1 - g->r_acc;
    r1_max = g->r_1 + g->r_acc;
    r2_min = g->r_2 - g->r_acc;
    r2_max = g->r_2 + g->r_acc;

    while ((read_len = read(fd, (uint8_t *)buffer, CHUNK_SIZE)) > 0) {
        read_samples = read_len / sizeof(float);
        for (i=0; i<read_samples; i++) {
            total_samples++;
            if (buffer[i] < g->r_oob_floor) {
                calib_check_stab(g, c);
                c->oob_floor_cnt++;
            } else if ((buffer[i] > r0_min) && (buffer[i] < r0_max)) {
                if (c->state == CALIB_P0) {
                    c->stab_cnt++;
                    c->stab_buff += buffer[i];
                } else {
                    calib_check_stab(g, c);
                    c->state = CALIB_P0;
                    c->stab_cnt = 1;
                    c->stab_buff = buffer[i];
                }
            } else if ((buffer[i] > r1_min) && (buffer[i] < r1_max)) {
                if (c->state == CALIB_P1) {
                    c->stab_cnt++;
                    c->stab_buff += buffer[i];
                } else {
                    calib_check_stab(g, c);
                    c->state = CALIB_P1;
                    c->stab_cnt = 1;
                    c->stab_buff = buffer[i];
                }
            } else if ((buffer[i] > r2_min) && (buffer[i] < r2_max)) {
                if (c->state == CALIB_P2) {
                    c->stab_cnt++;
                    c->stab_buff += buffer[i];
                } else {
                    calib_check_stab(g, c);
                    c->state = CALIB_P2;
                    c->stab_cnt = 1;
                    c->stab_buff = buffer[i];
                }
            } else if (buffer[i] > g->r_oob_ceil) {
                calib_check_stab(g, c);
                c->oob_ceil_cnt++;
            } else {
                calib_check_stab(g, c);
                c->state = CALIB_UNSTABLE;
                c->stab_cnt = 0;
                c->stab_buff = 0;
            }
        }
        calib_check_stab(g, c);
    }

    //printf("oob_floor_cnt %ld\noob_ceil_cnt %ld\ntotal samples %ld\n", c->oob_floor_cnt, c->oob_ceil_cnt, total_samples);
    //printf("stab_cnt %ld\nchecklist %x\n", c->stab_cnt, c->checklist);

    if (c->checklist != (CALIB_P0_DONE | CALIB_P1_DONE | CALIB_P2_DONE)) {
        fprintf(stderr, "calibration error: cannot detect stable signals\n");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    c->slope_0 = (g->t_1 - g->t_0)/(c->mean_1 - c->mean_0);
    c->slope_1 = (g->t_2 - g->t_1)/(c->mean_2 - c->mean_1);
    c->offset_0 = g->t_0 - (c->mean_0 * c->slope_0);
    c->offset_1 = g->t_1 - (c->mean_1 * c->slope_1);
    c->midpoint = c->mean_1;
    //printf("slope0 %lf\nslope1 %lf\n", c->slope_0, c->slope_1);
    //printf("offset0 %lf\noffset1 %lf\n", c->offset_0, c->offset_1);

    cco = (char *) calloc(4*1024, sizeof(char));
    if (!cco) {
        errMsg("during calloc");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    snprintf(ccol, MAX_LINE_SZ, "[CH%d]\n", c->id);
    strcat(cco, ccol);
    snprintf(ccol, MAX_LINE_SZ, "type=%d\n", c->calibration_type);
    strcat(cco, ccol);
    snprintf(ccol, MAX_LINE_SZ, "midpoint=%lf\n", c->midpoint);
    strcat(cco, ccol);
    snprintf(ccol, MAX_LINE_SZ, "slope_0=%lf\n", c->slope_0);
    strcat(cco, ccol);
    snprintf(ccol, MAX_LINE_SZ, "offset_0=%lf\n", c->offset_0);
    strcat(cco, ccol);
    snprintf(ccol, MAX_LINE_SZ, "slope_1=%lf\n", c->slope_1);
    strcat(cco, ccol);
    snprintf(ccol, MAX_LINE_SZ, "offset_1=%lf\n\n", c->offset_1);
    strcat(cco, ccol);

    if ((ifd = open(ini_file_name, O_WRONLY | O_APPEND)) < 0) {
        errMsg("opening ini file");
        ret = EXIT_FAILURE;
        goto cleanup;
    }
    if (flock(ifd, LOCK_EX) < 0) {
        errMsg("locking ini file");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    if (write(ifd, cco, strlen(cco)) != strlen(cco)) {
        errMsg("writing to ini file");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    if (flock(ifd, LOCK_UN) < 0) {
        errMsg("unlocking ini file");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

    //printf("%s\n", cco);

cleanup:
    if (buffer)
        free(buffer);
    if (cco)
        free(cco);
    if (fd >= 0)
        close(fd);
    if (ifd >= 0)
        close(ifd);

    return ret;
}

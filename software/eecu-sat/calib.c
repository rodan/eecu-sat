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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include "proj.h"
#include "error.h"
#include "ini.h"
#include "calib.h"

static int calib_inih_globals_handler(void* data, const char* section, const char* name,
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

static int calib_inih_channel_handler(void* data, const char* section, const char* name,
                   const char* value)
{
    calib_channel_t *ch = (calib_channel_t *)data;
    char ch_id[LINE_MAX_SZ];

    snprintf(ch_id, LINE_MAX_SZ, "CH%d", ch->id);

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH(ch_id, "type")) {
        ch->calibration_type = atoi(value);
    } else if (MATCH(ch_id, "midpoint")) {
        sscanf(value, "%lf", &ch->midpoint);
    } else if (MATCH(ch_id, "slope_0")) {
        sscanf(value, "%lf", &ch->slope_0);
    } else if (MATCH(ch_id, "offset_0")) {
        sscanf(value, "%lf", &ch->offset_0);
    } else if (MATCH(ch_id, "slope_1")) {
        sscanf(value, "%lf", &ch->slope_1);
    } else if (MATCH(ch_id, "offset_1")) {
        sscanf(value, "%lf", &ch->offset_1);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

int calib_read_params_from_file(char *file_name, void *ctx, uint8_t flags)
{
    int ret = SR_OK;

    if (flags == CALIB_INI_GLOBALS) {
        if (ini_parse(file_name, calib_inih_globals_handler, ctx) < 0) {
            err_msg("%s:%d can't load calibration file", __FILE__, __LINE__);
            ret = SR_ERR_ARG;
        }
    } else if (flags == CALIB_INI_CHANNEL) {
        if (ini_parse(file_name, calib_inih_channel_handler, ctx) < 0) {
            err_msg("%s:%d can't load calibration file", __FILE__, __LINE__);
            ret = SR_ERR_ARG;
        }
    } else {
        err_msg("%s:%d improper use of 'flags' in calib_read_params_from_file()", __FILE__, __LINE__);
        ret = SR_ERR_ARG;
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

int calib_init_from_buffer(float *buffer, ssize_t num_samples, calib_context_t *ctx)
{
    int ret = SR_OK;
    ssize_t i;
    calib_globals_t *g = &ctx->globals;
    calib_channel_t *c = &ctx->channel;
    double r0_min, r0_max, r1_min, r1_max, r2_min, r2_max;
    char *cco = NULL;
    char ccol[LINE_MAX_SZ];

    r0_min = g->r_0 - g->r_acc;
    r0_max = g->r_0 + g->r_acc;
    r1_min = g->r_1 - g->r_acc;
    r1_max = g->r_1 + g->r_acc;
    r2_min = g->r_2 - g->r_acc;
    r2_max = g->r_2 + g->r_acc;

    for (i=0; i<num_samples; i++) {
        c->total_samples++;
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

    //printf("oob_floor_cnt %ld\noob_ceil_cnt %ld\ntotal samples %ld\n", c->oob_floor_cnt, c->oob_ceil_cnt, c->total_samples);
    //printf("stab_cnt %ld\nchecklist %x\n", c->stab_cnt, c->checklist);

    if (c->checklist != (CALIB_P0_DONE | CALIB_P1_DONE | CALIB_P2_DONE)) {
        err_msg("%s:%d calibration error: cannot detect stable signals", __FILE__, __LINE__);
        ret = SR_ERR_DATA;
        goto cleanup;
    }

    c->slope_0 = (g->t_1 - g->t_0)/(c->mean_1 - c->mean_0);
    c->slope_1 = (g->t_2 - g->t_1)/(c->mean_2 - c->mean_1);
    c->offset_0 = g->t_0 - (c->mean_0 * c->slope_0);
    c->offset_1 = g->t_1 - (c->mean_1 * c->slope_1);
    c->midpoint = c->mean_1;
    //printf("slope0 %lf\nslope1 %lf\n", c->slope_0, c->slope_1);
    //printf("offset0 %lf\noffset1 %lf\n", c->offset_0, c->offset_1);

    cco = (char *) g_malloc0(4 * 1024 * sizeof(char));

    snprintf(ccol, LINE_MAX_SZ, "[CH%d]\n", c->id);
    strcat(cco, ccol);
    snprintf(ccol, LINE_MAX_SZ, "type=%d\n", c->calibration_type);
    strcat(cco, ccol);
    snprintf(ccol, LINE_MAX_SZ, "midpoint=%lf\n", c->midpoint);
    strcat(cco, ccol);
    snprintf(ccol, LINE_MAX_SZ, "slope_0=%lf\n", c->slope_0);
    strcat(cco, ccol);
    snprintf(ccol, LINE_MAX_SZ, "offset_0=%lf\n", c->offset_0);
    strcat(cco, ccol);
    snprintf(ccol, LINE_MAX_SZ, "slope_1=%lf\n", c->slope_1);
    strcat(cco, ccol);
    snprintf(ccol, LINE_MAX_SZ, "offset_1=%lf\n\n", c->offset_1);
    strcat(cco, ccol);

cleanup:
    if (cco)
        g_free(cco);

    return ret;
}

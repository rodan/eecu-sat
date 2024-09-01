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
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include "proj.h"
#include "tlpi_hdr.h"
#include "calib.h"
#include "transform.h"

struct context {
    gchar *calib_file;
    calib_globals_t globals;
    calib_channel_t channel;
};

static int init(struct sr_transform *t, GHashTable *options)
{
    struct context *ctx;

    if (!t || !options)
        return SR_ERR_ARG;

    t->priv = ctx = g_malloc0(sizeof(struct context));

    /* Options */
    ctx->calib_file = g_strdup(g_variant_get_string(g_hash_table_lookup(options, "calib_file"), NULL));
    
    if ((calib_read_params_from_file(ctx->calib_file, &ctx->globals, CALIB_INI_GLOBALS)) != EXIT_SUCCESS) {
        fprintf(stderr, "error during calib_read_params_from_file()\n");
        return SR_ERR_ARG;
    }

    return SR_OK;
}

static int receive(const struct sr_transform *t,
                   struct sr_datafeed_packet *packet_in, struct sr_datafeed_packet **packet_out)
{
    struct context *ctx;
    const struct sr_datafeed_analog *analog;
    calib_globals_t *g;
    calib_channel_t *c;
    ssize_t i;
    float cur;
    float *samples;
    struct dev_frame *frame;

    if (!t || !packet_in || !packet_out)
        return SR_ERR_ARG;

    frame = t->sdi->priv;
    ctx = t->priv;
    g = &ctx->globals;
    c = &ctx->channel;

    switch (packet_in->type) {
    case SR_DF_FRAME_BEGIN:
        ctx->channel.id = frame->ch;
        if ((calib_read_params_from_file(ctx->calib_file, &ctx->channel, CALIB_INI_CHANNEL)) != EXIT_SUCCESS) {
            fprintf(stderr, "error during calib_read_params_from_file()\n");
            return SR_ERR_ARG;
        }
        break;
    case SR_DF_ANALOG:
        analog = packet_in->payload;
        samples = analog->data;
        for (i=0; i<analog->num_samples; i++) {
            cur = samples[i];
            if (cur < g->r_oob_floor) {
                samples[i] = g->r_oob_floor;
            } else if (cur > g->r_oob_ceil) {
                samples[i] = g->r_oob_ceil;
            } else if (cur <= c->midpoint) {
                samples[i] = cur * c->slope_0 + c->offset_0;
            } else if (cur > c->midpoint) {
                samples[i] = cur * c->slope_1 + c->offset_1;
            }
        }
        break;
    default:
        break;
    }

    /* Return the in-place-modified packet. */
    *packet_out = packet_in;

    return SR_OK;
}

static int cleanup(struct sr_transform *t)
{
    struct context *ctx;

    if (!t)
        return SR_ERR_ARG;

    if (t->priv) {
        ctx = t->priv;
        if (ctx->calib_file)
            g_free(ctx->calib_file);
        g_free(ctx);
        t->priv = NULL;
    }

    return SR_OK;
}

static struct sr_option options[] = {
    {"calib_file", "Calibration file", "ini file containing two slope and offset pairs for each channel", NULL, NULL},
    ALL_ZERO
};

static const struct sr_option *get_options(void)
{
    if (!options[0].def) {
        options[0].def = g_variant_ref_sink(g_variant_new_string(""));
    }

    return options;
}

struct sr_transform_module transform_calibrate_linear_3p = {
    .id = "calibrate_linear_3p",
    .name = "calibrate_linear_3p",
    .desc = "linear calibration in 3 points",
    .options = get_options,
    .init = init,
    .receive = receive,
    .cleanup = cleanup,
};

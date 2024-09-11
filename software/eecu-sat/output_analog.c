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
#include "output.h"

struct out_context {
    uint32_t channel_offset;
    uint64_t samples_written;
    bool header_present;
};

static int init(struct sr_output *o, GHashTable *options)
{
    struct out_context *outc;

    if (!o || !options)
        return SR_ERR_ARG;

    outc = (struct out_context *)calloc(1, sizeof(struct out_context));
    if (outc == NULL) {
        errMsg("calloc error");
        return SR_ERR_MALLOC;
    }
    o->priv = outc;

    outc->channel_offset = g_variant_get_uint32(g_hash_table_lookup(options, "channel_offset"));

    return SR_OK;
}

static int receive(const struct sr_output *o, const struct sr_datafeed_packet *pkt, GString **out)
{
    int ret = SR_OK;
    char *filename;
    FILE *fp = NULL;
    ssize_t byte_cnt;
    const struct sr_datafeed_analog *analog;
    const struct dev_frame *frame = o->sdi->priv;
    struct out_context *outc = o->priv;
    ch_data_t *ch_data_ptr = NULL;
    GSList *l;

    UNUSED(out);

    filename = (char *)calloc(PATH_MAX, 1);
    if (filename == NULL) {
        errMsg("calloc error");
        return SR_ERR_MALLOC;
    }

    analog = pkt->payload;

    switch (pkt->type) {
    case SR_DF_ANALOG:
        snprintf(filename, PATH_MAX, "%s%d.bin", o->filename, frame->ch + outc->channel_offset);
        break;
    default:
        goto cleanup;
    }

    if (frame->chunk == 1) {
        outc->samples_written = 0;
        outc->header_present = 0;
        fp = fopen(filename, "wb");
        // add header
        for (l = o->sdi->channels; l; l = l->next) {
            ch_data_ptr = l->data;
            if ((ch_data_ptr->id == frame->ch) && (ch_data_ptr->file_type == SALEAE_ANALOG)) {
                if (fwrite(&ch_data_ptr->header, 1, SALEAE_ANALOG_HDR_SIZE, fp) != SALEAE_ANALOG_HDR_SIZE) {
                    errMsg("during fwrite()");
                    ret = SR_ERR_IO;
                    goto cleanup;
                }
                outc->header_present = true;
                break;
            }
        }
    } else {
        fp = fopen(filename, "ab");
    }

    if (!fp) {
        errMsg("during fopen()");
        ret = SR_ERR_IO;
        goto cleanup;
    }

    //printf("fn: %s\n", filename);
    byte_cnt = analog->num_samples * sizeof(float);
    if (fwrite(analog->data, 1, byte_cnt, fp) != byte_cnt) {
        errMsg("during fwrite()");
        ret = SR_ERR_IO;
        goto cleanup;
    }

    outc->samples_written += analog->num_samples;

    if (fp) {
        fclose(fp);
        fp = NULL;
    }

    if (outc->header_present) {
        // update header
        fp = fopen(filename, "r+b");
        if (!fp) {
            errMsg("during fopen()");
            ret = SR_ERR_IO;
            goto cleanup;
        }
        if (fseek(fp, SALEAE_ANALOG_HDR_SC_POS, SEEK_SET) < 0) {
            errMsg("during lseek()");
            ret = SR_ERR_IO;
            goto cleanup;
        }
        if (fwrite(&outc->samples_written, 1, sizeof(uint64_t), fp) != sizeof(uint64_t)) {
            errMsg("during fwrite()");
            ret = SR_ERR_IO;
            goto cleanup;
        }
    }

cleanup:
    if (filename)
        free(filename);

    if (fp)
        fclose(fp);

    return ret;
}

static struct sr_option options[] = {
    {"channel_offset", "channel offset", "offset to add to all channel identifiers", NULL, NULL},
    ALL_ZERO
};

static const struct sr_option *get_options(void)
{
    if (!options[0].def) {
        options[0].def = g_variant_new_uint32(0);
    }

    return options;
}

static int cleanup(struct sr_output *o)
{
    struct out_context *outc;

    if (o == NULL)
        return SR_ERR_BUG;

    if (o->priv) {
        outc = o->priv;
        free(outc);
        o->priv = NULL;
    }

    return SR_OK;
}

struct sr_output_module output_analog = {
    .id = "analog",
    .name = "analog",
    .desc = "one channel per file raw analog",
    .exts = (const char *[]) {"sr", NULL},
    .flags = SR_OUTPUT_INTERNAL_IO_HANDLING,
    .options = get_options,
    .init = init,
    .receive = receive,
    .cleanup = cleanup,
};

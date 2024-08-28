
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
};

static int init(struct sat_output *o, GHashTable *options)
{
    struct out_context *outc;

    if (!o || !options)
        return SR_ERR_ARG;

    outc = (struct out_context *)calloc(1, sizeof(struct out_context));
    if (outc == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }
    o->priv = outc;

    outc->channel_offset = g_variant_get_uint32(g_hash_table_lookup(options, "channel_offset"));

    return EXIT_SUCCESS;
}

static int receive(const struct sat_output *o, const struct sr_datafeed_packet *pkt)
{
    int ret = EXIT_SUCCESS;
    char *filename;
    FILE *fp = NULL;
    ssize_t byte_cnt;
    const struct sr_datafeed_analog *analog;
    const struct dev_frame *frame = o->sdi->priv;
    const struct out_context *outc = o->priv;

    filename = (char *)calloc(PATH_MAX, 1);
    if (filename == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }

    analog = pkt->payload;

    switch (pkt->type) {
    case SR_DF_META:
        snprintf(filename, PATH_MAX - 1, "metadata");
        break;
    case SR_DF_ANALOG:
        snprintf(filename, PATH_MAX, "%s%d.bin", o->filename, frame->ch + outc->channel_offset);
        break;
    default:
        goto cleanup;
    }

    if (frame->chunk == 1) {
        fp = fopen(filename, "w");
    } else {
        fp = fopen(filename, "a");
    }

    //printf("fn: %s\n", filename);
    byte_cnt = analog->num_samples * sizeof(float);
    if (fwrite(analog->data, 1, byte_cnt, fp) != byte_cnt) {
        errMsg("during fwrite()");
        ret = EXIT_FAILURE;
        goto cleanup;
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

static int cleanup(struct sat_output *o)
{
    struct out_context *outc;

    if (o == NULL)
        return EXIT_FAILURE;

    if (o->priv) {
        outc = o->priv;
        free(outc);
        o->priv = NULL;
    }

    return EXIT_SUCCESS;
}

struct sat_output_module output_analog = {
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

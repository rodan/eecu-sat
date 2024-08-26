
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include "proj.h"
#include "tlpi_hdr.h"
#include "transform.h"

struct context {
    char *calib;
};

static int init(struct sat_transform *t, GHashTable *options)
{
    struct context *ctx;

    if (!t || !options)
        return SR_ERR_ARG;

    t->priv = ctx = g_malloc0(sizeof(struct context));

    /* Options */
    ctx->calib = g_strdup(g_variant_get_string(
        g_hash_table_lookup(options, "calib"), NULL));
    printf("calib is %s\n", ctx->calib);

    return SR_OK;
}

static int receive(const struct sat_transform *t,
        struct sr_datafeed_packet *packet_in,
        struct sr_datafeed_packet **packet_out)
{
    struct context *ctx;
    //const struct sr_datafeed_analog *analog;

    if (!t || !packet_in || !packet_out)
        return SR_ERR_ARG;
    ctx = t->priv;

    switch (packet_in->type) {
    case SR_DF_ANALOG:
        //analog = packet_in->payload;
        //analog->encoding->scale.p *= ctx->factor.p;
        //analog->encoding->scale.q *= ctx->factor.q;
        break;
    default:
        //sat_spew("Unsupported packet type %d, ignoring.", packet_in->type);
        break;
    }

    /* Return the in-place-modified packet. */
    *packet_out = packet_in;

    return SR_OK;
}

static int cleanup(struct sat_transform *t)
{
    struct context *ctx;

    if (!t)
        return SR_ERR_ARG;
    ctx = t->priv;

    g_free(ctx);
    t->priv = NULL;

    return SR_OK;
}

static struct sr_option options[] = {
    { "calib", "Calibration file", "ini file containing two slope and offset pairs for each channel", NULL, NULL },
    ALL_ZERO
};

static const struct sr_option *get_options(void)
{
    GSList *l = NULL;

    if (!options[0].def) {
        options[0].def = g_variant_ref_sink(g_variant_new_string(""));
    }

    return options;
}

struct sat_transform_module transform_calibrate_linear_3p = {
    .id = "calibrate_linear_3p",
    .name = "calibrate_linear_3p",
    .desc = "linear calibration in 3 points",
    .options = get_options,
    .init = init,
    .receive = receive,
    .cleanup = cleanup,
};


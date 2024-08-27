
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <zip.h>
#include "proj.h"
#include "tlpi_hdr.h"
#include "calib.h"
#include "output.h"

struct out_context {
    gchar *calib_file;
    calib_context_t cal;
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

    /* Options */
    outc->calib_file = g_strdup(g_variant_get_string(g_hash_table_lookup(options, "calib_file"), NULL));

    if ((calib_read_params_from_file(outc->calib_file, &outc->cal.globals, CALIB_INI_GLOBALS)) != EXIT_SUCCESS) {
        fprintf(stderr, "error during calib_read_params_from_file()\n");
        return SR_ERR_ARG;
    }

    return EXIT_SUCCESS;
}

static int receive(const struct sat_output *o, const struct sr_datafeed_packet *pkt)
{
    struct out_context *outc = o->priv;
    const struct sr_datafeed_analog *analog;
    float *samples;
    calib_globals_t *g = &outc->cal.globals;
    calib_channel_t *c = &outc->cal.channel;
    int ifd = -1;
    int ret = SR_OK;
    char *cco = NULL;
    char ccol[LINE_MAX_SZ];
    struct dev_frame *frame = o->sdi->priv;

    switch (pkt->type) {
    case SR_DF_ANALOG:
        analog = pkt->payload;
        samples = analog->data;
        calib_init_from_buffer(samples, analog->num_samples, &outc->cal);
        break;
    case SR_DF_FRAME_BEGIN:
        memset(&outc->cal.channel, 0, sizeof(struct calib_channel));
        outc->cal.channel.id = frame->ch;
        outc->cal.channel.calibration_type = CALIB_TYPE_3_POINT;
        break;
    case SR_DF_FRAME_END:
        if (c->checklist != (CALIB_P0_DONE | CALIB_P1_DONE | CALIB_P2_DONE)) {
            fprintf(stderr, "calibration error: cannot detect stable signals\n");
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

        cco = (char *) calloc(4*1024, sizeof(char));
        if (!cco) {
            errMsg("during calloc");
            ret = SR_ERR_MALLOC;
            goto cleanup;
        }

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

        if ((ifd = open(outc->calib_file, O_WRONLY | O_APPEND)) < 0) {
            errMsg("opening ini file");
            ret = SR_ERR_ARG;
            goto cleanup;
        }

        if (write(ifd, cco, strlen(cco)) != strlen(cco)) {
            errMsg("writing to ini file");
            ret = EXIT_FAILURE;
            goto cleanup;
        }
        break;
    default:
        goto cleanup;
    }

cleanup:
    if (ifd >= 0)
        close(ifd);
    if (cco)
        free(cco);

    return ret;
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

static int cleanup(struct sat_output *o)
{
    struct out_context *outc;

    if (o == NULL)
        return EXIT_FAILURE;

    outc = o->priv;

    if (outc->calib_file)
        free(outc->calib_file);

    if (o->priv)
        free(o->priv);

    return EXIT_SUCCESS;
}

struct sat_output_module output_calibrate_linear_3p = {
    .id = "calibrate_linear_3p",
    .name = "calibrate linear 3p",
    .desc = "parameters for linear calibration in 3 points",
    .exts = (const char *[]) { "ini", NULL },
    .flags = SR_OUTPUT_INTERNAL_IO_HANDLING,
    .options = get_options,
    .init = init,
    .receive = receive,
    .cleanup = cleanup
};

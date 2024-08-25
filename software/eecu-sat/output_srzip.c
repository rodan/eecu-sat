
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <zip.h>
#include "proj.h"
#include "tlpi_hdr.h"
#include "output.h"
#include "output_srzip.h"

struct out_context {
    char *target_filename; // filename within the archive
};

static int init(struct sat_output *o)
{
    struct zip_source *src;
    struct zip *archive;
    struct out_context *outc;
    unlink(o->filename);

    outc = (struct out_context *)calloc(1, sizeof(struct out_context));
    o->priv = outc;

    outc->target_filename = (char *)calloc(PATH_MAX, 1);
    if (outc->target_filename == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }

    archive = zip_open(o->filename, ZIP_CREATE, NULL);
    if (!archive) {
        fprintf(stderr, "error: zip_open() has failed\n");
        return EXIT_FAILURE;
    }

    src = zip_source_buffer(archive, "2", 1, 0);
    if (zip_file_add(archive, "version", src, ZIP_FL_ENC_UTF_8) < 0) {
        fprintf(stderr, "Error adding file into archive: %s", zip_strerror(archive));
        zip_source_free(src);
        zip_discard(archive);
        return EXIT_FAILURE;
    }

    if (zip_close(archive) < 0) {
        fprintf(stderr, "Error saving zipfile: %s", zip_strerror(archive));
        zip_discard(archive);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int receive(struct sat_output *o, struct sr_datafeed_packet *pkt)
{
    struct zip *archive;
    struct zip_source *src;
    struct out_context *outc = o->priv;

    if (!(archive = zip_open(o->filename, 0, NULL)))
        return EXIT_FAILURE;

    src = zip_source_buffer(archive, pkt->payload, pkt->payload_size, 0);

    switch (pkt->type) {
    case SR_DF_META:
        snprintf(outc->target_filename, PATH_MAX - 1, "metadata");
        break;
    case SR_DF_ANALOG:
        snprintf(outc->target_filename, PATH_MAX - 1, "analog-1-%d-%d", o->ch, o->chunk);
        break;
    default:
        goto cleanup;
    }

    if (zip_file_add(archive, outc->target_filename, src, ZIP_FL_ENC_UTF_8) < 0) {
        fprintf(stderr, "Failed to add chunk: %s", zip_strerror(archive));
        zip_source_free(src);
        zip_discard(archive);
        return EXIT_FAILURE;
    }

 cleanup:
    if (zip_close(archive) < 0) {
        fprintf(stderr, "Error saving session file: %s", zip_strerror(archive));
        zip_discard(archive);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int cleanup(struct sat_output *o)
{
    struct out_context *outc;

    if (o == NULL)
        return EXIT_FAILURE;

    outc = o->priv;

    if (outc->target_filename)
        free(outc->target_filename);

    if (o->priv)
        free(o->priv);

    return EXIT_SUCCESS;
}

struct sat_output_module output_srzip = {
    .id = "srzip",
    .name = "srzip",
    .desc = "sigrok session file format data",
    .exts = (const char *[]) {"sr", NULL},
    .init = init,
    .receive = receive,
    .cleanup = cleanup,
};

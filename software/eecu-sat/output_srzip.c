
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

struct out_context {
    char *target_filename;      // filename within the archive
    char *metadata_file;
};

static int init(struct sat_output *o, GHashTable *options)
{
    struct zip_source *src;
    struct zip_source *metadata = NULL;
    struct zip *archive;
    struct out_context *outc;

    unlink(o->filename);

    outc = (struct out_context *)calloc(1, sizeof(struct out_context));
    o->priv = outc;

    /* Options */
    outc->metadata_file = g_strdup(g_variant_get_string(g_hash_table_lookup(options, "metadata_file"), NULL));

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

    if (outc->metadata_file && (outc->metadata_file[0] != 0)) {
        metadata = zip_source_file(archive, outc->metadata_file, 0, -1);
        if (!metadata) {
            fprintf(stderr, "Error adding file into archive: %s", zip_strerror(archive));
            return EXIT_FAILURE;
        }
        if (zip_file_add(archive, "metadata", metadata, ZIP_FL_ENC_UTF_8) < 0) {
            fprintf(stderr, "Error adding file into archive: %s", zip_strerror(archive));
            zip_source_free(metadata);
            zip_discard(archive);
            return EXIT_FAILURE;
        }
    }

    if (zip_close(archive) < 0) {
        fprintf(stderr, "Error saving zipfile: %s", zip_strerror(archive));
        zip_discard(archive);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int receive(const struct sat_output *o, const struct sr_datafeed_packet *pkt)
{
    struct zip *archive;
    struct zip_source *src;
    struct out_context *outc = o->priv;
    const struct sr_datafeed_analog *analog;
    const struct sat_generic_pkt *gpkt;

    if (!(archive = zip_open(o->filename, 0, NULL)))
        return EXIT_FAILURE;

    switch (pkt->type) {
    case SR_DF_META:
        gpkt = pkt->payload;
        src = zip_source_buffer(archive, gpkt->payload, gpkt->payload_sz, 0);
        snprintf(outc->target_filename, PATH_MAX - 1, "metadata");
        break;
    case SR_DF_ANALOG:
        analog = pkt->payload;
        src = zip_source_buffer(archive, analog->data, analog->num_samples * sizeof(float), 0);
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

static struct sr_option options[] = {
    {"metadata_file", "metadata file", "custom metadata file to include in the output srzip archive", NULL, NULL},
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

    if (outc->target_filename)
        free(outc->target_filename);
    if (outc->metadata_file)
        free(outc->metadata_file);

    if (o->priv)
        free(o->priv);

    return EXIT_SUCCESS;
}

struct sat_output_module output_srzip = {
    .id = "srzip",
    .name = "srzip",
    .desc = "sigrok session file format data",
    .exts = (const char *[]) {"sr", NULL},
    .flags = SR_OUTPUT_INTERNAL_IO_HANDLING,
    .options = get_options,
    .init = init,
    .receive = receive,
    .cleanup = cleanup,
};

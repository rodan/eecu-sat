
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>

#include <linux/limits.h>

#include "proj.h"
#include "tlpi_hdr.h"
#include "output_srzip.h"

int output_srzip_init(srzip_context_t *ctx)
{
    unlink(ctx->archive_file_name);

    ctx->buffer = (char *)calloc(CHUNK_SIZE, 1);
    if (ctx->buffer == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }

    ctx->target_file_name = (char *)calloc(PATH_MAX, 1);
    if (ctx->target_file_name == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }

    ctx->archive = zip_open(ctx->archive_file_name, ZIP_CREATE, NULL);
    if (!ctx->archive) {
        fprintf(stderr, "error: zip_open() has failed\n");
        return EXIT_FAILURE;
    }

    sprintf(ctx->target_file_name, "version");
    snprintf(ctx->buffer, 2, "2");
    ctx->buffer_len = 1;
    output_srzip_add(ctx);

    if (zip_close(ctx->archive) < 0) {
        fprintf(stderr, "Error saving zipfile: %s", zip_strerror(ctx->archive));
        zip_discard(ctx->archive);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int output_srzip_add(srzip_context_t *ctx)
{
    struct zip_source *src;

    src = zip_source_buffer(ctx->archive, ctx->buffer, ctx->buffer_len, 0);
    if (zip_file_add(ctx->archive, ctx->target_file_name, src, ZIP_FL_ENC_UTF_8) < 0) {
        fprintf(stderr, "Error adding file into archive: %s", zip_strerror(ctx->archive));
        zip_source_free(src);
        zip_discard(ctx->archive);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int output_srzip_append(srzip_context_t *ctx)
{
    struct zip *archive;
    struct zip_source *src;

    if (!(archive = zip_open(ctx->archive_file_name, 0, NULL)))
        return EXIT_FAILURE;

    src = zip_source_buffer(archive, ctx->buffer, ctx->buffer_len, 0);
    if (zip_file_add(archive, ctx->target_file_name, src, ZIP_FL_ENC_UTF_8) < 0) {
        fprintf(stderr, "Failed to add chunk: %s", zip_strerror(archive));
        zip_source_free(src);
        zip_discard(archive);
        return EXIT_FAILURE;
    }

    if (zip_close(archive) < 0) {
        fprintf(stderr, "Error saving session file: %s", zip_strerror(archive));
        zip_discard(archive);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int output_srzip_free(srzip_context_t **ctx)
{
    srzip_context_t *c = *ctx;
    if (c->buffer != NULL) {
        free(c->buffer);
    }

    if (c->target_file_name)
        free(c->target_file_name);

    if (c)
        free(c);

    *ctx = NULL;
    return EXIT_SUCCESS;
}

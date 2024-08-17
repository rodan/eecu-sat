#ifndef __OUTPUT_SRZIP_H__
#define __OUTPUT_SRZIP_H__

#include <stdint.h>
#include <zip.h>

struct srzip_context {
    char *archive_file_name;
    char *target_file_name;
    struct zip *archive;
    char *buffer;
    ssize_t buffer_len;
};
typedef struct srzip_context srzip_context_t;

int output_srzip_init(srzip_context_t *ctx);
int output_srzip_add(srzip_context_t *ctx);
int output_srzip_append(srzip_context_t *ctx);
int output_srzip_free(srzip_context_t **ctx);

#endif

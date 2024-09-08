#ifndef SIGROK_GLUE_H
#define SIGROK_GLUE_H

#include <stdint.h>
#include <stdbool.h>
/* we use some of the libsigrok structures and macros*/
#include <libsigrok/libsigrok.h>
#include "sigrok_internal.h"
#include "saleae.h"

#define                  CHUNK_SIZE  (8 * 1024 * 1024)
#define                 LINE_MAX_SZ  64
#define  DEFAULT_OUTPUT_FORMAT_FILE  "srzip"

#define UNUSED(x) (void)(x)

struct cmdline_opt {
    char *input_prefix;
    char *output_file;
    char *output_format;
    char *transform_module;
    char *triggers;
    bool skip_header;
    uint32_t action;
};

struct sat_trigger {
    uint16_t id;
    char *name;
    uint16_t type;
    float level;
    ssize_t nth;
    ssize_t a;
    ssize_t b;
    GSList *matches;
    void *priv;
};

struct ch_data {
    uint16_t id;
    char *input_file_name;
    struct sat_trigger *trigger;
    uint8_t file_type;
    ssize_t input_file_size;
    uint8_t sample_size;
    ssize_t sample_count;
    struct saleae_ana_bh0 header;
};
typedef struct ch_data ch_data_t;

struct sat_generic_pkt {
    ssize_t payload_sz;
    void *payload;
};

struct dev_frame {
    uint16_t ch;
    uint16_t chunk;
};

#endif

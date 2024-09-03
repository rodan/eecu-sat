#ifndef SIGROK_GLUE_H
#define SIGROK_GLUE_H

#include <stdint.h>
#include <stdbool.h>
/* we use some of the libsigrok structures and macros*/
#include <libsigrok/libsigrok.h>
#include "sigrok_internal.h"

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
    uint16_t ch_id;
    char *name;
    uint16_t type;
    float level;
    ssize_t nth;
};

struct ch_data {
    uint16_t id;
    char *input_file_name;
    struct sat_trigger *t;
    ssize_t input_file_size;
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

int maybe_config_set(struct sr_dev_driver *driver,
		const struct sr_dev_inst *sdi, struct sr_channel_group *cg,
		uint32_t key, GVariant *gvar);
int maybe_config_list(struct sr_dev_driver *driver,
		const struct sr_dev_inst *sdi, struct sr_channel_group *cg,
		uint32_t key, GVariant **gvar);

#endif

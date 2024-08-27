#ifndef SIGROK_GLUE_H
#define SIGROK_GLUE_H

#include <stdint.h>
#include <stdbool.h>
/* we use some of the libsigrok structures and macros*/
#include <libsigrok/libsigrok.h>

#define                  CHUNK_SIZE  (8 * 1024 * 1024)
#define                 LINE_MAX_SZ  64

#define           ACTION_DO_CONVERT  0x01
#define       ACTION_DO_CALIBRATION  0x02
#define        ACTION_DO_CALIB_INIT  0x04

#define  DEFAULT_OUTPUT_FORMAT_FILE  "srzip"

#define UNUSED(x) (void)(x)

#ifdef __clang__
#define ALL_ZERO { }
#else
#define ALL_ZERO { 0 }
#endif

struct cmdline_opt {
    char *input_prefix;
    char *output_file;
    char *output_format;
    char *transform_module;
    bool skip_header;
    uint32_t action;
};

struct ch_data {
    struct ch_data *next;
    char *input_file_name;
    char *output_file_name;
    //char *channel_name;
    uint16_t id;
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

#endif

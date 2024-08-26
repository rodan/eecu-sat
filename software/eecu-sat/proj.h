#ifndef SIGROK_GLUE_H
#define SIGROK_GLUE_H

//#include "libsigrok_l.h"
#include <libsigrok/libsigrok.h>

#define            CHUNK_SIZE  (8 * 1024 * 1024)
#define           LINE_MAX_SZ  64

#define     ACTION_DO_CONVERT  0x01
#define ACTION_DO_CALIBRATION  0x02
#define  ACTION_DO_CALIB_INIT  0x04

#define UNUSED(x) (void)(x)

#ifdef __clang__
#define ALL_ZERO { }
#else
#define ALL_ZERO { 0 }
#endif

struct sat_generic_pkt {
    ssize_t payload_sz;
    void *payload;
};

#endif

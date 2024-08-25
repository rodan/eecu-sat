
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

static int init(struct sat_output *o)
{
    UNUSED(o);
    return EXIT_SUCCESS;
}

static int receive(struct sat_output *o, struct sr_datafeed_packet *pkt)
{
    char *filename;

    filename = (char *)calloc(PATH_MAX, 1);
    if (filename == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }

    switch (pkt->type) {
    case SR_DF_META:
        snprintf(filename, PATH_MAX - 1, "metadata");
        break;
    case SR_DF_ANALOG:
        snprintf(filename, PATH_MAX, "analooog%d.bin", o->ch);
        break;
    default:
        goto cleanup;
    }

    printf("fn: %s\n", filename);

cleanup:
    if (filename)
        free(filename);

    return EXIT_SUCCESS;
}

static int cleanup(struct sat_output *o)
{
    UNUSED(o);
    return EXIT_SUCCESS;
}

struct sat_output_module output_analog = {
    .id = "analog",
    .name = "analog",
    .desc = "one channel per file raw analog",
    .exts = (const char *[]) {"sr", NULL},
    .init = init,
    .receive = receive,
    .cleanup = cleanup,
};

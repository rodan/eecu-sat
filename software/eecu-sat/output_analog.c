
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/limits.h>
#include "proj.h"
#include "tlpi_hdr.h"
#include "output.h"

static int init(struct sat_output *o)
{
    UNUSED(o);
    return EXIT_SUCCESS;
}

static int receive(struct sat_output *o, struct sr_datafeed_packet *pkt)
{
    int ret = EXIT_SUCCESS;
    char *filename;
    FILE *fp = NULL;
    ssize_t byte_cnt;
    const struct sr_datafeed_analog *analog;

    filename = (char *)calloc(PATH_MAX, 1);
    if (filename == NULL) {
        errMsg("calloc error");
        return EXIT_FAILURE;
    }

    analog = pkt->payload;

    switch (pkt->type) {
    case SR_DF_META:
        snprintf(filename, PATH_MAX - 1, "metadata");
        break;
    case SR_DF_ANALOG:
        snprintf(filename, PATH_MAX, "%s%d.bin", o->filename, o->ch);
        //printf("ch %d chunk%d\n", o->ch, o->chunk);
        break;
    default:
        goto cleanup;
    }

    if (o->chunk == 1) {
        fp = fopen(filename, "w");
    } else {
        fp = fopen(filename, "a");
    }

    //printf("fn: %s\n", filename);
    byte_cnt = analog->num_samples * sizeof(float);
    if (fwrite(analog->data, 1, byte_cnt, fp) != byte_cnt) {
        errMsg("during fwrite()");
        ret = EXIT_FAILURE;
        goto cleanup;
    }

 cleanup:
    if (filename)
        free(filename);

    if (fp)
        fclose(fp);

    return ret;
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

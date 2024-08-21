
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ini.h"
#include "calib.h"

static int calib_handler(void* data, const char* section, const char* name,
                   const char* value)
{
    calib_context_t *ctx = (calib_context_t *)data;
    cg_t *globals = &ctx->globals;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("globals", "r_0")) {
        sscanf(value, "%lf", &globals->r_0);
    } else if (MATCH("globals", "r_1")) {
        sscanf(value, "%lf", &globals->r_1);
    } else if (MATCH("globals", "r_2")) {
        sscanf(value, "%lf", &globals->r_2);
    } else if (MATCH("globals", "r_acc")) {
        sscanf(value, "%lf", &globals->r_acc);
    } else if (MATCH("globals", "r_stab")) {
        sscanf(value, "%lf", &globals->r_stab);
    } else if (MATCH("globals", "r_stab_cnt")) {
        globals->r_stab_cnt = atoi(value);
    } else if (MATCH("globals", "r_oob_floor")) {
        sscanf(value, "%lf", &globals->r_oob_floor);
    } else if (MATCH("globals", "r_oob_ceil")) {
        sscanf(value, "%lf", &globals->r_oob_ceil);
    } else if (MATCH("globals", "t_0")) {
        sscanf(value, "%lf", &globals->t_0);
    } else if (MATCH("globals", "t_1")) {
        sscanf(value, "%lf", &globals->t_1);
    } else if (MATCH("globals", "t_2")) {
        sscanf(value, "%lf", &globals->t_2);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

int calib_read_params_from_file(char *file_name, calib_context_t *ctx)
{
    int ret = EXIT_SUCCESS;

    if (ini_parse(file_name, calib_handler, ctx) < 0) {
        fprintf(stderr, "error: can't load calibration file\n");
        ret = EXIT_FAILURE;
    }

    printf("r0 %lf\nr1 %lf\n", ctx->globals.r_0, ctx->globals.r_1);

    return ret;
}

int calib_extract_from_data_file(char *file_name, calib_context_t *ctx)
{
    int ret = EXIT_SUCCESS;

    return ret;
}

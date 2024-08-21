#ifndef __EECU_SAT_CALIB_H__
#define __EECU_SAT_CALIB_H__

#define          CALIB_VER  0x1
#define CALIB_TYPE_3_POINT  0x1

struct calib_globals {
    double r_0;
    double r_1;
    double r_2;
    double r_acc;
    double r_stab;
    uint64_t r_stab_cnt;
    double r_oob_floor;
    double r_oob_ceil;
    double t_0;
    double t_1;
    double t_2;
};
typedef struct calib_globals cg_t;

struct calib_channel {
    uint8_t calibration_ver;
    uint8_t calibration_type;
    double oob_floor;
    double oob_ceil;
    double midpoint;
    double slope_0;
    double offset_0;
    double slope_1;
    double offset_1;
};
typedef struct calib_channel cc_t;

struct calib_context {
    cg_t globals;
};
typedef struct calib_context calib_context_t;

int calib_read_params_from_file(char *file_name, calib_context_t *ctx);
int calib_extract_from_data_file(char *file_name, calib_context_t *ctx);

#endif

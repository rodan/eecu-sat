#ifndef __EECU_SAT_CALIB_H__
#define __EECU_SAT_CALIB_H__

#define          CALIB_VER  0x1
#define CALIB_TYPE_3_POINT  0x1

#define      CALIB_P0_DONE  0x1
#define      CALIB_P1_DONE  0x2
#define      CALIB_P2_DONE  0x4

#define  CALIB_INI_GLOBALS  0x1
#define  CALIB_INI_CHANNEL  0x2

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
typedef struct calib_globals calib_globals_t;

enum calib_state {
    CALIB_UNSTABLE = 0,
    CALIB_P0,
    CALIB_P1,
    CALIB_P2,
};

struct calib_channel {
    uint16_t id;
    uint8_t calibration_ver;
    uint8_t calibration_type;
    uint8_t checklist;
    double midpoint;
    double slope_0;
    double offset_0;
    double slope_1;
    double offset_1;
    enum calib_state state;
    uint64_t stab_cnt;
    double stab_buff;
    uint64_t oob_floor_cnt;
    uint64_t oob_ceil_cnt;
    double mean_0;
    double mean_1;
    double mean_2;
    ssize_t total_samples;
};
typedef struct calib_channel calib_channel_t;

struct calib_context {
    calib_globals_t globals;
    calib_channel_t channel;
};
typedef struct calib_context calib_context_t;

int calib_read_params_from_file(char *file_name, void *ctx, uint8_t flags);
//int calib_init_from_data_file(char *data_file_name, char *ini_file_name, calib_context_t *ctx);
int calib_init_from_buffer(float *data, ssize_t num_samples, calib_context_t *ctx);

#endif

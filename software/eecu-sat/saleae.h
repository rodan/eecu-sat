#ifndef __SALEAE_H__
#define __SALEAE_H__

#define SALEAE_TYPE_DIGITAL  0
#define  SALEAE_TYPE_ANALOG  1

#define      SALEAE_UNKNOWN  0
#define  SALEAE_UNKNOWN_VER  1
#define       SALEAE_ANALOG  2
#define      SALEAE_DIGITAL  4

// version 0 of the saleae binary header
struct __attribute__((packed)) saleae_dig_bh0 {
    uint8_t identifier[8];
    int32_t version;
    int32_t type;
    uint32_t initial_state;
    double begin_time;
    double end_time;
    uint64_t num_transitions;
}; // 44bytes

struct __attribute__((packed)) saleae_ana_bh0 {
    uint8_t identifier[8];
    int32_t version;
    int32_t type;
    double begin_time;
    uint64_t sample_rate;
    uint64_t downsample;
    uint64_t num_samples;
}; // 48bytes

#define  SALEAE_ANALOG_HDR_SIZE  48

uint8_t saleae_get_hdr_type(uint8_t *data);

#endif

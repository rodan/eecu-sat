#ifndef __SAT_TRIGGER_H__
#define __SAT_TRIGGER_H__

struct trigger_match {
    ssize_t sample_cnt;
};

// last_state 
#define STATE_UNK    0x0
#define STATE_BELOW  0x1
#define STATE_ABOVE  0x2

enum trigger_state {
    TRIGGER_UNK = 0,
    TRIGGER_BELLOW,
    TRIGGER_ABOVE
};

struct trigger_context {
    uint16_t last_state;
    ssize_t cur_sample;
};

struct sat_trigger *sat_trigger_new(const char *name);
void sat_trigger_free(struct sat_trigger *trig);
int sat_trigger_receive(struct sat_trigger *t, struct sr_datafeed_packet *packet_in);
void sat_trigger_show(const struct sat_trigger *t);

#endif

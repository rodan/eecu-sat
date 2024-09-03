#ifndef __SAT_TRIGGER_H__
#define __SAT_TRIGGER_H__

struct sat_trigger *sat_trigger_new(const char *name);
void sat_trigger_free(struct sat_trigger *trig);

#endif

#ifndef __SAT_OUTPUT_H__
#define __SAT_OUTPUT_H__

#include <stdint.h>
#include <sys/types.h>

#include "proj.h"

const struct sr_output_module **sat_output_list(void);
const char *sat_output_id_get(const struct sr_output_module *omod);
const char *sat_output_name_get(const struct sr_output_module *omod);
const char *sat_output_description_get(const struct sr_output_module *omod);
const char *const *sat_output_extensions_get(const struct sr_output_module *omod);
gboolean sat_output_test_flag(const struct sr_output_module *omod, uint64_t flag);
const struct sr_output_module *sat_output_find(char *id);
const struct sr_option **sat_output_options_get(const struct sr_output_module *omod);
void sat_output_options_free(const struct sr_option **options);
const struct sr_output *sat_output_new(const struct sr_output_module *omod,
                                      GHashTable *options, const struct sr_dev_inst *sdi, const char *filename);
int sat_output_send(const struct sr_output *o, const struct sr_datafeed_packet *packet);
int sat_output_free(const struct sr_output *o);

#endif

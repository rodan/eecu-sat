#ifndef __SAT_TRANSFORM_H__
#define __SAT_TRANSFORM_H__

#include <stdint.h>
#include <glib.h>
#include "proj.h"

const struct sr_transform_module **sat_transform_list(void);
const char *sat_transform_id_get(const struct sr_transform_module *tmod);
const char *sat_transform_name_get(const struct sr_transform_module *tmod);
const char *sat_transform_description_get(const struct sr_transform_module *tmod);
const struct sr_transform_module *sat_transform_find(const char *id);
const struct sr_option **sat_transform_options_get(const struct sr_transform_module *tmod);
void sat_transform_options_free(const struct sr_option **options);
const struct sr_transform *sat_transform_new(const struct sr_transform_module *tmod,
		GHashTable *options, const struct sr_dev_inst *sdi);
int sat_transform_free(const struct sr_transform *t);

#endif

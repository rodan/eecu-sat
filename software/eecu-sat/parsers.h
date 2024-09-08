#ifndef __SAT_PARSERS_H__
#define __SAT_PARSERS_H__

#include <glib.h>

int parse_triggerstring(const struct sr_dev_inst *sdi, const char *s,
		struct sat_trigger **trigger);
GHashTable *parse_generic_arg(const char *arg, gboolean sep_first, const char *key_first);
GHashTable *generic_arg_to_opt(const struct sr_option **opts, GHashTable *genargs);
GSList *check_unknown_keys(const struct sr_option **avail, GHashTable *used);
gboolean warn_unknown_keys(const struct sr_option **avail, GHashTable *used,
	const char *caption);

#endif

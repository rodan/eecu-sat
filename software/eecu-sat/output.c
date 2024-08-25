
#include <stdio.h>
#include <string.h>
#include "output.h"
#include "output_analog.h"
#include "output_srzip.h"

static const struct sat_output_module *output_module_list[] = {
	&output_analog,
	&output_srzip,
	NULL,
};

/**
 * Returns a NULL-terminated list of all available output modules.
 *
 * @since 0.4.0
 */
const struct sat_output_module **sat_output_list(void)
{
	return output_module_list;
}

/**
 * Returns the specified output module's ID.
 *
 * @since 0.4.0
 */
const char *sat_output_id_get(const struct sat_output_module *omod)
{
	if (!omod) {
		fprintf(stderr, "Invalid output module NULL!\n");
		return NULL;
	}

	return omod->id;
}

/**
 * Returns the specified output module's name.
 *
 * @since 0.4.0
 */
const char *sat_output_name_get(const struct sat_output_module *omod)
{
	if (!omod) {
		fprintf(stderr, "Invalid output module NULL!\n");
		return NULL;
	}

	return omod->name;
}

/**
 * Returns the specified output module's description.
 *
 * @since 0.4.0
 */
const char *sat_output_description_get(const struct sat_output_module *omod)
{
	if (!omod) {
		fprintf(stderr, "Invalid output module NULL!\n");
		return NULL;
	}

	return omod->desc;
}

/**
 * Returns the specified output module's file extensions typical for the file
 * format, as a NULL terminated array, or returns a NULL pointer if there is
 * no preferred extension.
 * @note these are a suggestions only.
 *
 * @since 0.4.0
 */
const char *const *sat_output_extensions_get(
		const struct sat_output_module *omod)
{
	if (!omod) {
		fprintf(stderr, "Invalid output module NULL!\n");
		return NULL;
	}

	return omod->exts;
}

/**
 * Return the output module with the specified ID, or NULL if no module
 * with that id is found.
 *
 * @since 0.4.0
 */
const struct sat_output_module *sat_output_find(char *id)
{
	int i;

	for (i = 0; output_module_list[i]; i++) {
		if (!strcmp(output_module_list[i]->id, id))
			return output_module_list[i];
	}

	return NULL;
}


#ifndef __EECPU_SAT_OUTPUT_H__
#define __EECPU_SAT_OUTPUT_H__

#include <stdint.h>
#include <sys/types.h>

#include "proj.h"

struct sat_output {
	/** A pointer to this output's module. */
	const struct sat_output_module *module;

	/**
	 * The device for which this output module is creating output. This
	 * can be used by the module to find out channel names and numbers.
	 */
	//const struct sr_dev_inst *sdi;

	/**
	 * The name of the (container) file that the data should be written to.
	 */
	char *filename;

    uint16_t ch;
    uint16_t chunk;

    /**
     * CHUNK_SIZE buffer where received data comes in. allocation is controlled by the output module.
     */
    //uint8_t *buffer;

    /**
     * current size of data inside buffer - must be smaller than CHUNK_SIZE.
     */
    //ssize_t buffer_len;

	/**
	 * A generic pointer which can be used by the module to keep internal
	 * state between calls into its callback functions.
	 *
	 * For example, the module might store a pointer to a chunk of output
	 * there, and only flush it when it reaches a certain size.
	 */
	void *priv;
};

struct sat_output_module {
	/**
	 * A unique ID for this output module, suitable for use in command-line
	 * clients, [a-z0-9-]. Must not be NULL.
	 */
	const char *id;

	/**
	 * A unique name for this output module, suitable for use in GUI
	 * clients, can contain UTF-8. Must not be NULL.
	 */
	const char *name;

	/**
	 * A short description of the output module. Must not be NULL.
	 *
	 * This can be displayed by frontends, e.g. when selecting the output
	 * module for saving a file.
	 */
	const char *desc;

	/**
	 * A NULL terminated array of strings containing a list of file name
	 * extensions typical for the input file format, or NULL if there is
	 * no typical extension for this file format.
	 */
	const char *const *exts;

	/**
	 * This function is called once, at the beginning of an output stream.
	 *
	 * The device struct will be available in the output struct passed in,
	 * as well as the param field -- which may be NULL or an empty string,
	 * if no parameter was passed.
	 *
	 * The module can use this to initialize itself, create a struct for
	 * keeping state and storing it in the <code>internal</code> field.
	 *
	 * @param o Pointer to the respective 'struct sr_output'.
	 *
	 * @retval SR_OK Success
	 * @retval other Negative error code.
	 */
	int (*init) (struct sat_output *o);

	/**
	 * This function is passed a copy of every packet in the data feed.
	 * Any output generated by the output module in response to the
	 * packet should be returned in a newly allocated GString
	 * <code>out</code>, which will be freed by the caller.
	 *
	 * Packets not of interest to the output module can just be ignored,
	 * and the <code>out</code> parameter set to NULL.
	 *
	 * @param o Pointer to the respective 'struct sr_output'.
	 * @param sdi The device instance that generated the packet.
	 * @param packet The complete packet.
	 * @param out A pointer where a GString * should be stored if
	 * the module generates output, or NULL if not.
	 *
	 * @retval SR_OK Success
	 * @retval other Negative error code.
	 */
	int (*receive) (struct sat_output *o, struct sr_datafeed_packet *packet);

	/**
	 * This function is called after the caller is finished using
	 * the output module, and can be used to free any internal
	 * resources the module may keep.
	 *
	 * @retval SR_OK Success
	 * @retval other Negative error code.
	 */
	int (*cleanup) (struct sat_output *o);
};

const struct sat_output_module **sat_output_list(void);
const char *sat_output_id_get(const struct sat_output_module *omod);
const char *sat_output_name_get(const struct sat_output_module *omod);
const char *sat_output_description_get(const struct sat_output_module *omod);
const char *const *sat_output_extensions_get(const struct sat_output_module *omod);
const struct sat_output_module *sat_output_find(char *id);

#endif

#ifndef __SAT_TRANSFORM_H__
#define __SAT_TRANSFORM_H__

#include <stdint.h>
#include <glib.h>
#include "proj.h"

/** Transform module instance. */
struct sat_transform {
    /** A pointer to this transform's module. */
    const struct sat_transform_module *module;

    /**
     * The device for which this transform module is used. This
     * can be used by the module to find out channel names and numbers.
     */
    const struct sr_dev_inst *sdi;

    /**
     * A generic pointer which can be used by the module to keep internal
     * state between calls into its callback functions.
     */
    void *priv;
};

struct sat_transform_module {
    /**
     * A unique ID for this transform module, suitable for use in
     * command-line clients, [a-z0-9-]. Must not be NULL.
     */
    const char *id;

    /**
     * A unique name for this transform module, suitable for use in GUI
     * clients, can contain UTF-8. Must not be NULL.
     */
    const char *name;

    /**
     * A short description of the transform module. Must not be NULL.
     *
     * This can be displayed by frontends, e.g. when selecting
     * which transform module(s) to add.
     */
    const char *desc;

    /**
     * Returns a NULL-terminated list of options this transform module
     * can take. Can be NULL, if the transform module has no options.
     */
    const struct sr_option *(*options) (void);

    /**
     * This function is called once, at the beginning of a stream.
     *
     * @param t Pointer to the respective 'struct sat_transform'.
     * @param options Hash table of options for this transform module.
     *                Can be NULL if no options are to be used.
     *
     * @retval sat_OK Success
     * @retval other Negative error code.
     */
    int (*init)(struct sat_transform * t, GHashTable * options);

    /**
     * This function is passed a pointer to every packet in the data feed.
     *
     * It can either return (in packet_out) a pointer to another packet
     * (possibly the exact same packet it got as input), or NULL.
     *
     * @param t Pointer to the respective 'struct sat_transform'.
     * @param packet_in Pointer to a datafeed packet.
     * @param packet_out Pointer to the resulting datafeed packet after
     *                   this function was run. If NULL, the transform
     *                   module intentionally didn't output a new packet.
     *
     * @retval sat_OK Success
     * @retval other Negative error code.
     */
    int (*receive)(const struct sat_transform * t,
                   struct sr_datafeed_packet * packet_in, struct sr_datafeed_packet ** packet_out);

        /**
     * This function is called after the caller is finished using
     * the transform module, and can be used to free any internal
     * resources the module may keep.
     *
     * @retval sat_OK Success
     * @retval other Negative error code.
     */
    int (*cleanup)(struct sat_transform * t);
};

const struct sat_transform_module **sat_transform_list(void);
const char *sat_transform_id_get(const struct sat_transform_module *tmod);
const char *sat_transform_name_get(const struct sat_transform_module *tmod);
const char *sat_transform_description_get(const struct sat_transform_module *tmod);
const struct sat_transform_module *sat_transform_find(const char *id);
const struct sr_option **sat_transform_options_get(const struct sat_transform_module *tmod);
void sat_transform_options_free(const struct sr_option **options);
const struct sat_transform *sat_transform_new(const struct sat_transform_module *tmod,
		GHashTable *options, const struct sr_dev_inst *sdi);
int sat_transform_free(const struct sat_transform *t);

#endif

/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBSIGROK_LIBSIGROK_INTERNAL_H
#define LIBSIGROK_LIBSIGROK_INTERNAL_H

/* Static definitions of structs ending with an all-zero entry are a
 * problem when compiling with -Wmissing-field-initializers: GCC
 * suppresses the warning only with { 0 }, clang wants { } */
#ifdef __clang__
#define ALL_ZERO { }
#else
#define ALL_ZERO { 0 }
#endif

/** Input module metadata keys. */
enum sr_input_meta_keys {
	/** The input filename, if there is one. */
	SR_INPUT_META_FILENAME = 0x01,
	/** The input file's size in bytes. */
	SR_INPUT_META_FILESIZE = 0x02,
	/** The first 128 bytes of the file, provided as a GString. */
	SR_INPUT_META_HEADER = 0x04,

	/** The module cannot identify a file without this metadata. */
	SR_INPUT_META_REQUIRED = 0x80,
};

/** Input (file) module struct. */
struct sr_input {
	/**
	 * A pointer to this input module's 'struct sr_input_module'.
	 */
	const struct sr_input_module *module;
	GString *buf;
	struct sr_dev_inst *sdi;
	gboolean sdi_ready;
	void *priv;
};

/** Input (file) module driver. */
struct sr_input_module {
    /**
     * A unique ID for this input module, suitable for use in command-line
     * clients, [a-z0-9-]. Must not be NULL.
     */
    const char *id;

    /**
     * A unique name for this input module, suitable for use in GUI
     * clients, can contain UTF-8. Must not be NULL.
     */
    const char *name;

    /**
     * A short description of the input module. Must not be NULL.
     *
     * This can be displayed by frontends, e.g. when selecting the input
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
     * Zero-terminated list of metadata items the module needs to be able
     * to identify an input stream. Can be all-zero, if the module cannot
     * identify streams at all, i.e. has to be forced into use.
     *
     * Each item is one of:
     *   SR_INPUT_META_FILENAME
     *   SR_INPUT_META_FILESIZE
     *   SR_INPUT_META_HEADER
     *
     * If the high bit (SR_INPUT META_REQUIRED) is set, the module cannot
     * identify a stream without the given metadata.
     */
    const uint8_t metadata[8];

    /**
     * Returns a NULL-terminated list of options this module can take.
     * Can be NULL, if the module has no options.
     */
    const struct sr_option *(*options) (void);

    /**
     * Check if this input module can load and parse the specified stream.
     *
     * @param[in] metadata Metadata the module can use to identify the stream.
     * @param[out] confidence "Strength" of the detection.
     *   Specialized handlers can take precedence over generic/basic support.
     *
     * @retval SR_OK This module knows the format.
     * @retval SR_ERR_NA There wasn't enough data for this module to
     *   positively identify the format.
     * @retval SR_ERR_DATA This module knows the format, but cannot handle
     *   it. This means the stream is either corrupt, or indicates a
     *   feature that the module does not support.
     * @retval SR_ERR This module does not know the format.
     *
     * Lower numeric values of 'confidence' mean that the input module
     * stronger believes in its capability to handle this specific format.
     * This way, multiple input modules can claim support for a format,
     * and the application can pick the best match, or try fallbacks
     * in case of errors. This approach also copes with formats that
     * are unreliable to detect in the absence of magic signatures.
     */
    int (*format_match) (GHashTable *metadata, unsigned int *confidence);

    /**
     * Initialize the input module.
     *
     * @retval SR_OK Success
     * @retval other Negative error code.
     */
    int (*init) (struct sr_input *in, GHashTable *options);

    /**
     * Send data to the specified input instance.
     *
     * When an input module instance is created with sr_input_new(), this
     * function is used to feed data to the instance.
     *
     * As enough data gets fed into this function to completely populate
     * the device instance associated with this input instance, this is
     * guaranteed to return the moment it's ready. This gives the caller
     * the chance to examine the device instance, attach session callbacks
     * and so on.
     *
     * @retval SR_OK Success
     * @retval other Negative error code.
     */
    int (*receive) (struct sr_input *in, GString *buf);

    /**
     * Signal the input module no more data will come.
     *
     * This will cause the module to process any data it may have buffered.
     * The SR_DF_END packet will also typically be sent at this time.
     */
    int (*end) (struct sr_input *in);

    /**
     * Reset the input module's input handling structures.
     *
     * Causes the input module to reset its internal state so that we can
     * re-send the input data from the beginning without having to
     * re-create the entire input module.
     *
     * @retval SR_OK Success.
     * @retval other Negative error code.
     */
    int (*reset) (struct sr_input *in);

    /**
     * This function is called after the caller is finished using
     * the input module, and can be used to free any internal
     * resources the module may keep.
     *
     * This function is optional.
     *
     * @retval SR_OK Success
     * @retval other Negative error code.
     */
    void (*cleanup) (struct sr_input *in);
};

/** Output module instance. */
struct sr_output {
        /** A pointer to this output's module. */
    const struct sr_output_module *module;

    /**
     * The device for which this output module is creating output. This
     * can be used by the module to find out channel names and numbers.
     */
    const struct sr_dev_inst *sdi;

    /**
     * The name of the (container) file that the data should be written to.
     */
    char *filename;

    /**
     * A generic pointer which can be used by the module to keep internal
     * state between calls into its callback functions.
     *
     * For example, the module might store a pointer to a chunk of output
     * there, and only flush it when it reaches a certain size.
     */
    void *priv;
};

struct sr_output_module {
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
     * Bitfield containing flags that describe certain properties
     * this output module may or may not have.
     * @see sr_output_flags
     */
    const uint64_t flags;

    /**
     * Returns a NULL-terminated list of options this module can take.
     * Can be NULL, if the module has no options.
     */
    const struct sr_option *(*options) (void);

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
    int (*init)(struct sr_output * o, GHashTable *options);

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
    int (*receive)(const struct sr_output * o, const struct sr_datafeed_packet * packet, GString **out);

    /**
     * This function is called after the caller is finished using
     * the output module, and can be used to free any internal
     * resources the module may keep.
     *
     * @retval SR_OK Success
     * @retval other Negative error code.
     */
    int (*cleanup)(struct sr_output * o);
};

/** Transform module instance. */
struct sr_transform {
    /** A pointer to this transform's module. */
    const struct sr_transform_module *module;

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

struct sr_transform_module {
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
     * @param t Pointer to the respective 'struct sr_transform'.
     * @param options Hash table of options for this transform module.
     *                Can be NULL if no options are to be used.
     *
     * @retval sat_OK Success
     * @retval other Negative error code.
     */
    int (*init)(struct sr_transform * t, GHashTable * options);

    /**
     * This function is passed a pointer to every packet in the data feed.
     *
     * It can either return (in packet_out) a pointer to another packet
     * (possibly the exact same packet it got as input), or NULL.
     *
     * @param t Pointer to the respective 'struct sr_transform'.
     * @param packet_in Pointer to a datafeed packet.
     * @param packet_out Pointer to the resulting datafeed packet after
     *                   this function was run. If NULL, the transform
     *                   module intentionally didn't output a new packet.
     *
     * @retval sat_OK Success
     * @retval other Negative error code.
     */
    int (*receive)(const struct sr_transform * t,
                   struct sr_datafeed_packet * packet_in, struct sr_datafeed_packet ** packet_out);

        /**
     * This function is called after the caller is finished using
     * the transform module, and can be used to free any internal
     * resources the module may keep.
     *
     * @retval sat_OK Success
     * @retval other Negative error code.
     */
    int (*cleanup)(struct sr_transform * t);
};

/** Device instance data */
struct sr_dev_inst {
    /** Device driver. */
    struct sr_dev_driver *driver;
    /** Device instance status. SR_ST_NOT_FOUND, etc. */
    int status;
    /** Device instance type. SR_INST_USB, etc. */
    int inst_type;
    /** Device vendor. */
    char *vendor;
    /** Device model. */
    char *model;
    /** Device version. */
    char *version;
    /** Serial number. */
    char *serial_num;
    /** Connection string to uniquely identify devices. */
    char *connection_id;
    /** List of channels. */
    GSList *channels;
    /** List of sr_channel_group structs */
    GSList *channel_groups;
    /** Device instance connection data (used?) */
    void *conn;
    /** Device instance private data (used?) */
    void *priv;
    /** Session to which this device is currently assigned. */
    struct sr_session *session;
};

#endif

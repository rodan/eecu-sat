#ifndef SIGROK_GLUE_H
#define SIGROK_GLUE_H

#include <stdint.h>
#include <stdbool.h>
/* we use some of the libsigrok structures and macros*/
#include <libsigrok/libsigrok.h>
#include "sigrok_internal.h"

#define                  CHUNK_SIZE  (8 * 1024 * 1024)
#define                 LINE_MAX_SZ  64
#define  DEFAULT_OUTPUT_FORMAT_FILE  "srzip"

#define UNUSED(x) (void)(x)

struct cmdline_opt {
    char *input_prefix;
    char *output_file;
    char *output_format;
    char *transform_module;
    char *triggers;
    bool skip_header;
    uint32_t action;
};

struct ch_data {
    char *input_file_name;
    char *output_file_name;
    //char *channel_name;
    uint16_t id;
    ssize_t input_file_size;
};
typedef struct ch_data ch_data_t;

struct sat_generic_pkt {
    ssize_t payload_sz;
    void *payload;
};

struct dev_frame {
    uint16_t ch;
    uint16_t chunk;
};

#if 0
struct sr_dev_inst {
	/** Device driver. */
	//struct sr_dev_driver *driver;
	/** Device instance status. SR_ST_NOT_FOUND, etc. */
	int status;
	/** Device instance type. SR_INST_USB, etc. */
	//int inst_type;
	/** Device vendor. */
	//char *vendor;
	/** Device model. */
	//char *model;
	/** Device version. */
	//char *version;
	/** Serial number. */
	//char *serial_num;
	/** Connection string to uniquely identify devices. */
	//char *connection_id;
	/** List of channels. */
	GSList *channels;
	/** List of sr_channel_group structs */
	//GSList *channel_groups;
	/** Device instance connection data (used?) */
	//void *conn;
	/** Device instance private data (used?) */
	void *priv;
	/** Session to which this device is currently assigned. */
	//struct sr_session *session;
};
#endif

int maybe_config_set(struct sr_dev_driver *driver,
		const struct sr_dev_inst *sdi, struct sr_channel_group *cg,
		uint32_t key, GVariant *gvar);
int maybe_config_list(struct sr_dev_driver *driver,
		const struct sr_dev_inst *sdi, struct sr_channel_group *cg,
		uint32_t key, GVariant **gvar);

#endif

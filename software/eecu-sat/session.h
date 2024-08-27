#ifndef __SAT_SESSION_H__
#define __SAT_SESSION_H__

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

const struct sat_output *setup_output_format(const struct sr_dev_inst *sdi, char *opt_output_file, char *opt_output_format);
const struct sat_transform *setup_transform_module(const struct sr_dev_inst *sdi, char *mod);

#endif

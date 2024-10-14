#ifndef __SAT_SESSION_H__
#define __SAT_SESSION_H__

const struct sat_output *setup_output_format(const struct sr_dev_inst *sdi, char *opt_output_file, char *opt_output_format);
const struct sat_transform *setup_transform_module(const struct sr_dev_inst *sdi, char *mod);
int run_session(const struct sr_dev_inst *sdi, const struct cmdline_opt *opt);

#endif

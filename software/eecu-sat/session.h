#ifndef __SAT_SESSION_H__
#define __SAT_SESSION_H__

const struct sat_output *setup_output_format(char *opt_output_file, char *opt_output_format);
const struct sat_transform *setup_transform_module(char *mod);

#endif

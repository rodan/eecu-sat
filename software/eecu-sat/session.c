
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "proj.h"
#include "parsers.h"
#include "output.h"
#include "transform.h"

const struct sat_output *setup_output_format(const struct sr_dev_inst *sdi, char *opt_output_file, char *opt_output_format)
{
	const struct sat_output_module *omod;
	const struct sr_option **options;
	const struct sat_output *o;
	GHashTable *fmtargs, *fmtopts;
	char *fmtspec;

	if (!opt_output_format) {
		if (opt_output_file) {
			opt_output_format = DEFAULT_OUTPUT_FORMAT_FILE;
		} else {
			//opt_output_format = DEFAULT_OUTPUT_FORMAT_NOFILE;
		    g_critical("output file not defined, exiting");
		}
	}

	fmtargs = parse_generic_arg(opt_output_format, TRUE, NULL);
	fmtspec = g_hash_table_lookup(fmtargs, "sigrok_key");
	if (!fmtspec)
		g_critical("Invalid output format.");
	if (!(omod = sat_output_find(fmtspec)))
		g_critical("Unknown output module '%s'.", fmtspec);
	g_hash_table_remove(fmtargs, "sigrok_key");
	if ((options = sat_output_options_get(omod))) {
		fmtopts = generic_arg_to_opt(options, fmtargs);
		(void)warn_unknown_keys(options, fmtargs, NULL);
		sat_output_options_free(options);
	} else {
		fmtopts = NULL;
	}
	o = sat_output_new(omod, fmtopts, sdi, opt_output_file);

#if 0
	if (opt_output_file) {
		if (!sat_output_test_flag(omod, SR_OUTPUT_INTERNAL_IO_HANDLING)) {
			*outfile = g_fopen(opt_output_file, "wb");
			if (!*outfile) {
				g_critical("Cannot write to output file '%s'.",
					opt_output_file);
			}
		} else {
			*outfile = NULL;
		}
	} else {
	    g_critical("output file not defined, exiting");
		//setup_binary_stdout();
		//*outfile = stdout;
	}
#endif

	if (fmtopts)
		g_hash_table_destroy(fmtopts);
	g_hash_table_destroy(fmtargs);

	return o;
}

const struct sat_transform *setup_transform_module(const struct sr_dev_inst *sdi, char *opt_transform_module)
{
    const struct sat_transform_module *tmod;
    const struct sr_option **options;
    const struct sat_transform *t;
    GHashTable *fmtargs, *fmtopts;
    char *fmtspec;

    fmtargs = parse_generic_arg(opt_transform_module, TRUE, NULL);
    fmtspec = g_hash_table_lookup(fmtargs, "sigrok_key");
    if (!fmtspec) {
        fprintf(stderr, "Invalid transform module.\n");
        exit(EXIT_FAILURE);
    }
    if (!(tmod = sat_transform_find(fmtspec))) {
        fprintf(stderr, "Unknown transform module '%s'.\n", fmtspec);
        exit(EXIT_FAILURE);
    }
    g_hash_table_remove(fmtargs, "sigrok_key");
    if ((options = sat_transform_options_get(tmod))) {
        fmtopts = generic_arg_to_opt(options, fmtargs);
        (void)warn_unknown_keys(options, fmtargs, NULL);
        sat_transform_options_free(options);
    } else {
        fmtopts = NULL;
    }
    t = sat_transform_new(tmod, fmtopts, sdi);

    if (fmtopts)
        g_hash_table_destroy(fmtopts);
    g_hash_table_destroy(fmtargs);

    return t;
}



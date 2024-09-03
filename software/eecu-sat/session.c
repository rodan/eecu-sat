/*
 * This file is part of the sigrok-cli project.
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "proj.h"
#include "tlpi_hdr.h"
#include "parsers.h"
#include "output.h"
#include "transform.h"
#include "trigger.h"

const struct sr_output *setup_output_format(const struct sr_dev_inst *sdi, char *opt_output_file, char *opt_output_format)
{
	const struct sr_output_module *omod;
	const struct sr_option **options;
	const struct sr_output *o;
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

	if (fmtopts)
		g_hash_table_destroy(fmtopts);
	g_hash_table_destroy(fmtargs);

	return o;
}

const struct sr_transform *setup_transform_module(const struct sr_dev_inst *sdi, char *opt_transform_module)
{
    const struct sr_transform_module *tmod;
    const struct sr_option **options;
    const struct sr_transform *t;
    GHashTable *fmtargs, *fmtopts;
    char *fmtspec;

    fmtargs = parse_generic_arg(opt_transform_module, TRUE, NULL);
    fmtspec = g_hash_table_lookup(fmtargs, "sigrok_key");
    if (!fmtspec) {
        fprintf(stderr, "Invalid transform module.\n");
        exit(SR_ERR_ARG);
    }
    if (!(tmod = sat_transform_find(fmtspec))) {
        fprintf(stderr, "Unknown transform module '%s'.\n", fmtspec);
        exit(SR_ERR_ARG);
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

int run_session(const struct sr_dev_inst *sdi, struct cmdline_opt *opt)
{
    int ret = SR_OK;
    const struct sr_output *o = NULL;
    const struct sr_transform *t = NULL;
    ch_data_t *ch_data_ptr;
    ssize_t read_len;
    int i, j;
    int fd;
    struct sr_datafeed_packet pkt = { 0 };
    struct sr_datafeed_packet *tpkt;
    struct sr_datafeed_analog analog = { 0 };
    struct sr_analog_encoding encoding = { 0 };
    struct sr_analog_meaning meaning = { 0 };
    struct sr_analog_spec spec = { 0 };
    struct sat_trigger *trigger = NULL;
    bool transform_initialized = 0;
    GSList *l;
    struct dev_frame *frame = sdi->priv;

    analog.encoding = &encoding;
    analog.meaning = &meaning;
    analog.spec = &spec;

    if (!opt->output_format) {
        fprintf(stderr, "output format not selected\n");
        return SR_ERR_ARG;
    }

    if (opt->output_file) {
        if (!(o = setup_output_format(sdi, opt->output_file, opt->output_format))) {
            fprintf(stderr, "Failed to initialize transform module.\n");
            return SR_ERR_ARG;
        }
    } else {
        fprintf(stderr, "output file not defined, exiting.\n");
        return SR_ERR_ARG;
    }

	if (opt->triggers) {
		if (!parse_triggerstring(sdi, opt->triggers, &trigger)) {
            fprintf(stderr, "Failed to initialize trigger module.\n");
			return SR_ERR_ARG;
		}
	}

    if (opt->transform_module) {
        if (!(t = setup_transform_module(sdi, opt->transform_module))) {
            fprintf(stderr, "Failed to initialize transform module.\n");
            return SR_ERR_ARG;
        }
        transform_initialized = 1;
    }

    analog.data = (uint8_t *) calloc(CHUNK_SIZE, 1);
    if (!analog.data) {
        errMsg("during calloc");
        ret = SR_ERR_MALLOC;
        goto cleanup;
    }

    i = 0;
    pkt.payload = &analog;

    // send data to trigger module
    if (trigger) {
        for (l = sdi->channels; l; l = l->next) {
            ch_data_ptr = l->data;
            if (ch_data_ptr->trigger) {
                if ((fd = open(ch_data_ptr->input_file_name, O_RDONLY)) < 0) {
                    errMsg("opening input file");
                    ret = SR_ERR_IO;
                    goto cleanup;
                }

                // skip saleae header if present
                if (read(fd, analog.data, 8) != 8) {
                    errMsg("during read()");
                    close(fd);
                    ret = SR_ERR_IO;
                    goto cleanup;
                }
                if (saleae_magic_is_present(analog.data)) {
                    if (lseek(fd, SALEAE_HEADER_SZ, SEEK_SET) < 0) {
                        errMsg("during lseek()");
                        close(fd);
                        ret = SR_ERR_IO;
                        goto cleanup;
                    }
                } else {
                    if (lseek(fd, 0x0, SEEK_SET) < 0) {
                        errMsg("during lseek()");
                        close(fd);
                        ret = SR_ERR_IO;
                        goto cleanup;
                    }
                }

                while ((read_len = read(fd, analog.data, CHUNK_SIZE)) > 0) {
                    pkt.type = SR_DF_ANALOG;
                    analog.num_samples = read_len / sizeof(float);
                    sat_trigger_receive(ch_data_ptr->trigger, &pkt);
                }
                close(fd);

                sat_trigger_show(trigger);
            }
        }
        //goto cleanup;
    }

    // send data to transform and output modules
    for (l = sdi->channels; l; l = l->next) {
        ch_data_ptr = l->data;
        i++;

        if ((fd = open(ch_data_ptr->input_file_name, O_RDONLY)) < 0) {
            errMsg("opening input file");
            ret = SR_ERR_IO;
            goto cleanup;
        }

        // skip saleae header if present
        if (read(fd, analog.data, 8) != 8) {
            errMsg("during read()");
            close(fd);
            ret = SR_ERR_IO;
            goto cleanup;
        }
        if (saleae_magic_is_present(analog.data)) {
            if (lseek(fd, SALEAE_HEADER_SZ, SEEK_SET) < 0) {
                errMsg("during lseek()");
                close(fd);
                ret = SR_ERR_IO;
                goto cleanup;
            }
        } else {
            if (lseek(fd, 0x0, SEEK_SET) < 0) {
                errMsg("during lseek()");
                close(fd);
                ret = SR_ERR_IO;
                goto cleanup;
            }
        }

        j = 1;
        while ((read_len = read(fd, analog.data, CHUNK_SIZE)) > 0) {
            frame->ch = i;
            frame->chunk = j;
            if (j == 1) {
                pkt.type = SR_DF_FRAME_BEGIN;
                if (transform_initialized)
                    t->module->receive(t, &pkt, &tpkt);
                if (o->module->receive(o, &pkt, NULL) != SR_OK) {
                    goto cleanup;
                }
            }
            pkt.type = SR_DF_ANALOG;
            analog.num_samples = read_len / sizeof(float);
            if (transform_initialized) {
                t->module->receive(t, &pkt, &tpkt);
                if (o->module->receive(o, tpkt, NULL) != SR_OK) {
                    goto cleanup;
                }
            } else {
                if (o->module->receive(o, &pkt, NULL) != SR_OK) {
                    goto cleanup;
                }
            }
            j++;
        }
        close(fd);

        pkt.type = SR_DF_FRAME_END;
        if (transform_initialized) {
            t->module->receive(t, &pkt, &tpkt);
        }
        o->module->receive(o, &pkt, NULL);
    }

    //printf("%d channels exported\n", i);

 cleanup:
    if (o)
        sat_output_free(o);
    if (t)
        sat_transform_free(t);
    if (analog.data)
        free(analog.data);

    return ret;
}

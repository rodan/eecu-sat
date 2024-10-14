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
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "proj.h"
#include "error.h"
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
		    err_msg("%s:%d output file not defined, exiting", __FILE__, __LINE__);
            return NULL;
		}
	}

	fmtargs = parse_generic_arg(opt_output_format, TRUE, NULL);
	fmtspec = g_hash_table_lookup(fmtargs, "sigrok_key");
	if (!fmtspec) {
		err_msg("%s:%d Invalid output format.", __FILE__, __LINE__);
        return NULL;
    }
	if (!(omod = sat_output_find(fmtspec))) {
		err_msg("%s:%d Unknown output module '%s'.", __FILE__, __LINE__, fmtspec);
        return NULL;
    }
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
        err_msg("%s:%d Invalid transform module.", __FILE__, __LINE__);
        return NULL;
    }
    if (!(tmod = sat_transform_find(fmtspec))) {
        err_msg("%s:%d Unknown transform module '%s'.", __FILE__, __LINE__, fmtspec);
        return NULL;
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

int run_session(const struct sr_dev_inst *sdi, const struct cmdline_opt *opt)
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
    ssize_t after_trigger = 0;
    ssize_t before_trigger = 0;
    ssize_t seek = 0;
    ssize_t trigger_at_sample = 0;
    ssize_t bytes_remaining = 0;
    ssize_t samples_remaining = 0;

    analog.encoding = &encoding;
    analog.meaning = &meaning;
    analog.spec = &spec;

    if (!opt->output_format) {
        err_msg("%s:%d output format not selected", __FILE__, __LINE__);
        return SR_ERR_ARG;
    }

    if (opt->output_file) {
        if (!(o = setup_output_format(sdi, opt->output_file, opt->output_format))) {
            err_msg("%s:%d Failed to initialize output module.", __FILE__, __LINE__);
            return SR_ERR_ARG;
        }
    } else {
        err_msg("%s:%d output file not defined", __FILE__, __LINE__);
        return SR_ERR_ARG;
    }

	if (opt->triggers) {
		if (!parse_triggerstring(sdi, opt->triggers, &trigger)) {
            err_msg("%s:%d Failed to initialize trigger module", __FILE__, __LINE__);
			return SR_ERR_ARG;
		}
        after_trigger = trigger->a;
        before_trigger = trigger->b;
	}

    if (opt->transform_module) {
        if (!(t = setup_transform_module(sdi, opt->transform_module))) {
            err_msg("%s:%d Failed to initialize transform module", __FILE__, __LINE__);
            return SR_ERR_ARG;
        }
        transform_initialized = 1;
    }

    analog.data = (uint8_t *) g_malloc0(CHUNK_SIZE);

    i = 0;
    pkt.payload = &analog;

    // send data to trigger module
    if (trigger) {
        for (l = sdi->channels; l; l = l->next) {
            ch_data_ptr = l->data;
            if (ch_data_ptr->trigger) {
                if ((fd = open(ch_data_ptr->input_file_name, O_RDONLY)) < 0) {
                    err_msg("%s:%d Failed to open input file", __FILE__, __LINE__);
                    ret = SR_ERR_IO;
                    goto cleanup;
                }

                seek = 0;

                if (ch_data_ptr->file_type == SALEAE_ANALOG)
                    seek += SALEAE_ANALOG_HDR_SIZE;

                if (lseek(fd, seek, SEEK_SET) < 0) {
                    err_msg("%s:%d failed during lseek()", __FILE__, __LINE__);
                    close(fd);
                    ret = SR_ERR_IO;
                    goto cleanup;
                }

                while ((read_len = read(fd, analog.data, CHUNK_SIZE)) > 0) {
                    pkt.type = SR_DF_ANALOG;
                    analog.num_samples = read_len / ch_data_ptr->sample_size;
                    sat_trigger_receive(ch_data_ptr->trigger, &pkt);
                }
                close(fd);

                if (! trigger->matches) {
                    fprintf(stdout, "warning, trigger #%d '%s' did not activate\n", trigger->id, trigger->name);
                }
                //sat_trigger_show(trigger);
            }
        }
        //goto cleanup;
    }

    // send data to transform and output modules
    for (l = sdi->channels; l; l = l->next) {
        ch_data_ptr = l->data;
        i++;

        if ((fd = open(ch_data_ptr->input_file_name, O_RDONLY)) < 0) {
            err_msg("%s:%d failed to open input file", __FILE__, __LINE__);
            ret = SR_ERR_IO;
            goto cleanup;
        }

        // crop based on after,before_trigger
        seek = 0;
        if (before_trigger && sat_trigger_activated(trigger)) {
            trigger_at_sample = sat_trigger_loc(trigger);

            if (before_trigger > trigger_at_sample) {
                err_msg("warning: cannot read %ld samples before trigger %d at sample %ld,\n", before_trigger, trigger->id, trigger_at_sample);
                err_msg("cropping will begin at sample #0 instead!\n");
                samples_remaining = trigger_at_sample;
            } else {
                samples_remaining = before_trigger;
                seek = (trigger_at_sample - before_trigger) * ch_data_ptr->sample_size;
                //printf("seek to %ld - %ld\n", trigger_at_sample, before_trigger);
            }

            if (trigger_at_sample + after_trigger > ch_data_ptr->sample_count) {
                err_msg("warning: cannot read %ld samples after trigger %d at sample %ld/%ld,\n", after_trigger, trigger->id, trigger_at_sample, ch_data_ptr->sample_count);
                err_msg("cropping will end at sample %ld instead!\n", ch_data_ptr->sample_count);
                samples_remaining += ch_data_ptr->sample_count - trigger_at_sample;
            } else {
                samples_remaining += after_trigger;
            }

            bytes_remaining = samples_remaining * ch_data_ptr->sample_size;
            //printf("attempting to save %ld samples after sample %ld\n", after_trigger, trigger_at_sample);
        } else {
            bytes_remaining = ch_data_ptr->input_file_size;
        }

        // skip saleae header if present
        if (ch_data_ptr->file_type == SALEAE_ANALOG)
            seek += SALEAE_ANALOG_HDR_SIZE;

        if (lseek(fd, seek, SEEK_SET) < 0) {
            err_msg("%s:%d failed during lseek()", __FILE__, __LINE__);
            close(fd);
            ret = SR_ERR_IO;
            goto cleanup;
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
                    close(fd);
                    goto cleanup;
                }
            }
            pkt.type = SR_DF_ANALOG;
            if (read_len > bytes_remaining)
                read_len = bytes_remaining;
            analog.num_samples = read_len / ch_data_ptr->sample_size;
            if (transform_initialized) {
                t->module->receive(t, &pkt, &tpkt);
                if (o->module->receive(o, tpkt, NULL) != SR_OK) {
                    close(fd);
                    goto cleanup;
                }
            } else {
                if (o->module->receive(o, &pkt, NULL) != SR_OK) {
                    close(fd);
                    goto cleanup;
                }
            }
            j++;
            bytes_remaining -= read_len;
            //printf("bytes_remaining %ld\n", bytes_remaining);
            if (bytes_remaining <= 0)
                break;
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
        g_free(analog.data);

    return ret;
}

/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
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

#include <stdio.h>
#include <string.h>
#include "output.h"
#include "output_analog.h"
#include "output_srzip.h"
#include "output_calibrate_linear_3p.h"

static const struct sr_output_module *output_module_list[] = {
    &output_analog,
    &output_srzip,
    &output_calibrate_linear_3p,
    NULL,
};

/**
 * Returns a NULL-terminated list of all available output modules.
 */
const struct sr_output_module **sat_output_list(void)
{
    return output_module_list;
}

/**
 * Returns the specified output module's ID.
 */
const char *sat_output_id_get(const struct sr_output_module *omod)
{
    if (!omod) {
        fprintf(stderr, "Invalid output module NULL!\n");
        return NULL;
    }

    return omod->id;
}

/**
 * Returns the specified output module's name.
 */
const char *sat_output_name_get(const struct sr_output_module *omod)
{
    if (!omod) {
        fprintf(stderr, "Invalid output module NULL!\n");
        return NULL;
    }

    return omod->name;
}

/**
 * Returns the specified output module's description.
 */
const char *sat_output_description_get(const struct sr_output_module *omod)
{
    if (!omod) {
        fprintf(stderr, "Invalid output module NULL!\n");
        return NULL;
    }

    return omod->desc;
}

/**
 * Returns the specified output module's file extensions typical for the file
 * format, as a NULL terminated array, or returns a NULL pointer if there is
 * no preferred extension.
 * @note these are a suggestions only.
 */
const char *const *sat_output_extensions_get(const struct sr_output_module *omod)
{
    if (!omod) {
        fprintf(stderr, "Invalid output module NULL!\n");
        return NULL;
    }

    return omod->exts;
}

/*
 * Checks whether a given flag is set.
 *
 * @see sat_output_flag
 */
gboolean sat_output_test_flag(const struct sr_output_module *omod, uint64_t flag)
{
    return (flag & omod->flags);
}

/**
 * Return the output module with the specified ID, or NULL if no module
 * with that id is found.
 */
const struct sr_output_module *sat_output_find(char *id)
{
    int i;

    for (i = 0; output_module_list[i]; i++) {
        if (!strcmp(output_module_list[i]->id, id))
            return output_module_list[i];
    }

    return NULL;
}

/**
 * Returns a NULL-terminated array of struct sr_option, or NULL if the
 * module takes no options.
 *
 * Each call to this function must be followed by a call to
 * sat_output_options_free().
 */
const struct sr_option **sat_output_options_get(const struct sr_output_module *omod)
{
    const struct sr_option *mod_opts, **opts;
    int size, i;

    if (!omod || !omod->options)
        return NULL;

    mod_opts = omod->options();

    for (size = 0; mod_opts[size].id; size++) ;
    opts = g_malloc((size + 1) * sizeof(struct sr_option *));

    for (i = 0; i < size; i++)
        opts[i] = &mod_opts[i];
    opts[i] = NULL;

    return opts;
}

/**
 * After a call to sat_output_options_get(), this function cleans up all
 * resources returned by that call.
 */
void sat_output_options_free(const struct sr_option **options)
{
    int i;

    if (!options)
        return;

    for (i = 0; options[i]; i++) {
        if (options[i]->def) {
            g_variant_unref(options[i]->def);
            ((struct sr_option *)options[i])->def = NULL;
        }

        if (options[i]->values) {
            g_slist_free_full(options[i]->values, (GDestroyNotify) g_variant_unref);
            ((struct sr_option *)options[i])->values = NULL;
        }
    }
    g_free(options);
}

/**
 * Create a new output instance using the specified output module.
 *
 * <code>options</code> is a *HashTable with the keys corresponding with
 * the module options' <code>id</code> field. The values should be GVariant
 * pointers with sunk * references, of the same GVariantType as the option's
 * default value.
 *
 */
const struct sr_output *sat_output_new(const struct sr_output_module *omod,
       GHashTable *options, const struct sr_dev_inst *sdi, const char *filename)
{
    struct sr_output *op;
    const struct sr_option *mod_opts;
    const GVariantType *gvt;
    GHashTable *new_opts;
    GHashTableIter iter;
    gpointer key, value;
    int i;

    op = g_malloc(sizeof(struct sr_output));
    op->module = omod;
    op->sdi = sdi;
    op->filename = g_strdup(filename);

    new_opts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
    if (omod->options) {
        mod_opts = omod->options();
        for (i = 0; mod_opts[i].id; i++) {
            if (options && g_hash_table_lookup_extended(options, mod_opts[i].id, &key, &value)) {
                /* Pass option along. */
                gvt = g_variant_get_type(mod_opts[i].def);
                if (!g_variant_is_of_type(value, gvt)) {
                    fprintf(stderr, "Invalid type for '%s' option.", (char *)key);
                    g_free(op);
                    return NULL;
                }
                g_hash_table_insert(new_opts, g_strdup(mod_opts[i].id), g_variant_ref(value));
            } else {
                /* Option not given: insert the default value. */
                g_hash_table_insert(new_opts, g_strdup(mod_opts[i].id), g_variant_ref(mod_opts[i].def));
            }
        }

        /* Make sure no invalid options were given. */
        if (options) {
            g_hash_table_iter_init(&iter, options);
            while (g_hash_table_iter_next(&iter, &key, &value)) {
                if (!g_hash_table_lookup(new_opts, key)) {
                    fprintf(stderr, "Output module '%s' has no option '%s'", omod->id, (char *)key);
                    g_hash_table_destroy(new_opts);
                    g_free(op);
                    return NULL;
                }
            }
        }
    }

    if (op->module->init && op->module->init(op, new_opts) != SR_OK) {
        g_free(op);
        op = NULL;
    }
    if (new_opts)
        g_hash_table_destroy(new_opts);

    return op;
}

/**
 * Send a packet to the specified output instance.
 */
int sat_output_send(const struct sr_output *o, const struct sr_datafeed_packet *packet)
{
    return o->module->receive(o, packet, NULL);
}

/**
 * Free the specified output instance and all associated resources.
 */
int sat_output_free(const struct sr_output *o)
{
    int ret;

    if (!o)
        return SR_ERR_ARG;

    ret = SR_OK;
    if (o->module->cleanup)
        ret = o->module->cleanup((struct sr_output *)o);
    g_free((char *)o->filename);
    g_free((gpointer) o);

    return ret;
}

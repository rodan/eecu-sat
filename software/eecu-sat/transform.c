
#include <stdio.h>
#include <string.h>
#include "transform.h"
#include "transform_calibrate_linear_3p.h"

static const struct sat_transform_module *transform_module_list[] = {
    &transform_calibrate_linear_3p,
    NULL,
};

/**
 * Returns a NULL-terminated list of all available transform modules.
 */
const struct sat_transform_module **sat_transform_list(void)
{
    return transform_module_list;
}

/**
 * Returns the specified transform module's ID.
 */
const char *sat_transform_id_get(const struct sat_transform_module *tmod)
{
    if (!tmod) {
        fprintf(stderr, "Invalid transform module NULL!\n");
        return NULL;
    }

    return tmod->id;
}

/**
 * Returns the specified transform module's name.
 */
const char *sat_transform_name_get(const struct sat_transform_module *tmod)
{
    if (!tmod) {
        fprintf(stderr, "Invalid transform module NULL!\n");
        return NULL;
    }

    return tmod->name;
}

/**
 * Returns the specified transform module's description.
 */
const char *sat_transform_description_get(const struct sat_transform_module *tmod)
{
    if (!tmod) {
        fprintf(stderr, "Invalid transform module NULL!\n");
        return NULL;
    }

    return tmod->desc;
}

/**
 * Return the transform module with the specified ID, or NULL if no module
 * with that ID is found.
 */
const struct sat_transform_module *sat_transform_find(const char *id)
{
    int i;

    for (i = 0; transform_module_list[i]; i++) {
        if (!strcmp(transform_module_list[i]->id, id))
            return transform_module_list[i];
    }

    return NULL;
}

/**
 * Returns a NULL-terminated array of struct sr_option, or NULL if the
 * module takes no options.
 *
 * Each call to this function must be followed by a call to
 * sat_transform_options_free().
 *
 * @since 0.4.0
 */
const struct sr_option **sat_transform_options_get(const struct sat_transform_module *tmod)
{
    const struct sr_option *mod_opts, **opts;
    int size, i;

    if (!tmod || !tmod->options)
        return NULL;

    mod_opts = tmod->options();

    for (size = 0; mod_opts[size].id; size++) ;
    opts = g_malloc((size + 1) * sizeof(struct sr_option *));

    for (i = 0; i < size; i++)
        opts[i] = &mod_opts[i];
    opts[i] = NULL;

    return opts;
}

/**
 * After a call to sr_transform_options_get(), this function cleans up all
 * resources returned by that call.
 *
 * @since 0.4.0
 */
void sat_transform_options_free(const struct sr_option **options)
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
 * Create a new transform instance using the specified transform module.
 *
 * <code>options</code> is a *GHashTable with the keys corresponding with
 * the module options' <code>id</code> field. The values should be GVariant
 * pointers with sunk * references, of the same GVariantType as the option's
 * default value.
 *
 * The sr_dev_inst passed in can be used by the instance to determine
 * channel names, samplerate, and so on.
 *
 * @since 0.4.0
 */
const struct sat_transform *sat_transform_new(const struct sat_transform_module *tmod, GHashTable *options)     //, const struct sr_dev_inst *sdi)
{
    struct sat_transform *t;
    const struct sr_option *mod_opts;
    const GVariantType *gvt;
    GHashTable *new_opts;
    GHashTableIter iter;
    gpointer key, value;
    int i;

    t = g_malloc(sizeof(struct sat_transform));
    t->module = tmod;
    //t->sdi = sdi;

    new_opts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
    if (tmod->options) {
        mod_opts = tmod->options();
        for (i = 0; mod_opts[i].id; i++) {
            if (options && g_hash_table_lookup_extended(options, mod_opts[i].id, &key, &value)) {
                /* Pass option along. */
                gvt = g_variant_get_type(mod_opts[i].def);
                if (!g_variant_is_of_type(value, gvt)) {
                    fprintf(stderr, "Invalid type for '%s' option.\n", (char *)key);
                    g_free(t);
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
                    fprintf(stderr, "Transform module '%s' has no option '%s'.\n", tmod->id, (char *)key);
                    g_hash_table_destroy(new_opts);
                    g_free(t);
                    return NULL;
                }
            }
        }
    }

    if (t->module->init && t->module->init(t, new_opts) != SR_OK) {
        g_free(t);
        t = NULL;
    }
    if (new_opts)
        g_hash_table_destroy(new_opts);

    /* Add the transform to the session's list of transforms. */
    //sdi->session->transforms = g_slist_append(sdi->session->transforms, t);

    return t;
}

/**
 * Free the specified transform instance and all associated resources.
 *
 * @since 0.4.0
 */
int sat_transform_free(const struct sat_transform *t)
{
    int ret;

    if (!t)
        return SR_ERR_ARG;

    ret = SR_OK;
    if (t->module->cleanup)
        ret = t->module->cleanup((struct sat_transform *)t);
    g_free((gpointer) t);

    return ret;
}

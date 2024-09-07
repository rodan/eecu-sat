/*
 * This file is part of the sigrok-cli project.
 *
 * Copyright (C) 2011 Bert Vermeulen <bert@biot.com>
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "proj.h"
#include "trigger.h"
#include "parsers.h"

bool saleae_magic_is_present(uint8_t *data)
{
    // 00000000  3c 53 41 4c 45 41 45 3e  00 00 00 00 01 00 00 00  |<SALEAE>........|
    static const char saleae_magic[8] = {0x3c, 0x53, 0x41, 0x4c, 0x45, 0x41, 0x45, 0x3e};

    if (memcmp(data, saleae_magic, 8) == 0)
        return true;
    return false;
}

int parse_trigger_match(char c)
{
    int match;

    if (c == '0')
        match = SR_TRIGGER_ZERO;
    else if (c == '1')
        match = SR_TRIGGER_ONE;
    else if (c == 'r')
        match = SR_TRIGGER_RISING;
    else if (c == 'f')
        match = SR_TRIGGER_FALLING;
    else if (c == 'e')
        match = SR_TRIGGER_EDGE;
    else if (c == 'o')
        match = SR_TRIGGER_OVER;
    else if (c == 'u')
        match = SR_TRIGGER_UNDER;
    else
        match = 0;

    return match;
}

/*
 * ch=FILENAME:name=STR:type=CHAR:level=FLOAT:nth=INT:a=INT:b=INT
 * ch - the channel filename this trigger follows (basename !)
 * name - a name for this trigger, can be NULL
 * type - for digital signals it can be  (NOT IMPLEMENTED YET)
 *           0 - low level trigger
 *           1 - high signal trigger
 *           r - rising signal
 *           f - falling signal
 *           e - edge triggerring (either rising or falling)
 *      - for analog signals
 *           o - signal rising above the level defined in 'level'
 *           u - signal falling below the level defined in 'level'
 * level - triger voltage used by 'o' and 'u' analog triggering - see above
 * nth - stop after trigger was activated nth times
 * a INT - crop signal INT samples after nth trigger
 * b INT - crop signal INT samples before nth trigger
 */
int parse_triggerstring(const struct sr_dev_inst *sdi, const char *s,
        struct sat_trigger **trigger)
{
    gboolean error = true;
    ch_data_t *ch;
    GSList *l, *channels;
    int i;
    char **tokens, *sep;
    char *val;
    uint16_t id=0;
    //struct sat_trigger **found_trigger = NULL;

    channels = sr_dev_inst_channels_get(sdi);

    *trigger = sat_trigger_new(NULL);
    (*trigger)->id = id;
    (*trigger)->nth = 1;
    error = FALSE;
    tokens = g_strsplit(s, ":", -1);
    for (i = 0; tokens[i]; i++) {
        if (!(sep = strchr(tokens[i], '='))) {
            fprintf(stderr, "Invalid trigger '%s'.", tokens[i]);
            error = TRUE;
            break;
        }
        if (strstr(tokens[i], "ch=") == tokens[i]) {
            val = tokens[i] + strlen("ch=");
            ch = NULL;
            for (l = channels; l; l = l->next) {
                ch = l->data;
                if (strstr(ch->input_file_name, val)) {
                    ch->trigger = *trigger;
                    //found_trigger = &ch->trigger;
                    break;
                }
                ch = NULL;
            }
            if (!ch) {
                fprintf(stderr, "Invalid channel '%s'.", tokens[i]);
                error = TRUE;
                break;
            }
        } else if (strstr(tokens[i], "name=") == tokens[i]) {
            (*trigger)->name = g_strdup(tokens[i] + strlen("name="));
        } else if (strstr(tokens[i], "type=") == tokens[i]) {
            val = tokens[i] + strlen("type=");
            (*trigger)->type = parse_trigger_match(val[0]);
            if (!(*trigger)->type) {
                fprintf(stderr, "Invalid trigger type '%s'.", tokens[i]);
                error = TRUE;
                break;
            }
        } else if (strstr(tokens[i], "level=") == tokens[i]) {
            (*trigger)->level = atof(tokens[i] + strlen("level="));
        } else if (strstr(tokens[i], "nth=") == tokens[i]) {
            (*trigger)->nth = atoi(tokens[i] + strlen("nth="));
        } else if (strstr(tokens[i], "a=") == tokens[i]) {
            (*trigger)->a = atoi(tokens[i] + strlen("a="));
        } else if (strstr(tokens[i], "b=") == tokens[i]) {
            (*trigger)->b = atoi(tokens[i] + strlen("b="));
        } else {
            fprintf(stderr, "Invalid trigger '%s'.", tokens[i]);
            error = TRUE;
            break;
        }
    }
    g_strfreev(tokens);

    printf("trigger id=%d name=%s type=%d level=%f nth=%ld\n", (*trigger)->id, (*trigger)->name, (*trigger)->type, (*trigger)->level, (*trigger)->nth);

    if (error) {
        sat_trigger_free(*trigger);
        channels = sr_dev_inst_channels_get(sdi);
        for (l = channels; l; l = l->next) {
            ch = l->data;
            ch->trigger = NULL;
        }
    }

    return !error;
}

/**
 * Split an input text into a key and value respectively ('=' separator).
 *
 * @param[in] text Writeable copy of the input text, gets modified.
 * @param[out] key Position of the keyword.
 * @param[out] val Position of the value.
 *
 * TODO In theory the returned key/value locations could be const pointers.
 * Which even would be preferrable. Unfortunately most call sites deal with
 * glib hashes, and their insert API seriously lacks the const attribute.
 * So we drop it here as well to avoid clutter at callers'.
 */
static void split_key_value(char *text, char **key, char **val)
{
    char *k, *v;
    char *pos;

    if (key)
        *key = NULL;
    if (val)
        *val = NULL;
    if (!text || !*text)
        return;

    k = text;
    v = NULL;
    pos = strchr(k, '=');
    if (pos) {
        *pos = '\0';
        v = ++pos;
    }
    if (key)
        *key = k;
    if (val)
        *val = v;
}

/**
 * Create hash table from colon separated key-value pairs input text.
 *
 * Accepts input text as it was specified by users. Splits the colon
 * separated key-value pairs and creates a hash table from these items.
 * Optionally supports special forms which are useful for different CLI
 * features.
 *
 * Typical form: <key>=<val>[:<key>=<val>]*
 * Generic list of key-value pairs, all items being equal. Mere set.
 *
 * ID form: <id>[:<key>=<val>]*
 * First item is not a key-value pair, instead it's an identifier. Used
 * to specify a protocol decoder, or a device driver, or an input/output
 * file format, optionally followed by more parameters' values. The ID
 * part of the input spec is not optional.
 *
 * Optional ID: [<sel>=<id>][:<key>=<val>]*
 * All items are key-value pairs. The first item _may_ be an identifier,
 * if its key matches a caller specified key name. Otherwise the input
 * text is the above typical form, a mere list of key-value pairs while
 * none of them is special.
 *
 * @param[in] arg Input text.
 * @param[in] sep_first Boolean, whether ID form is required.
 * @param[in] key_first Keyword name if optional ID is applicable.
 *
 * @returns A hash table which contains the key/value pairs, or #NULL
 *   when the input is invalid.
 */
GHashTable *parse_generic_arg(const char *arg, gboolean sep_first, const char *key_first)
{
    GHashTable *hash;
    char **elements;
    int i;
    char *k, *v;

    if (!arg || !arg[0])
        return NULL;
    if (key_first && !key_first[0])
        key_first = NULL;

    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    elements = g_strsplit(arg, ":", 0);
    i = 0;
    if (sep_first) {
        k = g_strdup("sigrok_key");
        v = g_strdup(elements[i++]);
        g_hash_table_insert(hash, k, v);
    } else if (key_first) {
        split_key_value(elements[i], &k, &v);
        if (g_ascii_strcasecmp(k, key_first) == 0) {
            k = "sigrok_key";
        }
        k = g_strdup(k);
        v = g_strdup(v);
        g_hash_table_insert(hash, k, v);
        i++;
    }
    for (; elements[i]; i++) {
        if (!elements[i][0])
            continue;
        split_key_value(elements[i], &k, &v);
        k = g_strdup(k);
        v = v ? g_strdup(v) : NULL;
        g_hash_table_insert(hash, k, v);
    }
    g_strfreev(elements);

    return hash;
}

GSList *check_unknown_keys(const struct sr_option **avail, GHashTable *used)
{
    GSList *unknown;
    GHashTableIter iter;
    void *key;
    const char *used_id;
    size_t avail_idx;
    const char *avail_id, *found_id;

    /* Collect a list of used but not available keywords. */
    unknown = NULL;
    g_hash_table_iter_init(&iter, used);
    while (g_hash_table_iter_next(&iter, &key, NULL)) {
        used_id = key;
        found_id = NULL;
        for (avail_idx = 0; avail[avail_idx] && avail[avail_idx]->id; avail_idx++) {
            avail_id = avail[avail_idx]->id;
            if (strcmp(avail_id, used_id) == 0) {
                found_id = avail_id;
                break;
            }
        }
        if (!found_id)
            unknown = g_slist_append(unknown, g_strdup(used_id));
    }

    /* Return the list of unknown keywords, or NULL if empty. */
    return unknown;
}

gboolean warn_unknown_keys(const struct sr_option **avail, GHashTable *used,
    const char *caption)
{
    GSList *unknown, *l;
    gboolean had_unknown;
    const char *s;

    if (!caption || !*caption)
        caption = "Unknown keyword";

    unknown = check_unknown_keys(avail, used);
    had_unknown = unknown != NULL;
    for (l = unknown; l; l = l->next) {
        s = l->data;
        g_warning("%s: %s.", caption, s);
    }
    g_slist_free_full(unknown, g_free);

    return had_unknown;
}

GHashTable *generic_arg_to_opt(const struct sr_option **opts, GHashTable *genargs)
{
    GHashTable *hash;
    GVariant *gvar;
    int i;
    char *s;
    gboolean b;

    hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            (GDestroyNotify)g_variant_unref);
    for (i = 0; opts[i]; i++) {
        if (!(s = g_hash_table_lookup(genargs, opts[i]->id)))
            continue;
        if (g_variant_is_of_type(opts[i]->def, G_VARIANT_TYPE_UINT32)) {
            gvar = g_variant_new_uint32(strtoul(s, NULL, 10));
            g_hash_table_insert(hash, g_strdup(opts[i]->id),
                    g_variant_ref_sink(gvar));
        } else if (g_variant_is_of_type(opts[i]->def, G_VARIANT_TYPE_INT32)) {
            gvar = g_variant_new_int32(strtol(s, NULL, 10));
            g_hash_table_insert(hash, g_strdup(opts[i]->id),
                    g_variant_ref_sink(gvar));
        } else if (g_variant_is_of_type(opts[i]->def, G_VARIANT_TYPE_UINT64)) {
            gvar = g_variant_new_uint64(strtoull(s, NULL, 10));
            g_hash_table_insert(hash, g_strdup(opts[i]->id),
                    g_variant_ref_sink(gvar));
        } else if (g_variant_is_of_type(opts[i]->def, G_VARIANT_TYPE_DOUBLE)) {
            gvar = g_variant_new_double(strtod(s, NULL));
            g_hash_table_insert(hash, g_strdup(opts[i]->id),
                    g_variant_ref_sink(gvar));
        } else if (g_variant_is_of_type(opts[i]->def, G_VARIANT_TYPE_STRING)) {
            gvar = g_variant_new_string(s);
            g_hash_table_insert(hash, g_strdup(opts[i]->id),
                    g_variant_ref_sink(gvar));
        } else if (g_variant_is_of_type(opts[i]->def, G_VARIANT_TYPE_BOOLEAN)) {
            b = TRUE;
            if (0 == strcmp(s, "false") || 0 == strcmp(s, "no")) {
                b = FALSE;
            } else if (!(0 == strcmp(s, "true") || 0 == strcmp(s, "yes"))) {
                g_critical("Unable to convert '%s' to boolean!", s);
            }

            gvar = g_variant_new_boolean(b);
            g_hash_table_insert(hash, g_strdup(opts[i]->id),
                    g_variant_ref_sink(gvar));
        } else {
            g_critical("Don't know GVariant type for option '%s'!", opts[i]->id);
         }
    }

    return hash;
}

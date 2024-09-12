/*
 * This file is part of the eecu-sat project.
 *
 * Copyright (C) 2024 Petre Rodan <2b4eda@subdimension.ro>
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
#include "proj.h"
#include "error.h"
#include "trigger.h"

/**
 * Create a new trigger.
 *
 * The caller is responsible to free the trigger via sat_trigger_free()
 *
 * @param name The trigger name to use. Can be NULL.
 *
 * @return A newly allocated trigger.
 */
struct sat_trigger *sat_trigger_new(const char *name)
{
    struct sat_trigger *trig;

    trig = g_malloc0(sizeof(struct sat_trigger));
    if (name)
        trig->name = g_strdup(name);
    trig->priv = g_malloc0(sizeof(struct trigger_context));

    return trig;
}

/**
 * Free a previously allocated trigger.
 *
 * This will also free any trigger stages/matches in this trigger.
 *
 * @param trig The trigger to free. Must not be NULL.
 */
void sat_trigger_free(struct sat_trigger *trig)
{
    GSList *l;

    if (!trig)
        return;

    if (trig->matches) {
        for (l = trig->matches; l; l = l->next) {
            g_free(l->data);
        }
        g_slist_free(trig->matches);
    }

    g_free(trig->priv);
    g_free(trig->name);
    g_free(trig);
}

void sat_trigger_show(const struct sat_trigger *t)
{
    GSList *l;
    struct trigger_match *match;

    if (! t->matches)
        return;

    for (l = t->matches; l; l = l->next) {
        match = l->data;
        fprintf(stdout, " * trigger #%d activated at sample %ld\n", t->id, match->sample_cnt);
    }
}

bool sat_trigger_activated(const struct sat_trigger *t)
{
    if (!t)
        return false;

    if (t->matches)
        return true;

    return false;
}

// get sample number based of the matched trigger
ssize_t sat_trigger_loc(const struct sat_trigger *t)
{
    GSList *l;
    struct trigger_match *match;
    ssize_t ret = 0;
    ssize_t cnt;

    if (! t->matches)
        return 0;

    cnt = 1;
    for (l = t->matches; l; l = l->next) {
        match = l->data;
        if (t->nth && (t->nth == cnt)) {
            ret = match->sample_cnt;
        }
        cnt++;
    }

    if (t->nth && !ret)
        fprintf(stderr, "warning: nth not matched\n");

    return ret;
}

int sat_trigger_receive(struct sat_trigger *t, struct sr_datafeed_packet *packet_in)
{
    struct trigger_context *ctx;
    ssize_t i;
    const struct sr_datafeed_analog *analog;
    float cur;
    float *samples;
    struct trigger_match *match;
    int ret = SR_OK;

    if (!t || !packet_in)
        return SR_ERR_ARG;

    ctx = t->priv;

    switch (packet_in->type) {
        case SR_DF_ANALOG:
        analog = packet_in->payload;
        samples = analog->data;

        if (t->type == SR_TRIGGER_OVER) {
            for (i=0; i<analog->num_samples; i++) {
                cur = samples[i];
                if (cur < t->level) {
                    ctx->last_state = TRIGGER_BELLOW;
                } else if ((cur >= t->level) && (ctx->last_state == TRIGGER_BELLOW)) {
                    ctx->last_state = TRIGGER_ABOVE;
                    match = g_malloc0(sizeof(struct trigger_match));
                    match->sample_cnt = ctx->cur_sample;
                    t->matches = g_slist_append(t->matches, match);
                }
                ctx->cur_sample++;
            }
        } else if (t->type == SR_TRIGGER_UNDER) {
            for (i=0; i<analog->num_samples; i++) {
                cur = samples[i];
                if ((cur <= t->level) && (ctx->last_state == TRIGGER_ABOVE)) {
                    ctx->last_state = TRIGGER_BELLOW;
                    match = g_malloc0(sizeof(struct trigger_match));
                    match->sample_cnt = ctx->cur_sample;
                    t->matches = g_slist_append(t->matches, match);
                } else if (cur > t->level) {
                    ctx->last_state = TRIGGER_ABOVE;
                }
                ctx->cur_sample++;
            }
        }
        break;
    }

    return ret;
}


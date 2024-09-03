#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "proj.h"
#include "tlpi_hdr.h"
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
    if (!trig)
        return;

    g_free(trig->name);
    g_free(trig);
}


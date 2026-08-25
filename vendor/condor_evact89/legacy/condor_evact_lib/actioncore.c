/* actioncore.c - C89, acciones estáticas con handles estables */

#include "actioncore.h"

#ifndef ACTCORE_MAX_ITEMS
#define ACTCORE_MAX_ITEMS 16
#endif

typedef struct {
    actcore_exec_fn fn;
    void           *user;
} actcore_entry;

static actcore_entry actcore_table[ACTCORE_MAX_ITEMS];
static actcore_id    actcore_serials[ACTCORE_MAX_ITEMS];

static actcore_id actcore_serial_limit(void)
{
    actcore_id max_id;

    max_id = (actcore_id)~((actcore_id)0);
    max_id = max_id / (actcore_id)ACTCORE_MAX_ITEMS;
    if (max_id == (actcore_id)0) {
        max_id = (actcore_id)1;
    }
    return max_id;
}

static actcore_id actcore_make_id(int slot, actcore_id serial)
{
    return (actcore_id)(((serial - (actcore_id)1) *
                         (actcore_id)ACTCORE_MAX_ITEMS) +
                        (actcore_id)(slot + 1));
}

static int actcore_decode_id(actcore_id id, int *slot, actcore_id *serial)
{
    actcore_id raw;

    if (id == ACTCORE_ID_INVALID) {
        return 0;
    }

    raw = id - (actcore_id)1;
    *slot = (int)(raw % (actcore_id)ACTCORE_MAX_ITEMS);
    *serial = (raw / (actcore_id)ACTCORE_MAX_ITEMS) + (actcore_id)1;
    return 1;
}

actcore_id actcore_register(actcore_exec_fn fn, void *user)
{
    int i;
    actcore_id limit;
    actcore_id serial;

    if (fn == 0) {
        return ACTCORE_ID_INVALID;
    }

    limit = actcore_serial_limit();

    for (i = 0; i < ACTCORE_MAX_ITEMS; ++i) {
        if (actcore_table[i].fn == 0) {
            serial = actcore_serials[i] + (actcore_id)1;
            if (serial == (actcore_id)0 || serial > limit) {
                serial = (actcore_id)1;
            }

            actcore_serials[i] = serial;
            actcore_table[i].fn = fn;
            actcore_table[i].user = user;
            return actcore_make_id(i, serial);
        }
    }

    return ACTCORE_ID_INVALID;
}

int actcore_unregister(actcore_id id)
{
    int slot;
    actcore_id serial;

    if (!actcore_decode_id(id, &slot, &serial)) {
        return 0;
    }

    if (actcore_table[slot].fn == 0) {
        return 0;
    }

    if (actcore_serials[slot] != serial) {
        return 0;
    }

    actcore_table[slot].fn = 0;
    actcore_table[slot].user = 0;
    return 1;
}

int actcore_exec(actcore_id id, const evcore_event_t *evt)
{
    int slot;
    actcore_id serial;
    actcore_exec_fn fn;
    void *user;

    if (!actcore_decode_id(id, &slot, &serial)) {
        return 0;
    }

    if (actcore_table[slot].fn == 0) {
        return 0;
    }

    if (actcore_serials[slot] != serial) {
        return 0;
    }

    fn = actcore_table[slot].fn;
    user = actcore_table[slot].user;
    fn(evt, user);
    return 1;
}

void actcore_reset(void)
{
    int i;

    for (i = 0; i < ACTCORE_MAX_ITEMS; ++i) {
        actcore_table[i].fn = 0;
        actcore_table[i].user = 0;
    }
}

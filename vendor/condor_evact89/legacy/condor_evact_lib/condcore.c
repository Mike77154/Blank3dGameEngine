/* condcore.c - C89, condiciones estáticas con handles estables */

#include "condcore.h"

#ifndef CONDCORE_MAX_ITEMS
#define CONDCORE_MAX_ITEMS 16
#endif

typedef struct {
    condcore_eval_fn fn;
    void            *user;
} condcore_entry;

static condcore_entry condcore_table[CONDCORE_MAX_ITEMS];
static condcore_id    condcore_serials[CONDCORE_MAX_ITEMS];

static condcore_id condcore_serial_limit(void)
{
    condcore_id max_id;

    max_id = (condcore_id)~((condcore_id)0);
    max_id = max_id / (condcore_id)CONDCORE_MAX_ITEMS;
    if (max_id == (condcore_id)0) {
        max_id = (condcore_id)1;
    }
    return max_id;
}

static condcore_id condcore_make_id(int slot, condcore_id serial)
{
    return (condcore_id)(((serial - (condcore_id)1) *
                          (condcore_id)CONDCORE_MAX_ITEMS) +
                         (condcore_id)(slot + 1));
}

static int condcore_decode_id(condcore_id id, int *slot, condcore_id *serial)
{
    condcore_id raw;

    if (id == CONDCORE_ID_INVALID) {
        return 0;
    }

    raw = id - (condcore_id)1;
    *slot = (int)(raw % (condcore_id)CONDCORE_MAX_ITEMS);
    *serial = (raw / (condcore_id)CONDCORE_MAX_ITEMS) + (condcore_id)1;
    return 1;
}

condcore_id condcore_register(condcore_eval_fn fn, void *user)
{
    int i;
    condcore_id limit;
    condcore_id serial;

    if (fn == 0) {
        return CONDCORE_ID_INVALID;
    }

    limit = condcore_serial_limit();

    for (i = 0; i < CONDCORE_MAX_ITEMS; ++i) {
        if (condcore_table[i].fn == 0) {
            serial = condcore_serials[i] + (condcore_id)1;
            if (serial == (condcore_id)0 || serial > limit) {
                serial = (condcore_id)1;
            }

            condcore_serials[i] = serial;
            condcore_table[i].fn = fn;
            condcore_table[i].user = user;
            return condcore_make_id(i, serial);
        }
    }

    return CONDCORE_ID_INVALID;
}

int condcore_unregister(condcore_id id)
{
    int slot;
    condcore_id serial;

    if (!condcore_decode_id(id, &slot, &serial)) {
        return 0;
    }

    if (condcore_table[slot].fn == 0) {
        return 0;
    }

    if (condcore_serials[slot] != serial) {
        return 0;
    }

    condcore_table[slot].fn = 0;
    condcore_table[slot].user = 0;
    return 1;
}

int condcore_eval(condcore_id id, const evcore_event_t *evt)
{
    int slot;
    condcore_id serial;
    condcore_eval_fn fn;
    void *user;

    if (!condcore_decode_id(id, &slot, &serial)) {
        return 0;
    }

    if (condcore_table[slot].fn == 0) {
        return 0;
    }

    if (condcore_serials[slot] != serial) {
        return 0;
    }

    fn = condcore_table[slot].fn;
    user = condcore_table[slot].user;
    return fn(evt, user);
}

void condcore_reset(void)
{
    int i;

    for (i = 0; i < CONDCORE_MAX_ITEMS; ++i) {
        condcore_table[i].fn = 0;
        condcore_table[i].user = 0;
    }
}

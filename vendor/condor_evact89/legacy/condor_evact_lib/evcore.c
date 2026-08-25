/* evcore.c - C89, estático, reentrante por snapshot local */

#include "evcore.h"

#ifndef EVCORE_MAX_LISTENERS
#define EVCORE_MAX_LISTENERS 16
#endif

typedef struct {
    evcore_value_t    type;
    evcore_listener_fn fn;
    void             *user;
} evcore_listener_entry;

static evcore_listener_entry evcore_listeners[EVCORE_MAX_LISTENERS];
static evcore_sub_id         evcore_serials[EVCORE_MAX_LISTENERS];

static evcore_sub_id evcore_serial_limit(void)
{
    evcore_sub_id max_id;

    max_id = (evcore_sub_id)~((evcore_sub_id)0);
    max_id = max_id / (evcore_sub_id)EVCORE_MAX_LISTENERS;
    if (max_id == (evcore_sub_id)0) {
        max_id = (evcore_sub_id)1;
    }
    return max_id;
}

static evcore_sub_id evcore_make_id(int slot, evcore_sub_id serial)
{
    return (evcore_sub_id)(((serial - (evcore_sub_id)1) *
                            (evcore_sub_id)EVCORE_MAX_LISTENERS) +
                           (evcore_sub_id)(slot + 1));
}

static int evcore_decode_id(evcore_sub_id id, int *slot, evcore_sub_id *serial)
{
    evcore_sub_id raw;

    if (id == EVCORE_SUB_INVALID) {
        return 0;
    }

    raw = id - (evcore_sub_id)1;
    *slot = (int)(raw % (evcore_sub_id)EVCORE_MAX_LISTENERS);
    *serial = (raw / (evcore_sub_id)EVCORE_MAX_LISTENERS) + (evcore_sub_id)1;
    return 1;
}

static int evcore_find_free_slot(void)
{
    int i;

    for (i = 0; i < EVCORE_MAX_LISTENERS; ++i) {
        if (evcore_listeners[i].fn == 0) {
            return i;
        }
    }
    return -1;
}

static evcore_sub_id evcore_fill_slot(int slot,
                                      evcore_value_t type,
                                      evcore_listener_fn fn,
                                      void *user)
{
    evcore_sub_id limit;
    evcore_sub_id serial;

    limit = evcore_serial_limit();
    serial = evcore_serials[slot] + (evcore_sub_id)1;
    if (serial == (evcore_sub_id)0 || serial > limit) {
        serial = (evcore_sub_id)1;
    }

    evcore_serials[slot] = serial;
    evcore_listeners[slot].type = type;
    evcore_listeners[slot].fn   = fn;
    evcore_listeners[slot].user = user;

    return evcore_make_id(slot, serial);
}

evcore_sub_id evcore_subscribe(evcore_value_t type,
                               evcore_listener_fn fn,
                               void *user)
{
    int slot;

    if (fn == 0) {
        return EVCORE_SUB_INVALID;
    }

    slot = evcore_find_free_slot();
    if (slot < 0) {
        return EVCORE_SUB_INVALID;
    }

    return evcore_fill_slot(slot, type, fn, user);
}

evcore_sub_id evcore_subscribe_unique(evcore_value_t type,
                                      evcore_listener_fn fn,
                                      void *user)
{
    int i;

    if (fn == 0) {
        return EVCORE_SUB_INVALID;
    }

    for (i = 0; i < EVCORE_MAX_LISTENERS; ++i) {
        if (evcore_listeners[i].fn != 0 &&
            evcore_listeners[i].type == type &&
            evcore_listeners[i].fn == fn &&
            evcore_listeners[i].user == user) {
            return evcore_make_id(i, evcore_serials[i]);
        }
    }

    return evcore_subscribe(type, fn, user);
}

int evcore_unsubscribe_id(evcore_sub_id id)
{
    int slot;
    evcore_sub_id serial;

    if (!evcore_decode_id(id, &slot, &serial)) {
        return 0;
    }

    if (evcore_listeners[slot].fn == 0) {
        return 0;
    }

    if (evcore_serials[slot] != serial) {
        return 0;
    }

    evcore_listeners[slot].type = (evcore_value_t)0;
    evcore_listeners[slot].fn   = 0;
    evcore_listeners[slot].user = 0;
    return 1;
}

int evcore_unsubscribe(evcore_listener_fn fn, void *user)
{
    int i;
    int removed;

    removed = 0;
    if (fn == 0) {
        return 0;
    }

    for (i = 0; i < EVCORE_MAX_LISTENERS; ++i) {
        if (evcore_listeners[i].fn != 0 &&
            evcore_listeners[i].fn == fn &&
            evcore_listeners[i].user == user) {
            evcore_listeners[i].type = (evcore_value_t)0;
            evcore_listeners[i].fn   = 0;
            evcore_listeners[i].user = 0;
            ++removed;
        }
    }

    return removed;
}

void evcore_emit(const evcore_event_t *evt)
{
    int i;
    int snapshot_count;
    int snapshot_slots[EVCORE_MAX_LISTENERS];
    evcore_sub_id snapshot_serials[EVCORE_MAX_LISTENERS];
    int slot;
    evcore_event_t local_evt;
    evcore_listener_fn fn;
    void *user;

    if (evt == 0) {
        return;
    }

    local_evt = *evt;
    snapshot_count = 0;

    for (i = 0; i < EVCORE_MAX_LISTENERS; ++i) {
        if (evcore_listeners[i].fn != 0 &&
            (evcore_listeners[i].type == EVCORE_MATCH_ANY ||
             evcore_listeners[i].type == local_evt.type)) {
            snapshot_slots[snapshot_count] = i;
            snapshot_serials[snapshot_count] = evcore_serials[i];
            ++snapshot_count;
        }
    }

    for (i = 0; i < snapshot_count; ++i) {
        slot = snapshot_slots[i];
        if (evcore_listeners[slot].fn == 0) {
            continue;
        }
        if (evcore_serials[slot] != snapshot_serials[i]) {
            continue;
        }
        if (evcore_listeners[slot].type != EVCORE_MATCH_ANY &&
            evcore_listeners[slot].type != local_evt.type) {
            continue;
        }

        fn = evcore_listeners[slot].fn;
        user = evcore_listeners[slot].user;
        fn(&local_evt, user);
    }
}

void evcore_emit_values(evcore_value_t type,
                        evcore_value_t code,
                        void *data)
{
    evcore_event_t evt;

    evt.type = type;
    evt.code = code;
    evt.data = data;
    evcore_emit(&evt);
}

void evcore_reset(void)
{
    int i;

    for (i = 0; i < EVCORE_MAX_LISTENERS; ++i) {
        evcore_listeners[i].type = (evcore_value_t)0;
        evcore_listeners[i].fn   = 0;
        evcore_listeners[i].user = 0;
    }
}

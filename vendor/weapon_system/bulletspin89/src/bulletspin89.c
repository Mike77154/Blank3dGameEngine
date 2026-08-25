#include "bulletspin89.h"

static int bs89_wrap(int value, int count)
{
    if (count <= 0) return 0;
    while (value < 0) value += count;
    while (value >= count) value -= count;
    return value;
}

void bulletspin89_init(bs89_state *state, const bs89_config *config)
{
    int count;
    int step;
    int direction;
    if (!state) return;
    count = 1;
    step = 1;
    direction = BS89_DIRECTION_FORWARD;
    if (config) {
        if (config->slot_count > 0) count = config->slot_count;
        if (config->step != 0) step = config->step;
        if (config->direction < 0) direction = BS89_DIRECTION_REVERSE;
        else direction = BS89_DIRECTION_FORWARD;
    }
    if (step < 0) step = -step;
    if (step < 1) step = 1;
    state->initialized = 1;
    state->slot_count = count;
    state->step = step;
    state->direction = direction;
    state->shot_count = 0UL;
    state->current_slot = config ? bs89_wrap(config->start_slot, count) : 0;
}

int bulletspin89_peek(const bs89_state *state, int *slot_out)
{
    if (!state || !slot_out || !state->initialized || state->slot_count < 1)
        return 0;
    *slot_out = bs89_wrap(state->current_slot, state->slot_count);
    return 1;
}

int bulletspin89_next(bs89_state *state, int *slot_out)
{
    int slot;
    int delta;
    if (!state || !slot_out || !state->initialized || state->slot_count < 1)
        return 0;
    slot = bs89_wrap(state->current_slot, state->slot_count);
    *slot_out = slot;
    delta = state->step * state->direction;
    state->current_slot = bs89_wrap(slot + delta, state->slot_count);
    state->shot_count += 1UL;
    return 1;
}

void bulletspin89_reset(bs89_state *state, const bs89_config *config)
{
    bulletspin89_init(state, config);
}

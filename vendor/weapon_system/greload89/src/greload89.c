#include "greload89.h"

#include <string.h>

void grel89_init(GREL89_State *reload)
{
    if (!reload) return;
    memset(reload, 0, sizeof(*reload));
}

int grel89_begin(GREL89_State *reload,
                 unsigned long duration_ms,
                 int rounds_requested)
{
    if (!reload || rounds_requested < 0) return GREL89_BAD_ARG;
    if (reload->active) return GREL89_ALREADY_ACTIVE;
    if (reload->completion_pending) return GREL89_COMPLETION_PENDING;
    if (rounds_requested == 0) return GREL89_NOT_NEEDED;

    reload->duration_ms = duration_ms;
    reload->remaining_ms = duration_ms;
    reload->rounds_requested = rounds_requested;

    if (duration_ms == 0ul) {
        reload->active = 0;
        reload->completion_pending = 1;
        return GREL89_COMPLETED;
    }

    reload->active = 1;
    reload->completion_pending = 0;
    return GREL89_STARTED;
}

int grel89_begin_full(GREL89_State *reload,
                      unsigned long duration_ms,
                      int current_rounds,
                      int capacity)
{
    int missing;
    if (!reload || capacity < 0 || current_rounds < 0) return GREL89_BAD_ARG;
    if (current_rounds > capacity) current_rounds = capacity;
    missing = capacity - current_rounds;
    return grel89_begin(reload, duration_ms, missing);
}

int grel89_update(GREL89_State *reload, unsigned long dt_ms)
{
    if (!reload) return GREL89_BAD_ARG;
    if (!reload->active) {
        if (reload->completion_pending) return GREL89_COMPLETED;
        return GREL89_IDLE;
    }

    if (dt_ms >= reload->remaining_ms) reload->remaining_ms = 0ul;
    else reload->remaining_ms -= dt_ms;

    if (reload->remaining_ms == 0ul) {
        reload->active = 0;
        reload->completion_pending = 1;
        return GREL89_COMPLETED;
    }
    return GREL89_OK;
}

int grel89_cancel(GREL89_State *reload)
{
    if (!reload) return GREL89_BAD_ARG;
    reload->active = 0;
    reload->completion_pending = 0;
    reload->rounds_requested = 0;
    reload->remaining_ms = 0ul;
    reload->duration_ms = 0ul;
    return GREL89_OK;
}

int grel89_is_active(const GREL89_State *reload)
{
    if (!reload) return 0;
    return reload->active ? 1 : 0;
}

int grel89_has_completion(const GREL89_State *reload)
{
    if (!reload) return 0;
    return reload->completion_pending ? 1 : 0;
}

int grel89_rounds_requested(const GREL89_State *reload)
{
    if (!reload) return 0;
    return reload->rounds_requested;
}

int grel89_take_completed_rounds(GREL89_State *reload)
{
    int amount;
    if (!reload || !reload->completion_pending) return 0;
    amount = reload->rounds_requested;
    reload->completion_pending = 0;
    reload->rounds_requested = 0;
    reload->duration_ms = 0ul;
    reload->remaining_ms = 0ul;
    return amount;
}

unsigned long grel89_remaining_ms(const GREL89_State *reload)
{
    if (!reload) return 0ul;
    return reload->remaining_ms;
}

unsigned long grel89_duration_ms(const GREL89_State *reload)
{
    if (!reload) return 0ul;
    return reload->duration_ms;
}

int grel89_progress_permyriad(const GREL89_State *reload)
{
    unsigned long elapsed;
    unsigned long scaled;
    if (!reload) return 0;
    if (reload->completion_pending) return 10000;
    if (reload->duration_ms == 0ul) return reload->active ? 0 : 10000;
    elapsed = reload->duration_ms - reload->remaining_ms;
    scaled = (elapsed * 10000ul) / reload->duration_ms;
    if (scaled > 10000ul) scaled = 10000ul;
    return (int)scaled;
}

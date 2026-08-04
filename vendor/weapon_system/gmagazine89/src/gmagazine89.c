#include "gmagazine89.h"

#include <string.h>

static int gmag89_clamp(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

void gmag89_init(GMAG89_State *magazine,
                 int capacity,
                 int initial_rounds,
                 int rounds_per_shot)
{
    if (!magazine) return;
    memset(magazine, 0, sizeof(*magazine));
    gmag89_configure(magazine, capacity, initial_rounds, rounds_per_shot);
}

int gmag89_configure(GMAG89_State *magazine,
                     int capacity,
                     int initial_rounds,
                     int rounds_per_shot)
{
    if (!magazine || capacity < 0) return GMAG89_BAD_ARG;
    if (rounds_per_shot <= 0) rounds_per_shot = 1;
    magazine->capacity = capacity;
    magazine->rounds_per_shot = rounds_per_shot;
    if (capacity == 0) magazine->rounds = 0;
    else magazine->rounds = gmag89_clamp(initial_rounds, 0, capacity);
    return GMAG89_OK;
}

int gmag89_set_capacity(GMAG89_State *magazine, int capacity)
{
    if (!magazine || capacity < 0) return GMAG89_BAD_ARG;
    magazine->capacity = capacity;
    if (capacity == 0) magazine->rounds = 0;
    else if (magazine->rounds > capacity) magazine->rounds = capacity;
    return magazine->capacity;
}

int gmag89_set_rounds(GMAG89_State *magazine, int rounds)
{
    if (!magazine || rounds < 0) return GMAG89_BAD_ARG;
    if (magazine->capacity <= 0) {
        magazine->rounds = 0;
        return GMAG89_BYPASS;
    }
    magazine->rounds = gmag89_clamp(rounds, 0, magazine->capacity);
    return magazine->rounds;
}

int gmag89_set_rounds_per_shot(GMAG89_State *magazine, int rounds_per_shot)
{
    if (!magazine || rounds_per_shot <= 0) return GMAG89_BAD_ARG;
    magazine->rounds_per_shot = rounds_per_shot;
    return magazine->rounds_per_shot;
}

int gmag89_load(GMAG89_State *magazine, int amount)
{
    int before;
    long next;
    if (!magazine || amount < 0) return GMAG89_BAD_ARG;
    if (magazine->capacity <= 0) return GMAG89_BYPASS;
    before = magazine->rounds;
    next = (long)magazine->rounds + (long)amount;
    if (next > (long)magazine->capacity) next = (long)magazine->capacity;
    magazine->rounds = (int)next;
    return magazine->rounds - before;
}

int gmag89_fill(GMAG89_State *magazine)
{
    if (!magazine) return GMAG89_BAD_ARG;
    return gmag89_load(magazine, gmag89_missing(magazine));
}

int gmag89_spend(GMAG89_State *magazine, int amount)
{
    if (!magazine || amount <= 0) return GMAG89_BAD_ARG;
    if (magazine->capacity <= 0) return GMAG89_BYPASS;
    if (magazine->rounds <= 0) return GMAG89_EMPTY;
    if (magazine->rounds < amount) return GMAG89_NOT_ENOUGH;
    magazine->rounds -= amount;
    return GMAG89_OK;
}

int gmag89_spend_shot(GMAG89_State *magazine)
{
    if (!magazine) return GMAG89_BAD_ARG;
    return gmag89_spend(magazine, magazine->rounds_per_shot);
}

int gmag89_is_enabled(const GMAG89_State *magazine)
{
    if (!magazine) return 0;
    return magazine->capacity > 0 ? 1 : 0;
}

int gmag89_is_empty(const GMAG89_State *magazine)
{
    if (!magazine || magazine->capacity <= 0) return 0;
    return magazine->rounds <= 0 ? 1 : 0;
}

int gmag89_is_full(const GMAG89_State *magazine)
{
    if (!magazine || magazine->capacity <= 0) return 0;
    return magazine->rounds >= magazine->capacity ? 1 : 0;
}

int gmag89_can_spend(const GMAG89_State *magazine, int amount)
{
    if (!magazine || amount <= 0) return 0;
    if (magazine->capacity <= 0) return 1;
    return magazine->rounds >= amount ? 1 : 0;
}

int gmag89_can_fire(const GMAG89_State *magazine)
{
    if (!magazine) return 0;
    return gmag89_can_spend(magazine, magazine->rounds_per_shot);
}

int gmag89_rounds(const GMAG89_State *magazine)
{
    if (!magazine) return 0;
    return magazine->rounds;
}

int gmag89_capacity(const GMAG89_State *magazine)
{
    if (!magazine) return 0;
    return magazine->capacity;
}

int gmag89_missing(const GMAG89_State *magazine)
{
    if (!magazine || magazine->capacity <= 0) return 0;
    if (magazine->rounds >= magazine->capacity) return 0;
    return magazine->capacity - magazine->rounds;
}

void gmag89_report(const GMAG89_State *magazine, GMAG89_Report *report)
{
    if (!report) return;
    memset(report, 0, sizeof(*report));
    if (!magazine) return;
    report->capacity = magazine->capacity;
    report->rounds = magazine->rounds;
    report->missing = gmag89_missing(magazine);
    report->rounds_per_shot = magazine->rounds_per_shot;
    if (gmag89_is_enabled(magazine)) report->flags |= GMAG89_FLAG_ENABLED;
    if (gmag89_is_empty(magazine)) report->flags |= GMAG89_FLAG_EMPTY;
    if (gmag89_is_full(magazine)) report->flags |= GMAG89_FLAG_FULL;
    if (gmag89_can_fire(magazine)) report->flags |= GMAG89_FLAG_CAN_FIRE;
}

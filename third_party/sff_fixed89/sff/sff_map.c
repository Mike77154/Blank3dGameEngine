#include "sff_map.h"
#include <string.h>

static sff_u32 sff__hash_u32(sff_u32 x)
{
    return (sff_u32)(x * 2654435761u);
}

static int sff__is_pow2(sff_u32 x)
{
    if (x == 0u) return 0;
    return (x & (x - 1u)) == 0u;
}

int sff_map_init_fixed(SffMap *m, SffMapSlot *slots, sff_u32 cap)
{
    if (!m || !slots || !sff__is_pow2(cap)) return 0;
    m->slots = slots;
    m->cap = cap;
    m->len = 0u;
    memset(m->slots, 0, (size_t)cap * sizeof(SffMapSlot));
    return 1;
}

void sff_map_clear(SffMap *m)
{
    if (!m || !m->slots) return;
    memset(m->slots, 0, (size_t)m->cap * sizeof(SffMapSlot));
    m->len = 0u;
}

static int sff__map_probe(const SffMap *m, sff_u32 key, sff_u32 *io_pos, sff_bool *out_found)
{
    sff_u32 mask;
    sff_u32 pos;
    sff_u32 step;

    if (!m || !m->slots || m->cap == 0u || !io_pos || !out_found) return 0;

    mask = m->cap - 1u;
    pos = sff__hash_u32(key) & mask;
    step = 0u;

    while (step < m->cap) {
        const SffMapSlot *s = &m->slots[pos];
        if (!s->used) {
            *io_pos = pos;
            *out_found = SFF_FALSE;
            return 1;
        }
        if (s->key == key) {
            *io_pos = pos;
            *out_found = SFF_TRUE;
            return 1;
        }
        pos = (pos + 1u) & mask;
        ++step;
    }

    return 0;
}

int sff_map_set(SffMap *m, sff_u32 key, sff_u32 val)
{
    sff_u32 pos;
    sff_bool found;

    if (!m || !m->slots) return 0;
    if (!sff__map_probe(m, key, &pos, &found)) return 0;

    if (!found) {
        m->slots[pos].used = 1u;
        m->slots[pos].key = key;
        m->slots[pos].val = val;
        ++m->len;
    } else {
        m->slots[pos].val = val;
    }

    return 1;
}

int sff_map_get(const SffMap *m, sff_u32 key, sff_u32 *out_val)
{
    sff_u32 pos;
    sff_bool found;

    if (!m || !m->slots) return 0;
    if (!sff__map_probe(m, key, &pos, &found)) return 0;
    if (!found) return 0;
    if (out_val) *out_val = m->slots[pos].val;
    return 1;
}

int sff_map_has(const SffMap *m, sff_u32 key)
{
    return sff_map_get(m, key, 0);
}

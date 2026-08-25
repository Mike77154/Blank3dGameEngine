#include "gveh_tank_modules.h"

void gveh_tank_modules_default(gveh_tank_modules_cfg *c)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_TANK_MODULE_COUNT) { c->max_health[i] = gveh_fx_from_int(100); i++; }
    c->max_health[GVEH_TANK_MODULE_AMMO] = gveh_fx_from_int(70);
    c->max_health[GVEH_TANK_MODULE_OPTICS] = gveh_fx_from_int(55);
    c->cookoff_threshold = gveh_fx_from_int(0);
    c->fire_threshold = gveh_fx_from_int(0);
}

void gveh_tank_modules_clear(gveh_tank_modules_state *s, const gveh_tank_modules_cfg *c)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_TANK_MODULE_COUNT) { s->health[i] = c->max_health[i]; i++; }
    s->engine_dead = 0;
    s->turret_stuck = 0;
    s->gun_dead = 0;
    s->optics_bad = 0;
    s->ammo_cookoff = 0;
    s->fire = 0;
    s->last_module = -1;
}

void gveh_tank_modules_apply_hit(const gveh_tank_modules_cfg *c, gveh_tank_modules_state *s, const gveh_tank_hit *hit, gveh_i32 seed, gveh_fx_queue *fxq, gveh_vec3 pos)
{
    gveh_i32 idx;
    gveh_fx dmg;
    if (hit->penetrated == 0) return;
    idx = (seed + hit->face * 3) % GVEH_TANK_MODULE_COUNT;
    if (idx < 0) idx = -idx;
    if (hit->face == GVEH_TANK_FACE_FRONT && (idx % 2) == 0) idx = GVEH_TANK_MODULE_TRANS;
    if (hit->face == GVEH_TANK_FACE_REAR && (idx % 2) == 0) idx = GVEH_TANK_MODULE_ENGINE;
    if (hit->face == GVEH_TANK_FACE_SIDE && (idx % 3) == 0) idx = GVEH_TANK_MODULE_AMMO;
    dmg = hit->spall;
    if (dmg < gveh_fx_from_int(12)) dmg = gveh_fx_from_int(12);
    s->health[idx] -= dmg;
    if (s->health[idx] < 0) s->health[idx] = 0;
    s->last_module = (gveh_i16)idx;
    if (s->health[GVEH_TANK_MODULE_ENGINE] <= c->fire_threshold) { s->engine_dead = 1; s->fire = 1; }
    if (s->health[GVEH_TANK_MODULE_TRANS] <= 0) s->engine_dead = 1;
    if (s->health[GVEH_TANK_MODULE_TURRET_RING] <= 0) s->turret_stuck = 1;
    if (s->health[GVEH_TANK_MODULE_GUN_BREECH] <= 0) s->gun_dead = 1;
    if (s->health[GVEH_TANK_MODULE_OPTICS] <= 0) s->optics_bad = 1;
    if (s->health[GVEH_TANK_MODULE_AMMO] <= c->cookoff_threshold) s->ammo_cookoff = 1;
    gveh_fx_push(fxq, GVEH_FX_EVENT_IMPACT, (gveh_i16)(40 + idx), pos.x, pos.y, pos.z);
}

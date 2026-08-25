#include "gveh_tank_gun.h"

static void ammo_set(gveh_tank_ammo *a, gveh_i16 k, gveh_i32 mv, gveh_i32 pc, gveh_i32 pf, gveh_i32 blast, gveh_i32 spall, gveh_i32 drop)
{
    a->kind = k;
    a->muzzle_vel = gveh_fx_from_int(mv);
    a->pen_close = gveh_fx_from_int(pc);
    a->pen_far = gveh_fx_from_int(pf);
    a->blast = gveh_fx_from_int(blast);
    a->spall = gveh_fx_from_int(spall);
    a->drop = gveh_fx_from_int(drop);
}

void gveh_tank_gun_default(gveh_tank_gun_cfg *c)
{
    gveh_i32 i;
    c->caliber = gveh_fx_from_int(105);
    c->reload_ticks = gveh_fx_from_int(6);
    c->ready_reload_ticks = gveh_fx_from_int(4);
    c->recoil_force = gveh_fx_from_int(9000);
    c->flags = 0;
    i = 0;
    while (i < GVEH_TANK_AMMO_COUNT) { c->ammo_count[i] = 0; i++; }
    ammo_set(&c->ammo[GVEH_TANK_AMMO_AP], GVEH_TANK_AMMO_AP, 900, 180, 120, 10, 80, 12);
    ammo_set(&c->ammo[GVEH_TANK_AMMO_APCR], GVEH_TANK_AMMO_APCR, 1100, 220, 100, 6, 50, 9);
    ammo_set(&c->ammo[GVEH_TANK_AMMO_APFSDS], GVEH_TANK_AMMO_APFSDS, 1500, 460, 330, 4, 95, 4);
    ammo_set(&c->ammo[GVEH_TANK_AMMO_HE], GVEH_TANK_AMMO_HE, 720, 40, 30, 170, 35, 18);
    ammo_set(&c->ammo[GVEH_TANK_AMMO_HEAT], GVEH_TANK_AMMO_HEAT, 850, 360, 350, 70, 55, 14);
    ammo_set(&c->ammo[GVEH_TANK_AMMO_HESH], GVEH_TANK_AMMO_HESH, 690, 120, 110, 110, 120, 19);
    ammo_set(&c->ammo[GVEH_TANK_AMMO_SMOKE], GVEH_TANK_AMMO_SMOKE, 500, 0, 0, 0, 0, 22);
    c->ammo_count[GVEH_TANK_AMMO_AP] = 12;
    c->ammo_count[GVEH_TANK_AMMO_HE] = 8;
    c->ammo_count[GVEH_TANK_AMMO_HEAT] = 6;
    c->ammo_count[GVEH_TANK_AMMO_SMOKE] = 4;
}

void gveh_tank_gun_modern120(gveh_tank_gun_cfg *c)
{
    gveh_tank_gun_default(c);
    c->caliber = gveh_fx_from_int(120);
    c->reload_ticks = gveh_fx_from_int(5);
    c->ready_reload_ticks = gveh_fx_from_int(35) / 10;
    c->recoil_force = gveh_fx_from_int(13000);
    c->ammo_count[GVEH_TANK_AMMO_AP] = 0;
    c->ammo_count[GVEH_TANK_AMMO_APFSDS] = 24;
    c->ammo_count[GVEH_TANK_AMMO_HEAT] = 12;
    c->ammo_count[GVEH_TANK_AMMO_SMOKE] = 6;
}

void gveh_tank_gun_soviet125(gveh_tank_gun_cfg *c)
{
    gveh_tank_gun_modern120(c);
    c->caliber = gveh_fx_from_int(125);
    c->reload_ticks = gveh_fx_from_int(7);
    c->ammo_count[GVEH_TANK_AMMO_APFSDS] = 18;
    c->ammo_count[GVEH_TANK_AMMO_HEAT] = 10;
    c->ammo_count[GVEH_TANK_AMMO_HE] = 8;
}

void gveh_tank_gun_ww2_88(gveh_tank_gun_cfg *c)
{
    gveh_tank_gun_default(c);
    c->caliber = gveh_fx_from_int(88);
    c->reload_ticks = gveh_fx_from_int(8);
    c->ready_reload_ticks = gveh_fx_from_int(6);
    c->ammo_count[GVEH_TANK_AMMO_AP] = 32;
    c->ammo_count[GVEH_TANK_AMMO_APCR] = 6;
    c->ammo_count[GVEH_TANK_AMMO_HE] = 16;
    c->ammo_count[GVEH_TANK_AMMO_HEAT] = 0;
    c->ammo[GVEH_TANK_AMMO_AP].pen_close = gveh_fx_from_int(200);
    c->ammo[GVEH_TANK_AMMO_AP].pen_far = gveh_fx_from_int(125);
}

void gveh_tank_gun_ww2_75(gveh_tank_gun_cfg *c)
{
    gveh_tank_gun_default(c);
    c->caliber = gveh_fx_from_int(75);
    c->reload_ticks = gveh_fx_from_int(55) / 10;
    c->ready_reload_ticks = gveh_fx_from_int(4);
    c->ammo_count[GVEH_TANK_AMMO_AP] = 24;
    c->ammo_count[GVEH_TANK_AMMO_APCR] = 3;
    c->ammo_count[GVEH_TANK_AMMO_HE] = 24;
    c->ammo_count[GVEH_TANK_AMMO_HEAT] = 0;
    c->ammo[GVEH_TANK_AMMO_AP].pen_close = gveh_fx_from_int(115);
    c->ammo[GVEH_TANK_AMMO_AP].pen_far = gveh_fx_from_int(75);
}

void gveh_tank_gun_clear(gveh_tank_gun_state *s, const gveh_tank_gun_cfg *c)
{
    gveh_i32 i;
    s->reload_timer = 0;
    s->selected = GVEH_TANK_AMMO_AP;
    if (c->ammo_count[GVEH_TANK_AMMO_APFSDS] > 0) s->selected = GVEH_TANK_AMMO_APFSDS;
    s->fired = 0;
    s->smoke_ready = 1;
    s->last_pen = 0;
    s->last_range = 0;
    i = 0;
    while (i < GVEH_TANK_AMMO_COUNT) { s->ammo_left[i] = c->ammo_count[i]; i++; }
}

const gveh_tank_ammo *gveh_tank_gun_selected_ammo(const gveh_tank_gun_cfg *c, const gveh_tank_gun_state *s)
{
    return &c->ammo[s->selected];
}

gveh_i16 gveh_tank_gun_step(const gveh_tank_gun_cfg *c, gveh_tank_gun_state *s, const gveh_input *in, gveh_fx range_hint, gveh_fx dt, gveh_fx_queue *fxq, gveh_vec3 pos)
{
    gveh_i32 sel;
    gveh_fx t;
    const gveh_tank_ammo *ammo;
    s->fired = 0;
    if (s->reload_timer > 0) {
        t = dt;
        if (t <= 0) t = GVEH_FX_ONE / 30;
        s->reload_timer -= t;
        if (s->reload_timer < 0) s->reload_timer = 0;
    }
    if (in->tank_ammo_next > 0) {
        sel = s->selected + 1;
        while (sel != s->selected) {
            if (sel >= GVEH_TANK_AMMO_COUNT) sel = 0;
            if (s->ammo_left[sel] > 0) { s->selected = (gveh_i16)sel; break; }
            sel++;
        }
    }
    if (in->tank_smoke > 0 && s->smoke_ready != 0 && s->ammo_left[GVEH_TANK_AMMO_SMOKE] > 0) {
        s->selected = GVEH_TANK_AMMO_SMOKE;
    }
    if (in->fire > 0 && s->reload_timer <= 0 && s->ammo_left[s->selected] > 0) {
        ammo = &c->ammo[s->selected];
        s->ammo_left[s->selected]--;
        s->reload_timer = c->reload_ticks;
        if (s->selected == GVEH_TANK_AMMO_SMOKE) s->smoke_ready = 0;
        s->last_range = range_hint;
        s->last_pen = ammo->pen_close;
        if (range_hint > gveh_fx_from_int(100)) {
            s->last_pen = ammo->pen_close - ((range_hint - gveh_fx_from_int(100)) / 4);
            if (s->last_pen < ammo->pen_far) s->last_pen = ammo->pen_far;
        }
        gveh_fx_push(fxq, GVEH_FX_EVENT_ENGINE, (gveh_i16)(20 + ammo->kind), pos.x, pos.y, pos.z);
        s->fired = 1;
    }
    return s->fired;
}

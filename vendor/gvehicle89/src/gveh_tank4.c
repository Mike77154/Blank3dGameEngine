#include "gveh_tank4.h"

void gveh_tank4_default(gveh_tank4_cfg *c)
{
    gveh_tank_tracks_default(&c->tracks);
    gveh_tank_turret_default(&c->turret);
    gveh_tank_gun_default(&c->gun);
    gveh_tank_armor_default(&c->armor);
    gveh_tank_modules_default(&c->modules);
    gveh_tank_crew_default(&c->crew);
    gveh_tank_firecontrol_default(&c->firecontrol);
    gveh_tank_station_default(&c->station);
    gveh_tank_platoon_default(&c->platoon);
    c->enabled = 1;
}

void gveh_tank4_modern(gveh_tank4_cfg *c)
{
    gveh_tank4_default(c);
    gveh_tank_tracks_heavy(&c->tracks);
    gveh_tank_turret_modern(&c->turret);
    gveh_tank_gun_modern120(&c->gun);
    gveh_tank_armor_modern(&c->armor);
    gveh_tank_firecontrol_modern(&c->firecontrol);
}

void gveh_tank4_t72(gveh_tank4_cfg *c)
{
    gveh_tank4_modern(c);
    gveh_tank_gun_soviet125(&c->gun);
    gveh_tank_armor_t72(&c->armor);
    gveh_tank_crew_autoloader(&c->crew);
}

void gveh_tank4_tiger(gveh_tank4_cfg *c)
{
    gveh_tank4_default(c);
    gveh_tank_tracks_ww2(&c->tracks);
    gveh_tank_turret_ww2(&c->turret);
    gveh_tank_gun_ww2_88(&c->gun);
    gveh_tank_armor_tiger(&c->armor);
    gveh_tank_firecontrol_ww2(&c->firecontrol);
}

void gveh_tank4_sherman(gveh_tank4_cfg *c)
{
    gveh_tank4_default(c);
    gveh_tank_tracks_ww2(&c->tracks);
    gveh_tank_turret_ww2(&c->turret);
    gveh_tank_gun_ww2_75(&c->gun);
    gveh_tank_armor_sherman(&c->armor);
    gveh_tank_firecontrol_ww2(&c->firecontrol);
}

void gveh_tank4_destroyer(gveh_tank4_cfg *c)
{
    gveh_tank4_tiger(c);
    gveh_tank_turret_destroyer(&c->turret);
    c->platoon.wing_tanks = 1;
}

void gveh_tank4_scout(gveh_tank4_cfg *c)
{
    gveh_tank4_default(c);
    gveh_tank_tracks_scout(&c->tracks);
    gveh_tank_turret_modern(&c->turret);
    gveh_tank_gun_default(&c->gun);
    gveh_tank_armor_sherman(&c->armor);
    gveh_tank_firecontrol_modern(&c->firecontrol);
    c->platoon.wing_tanks = 2;
}

void gveh_tank4_clear(gveh_tank4_state *s, const gveh_tank4_cfg *c)
{
    gveh_tank_tracks_clear(&s->tracks, &c->tracks);
    gveh_tank_turret_clear(&s->turret);
    gveh_tank_gun_clear(&s->gun, &c->gun);
    gveh_tank_modules_clear(&s->modules, &c->modules);
    gveh_tank_crew_clear(&s->crew, &c->crew);
    gveh_tank_firecontrol_clear(&s->firecontrol);
    gveh_tank_station_clear(&s->station, &c->station);
    gveh_tank_platoon_clear(&s->platoon, &c->platoon);
    s->last_hit.face = 0;
    s->last_hit.ricochet = 0;
    s->last_hit.penetrated = 0;
    s->last_hit.effective = 0;
    s->last_hit.residual_pen = 0;
    s->last_hit.spall = 0;
    s->last_hit.shock = 0;
    s->last_fired = 0;
    s->last_penetrated = 0;
    s->alive = 1;
}

void gveh_tank4_step(const gveh_tank4_cfg *c, gveh_tank4_state *s, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx grounded_ratio, gveh_fx speed_abs, gveh_fx dt, gveh_i32 tick, gveh_fx_queue *fxq)
{
    const gveh_tank_ammo *ammo;
    gveh_i16 fired;
    gveh_i16 face;
    gveh_fx angle;
    if (c->enabled == 0 || s->alive == 0) return;
    if (grounded_ratio <= 0) grounded_ratio = GVEH_FX_ONE;
    gveh_tank_station_step(&c->station, &s->station, in);
    gveh_tank_firecontrol_step(&c->firecontrol, &s->firecontrol, in, &s->turret, &s->gun, speed_abs, tick);
    if (s->modules.engine_dead == 0 && s->crew.bail == 0) gveh_tank_tracks_apply(&c->tracks, &s->tracks, body, basis, in, grounded_ratio, dt);
    if (s->modules.turret_stuck == 0) gveh_tank_turret_step(&c->turret, &s->turret, in, body->yaw, speed_abs, dt);
    fired = 0;
    if (s->modules.gun_dead == 0 && s->crew.bail == 0) fired = gveh_tank_gun_step(&c->gun, &s->gun, in, s->firecontrol.range, dt, fxq, body->pos);
    s->last_fired = fired;
    s->last_penetrated = 0;
    if (fired != 0) {
        gveh_tank_turret_fire_recoil(&c->turret, &s->turret);
        ammo = gveh_tank_gun_selected_ammo(&c->gun, &s->gun);
        if (in->tank_face > 0) face = (gveh_i16)gveh_fx_to_int(in->tank_face);
        else face = (gveh_i16)((tick / 90) & 3);
        if (face < 0) face = 0;
        if (face > 3) face = 3;
        angle = gveh_fx_abs(in->steer) * 24;
        if (s->platoon.hull_down != 0) angle += gveh_fx_from_int(20);
        s->last_hit = gveh_tank_armor_resolve(&c->armor, ammo, s->gun.last_pen, face, angle);
        s->last_penetrated = s->last_hit.penetrated;
        gveh_tank_modules_apply_hit(&c->modules, &s->modules, &s->last_hit, tick, fxq, body->pos);
        gveh_tank_crew_apply_hit(&c->crew, &s->crew, &s->last_hit, tick + 3);
        if (s->last_hit.penetrated != 0) gveh_fx_push(fxq, GVEH_FX_EVENT_IMPACT, 95, body->pos.x, body->pos.y, body->pos.z);
        else gveh_fx_push(fxq, GVEH_FX_EVENT_SKID, 50, body->pos.x, body->pos.y, body->pos.z);
        gveh_body_add_force(body, gveh_v3_scale(basis.fwd, -c->gun.recoil_force));
    }
    if (s->modules.health[GVEH_TANK_MODULE_LEFT_TRACK] <= 0) gveh_tank_tracks_damage(&s->tracks, gveh_fx_from_int(100), 0);
    if (s->modules.health[GVEH_TANK_MODULE_RIGHT_TRACK] <= 0) gveh_tank_tracks_damage(&s->tracks, 0, gveh_fx_from_int(100));
    gveh_tank_platoon_step(&c->platoon, &s->platoon, in, fired, s->last_penetrated, dt);
    if (s->modules.ammo_cookoff != 0 || s->crew.bail != 0) s->alive = 0;
}

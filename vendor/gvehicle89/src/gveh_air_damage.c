#include "gveh_air_damage.h"

static void gveh_ad_set_parts(gveh_air_damage_cfg *cfg, gveh_fx hp)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_AIRDAMAGE_COUNT) {
        cfg->part_hp[i] = hp;
        i++;
    }
}

void gveh_air_damage_default(gveh_air_damage_cfg *cfg)
{
    cfg->flags = 0;
    gveh_ad_set_parts(cfg, gveh_fx_from_int(100));
    cfg->impact_threshold = gveh_fx_from_int(18);
    cfg->leak_rate = GVEH_FX_ONE / 5;
    cfg->fire_rate = GVEH_FX_ONE / 3;
    cfg->control_loss_k = GVEH_FX_ONE / 3;
    cfg->autorotate_collective = GVEH_FX_ONE / 2;
}

void gveh_air_damage_arcade(gveh_air_damage_cfg *cfg)
{
    gveh_air_damage_default(cfg);
    cfg->flags = GVEH_AIRDAMAGE_FLAG_ENABLED | GVEH_AIRDAMAGE_FLAG_ARCADE | GVEH_AIRDAMAGE_FLAG_FUEL_LEAK;
    gveh_ad_set_parts(cfg, gveh_fx_from_int(220));
    cfg->impact_threshold = gveh_fx_from_int(28);
    cfg->leak_rate = GVEH_FX_ONE / 12;
    cfg->fire_rate = GVEH_FX_ONE / 10;
    cfg->control_loss_k = GVEH_FX_ONE / 6;
}

void gveh_air_damage_sim(gveh_air_damage_cfg *cfg)
{
    gveh_air_damage_default(cfg);
    cfg->flags = GVEH_AIRDAMAGE_FLAG_ENABLED | GVEH_AIRDAMAGE_FLAG_AUTOROTATE | GVEH_AIRDAMAGE_FLAG_FUEL_LEAK;
    cfg->part_hp[GVEH_AIRDAMAGE_ENGINE] = gveh_fx_from_int(120);
    cfg->part_hp[GVEH_AIRDAMAGE_ROTOR] = gveh_fx_from_int(110);
    cfg->part_hp[GVEH_AIRDAMAGE_WING_L] = gveh_fx_from_int(100);
    cfg->part_hp[GVEH_AIRDAMAGE_WING_R] = gveh_fx_from_int(100);
    cfg->part_hp[GVEH_AIRDAMAGE_TAIL] = gveh_fx_from_int(80);
    cfg->part_hp[GVEH_AIRDAMAGE_FUEL] = gveh_fx_from_int(70);
    cfg->impact_threshold = gveh_fx_from_int(12);
    cfg->leak_rate = GVEH_FX_ONE / 4;
    cfg->fire_rate = GVEH_FX_ONE / 5;
    cfg->control_loss_k = GVEH_FX_ONE / 2;
}

void gveh_air_damage_clear(gveh_air_damage_state *st, const gveh_air_damage_cfg *cfg)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_AIRDAMAGE_COUNT) {
        st->part_hp[i] = cfg->part_hp[i];
        i++;
    }
    st->fuel_leak = 0;
    st->fire = 0;
    st->control_loss = 0;
    st->last_damage = 0;
    st->engine_dead = 0;
    st->rotor_bad = 0;
    st->tail_bad = 0;
    st->autorotate = 0;
}

static void gveh_ad_damage_part(gveh_air_damage_state *st, gveh_i32 idx, gveh_fx dmg)
{
    if (idx < 0 || idx >= GVEH_AIRDAMAGE_COUNT) return;
    st->part_hp[idx] -= dmg;
    if (st->part_hp[idx] < 0) st->part_hp[idx] = 0;
}

void gveh_air_damage_step(gveh_air_damage_state *st, const gveh_air_damage_cfg *cfg, gveh_body *body, gveh_airgame_state *airgame, gveh_fx impact, gveh_fx hazard, gveh_fx dt)
{
    gveh_fx dmg;
    gveh_fx leak;
    gveh_fx loss;
    if ((cfg->flags & GVEH_AIRDAMAGE_FLAG_ENABLED) == 0u) return;
    st->last_damage = 0;
    dmg = 0;
    if (impact > cfg->impact_threshold) dmg += (impact - cfg->impact_threshold);
    if (hazard > 0) dmg += hazard;
    if (dmg > 0) {
        st->last_damage = dmg;
        if (gveh_fx_abs(body->yaw_vel) > gveh_fx_abs(body->pitch_vel)) gveh_ad_damage_part(st, GVEH_AIRDAMAGE_TAIL, dmg / 2);
        if (body->vel.y < -gveh_fx_from_int(8)) gveh_ad_damage_part(st, GVEH_AIRDAMAGE_ROTOR, dmg / 2);
        if (body->roll_vel > 0) gveh_ad_damage_part(st, GVEH_AIRDAMAGE_WING_L, dmg / 3);
        else gveh_ad_damage_part(st, GVEH_AIRDAMAGE_WING_R, dmg / 3);
        gveh_ad_damage_part(st, GVEH_AIRDAMAGE_ENGINE, dmg / 4);
        gveh_ad_damage_part(st, GVEH_AIRDAMAGE_FUEL, dmg / 5);
    }
    st->engine_dead = (st->part_hp[GVEH_AIRDAMAGE_ENGINE] <= 0) ? 1 : 0;
    st->rotor_bad = (st->part_hp[GVEH_AIRDAMAGE_ROTOR] < cfg->part_hp[GVEH_AIRDAMAGE_ROTOR] / 3) ? 1 : 0;
    st->tail_bad = (st->part_hp[GVEH_AIRDAMAGE_TAIL] < cfg->part_hp[GVEH_AIRDAMAGE_TAIL] / 3) ? 1 : 0;
    if ((cfg->flags & GVEH_AIRDAMAGE_FLAG_AUTOROTATE) != 0u && st->engine_dead != 0 && body->vel.y < 0) st->autorotate = 1;
    if ((cfg->flags & GVEH_AIRDAMAGE_FLAG_FUEL_LEAK) != 0u) {
        loss = cfg->part_hp[GVEH_AIRDAMAGE_FUEL] - st->part_hp[GVEH_AIRDAMAGE_FUEL];
        if (loss > 0) st->fuel_leak = gveh_fx_mul(loss, cfg->leak_rate) / 64;
        if (st->fuel_leak > 0 && airgame->fuel > 0) {
            leak = gveh_fx_mul(st->fuel_leak, dt);
            airgame->fuel -= leak;
            if (airgame->fuel < 0) airgame->fuel = 0;
        }
    }
    if (st->part_hp[GVEH_AIRDAMAGE_ENGINE] < cfg->part_hp[GVEH_AIRDAMAGE_ENGINE] / 4 && hazard > 0) st->fire += gveh_fx_mul(cfg->fire_rate, dt);
    if (st->fire > GVEH_FX_ONE && airgame->armor > 0) {
        airgame->armor -= gveh_fx_mul(st->fire, dt);
        if (airgame->armor < 0) airgame->armor = 0;
    }
    loss = 0;
    loss += cfg->part_hp[GVEH_AIRDAMAGE_TAIL] - st->part_hp[GVEH_AIRDAMAGE_TAIL];
    loss += cfg->part_hp[GVEH_AIRDAMAGE_WING_L] - st->part_hp[GVEH_AIRDAMAGE_WING_L];
    loss += cfg->part_hp[GVEH_AIRDAMAGE_WING_R] - st->part_hp[GVEH_AIRDAMAGE_WING_R];
    st->control_loss = gveh_fx_mul(loss / 128, cfg->control_loss_k);
    if (st->control_loss > GVEH_FX_ONE) st->control_loss = GVEH_FX_ONE;
    if (st->tail_bad != 0) body->torque.y += gveh_fx_mul(body->yaw_vel + GVEH_FX_ONE, cfg->control_loss_k * 4);
    if (st->autorotate != 0) body->vel.y += gveh_fx_mul(cfg->autorotate_collective, dt) * 2;
}

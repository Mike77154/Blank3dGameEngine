#include "gveh_airgame.h"

void gveh_airgame_default(gveh_airgame_cfg *cfg)
{
    cfg->flags = 0;
    cfg->fuel_max = 0;
    cfg->fuel_burn_idle = 0;
    cfg->fuel_burn_power = 0;
    cfg->cannon_max = 0;
    cfg->missile_max = 0;
    cfg->rocket_max = 0;
    cfg->fire_cooldown = 0;
    cfg->missile_cooldown = 0;
    cfg->lock_time = 0;
    cfg->lock_cone = 0;
    cfg->winch_rate = 0;
    cfg->armor_max = 0;
}

void gveh_airgame_arcade(gveh_airgame_cfg *cfg)
{
    cfg->flags = GVEH_AIRGAME_FLAG_ENABLED | GVEH_AIRGAME_FLAG_LOCKON | GVEH_AIRGAME_FLAG_ARCADE_AMMO;
    cfg->fuel_max = gveh_fx_from_int(999);
    cfg->fuel_burn_idle = 0;
    cfg->fuel_burn_power = 0;
    cfg->cannon_max = 999;
    cfg->missile_max = 80;
    cfg->rocket_max = 120;
    cfg->fire_cooldown = 4;
    cfg->missile_cooldown = 18;
    cfg->lock_time = 18;
    cfg->lock_cone = GVEH_FX_ONE / 2;
    cfg->winch_rate = 0;
    cfg->armor_max = gveh_fx_from_int(100);
}

void gveh_airgame_strike(gveh_airgame_cfg *cfg)
{
    cfg->flags = GVEH_AIRGAME_FLAG_ENABLED | GVEH_AIRGAME_FLAG_LOCKON | GVEH_AIRGAME_FLAG_RESCUE | GVEH_AIRGAME_FLAG_FUEL;
    cfg->fuel_max = gveh_fx_from_int(100);
    cfg->fuel_burn_idle = GVEH_FX_ONE / 60;
    cfg->fuel_burn_power = GVEH_FX_ONE / 20;
    cfg->cannon_max = 300;
    cfg->missile_max = 32;
    cfg->rocket_max = 76;
    cfg->fire_cooldown = 5;
    cfg->missile_cooldown = 24;
    cfg->lock_time = 24;
    cfg->lock_cone = GVEH_FX_ONE / 3;
    cfg->winch_rate = GVEH_FX_ONE / 8;
    cfg->armor_max = gveh_fx_from_int(100);
}

void gveh_airgame_clear(gveh_airgame_state *st, const gveh_airgame_cfg *cfg)
{
    st->fuel = cfg->fuel_max;
    st->armor = cfg->armor_max;
    st->cannon = cfg->cannon_max;
    st->missiles = cfg->missile_max;
    st->rockets = cfg->rocket_max;
    st->fire_timer = 0;
    st->missile_timer = 0;
    st->lock_timer = 0;
    st->locked = 0;
    st->cargo_count = 0;
    st->winch_len = 0;
    st->empty = 0;
}

void gveh_airgame_step(gveh_airgame_state *st, const gveh_airgame_cfg *cfg, const gveh_input *in, gveh_body *body, gveh_basis basis, gveh_fx engine_power, gveh_fx dt, gveh_fx_queue *fxq)
{
    gveh_fx burn;
    if ((cfg->flags & GVEH_AIRGAME_FLAG_ENABLED) == 0u) return;
    if (st->fire_timer > 0) st->fire_timer--;
    if (st->missile_timer > 0) st->missile_timer--;
    if ((cfg->flags & GVEH_AIRGAME_FLAG_FUEL) != 0u && st->fuel > 0) {
        burn = cfg->fuel_burn_idle + gveh_fx_mul(cfg->fuel_burn_power, engine_power);
        st->fuel -= gveh_fx_mul(burn, dt * 30);
        if (st->fuel <= 0) { st->fuel = 0; st->empty = 1; }
    }
    if (in->lock_on > 0 && cfg->lock_time > 0) {
        if (st->lock_timer < cfg->lock_time) st->lock_timer++;
        if (st->lock_timer >= cfg->lock_time) st->locked = 1;
    } else {
        if (st->lock_timer > 0) st->lock_timer--;
        if (st->lock_timer == 0) st->locked = 0;
    }
    if (in->fire > 0 && st->fire_timer == 0) {
        if (st->cannon > 0 || (cfg->flags & GVEH_AIRGAME_FLAG_ARCADE_AMMO) != 0u) {
            if (st->cannon > 0) st->cannon--;
            st->fire_timer = cfg->fire_cooldown;
            gveh_fx_push(fxq, GVEH_FX_EVENT_ENGINE, 7, body->pos.x + basis.fwd.x, body->pos.y + basis.fwd.y, body->pos.z + basis.fwd.z);
        }
    }
    if (in->lock_on > 0 && in->fire > 0 && st->locked != 0 && st->missile_timer == 0) {
        if (st->missiles > 0 || (cfg->flags & GVEH_AIRGAME_FLAG_ARCADE_AMMO) != 0u) {
            if (st->missiles > 0) st->missiles--;
            st->missile_timer = cfg->missile_cooldown;
            gveh_fx_push(fxq, GVEH_FX_EVENT_IMPACT, 20, body->pos.x + basis.fwd.x * 2, body->pos.y, body->pos.z + basis.fwd.z * 2);
        }
    }
    if ((cfg->flags & GVEH_AIRGAME_FLAG_RESCUE) != 0u && in->winch > 0) {
        st->winch_len += gveh_fx_mul(cfg->winch_rate, dt * 30);
        if (st->winch_len > gveh_fx_from_int(4)) {
            st->cargo_count++;
            st->winch_len = 0;
            gveh_fx_push(fxq, GVEH_FX_EVENT_SPLASH, 9, body->pos.x, body->pos.y - gveh_fx_from_int(2), body->pos.z);
        }
    }
}

#include "gveh_wingman.h"

void gveh_wingman_default(gveh_wingman_cfg *cfg)
{
    cfg->flags = 0;
    cfg->count = 0;
    cfg->order_delay = 30;
    cfg->attack_skill = GVEH_FX_ONE / 2;
    cfg->cover_skill = GVEH_FX_ONE / 2;
    cfg->morale_k = GVEH_FX_ONE / 4;
}

void gveh_wingman_arcade(gveh_wingman_cfg *cfg)
{
    gveh_wingman_default(cfg);
    cfg->flags = GVEH_WINGMAN_FLAG_ENABLED;
    cfg->count = 2;
    cfg->order_delay = 12;
    cfg->attack_skill = GVEH_FX_ONE;
    cfg->cover_skill = GVEH_FX_ONE / 2;
}

void gveh_wingman_sim(gveh_wingman_cfg *cfg)
{
    gveh_wingman_default(cfg);
    cfg->flags = GVEH_WINGMAN_FLAG_ENABLED;
    cfg->count = 3;
    cfg->order_delay = 32;
    cfg->attack_skill = GVEH_FX_ONE / 2;
    cfg->cover_skill = GVEH_FX_ONE;
}

void gveh_wingman_clear(gveh_wingman_state *st, const gveh_wingman_cfg *cfg)
{
    gveh_i32 i;
    st->active_count = cfg->count;
    if (st->active_count > GVEH_WINGMAN_MAX) st->active_count = GVEH_WINGMAN_MAX;
    st->squad_score = 0;
    st->last_ack = 0;
    i = 0;
    while (i < GVEH_WINGMAN_MAX) {
        st->unit[i].hp = 100;
        st->unit[i].ammo = 12;
        st->unit[i].command = GVEH_WINGMAN_CMD_FORM;
        st->unit[i].cooldown = 0;
        st->unit[i].morale = 60;
        i++;
    }
}

void gveh_wingman_step(gveh_wingman_state *st, const gveh_wingman_cfg *cfg, const gveh_input *in, gveh_i16 locked, gveh_i16 threat)
{
    gveh_i32 i;
    gveh_i16 cmd;
    if ((cfg->flags & GVEH_WINGMAN_FLAG_ENABLED) == 0u) return;
    cmd = GVEH_WINGMAN_CMD_FORM;
    if (in->aggression > GVEH_FX_HALF || locked != 0) cmd = GVEH_WINGMAN_CMD_ATTACK;
    if (threat > 0 && in->risk > 0) cmd = GVEH_WINGMAN_CMD_COVER;
    if (in->winch > 0) cmd = GVEH_WINGMAN_CMD_RESCUE;
    i = 0;
    while (i < st->active_count) {
        if (st->unit[i].cooldown > 0) st->unit[i].cooldown--;
        if (st->unit[i].cooldown == 0 && st->unit[i].command != cmd) {
            st->unit[i].command = cmd;
            st->unit[i].cooldown = cfg->order_delay;
            st->last_ack = (gveh_i16)(i + 1);
        }
        if (st->unit[i].command == GVEH_WINGMAN_CMD_ATTACK && st->unit[i].ammo > 0 && locked != 0) {
            st->unit[i].ammo--;
            st->squad_score += (gveh_i16)(1 + gveh_fx_to_int(cfg->attack_skill));
        }
        if (st->unit[i].command == GVEH_WINGMAN_CMD_COVER && threat > 0) st->unit[i].morale += 1;
        if (st->unit[i].morale > 100) st->unit[i].morale = 100;
        i++;
    }
}

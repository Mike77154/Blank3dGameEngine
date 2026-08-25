#include "gveh_air_mission.h"

void gveh_air_mission_default(gveh_air_mission_cfg *cfg)
{
    cfg->flags = 0;
    cfg->destroy_need = 0;
    cfg->rescue_need = 0;
    cfg->recon_need = 0;
    cfg->protect_need = 0;
    cfg->bronze_score = 20;
    cfg->silver_score = 45;
    cfg->gold_score = 80;
}

void gveh_air_mission_strike(gveh_air_mission_cfg *cfg)
{
    gveh_air_mission_default(cfg);
    cfg->flags = GVEH_AIRMISSION_FLAG_ENABLED | GVEH_AIRMISSION_FLAG_MEDALS | GVEH_AIRMISSION_FLAG_DYNAMIC;
    cfg->destroy_need = 6;
    cfg->rescue_need = 2;
    cfg->recon_need = 1;
    cfg->protect_need = 1;
}

void gveh_air_mission_ace(gveh_air_mission_cfg *cfg)
{
    gveh_air_mission_default(cfg);
    cfg->flags = GVEH_AIRMISSION_FLAG_ENABLED | GVEH_AIRMISSION_FLAG_MEDALS;
    cfg->destroy_need = 10;
    cfg->rescue_need = 0;
    cfg->recon_need = 1;
    cfg->protect_need = 0;
    cfg->bronze_score = 40;
    cfg->silver_score = 90;
    cfg->gold_score = 150;
}

void gveh_air_mission_clear(gveh_air_mission_state *st)
{
    st->destroyed = 0;
    st->rescued = 0;
    st->reconned = 0;
    st->protected_count = 0;
    st->score = 0;
    st->medal = GVEH_AIRMISSION_NONE;
    st->complete = 0;
    st->campaign_push = 0;
}

void gveh_air_mission_step(gveh_air_mission_state *st, const gveh_air_mission_cfg *cfg, const gveh_input *in, gveh_i16 locked, gveh_i16 fired, gveh_i16 cargo, gveh_i16 takedown)
{
    if ((cfg->flags & GVEH_AIRMISSION_FLAG_ENABLED) == 0u) return;
    if (locked != 0 && fired != 0) {
        st->destroyed++;
        st->score += 8;
    }
    if (takedown != 0) {
        st->destroyed++;
        st->score += 12;
    }
    if (in->winch > 0 && cargo > st->rescued) {
        st->rescued = cargo;
        st->score += 15;
    }
    if (locked != 0 && st->reconned < cfg->recon_need) {
        st->reconned++;
        st->score += 5;
    }
    if (in->risk > 0 && st->protected_count < cfg->protect_need) {
        st->protected_count++;
        st->score += 5;
    }
    if (st->destroyed >= cfg->destroy_need && st->rescued >= cfg->rescue_need && st->reconned >= cfg->recon_need && st->protected_count >= cfg->protect_need) {
        st->complete = 1;
    }
    if ((cfg->flags & GVEH_AIRMISSION_FLAG_MEDALS) != 0u) {
        if (st->score >= cfg->gold_score) st->medal = GVEH_AIRMISSION_GOLD;
        else if (st->score >= cfg->silver_score) st->medal = GVEH_AIRMISSION_SILVER;
        else if (st->score >= cfg->bronze_score) st->medal = GVEH_AIRMISSION_BRONZE;
    }
    if ((cfg->flags & GVEH_AIRMISSION_FLAG_DYNAMIC) != 0u && st->complete != 0) st->campaign_push = 1;
}

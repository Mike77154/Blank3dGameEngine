#include "gveh_aftertouch.h"

void gveh_aftertouch_default(gveh_aftertouch_cfg *cfg)
{
    cfg->max_ticks = 45;
    cfg->force = gveh_fx_from_int(650);
    cfg->yaw_force = gveh_fx_from_int(18);
    cfg->min_crash = gveh_fx_from_int(7);
}

void gveh_aftertouch_clear(gveh_aftertouch_state *st)
{
    st->ticks = 0;
    st->flags = 0;
}

void gveh_aftertouch_trigger(gveh_aftertouch_state *st, const gveh_aftertouch_cfg *cfg, gveh_fx impact)
{
    if (impact >= cfg->min_crash) {
        st->ticks = cfg->max_ticks;
        st->flags = GVEH_AFTERTOUCH_ACTIVE;
    }
}

void gveh_aftertouch_apply(gveh_aftertouch_state *st, const gveh_aftertouch_cfg *cfg, gveh_body *body, gveh_basis basis, gveh_fx control_x, gveh_fx control_z)
{
    gveh_vec3 f;
    if (st->ticks <= 0) {
        st->flags = 0;
        return;
    }
    f = gveh_v3_add(gveh_v3_scale(basis.right, gveh_fx_mul(control_x, cfg->force)), gveh_v3_scale(basis.fwd, gveh_fx_mul(control_z, cfg->force)));
    gveh_body_add_force(body, f);
    gveh_body_add_yaw_torque(body, gveh_fx_mul(control_x, cfg->yaw_force));
    st->ticks--;
    if (st->ticks <= 0) st->flags = 0;
}

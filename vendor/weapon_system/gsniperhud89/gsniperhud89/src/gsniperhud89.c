#include "../include/gsniperhud89.h"

#define GSH89_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

static gsh89_cmd gsh89_make_cmd(short kind, short id,
                                short x0, short y0,
                                short x1, short y1,
                                short thickness, short alpha)
{
    gsh89_cmd c;
    c.kind = kind;
    c.id = id;
    c.x0 = x0;
    c.y0 = y0;
    c.x1 = x1;
    c.y1 = y1;
    c.thickness = thickness;
    c.alpha = alpha;
    c.value0 = 0;
    c.value1 = 0;
    c.value2 = 0;
    c.value3 = 0;
    return c;
}

void gsh89_init(gsh89_ctx *ctx,
                const gsh89_profile *profile,
                short screen_w,
                short screen_h,
                gsh89_emit_cb emit_cb,
                void *user)
{
    if (!ctx) return;
    if (profile) ctx->profile = *profile;
    else ctx->profile = gsh89_profile_sniper_default();
    ctx->screen_w = screen_w;
    ctx->screen_h = screen_h;
    ctx->visible = 0;
    ctx->blend_x1000 = 0;
    ctx->emit_cb = emit_cb;
    ctx->user = user;
}

void gsh89_set_profile(gsh89_ctx *ctx, const gsh89_profile *profile)
{
    if (!ctx || !profile) return;
    ctx->profile = *profile;
}

void gsh89_set_screen(gsh89_ctx *ctx, short screen_w, short screen_h)
{
    if (!ctx) return;
    ctx->screen_w = screen_w;
    ctx->screen_h = screen_h;
}

void gsh89_set_visibility(gsh89_ctx *ctx,
                          short visible,
                          short blend_x1000)
{
    if (!ctx) return;
    ctx->visible = visible ? 1 : 0;
    ctx->blend_x1000 = (short)GSH89_CLAMP(blend_x1000, 0, 1000);
}

void gsh89_emit_overlay(gsh89_ctx *ctx)
{
    short cx;
    short cy;
    short radius;
    short gap;
    short arm;
    short i;
    short off;
    short alpha;
    gsh89_cmd c;

    if (!ctx || !ctx->emit_cb || !ctx->visible) return;
    cx = (short)(ctx->screen_w / 2);
    cy = (short)(ctx->screen_h / 2);
    radius = (short)((ctx->screen_w < ctx->screen_h ?
                      ctx->screen_w : ctx->screen_h) / 2);
    gap = ctx->profile.reticle_gap_px;
    arm = ctx->profile.reticle_arm_px;

    if (ctx->profile.flags & GSH89_HUD_BLACKOUT) {
        alpha = (short)(((long)ctx->profile.mask_alpha *
                         (long)ctx->blend_x1000) / 1000L);
        c = gsh89_make_cmd(GSH89_CMD_SCOPE_MASK,
                           ctx->profile.overlay_id,
                           cx, cy, radius, 0, 1, alpha);
        ctx->emit_cb(ctx->user, &c);
    }

    if (ctx->profile.flags & GSH89_HUD_CROSSHAIR) {
        c = gsh89_make_cmd(GSH89_CMD_RETICLE_LINE,
                           ctx->profile.overlay_id,
                           (short)(cx - gap - arm), cy,
                           (short)(cx - gap), cy,
                           ctx->profile.reticle_thickness, 230);
        ctx->emit_cb(ctx->user, &c);
        c = gsh89_make_cmd(GSH89_CMD_RETICLE_LINE,
                           ctx->profile.overlay_id,
                           (short)(cx + gap), cy,
                           (short)(cx + gap + arm), cy,
                           ctx->profile.reticle_thickness, 230);
        ctx->emit_cb(ctx->user, &c);
        c = gsh89_make_cmd(GSH89_CMD_RETICLE_LINE,
                           ctx->profile.overlay_id,
                           cx, (short)(cy - gap - arm),
                           cx, (short)(cy - gap),
                           ctx->profile.reticle_thickness, 230);
        ctx->emit_cb(ctx->user, &c);
        c = gsh89_make_cmd(GSH89_CMD_RETICLE_LINE,
                           ctx->profile.overlay_id,
                           cx, (short)(cy + gap),
                           cx, (short)(cy + gap + arm),
                           ctx->profile.reticle_thickness, 230);
        ctx->emit_cb(ctx->user, &c);
    }

    if (ctx->profile.flags & GSH89_HUD_CENTER_DOT) {
        c = gsh89_make_cmd(GSH89_CMD_RETICLE_DOT,
                           ctx->profile.overlay_id,
                           cx, cy, 2, 2, 1, 255);
        ctx->emit_cb(ctx->user, &c);
    }

    if (ctx->profile.flags & GSH89_HUD_RANGE_TICKS) {
        for (i = 1; i <= ctx->profile.tick_count; ++i) {
            off = (short)(i * ctx->profile.tick_spacing_px);
            c = gsh89_make_cmd(GSH89_CMD_RANGE_TICK,
                               ctx->profile.overlay_id,
                               (short)(cx - 6), (short)(cy + off),
                               (short)(cx + 6), (short)(cy + off),
                               1, 190);
            ctx->emit_cb(ctx->user, &c);
        }
    }

    if (ctx->profile.flags & GSH89_HUD_VIGNETTE) {
        alpha = (short)(((long)ctx->profile.vignette_alpha *
                         (long)ctx->blend_x1000) / 1000L);
        c = gsh89_make_cmd(GSH89_CMD_VIGNETTE,
                           ctx->profile.overlay_id,
                           0, 0, ctx->screen_w, ctx->screen_h,
                           1, alpha);
        ctx->emit_cb(ctx->user, &c);
    }
}

void gsh89_emit_telemetry(gsh89_ctx *ctx,
                          const gsh89_telemetry *telemetry)
{
    gsh89_cmd c;
    if (!ctx || !ctx->emit_cb || !telemetry) return;

    if (ctx->profile.flags & GSH89_HUD_OPTIC_DATA) {
        c = gsh89_make_cmd(GSH89_CMD_OPTIC_TELEMETRY,
                           ctx->profile.hud_id,
                           telemetry->current_fov_deg_x100,
                           telemetry->zoom_x100,
                           telemetry->sensitivity_pct,
                           ctx->blend_x1000,
                           0, 255);
        c.value0 = telemetry->sway_yaw_deg_x1000;
        c.value1 = telemetry->sway_pitch_deg_x1000;
        ctx->emit_cb(ctx->user, &c);
    }

    if (ctx->profile.flags & GSH89_HUD_BREATH_DATA) {
        c = gsh89_make_cmd(GSH89_CMD_BREATH_TELEMETRY,
                           ctx->profile.hud_id,
                           telemetry->hold_remaining_pct,
                           telemetry->breath_exhausted,
                           telemetry->breath_wave_x1000,
                           0, 0, 255);
        ctx->emit_cb(ctx->user, &c);
    }

    if ((ctx->profile.flags & GSH89_HUD_TARGET_DATA) &&
        telemetry->target_valid) {
        c = gsh89_make_cmd(GSH89_CMD_TARGET_TELEMETRY,
                           ctx->profile.hud_id,
                           telemetry->target_id,
                           telemetry->target_kind,
                           telemetry->target_part,
                           0, 0, 255);
        c.value0 = telemetry->target_distance;
        ctx->emit_cb(ctx->user, &c);
    }

    if ((ctx->profile.flags & GSH89_HUD_IMPACT_DATA) &&
        telemetry->target_valid) {
        c = gsh89_make_cmd(GSH89_CMD_IMPACT_WORLD,
                           ctx->profile.hud_id,
                           0, 0, 0, 0, 0, 255);
        c.value0 = telemetry->impact_position.x;
        c.value1 = telemetry->impact_position.y;
        c.value2 = telemetry->impact_position.z;
        ctx->emit_cb(ctx->user, &c);
    }
}

gsh89_profile gsh89_profile_sniper_default(void)
{
    gsh89_profile p;
    p.overlay_id = 3;
    p.hud_id = 3;
    p.flags = GSH89_HUD_BLACKOUT |
              GSH89_HUD_CROSSHAIR |
              GSH89_HUD_CENTER_DOT |
              GSH89_HUD_RANGE_TICKS |
              GSH89_HUD_VIGNETTE |
              GSH89_HUD_OPTIC_DATA |
              GSH89_HUD_TARGET_DATA |
              GSH89_HUD_BREATH_DATA |
              GSH89_HUD_IMPACT_DATA;
    p.reticle_gap_px = 8;
    p.reticle_arm_px = 70;
    p.reticle_thickness = 1;
    p.tick_count = 4;
    p.tick_spacing_px = 24;
    p.mask_alpha = 255;
    p.vignette_alpha = 180;
    return p;
}

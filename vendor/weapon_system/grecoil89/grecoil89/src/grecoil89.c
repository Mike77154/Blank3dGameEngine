#include "grecoil89.h"

static grec_fp grec_max_abs(grec_fp v, grec_fp max_abs)
{
    if (max_abs <= 0) return v;
    if (v > max_abs) return max_abs;
    if (v < -max_abs) return -max_abs;
    return v;
}

static grec_u32 grec_rng_next(GRecState *st)
{
    grec_u32 x;
    if (st->rng == 0u) st->rng = 0x1234567u;
    x = st->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    st->rng = x;
    return x;
}

static grec_fp grec_rand_signed(GRecState *st, grec_fp amplitude)
{
    grec_u32 r;
    grec_s32 span;
    grec_s32 v;
    if (amplitude <= 0) return 0;
    r = grec_rng_next(st);
    span = (grec_s32)(amplitude * 2 + 1);
    if (span <= 1) return 0;
    v = (grec_s32)(r % (grec_u32)span);
    v -= amplitude;
    return (grec_fp)v;
}

grec_fp grec_mul(grec_fp a, grec_fp b)
{
    /*
       C89/no-long-long safe fixed multiply.
       Q8.8 keeps values small enough for gameplay recoil ranges.
       Clamp big inputs before multiply to avoid signed int overflow.
    */
    grec_s32 sign;
    grec_s32 aa;
    grec_s32 bb;
    grec_s32 hi;
    grec_s32 lo;
    grec_s32 r;

    sign = 1;
    aa = a;
    bb = b;
    if (aa < 0) { aa = -aa; sign = -sign; }
    if (bb < 0) { bb = -bb; sign = -sign; }

    if (aa > 32767) aa = 32767;
    if (bb > 32767) bb = 32767;

    hi = (aa >> 4) * (bb >> 4);
    lo = ((aa & 15) * (bb >> 4)) + ((bb & 15) * (aa >> 4));
    r = (hi >> 0) + (lo >> 4);

    if (sign < 0) r = -r;
    return (grec_fp)r;
}

grec_fp grec_clamp(grec_fp v, grec_fp mn, grec_fp mx)
{
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

grec_fp grec_lerp(grec_fp a, grec_fp b, grec_fp t)
{
    return a + grec_mul((b - a), t);
}

grec_fp grec_apply_scale(grec_fp v, grec_fp s)
{
    return grec_mul(v, s);
}

const char *grec_version_string(void)
{
    return "grecoil89 1.1.0";
}

void grec_state_init(GRecState *st, grec_u32 seed)
{
    if (st == 0) return;
    st->angle_pos.pitch = 0;
    st->angle_pos.yaw = 0;
    st->angle_pos.roll = 0;
    st->angle_vel.pitch = 0;
    st->angle_vel.yaw = 0;
    st->angle_vel.roll = 0;
    st->weapon_pos.side = 0;
    st->weapon_pos.up = 0;
    st->weapon_pos.back = 0;
    st->weapon_vel.side = 0;
    st->weapon_vel.up = 0;
    st->weapon_vel.back = 0;
    st->spread = 0;
    st->recovery_ticks = 0;
    st->no_fire_ticks = 0;
    st->burst_count = 0;
    st->shot_index = 0;
    st->rng = seed;
    st->active = 0;
}

void grec_reset(GRecState *st)
{
    grec_u32 seed;
    if (st == 0) return;
    seed = st->rng;
    grec_state_init(st, seed);
}

void grec_set_rng(GRecState *st, grec_u32 seed)
{
    if (st == 0) return;
    st->rng = seed;
}

void grec_context_default(GRecContext *ctx)
{
    if (ctx == 0) return;
    ctx->mode = GREC_MODE_HIP;
    ctx->recoil_scale = GREC_FP_ONE;
    ctx->aim_scale = GREC_FP_ONE;
    ctx->camera_scale = GREC_FP_ONE;
    ctx->weapon_scale = GREC_FP_ONE;
    ctx->spread_scale = GREC_FP_ONE;
}

void grec_context_for_mode(GRecContext *ctx, grec_u16 mode)
{
    grec_context_default(ctx);
    if (ctx == 0) return;
    ctx->mode = mode;

    if (mode == GREC_MODE_ADS) {
        ctx->recoil_scale = GREC_FP_FROM_RATIO(75, 100);
        ctx->aim_scale = GREC_FP_FROM_RATIO(80, 100);
        ctx->camera_scale = GREC_FP_FROM_RATIO(85, 100);
        ctx->weapon_scale = GREC_FP_FROM_RATIO(60, 100);
        ctx->spread_scale = GREC_FP_FROM_RATIO(70, 100);
    } else if (mode == GREC_MODE_MOUNTED) {
        ctx->recoil_scale = GREC_FP_FROM_RATIO(45, 100);
        ctx->aim_scale = GREC_FP_FROM_RATIO(50, 100);
        ctx->camera_scale = GREC_FP_FROM_RATIO(35, 100);
        ctx->weapon_scale = GREC_FP_FROM_RATIO(45, 100);
        ctx->spread_scale = GREC_FP_FROM_RATIO(50, 100);
    } else if (mode == GREC_MODE_FIXED_CAM) {
        ctx->recoil_scale = GREC_FP_FROM_RATIO(90, 100);
        ctx->aim_scale = GREC_FP_ONE;
        ctx->camera_scale = GREC_FP_FROM_RATIO(15, 100);
        ctx->weapon_scale = GREC_FP_ONE;
        ctx->spread_scale = GREC_FP_ONE;
    } else if (mode == GREC_MODE_TURRET) {
        ctx->recoil_scale = GREC_FP_FROM_RATIO(60, 100);
        ctx->aim_scale = GREC_FP_FROM_RATIO(65, 100);
        ctx->camera_scale = GREC_FP_FROM_RATIO(25, 100);
        ctx->weapon_scale = GREC_FP_FROM_RATIO(70, 100);
        ctx->spread_scale = GREC_FP_FROM_RATIO(70, 100);
    } else if (mode == GREC_MODE_SNIPER) {
        ctx->recoil_scale = GREC_FP_FROM_RATIO(110, 100);
        ctx->aim_scale = GREC_FP_FROM_RATIO(115, 100);
        ctx->camera_scale = GREC_FP_FROM_RATIO(130, 100);
        ctx->weapon_scale = GREC_FP_FROM_RATIO(75, 100);
        ctx->spread_scale = GREC_FP_FROM_RATIO(45, 100);
    }
}

static void grec_solve_axis(grec_fp *pos, grec_fp *vel, grec_fp spring, grec_fp damping, grec_fp fire_damping, int delaying, grec_fp max_abs)
{
    grec_fp acc;
    grec_fp used_damping;

    used_damping = delaying ? fire_damping : damping;
    acc = -grec_mul(*vel, used_damping);
    if (!delaying) acc -= grec_mul(*pos, spring);

    *vel += acc;
    *pos += *vel;
    *pos = grec_max_abs(*pos, max_abs);

    if (*pos == max_abs || *pos == -max_abs) {
        if ((*pos > 0 && *vel > 0) || (*pos < 0 && *vel < 0)) *vel = 0;
    }

    if (GREC_ABS(*pos) < 1 && GREC_ABS(*vel) < 1) {
        *pos = 0;
        *vel = 0;
    }
}

static void grec_decay_spread(GRecState *st, const GRecProfile *profile)
{
    grec_fp base;
    if ((profile->flags & GREC_FLAG_SPREAD_BLOOM) == 0u) return;
    base = profile->spread_base;
    if (st->spread > base) {
        st->spread -= profile->spread_recover;
        if (st->spread < base) st->spread = base;
    } else if (st->spread < base) {
        st->spread = base;
    }
}

void grec_update(GRecState *st, const GRecProfile *profile)
{
    int delaying;
    if (st == 0 || profile == 0) return;

    if (st->recovery_ticks > 0) st->recovery_ticks--;
    st->no_fire_ticks++;

    if (profile->burst_reset_ticks > 0 && st->no_fire_ticks > profile->burst_reset_ticks) {
        if ((profile->flags & GREC_FLAG_KEEP_BURST_HEAT) == 0u) {
            if (st->burst_count > 0) st->burst_count--;
            if (st->shot_index > 0) st->shot_index--;
        }
    }

    delaying = (st->recovery_ticks > 0) ? GREC_TRUE : GREC_FALSE;

    grec_solve_axis(&st->angle_pos.pitch, &st->angle_vel.pitch,
                    profile->angle_spring, profile->angle_damping,
                    profile->angle_fire_damping, delaying, profile->max_pitch);
    grec_solve_axis(&st->angle_pos.yaw, &st->angle_vel.yaw,
                    profile->angle_spring, profile->angle_damping,
                    profile->angle_fire_damping, delaying, profile->max_yaw);
    grec_solve_axis(&st->angle_pos.roll, &st->angle_vel.roll,
                    profile->angle_spring, profile->angle_damping,
                    profile->angle_fire_damping, delaying, profile->max_roll);

    grec_solve_axis(&st->weapon_pos.side, &st->weapon_vel.side,
                    profile->pos_spring, profile->pos_damping,
                    profile->pos_fire_damping, delaying, profile->max_side);
    grec_solve_axis(&st->weapon_pos.up, &st->weapon_vel.up,
                    profile->pos_spring, profile->pos_damping,
                    profile->pos_fire_damping, delaying, profile->max_up);
    grec_solve_axis(&st->weapon_pos.back, &st->weapon_vel.back,
                    profile->pos_spring, profile->pos_damping,
                    profile->pos_fire_damping, delaying, profile->max_back);

    grec_decay_spread(st, profile);

    st->active = (GREC_ABS(st->angle_pos.pitch) | GREC_ABS(st->angle_pos.yaw) |
                  GREC_ABS(st->angle_pos.roll) | GREC_ABS(st->weapon_pos.side) |
                  GREC_ABS(st->weapon_pos.up) | GREC_ABS(st->weapon_pos.back) |
                  GREC_ABS(st->angle_vel.pitch) | GREC_ABS(st->angle_vel.yaw) |
                  GREC_ABS(st->angle_vel.roll) | GREC_ABS(st->weapon_vel.side) |
                  GREC_ABS(st->weapon_vel.up) | GREC_ABS(st->weapon_vel.back)) ? 1 : 0;
}

void grec_fire(GRecState *st, const GRecProfile *profile, const GRecContext *ctx)
{
    GRecContext local_ctx;
    const GRecPatternStep *step;
    grec_u16 pattern_index;
    grec_fp scale;
    grec_fp pitch;
    grec_fp yaw;
    grec_fp roll;
    grec_fp side;
    grec_fp up;
    grec_fp back;
    grec_fp spread_add;

    if (st == 0 || profile == 0) return;
    if (ctx == 0) {
        grec_context_default(&local_ctx);
        ctx = &local_ctx;
    }

    scale = ctx->recoil_scale;

    pitch = profile->kick_pitch_vel;
    yaw = profile->kick_yaw_vel;
    roll = profile->kick_roll_vel;
    side = profile->kick_side_vel;
    up = profile->kick_up_vel;
    back = profile->kick_back_vel;
    spread_add = profile->spread_per_shot;

    if ((profile->flags & GREC_FLAG_PATTERN) != 0u && profile->pattern != 0 && profile->pattern_count > 0) {
        if (profile->pattern_loop) pattern_index = (grec_u16)(st->shot_index % profile->pattern_count);
        else if (st->shot_index >= profile->pattern_count) pattern_index = (grec_u16)(profile->pattern_count - 1);
        else pattern_index = st->shot_index;
        step = &profile->pattern[pattern_index];
        pitch += step->pitch_vel;
        yaw += step->yaw_vel;
        roll += step->roll_vel;
        side += step->side_vel;
        up += step->up_vel;
        back += step->back_vel;
        spread_add += step->spread_add;
    }

    if ((profile->flags & GREC_FLAG_RANDOM_PITCH) != 0u) pitch += grec_rand_signed(st, profile->random_pitch_vel);
    if ((profile->flags & GREC_FLAG_RANDOM_YAW) != 0u) yaw += grec_rand_signed(st, profile->random_yaw_vel);

    st->angle_vel.pitch += grec_mul(pitch, scale);
    st->angle_vel.yaw += grec_mul(yaw, scale);
    st->angle_vel.roll += grec_mul(roll, scale);
    st->weapon_vel.side += grec_mul(side, scale);
    st->weapon_vel.up += grec_mul(up, scale);
    st->weapon_vel.back += grec_mul(back, scale);

    if ((profile->flags & GREC_FLAG_SPREAD_BLOOM) != 0u) {
        spread_add = grec_mul(spread_add, ctx->spread_scale);
        st->spread += spread_add;
        if (st->spread < profile->spread_base) st->spread = profile->spread_base;
        if (st->spread > profile->spread_max) st->spread = profile->spread_max;
    }

    st->recovery_ticks = profile->recovery_delay_ticks;
    st->no_fire_ticks = 0;
    st->burst_count++;
    if (profile->max_burst_count > 0 && st->burst_count > profile->max_burst_count) st->burst_count = profile->max_burst_count;
    st->shot_index++;
    if (profile->max_burst_count > 0 && st->shot_index > profile->max_burst_count) st->shot_index = profile->max_burst_count;
    st->active = 1;
}

void grec_sample(const GRecState *st, const GRecProfile *profile, const GRecContext *ctx, GRecOutput *out)
{
    GRecContext local_ctx;
    grec_fp aim_scale;
    grec_fp cam_scale;
    grec_fp weap_scale;
    grec_fp weap_pos_scale;
    grec_fp spread_scale;

    if (out == 0) return;
    out->aim_angles.pitch = 0;
    out->aim_angles.yaw = 0;
    out->aim_angles.roll = 0;
    out->camera_angles.pitch = 0;
    out->camera_angles.yaw = 0;
    out->camera_angles.roll = 0;
    out->weapon_angles.pitch = 0;
    out->weapon_angles.yaw = 0;
    out->weapon_angles.roll = 0;
    out->weapon_offset.side = 0;
    out->weapon_offset.up = 0;
    out->weapon_offset.back = 0;
    out->spread = 0;
    out->burst_count = 0;
    out->shot_index = 0;
    out->active = 0;

    if (st == 0 || profile == 0) return;
    if (ctx == 0) {
        grec_context_default(&local_ctx);
        ctx = &local_ctx;
    }

    aim_scale = grec_mul(profile->aim_scale, ctx->aim_scale);
    cam_scale = grec_mul(profile->camera_scale, ctx->camera_scale);
    weap_scale = grec_mul(profile->weapon_angle_scale, ctx->weapon_scale);
    weap_pos_scale = grec_mul(profile->weapon_pos_scale, ctx->weapon_scale);
    spread_scale = ctx->spread_scale;

    if ((profile->flags & GREC_FLAG_AIM_RECOIL) != 0u) {
        out->aim_angles.pitch = grec_mul(st->angle_pos.pitch, aim_scale);
        out->aim_angles.yaw = grec_mul(st->angle_pos.yaw, aim_scale);
        out->aim_angles.roll = grec_mul(st->angle_pos.roll, aim_scale);
    }

    if ((profile->flags & GREC_FLAG_CAMERA_RECOIL) != 0u) {
        out->camera_angles.pitch = grec_mul(st->angle_pos.pitch, cam_scale);
        out->camera_angles.yaw = grec_mul(st->angle_pos.yaw, cam_scale);
        out->camera_angles.roll = grec_mul(st->angle_pos.roll, cam_scale);
    }

    if ((profile->flags & GREC_FLAG_WEAPON_RECOIL) != 0u) {
        out->weapon_angles.pitch = grec_mul(st->angle_pos.pitch, weap_scale);
        out->weapon_angles.yaw = grec_mul(st->angle_pos.yaw, weap_scale);
        out->weapon_angles.roll = grec_mul(st->angle_pos.roll, weap_scale);
        out->weapon_offset.side = grec_mul(st->weapon_pos.side, weap_pos_scale);
        out->weapon_offset.up = grec_mul(st->weapon_pos.up, weap_pos_scale);
        out->weapon_offset.back = grec_mul(st->weapon_pos.back, weap_pos_scale);
    }

    if ((profile->flags & GREC_FLAG_SPREAD_BLOOM) != 0u) out->spread = grec_mul(st->spread, spread_scale);
    out->burst_count = st->burst_count;
    out->shot_index = st->shot_index;
    out->active = st->active;
}

void grec_apply_callbacks(const GRecCallbacks *cb, const GRecOutput *out, int fire_event)
{
    if (cb == 0 || out == 0) return;
    if (cb->apply_camera != 0) cb->apply_camera(cb->user, &out->camera_angles);
    if (cb->apply_aim != 0) cb->apply_aim(cb->user, &out->aim_angles, out->spread);
    if (cb->apply_weapon != 0) cb->apply_weapon(cb->user, &out->weapon_angles, &out->weapon_offset);
    if (fire_event && cb->on_fire != 0) cb->on_fire(cb->user, out);
    if (!fire_event && cb->on_update != 0) cb->on_update(cb->user, out);
}

int grec_is_active(const GRecState *st)
{
    if (st == 0) return 0;
    return st->active ? 1 : 0;
}

void grec_clear_burst(GRecState *st)
{
    if (st == 0) return;
    st->burst_count = 0;
    st->shot_index = 0;
    st->no_fire_ticks = 0;
}

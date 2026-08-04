#include "wsounddna89.h"

static wsound89_u32 wdna_rng(wsounddna89_context *ctx)
{
    wsound89_u32 x;
    x = ctx->state;
    if (x == 0U) x = 0x57444E41U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    ctx->state = x;
    return x;
}

static wsound89_u16 wdna_clamp_u15(wsound89_i32 v)
{
    if (v < 0) return 0U;
    if (v > 32767) return 32767U;
    return (wsound89_u16)v;
}

static wsound89_i16 wdna_clamp_i16(wsound89_i32 v)
{
    if (v < -32768) return -32768;
    if (v > 32767) return 32767;
    return (wsound89_i16)v;
}

static wsound89_i32 wdna_mul_q15(wsound89_i32 a, wsound89_i32 b)
{
    return (a * b) >> 15;
}

static wsound89_i32 wdna_jitter(wsounddna89_context *ctx, wsound89_i32 span)
{
    wsound89_u32 r;
    wsound89_i32 centered;
    r = wdna_rng(ctx);
    centered = (wsound89_i32)((r >> 16) & 65535U) - 32768;
    return (centered * span) / 32768;
}

void wsounddna89_profile_defaults(wsounddna89_profile_id id,
                                  wsounddna89_profile *p)
{
    if (p == 0) return;
    p->id = id;
    p->weapon_class = WSOUNDDNA89_PISTOL;
    p->action = WSOUNDDNA89_ACTION_CLOSED_BOLT;
    p->bore_mm_x100 = 900U;
    p->barrel_length_mm = 110U;
    p->projectile_speed_mps = 350U;
    p->cyclic_rate_rpm = 0U;
    p->propellant_energy_q15 = 18000U;
    p->muzzle_pressure_q15 = 18500U;
    p->mechanical_mass_q15 = 13000U;
    p->suppressor_q15 = 0U;
    p->muzzle_brake_q15 = 0U;
    p->supersonic_q15 = 9000U;
    switch (id) {
    case WSOUNDDNA89_PROFILE_COMPACT_PISTOL:
        p->bore_mm_x100 = 900U; p->barrel_length_mm = 85U;
        p->projectile_speed_mps = 325U; p->propellant_energy_q15 = 15500U;
        p->muzzle_pressure_q15 = 19000U; p->mechanical_mass_q15 = 10500U;
        p->supersonic_q15 = 3500U; break;
    case WSOUNDDNA89_PROFILE_SERVICE_PISTOL:
        p->bore_mm_x100 = 900U; p->barrel_length_mm = 120U;
        p->projectile_speed_mps = 360U; p->propellant_energy_q15 = 18500U;
        p->muzzle_pressure_q15 = 20500U; p->mechanical_mass_q15 = 13500U;
        p->supersonic_q15 = 12000U; break;
    case WSOUNDDNA89_PROFILE_MAGNUM:
        p->weapon_class = WSOUNDDNA89_HEAVY; p->action = WSOUNDDNA89_ACTION_REVOLVER;
        p->bore_mm_x100 = 1100U; p->barrel_length_mm = 165U;
        p->projectile_speed_mps = 430U; p->propellant_energy_q15 = 27000U;
        p->muzzle_pressure_q15 = 27500U; p->mechanical_mass_q15 = 20500U;
        p->supersonic_q15 = 24500U; break;
    case WSOUNDDNA89_PROFILE_SMG:
        p->weapon_class = WSOUNDDNA89_MACHINE; p->action = WSOUNDDNA89_ACTION_OPEN_BOLT;
        p->bore_mm_x100 = 900U; p->barrel_length_mm = 210U;
        p->projectile_speed_mps = 390U; p->cyclic_rate_rpm = 780U;
        p->propellant_energy_q15 = 19000U; p->muzzle_pressure_q15 = 19500U;
        p->mechanical_mass_q15 = 18500U; p->supersonic_q15 = 16000U; break;
    case WSOUNDDNA89_PROFILE_CARBINE:
        p->weapon_class = WSOUNDDNA89_RIFLE; p->bore_mm_x100 = 556U;
        p->barrel_length_mm = 370U; p->projectile_speed_mps = 820U;
        p->cyclic_rate_rpm = 720U; p->propellant_energy_q15 = 24500U;
        p->muzzle_pressure_q15 = 25500U; p->mechanical_mass_q15 = 17500U;
        p->muzzle_brake_q15 = 3500U; p->supersonic_q15 = 31500U; break;
    case WSOUNDDNA89_PROFILE_RIFLE:
        p->weapon_class = WSOUNDDNA89_RIFLE; p->bore_mm_x100 = 762U;
        p->barrel_length_mm = 510U; p->projectile_speed_mps = 790U;
        p->cyclic_rate_rpm = 600U; p->propellant_energy_q15 = 28000U;
        p->muzzle_pressure_q15 = 27000U; p->mechanical_mass_q15 = 21000U;
        p->muzzle_brake_q15 = 2200U; p->supersonic_q15 = 32000U; break;
    case WSOUNDDNA89_PROFILE_SNIPER:
        p->weapon_class = WSOUNDDNA89_RIFLE; p->action = WSOUNDDNA89_ACTION_BOLT;
        p->bore_mm_x100 = 762U; p->barrel_length_mm = 660U;
        p->projectile_speed_mps = 860U; p->propellant_energy_q15 = 30500U;
        p->muzzle_pressure_q15 = 28500U; p->mechanical_mass_q15 = 24500U;
        p->muzzle_brake_q15 = 5000U; p->supersonic_q15 = 32767U; break;
    case WSOUNDDNA89_PROFILE_SHOTGUN:
        p->weapon_class = WSOUNDDNA89_SHOTGUN; p->action = WSOUNDDNA89_ACTION_PUMP;
        p->bore_mm_x100 = 1850U; p->barrel_length_mm = 510U;
        p->projectile_speed_mps = 410U; p->propellant_energy_q15 = 28500U;
        p->muzzle_pressure_q15 = 26000U; p->mechanical_mass_q15 = 25000U;
        p->supersonic_q15 = 2500U; break;
    case WSOUNDDNA89_PROFILE_HEAVY:
        p->weapon_class = WSOUNDDNA89_HEAVY; p->action = WSOUNDDNA89_ACTION_OPEN_BOLT;
        p->bore_mm_x100 = 1270U; p->barrel_length_mm = 1140U;
        p->projectile_speed_mps = 890U; p->cyclic_rate_rpm = 550U;
        p->propellant_energy_q15 = 32700U; p->muzzle_pressure_q15 = 31500U;
        p->mechanical_mass_q15 = 31000U; p->muzzle_brake_q15 = 9000U;
        p->supersonic_q15 = 32767U; break;
    case WSOUNDDNA89_PROFILE_LAUNCHER:
        p->weapon_class = WSOUNDDNA89_HEAVY; p->action = WSOUNDDNA89_ACTION_LAUNCHER;
        p->bore_mm_x100 = 4000U; p->barrel_length_mm = 950U;
        p->projectile_speed_mps = 145U; p->propellant_energy_q15 = 29000U;
        p->muzzle_pressure_q15 = 24500U; p->mechanical_mass_q15 = 27500U;
        p->supersonic_q15 = 0U; break;
    default:
        p->id = WSOUNDDNA89_PROFILE_SERVICE_PISTOL; break;
    }
}

int wsounddna89_validate_profile(const wsounddna89_profile *p)
{
    if (p == 0) return 0;
    if ((wsound89_u32)p->id >= (wsound89_u32)WSOUNDDNA89_PROFILE_COUNT) return 0;
    if ((wsound89_u32)p->weapon_class > (wsound89_u32)WSOUNDDNA89_HEAVY) return 0;
    if ((wsound89_u32)p->action > (wsound89_u32)WSOUNDDNA89_ACTION_LAUNCHER) return 0;
    if (p->barrel_length_mm == 0U || p->projectile_speed_mps == 0U) return 0;
    return 1;
}

wsound89_result wsounddna89_init(wsounddna89_context *ctx, wsound89_u32 seed)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    if (seed == 0U) seed = 0x57444E41U;
    ctx->state = seed;
    ctx->shot_index = 0U;
    wsounddna89_profile_defaults(WSOUNDDNA89_PROFILE_SERVICE_PISTOL, &ctx->profile);
    ctx->mode = WSOUNDDNA89_MODE_HYBRID;
    return WSOUND89_OK;
}

wsound89_result wsounddna89_set_profile(wsounddna89_context *ctx,
                                        const wsounddna89_profile *profile)
{
    if (ctx == 0 || !wsounddna89_validate_profile(profile)) return WSOUND89_EINVAL;
    ctx->profile = *profile;
    return WSOUND89_OK;
}

wsound89_result wsounddna89_set_mode(wsounddna89_context *ctx,
                                     wsounddna89_mode mode)
{
    if (ctx == 0 || (wsound89_u32)mode > (wsound89_u32)WSOUNDDNA89_MODE_CINEMATIC)
        return WSOUND89_EINVAL;
    ctx->mode = mode;
    return WSOUND89_OK;
}

wsound89_result wsounddna89_next_profiled(wsounddna89_context *ctx,
                                          wsounddna89_shot *s)
{
    const wsounddna89_profile *p;
    wsound89_i32 main_span;
    wsound89_i32 secondary_span;
    wsound89_i32 mode_gain;
    wsound89_i32 evar;
    wsound89_i32 pvar;
    wsound89_i32 mvar;
    wsound89_i32 wvar;
    wsound89_i32 energy;
    wsound89_i32 pressure;
    wsound89_i32 gas;
    wsound89_i32 receiver;
    wsound89_i32 thump;
    wsound89_i32 crack;
    wsound89_i32 brightness;
    wsound89_i32 tail;
    wsound89_i32 short_barrel;
    wsound89_i32 brake;
    wsound89_i32 suppress;
    wsound89_i32 action_mass;
    wsound89_i32 pitch_delta;
    wsound89_i32 cycle_delta;
    if (ctx == 0 || s == 0) return WSOUND89_EINVAL;
    p = &ctx->profile;
    if (!wsounddna89_validate_profile(p)) return WSOUND89_EINVAL;
    main_span = ctx->mode == WSOUNDDNA89_MODE_REALISTIC ? 1200 :
                (ctx->mode == WSOUNDDNA89_MODE_HYBRID ? 1900 : 2800);
    secondary_span = ctx->mode == WSOUNDDNA89_MODE_REALISTIC ? 420 :
                     (ctx->mode == WSOUNDDNA89_MODE_HYBRID ? 700 : 1050);
    mode_gain = ctx->mode == WSOUNDDNA89_MODE_REALISTIC ? 30000 :
                (ctx->mode == WSOUNDDNA89_MODE_HYBRID ? 32767 : 32767);
    evar = wdna_jitter(ctx, main_span);
    pvar = (evar * 3) / 4 + wdna_jitter(ctx, secondary_span);
    mvar = (evar * 2) / 5 + wdna_jitter(ctx, secondary_span);
    wvar = (evar * 1) / 3 + wdna_jitter(ctx, secondary_span);
    short_barrel = p->barrel_length_mm < 400U ?
                   (wsound89_i32)(400U - p->barrel_length_mm) * 12 : 0;
    if (short_barrel > 4200) short_barrel = 4200;
    brake = p->muzzle_brake_q15;
    suppress = p->suppressor_q15;
    action_mass = p->mechanical_mass_q15;
    energy = p->propellant_energy_q15 + evar;
    pressure = p->muzzle_pressure_q15 + pvar + short_barrel + brake / 4 -
               wdna_mul_q15(suppress, 19000);
    gas = (p->propellant_energy_q15 * 3) / 4 + pvar + short_barrel + brake / 3 -
          wdna_mul_q15(suppress, 11000);
    receiver = (action_mass * 3) / 4 + mvar;
    if (p->action == WSOUNDDNA89_ACTION_OPEN_BOLT) receiver += 2600;
    if (p->action == WSOUNDDNA89_ACTION_PUMP) receiver += 1800;
    if (p->action == WSOUNDDNA89_ACTION_BOLT) receiver += 1200;
    thump = (energy * 2) / 3 + action_mass / 5 + wvar;
    if (p->weapon_class == WSOUNDDNA89_HEAVY || p->weapon_class == WSOUNDDNA89_SHOTGUN)
        thump += 3500;
    crack = wdna_mul_q15(p->supersonic_q15, 30000) + evar / 3 -
            wdna_mul_q15(suppress, 5000);
    brightness = 15000 + short_barrel + brake / 4 +
                 (wsound89_i32)p->projectile_speed_mps * 8 + pvar -
                 wdna_mul_q15(suppress, 16000);
    tail = 17000 + wvar + energy / 5;
    if (ctx->mode == WSOUNDDNA89_MODE_CINEMATIC) {
        pressure += 2200; thump += 4200; tail += 3000; receiver += 900;
    } else if (ctx->mode == WSOUNDDNA89_MODE_REALISTIC) {
        thump = wdna_mul_q15(thump, 25000); tail = wdna_mul_q15(tail, 27000);
    }
    energy = wdna_mul_q15(energy, mode_gain);
    pressure = wdna_mul_q15(pressure, mode_gain);
    s->energy_q15 = wdna_clamp_u15(energy);
    s->pressure_q15 = wdna_clamp_u15(pressure);
    s->gas_q15 = wdna_clamp_u15(gas);
    s->receiver_q15 = wdna_clamp_u15(receiver);
    s->thump_q15 = wdna_clamp_u15(thump);
    s->crack_q15 = wdna_clamp_u15(crack);
    s->brightness_q15 = wdna_clamp_u15(brightness);
    s->tail_q15 = wdna_clamp_u15(tail);
    s->energy_variation_q15 = wdna_clamp_i16(evar);
    s->powder_variation_q15 = wdna_clamp_i16(pvar);
    s->mechanism_variation_q15 = wdna_clamp_i16(mvar);
    s->environment_variation_q15 = wdna_clamp_i16(wvar);
    pitch_delta = (pvar * 3) / 8 + (short_barrel / 4) - suppress / 16;
    cycle_delta = mvar / 2;
    if (p->cyclic_rate_rpm != 0U)
        cycle_delta += ((wsound89_i32)p->cyclic_rate_rpm - 600) * 3;
    if (pitch_delta < -6000) pitch_delta = -6000;
    if (pitch_delta > 6000) pitch_delta = 6000;
    if (cycle_delta < -6000) cycle_delta = -6000;
    if (cycle_delta > 8000) cycle_delta = 8000;
    s->pitch_q16 = (wsound89_u32)(65536 + pitch_delta);
    s->cycle_q16 = (wsound89_u32)(65536 + cycle_delta);
    s->shot_index = ctx->shot_index++;
    s->seed = wdna_rng(ctx);
    return WSOUND89_OK;
}

wsound89_result wsounddna89_next(wsounddna89_context *ctx,
                                 wsounddna89_class weapon_class,
                                 wsound89_u16 base_energy_q15,
                                 wsounddna89_shot *out_shot)
{
    wsounddna89_profile p;
    wsounddna89_profile_id id;
    if (ctx == 0 || out_shot == 0) return WSOUND89_EINVAL;
    if ((wsound89_u32)weapon_class > (wsound89_u32)WSOUNDDNA89_HEAVY)
        return WSOUND89_EINVAL;
    id = WSOUNDDNA89_PROFILE_SERVICE_PISTOL;
    if (weapon_class == WSOUNDDNA89_MACHINE) id = WSOUNDDNA89_PROFILE_SMG;
    else if (weapon_class == WSOUNDDNA89_RIFLE) id = WSOUNDDNA89_PROFILE_RIFLE;
    else if (weapon_class == WSOUNDDNA89_SHOTGUN) id = WSOUNDDNA89_PROFILE_SHOTGUN;
    else if (weapon_class == WSOUNDDNA89_HEAVY) id = WSOUNDDNA89_PROFILE_HEAVY;
    wsounddna89_profile_defaults(id, &p);
    p.propellant_energy_q15 = base_energy_q15;
    p.muzzle_pressure_q15 = wdna_clamp_u15((wsound89_i32)base_energy_q15 + 1000);
    ctx->profile = p;
    return wsounddna89_next_profiled(ctx, out_shot);
}

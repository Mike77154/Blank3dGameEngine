#include "gsynthsoundexpansion89.h"

static wsound89_i16 sx_sat(wsound89_i32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (wsound89_i16)value;
}

static wsound89_u16 sx_q15(wsound89_u16 value)
{
    return value > 32767U ? 32767U : value;
}

static wsound89_i16 sx_mul_q15(wsound89_i16 value, wsound89_u16 gain)
{
    return sx_sat(((wsound89_i32)value * (wsound89_i32)sx_q15(gain)) >> 15);
}

wsound89_result gssexp89_init(gssexp89_context *ctx,
                              wsound89_u32 rate,
                              wsound89_u32 seed,
                              const gssexp89_memory *memory)
{
    wsound89_result result;
    wsound89_u32 outdoor_need;
    wsound89_u32 portal_delay;
    if (ctx == 0 || memory == 0 || rate < 8000U || rate > 48000U)
        return WSOUND89_EINVAL;
    if (memory->outdoor == 0 || memory->portal == 0 ||
        memory->spatial_left == 0 || memory->spatial_right == 0 ||
        memory->spatial_frames == 0U)
        return WSOUND89_EINVAL;
    outdoor_need = wsoundoutdoor89_required_frames(rate,
                                                    WSOUNDOUTDOOR89_URBAN_CANYON);
    portal_delay = (rate * 24U) / 1000U;
    if (memory->outdoor_frames < outdoor_need ||
        memory->portal_frames <= portal_delay)
        return WSOUND89_ECAPACITY;

    ctx->sample_rate = rate;
    ctx->enabled_mask = GSSEXP89_SAFE_DEFAULT;
    ctx->outdoor_send_q15 = 18000U;
    ctx->outdoor_wet_q15 = 12000U;
    ctx->outdoor_send_remaining = 0U;
    ctx->outdoor_send_total = (rate * 55U) / 1000U;
    if (ctx->outdoor_send_total == 0U) ctx->outdoor_send_total = 1U;

    result = wsoundmuzzledevice89_init(&ctx->muzzle, seed + 1U);
    if (result != WSOUND89_OK) return result;
    result = wsoundaero89_init(&ctx->aero, rate, seed + 2U);
    if (result != WSOUND89_OK) return result;
    result = wsoundfriction89_init(&ctx->friction, seed + 3U);
    if (result != WSOUND89_OK) return result;
    result = wsoundparticles89_init(&ctx->particles, seed + 4U);
    if (result != WSOUND89_OK) return result;
    result = wsoundammo89_init(&ctx->ammo, seed + 5U);
    if (result != WSOUND89_OK) return result;
    result = wsoundbeltfeed89_init(&ctx->belt, rate, seed + 6U);
    if (result != WSOUND89_OK) return result;
    result = wsoundoutdoor89_init(&ctx->outdoor, memory->outdoor,
                                   memory->outdoor_frames, rate,
                                   WSOUNDOUTDOOR89_URBAN_CANYON);
    if (result != WSOUND89_OK) return result;
    result = wsoundportal89_init(&ctx->portal, memory->portal,
                                  memory->portal_frames, portal_delay);
    if (result != WSOUND89_OK) return result;
    result = wsounddoppler89_init(&ctx->doppler);
    if (result != WSOUND89_OK) return result;
    result = wsoundspatial89_init(&ctx->spatial, memory->spatial_left,
                                   memory->spatial_right,
                                   memory->spatial_frames);
    if (result != WSOUND89_OK) return result;
    result = wsoundthermal89_init(&ctx->thermal, seed + 7U);
    if (result != WSOUND89_OK) return result;
    result = wsoundlistener89_init(&ctx->listener, rate);
    if (result != WSOUND89_OK) return result;
    result = wsoundmask89_init(&ctx->mask);
    return result;
}

void gssexp89_enable(gssexp89_context *ctx, wsound89_u16 mask)
{
    if (ctx != 0) ctx->enabled_mask = (wsound89_u16)(mask & GSSEXP89_ALL);
}

void gssexp89_set_outdoor_mix(gssexp89_context *ctx,
                              wsound89_u16 send_q15,
                              wsound89_u16 wet_q15)
{
    if (ctx == 0) return;
    ctx->outdoor_send_q15 = sx_q15(send_q15);
    ctx->outdoor_wet_q15 = sx_q15(wet_q15);
}

void gssexp89_trigger_shot(gssexp89_context *ctx,
                           wsoundmuzzledevice89_type device,
                           wsound89_u16 energy,
                           wsound89_u16 distance,
                           wsound89_u32 seed)
{
    wsound89_u16 qenergy;
    wsound89_u16 qdistance;
    if (ctx == 0) return;
    qenergy = sx_q15(energy);
    qdistance = sx_q15(distance);
    if (ctx->enabled_mask & GSSEXP89_MUZZLE)
        (void)wsoundmuzzledevice89_trigger(&ctx->muzzle, device, qenergy);
    if (ctx->enabled_mask & GSSEXP89_PARTICLES)
        (void)wsoundparticles89_trigger(&ctx->particles,
                                        WSOUNDPARTICLES89_METAL,
                                        (wsound89_u16)(qenergy >> 3),
                                        (wsound89_u16)(qenergy >> 2));
    if (ctx->enabled_mask & GSSEXP89_AMMO)
        (void)wsoundammo89_trigger(&ctx->ammo, WSOUNDAMMO89_BOX_MAG,
                                   21000U,
                                   (wsound89_u16)(qenergy >> 3));
    if (ctx->enabled_mask & GSSEXP89_THERMAL)
        wsoundthermal89_add_heat(&ctx->thermal,
            device == WSOUNDMUZZLEDEVICE89_SUPPRESSOR ?
            WSOUNDTHERMAL89_SUPPRESSOR : WSOUNDTHERMAL89_BARREL,
            (wsound89_u16)(qenergy >> 3));
    if (ctx->enabled_mask & GSSEXP89_LISTENER)
        wsoundlistener89_expose(&ctx->listener, qenergy, qdistance,
                                WSOUNDLISTENER89_ELECTRONIC);
    if (ctx->enabled_mask & GSSEXP89_MASK)
        wsoundmask89_trigger(&ctx->mask,
                             (wsound89_u16)(((wsound89_u32)qenergy * 3U) / 4U));
    if (ctx->enabled_mask & GSSEXP89_OUTDOOR)
        ctx->outdoor_send_remaining = ctx->outdoor_send_total;
    if (ctx->enabled_mask & GSSEXP89_SPATIAL) {
        if (seed & 1U)
            wsoundspatial89_set_azimuth(&ctx->spatial, 16000, 18U, 19000U);
        else
            wsoundspatial89_set_azimuth(&ctx->spatial, -16000, 18U, 19000U);
    }
}

void gssexp89_start_belt(gssexp89_context *ctx,
                         wsound89_u16 rpm,
                         wsound89_u16 tension)
{
    if (ctx != 0 && (ctx->enabled_mask & GSSEXP89_BELT))
        (void)wsoundbeltfeed89_start(&ctx->belt, rpm, sx_q15(tension), 27000U);
}

void gssexp89_stop_belt(gssexp89_context *ctx)
{
    if (ctx != 0) wsoundbeltfeed89_set_gate(&ctx->belt, 0);
}

void gssexp89_start_friction(gssexp89_context *ctx,
                             wsoundfriction89_material material,
                             wsound89_u16 speed,
                             wsound89_u16 pressure,
                             wsound89_u16 roughness)
{
    if (ctx != 0 && (ctx->enabled_mask & GSSEXP89_FRICTION))
        (void)wsoundfriction89_start(&ctx->friction, material,
                                      sx_q15(speed), sx_q15(pressure),
                                      sx_q15(roughness));
}

void gssexp89_stop_friction(gssexp89_context *ctx)
{
    if (ctx != 0) wsoundfriction89_set_gate(&ctx->friction, 0);
}

void gssexp89_start_aero(gssexp89_context *ctx,
                         wsoundaero89_mode mode,
                         wsound89_u16 speed,
                         wsound89_u16 size,
                         wsound89_u32 duration)
{
    if (ctx != 0 && (ctx->enabled_mask & GSSEXP89_AERO))
        (void)wsoundaero89_trigger(&ctx->aero, mode, sx_q15(speed),
                                    sx_q15(size), duration);
}

void gssexp89_process_mono(gssexp89_context *ctx,
                           wsound89_i16 input,
                           wsound89_i16 *out_left,
                           wsound89_i16 *out_right)
{
    wsound89_i32 extra;
    wsound89_i32 world;
    wsound89_i16 left;
    wsound89_i16 right;
    if (ctx == 0 || out_left == 0 || out_right == 0) return;
    extra = 0;
    if (ctx->enabled_mask & GSSEXP89_MUZZLE)
        extra += wsoundmuzzledevice89_process_sample(&ctx->muzzle) >> 2;
    if (ctx->enabled_mask & GSSEXP89_AERO)
        extra += wsoundaero89_process_sample(&ctx->aero) >> 3;
    if (ctx->enabled_mask & GSSEXP89_FRICTION)
        extra += wsoundfriction89_process_sample(&ctx->friction) >> 2;
    if (ctx->enabled_mask & GSSEXP89_PARTICLES)
        extra += wsoundparticles89_process_sample(&ctx->particles) >> 2;
    if (ctx->enabled_mask & GSSEXP89_AMMO)
        extra += wsoundammo89_process_sample(&ctx->ammo) >> 2;
    if (ctx->enabled_mask & GSSEXP89_BELT)
        extra += wsoundbeltfeed89_process_sample(&ctx->belt) >> 2;
    if (ctx->enabled_mask & GSSEXP89_THERMAL)
        extra += wsoundthermal89_process_sample(&ctx->thermal) >> 1;

    world = input;
    if (ctx->enabled_mask & GSSEXP89_PORTAL) {
        world = wsoundportal89_process_sample(&ctx->portal, sx_sat(world));
    } else if (ctx->enabled_mask & GSSEXP89_OUTDOOR) {
        wsound89_i16 send;
        wsound89_i16 wet;
        wsound89_u16 gain;
        send = 0;
        gain = 0U;
        if (ctx->outdoor_send_remaining != 0U) {
            wsound89_u32 elapsed;
            wsound89_u32 knee;
            wsound89_u32 span;
            elapsed = ctx->outdoor_send_total - ctx->outdoor_send_remaining;
            knee = (ctx->sample_rate * 12U) / 1000U;
            span = ctx->outdoor_send_total > knee ?
                   ctx->outdoor_send_total - knee : 0U;
            if (elapsed < knee) {
                gain = ctx->outdoor_send_q15;
            } else if (span != 0U) {
                wsound89_u32 tail;
                wsound89_u32 floor_gain;
                tail = elapsed - knee;
                floor_gain = (wsound89_u32)ctx->outdoor_send_q15 / 3U;
                if (tail < span) {
                    gain = (wsound89_u16)((wsound89_u32)ctx->outdoor_send_q15 -
                        (((wsound89_u32)ctx->outdoor_send_q15 - floor_gain) * tail) /
                        span);
                } else {
                    gain = (wsound89_u16)floor_gain;
                }
            }
            send = sx_mul_q15(input, gain);
            --ctx->outdoor_send_remaining;
        }
        wet = wsoundoutdoor89_process_wet_sample(&ctx->outdoor, send);
        world += sx_mul_q15(wet, ctx->outdoor_wet_q15);
    }

    if (ctx->enabled_mask & GSSEXP89_SPATIAL)
        wsoundspatial89_process_sample(&ctx->spatial, sx_sat(extra),
                                       &left, &right);
    else {
        left = sx_sat(extra);
        right = sx_sat(extra);
    }
    left = sx_sat(world + left);
    right = sx_sat(world + right);
    if (ctx->enabled_mask & GSSEXP89_MASK) {
        left = wsoundmask89_mix_sample(&ctx->mask, left, sx_sat(extra >> 2));
        right = wsoundmask89_mix_sample(&ctx->mask, right, sx_sat(extra >> 2));
    }
    if (ctx->enabled_mask & GSSEXP89_LISTENER)
        wsoundlistener89_process_stereo(&ctx->listener, left, right,
                                        out_left, out_right);
    else {
        *out_left = left;
        *out_right = right;
    }
}

wsound89_u32 gssexp89_context_bytes(void)
{
    return (wsound89_u32)sizeof(gssexp89_context);
}

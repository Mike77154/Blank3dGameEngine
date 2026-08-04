#include <stdio.h>
#include <string.h>
#include "gweaponvoice89.h"
#include "gsynthsoundengine89.h"
#include "gsynthordnance89.h"

#define OR_RATE 44100U
#define OR_LOGICAL 256U
#define OR_PHYSICAL 64U
#define OR_MAX_FRAMES (OR_RATE * 22U)
#define OR_BULLET_POOL 32U
#define OR_GRENADE_POOL 4U
#define OR_ROCKET_POOL 2U
#define OR_FIRE_POOL 4U

typedef signed short or_s16;
typedef signed int or_s32;
typedef unsigned int or_u32;
typedef unsigned short or_u16;

static gv89_voice or_voice_storage[OR_LOGICAL];
static gsse89_fire_voice or_fire_storage[OR_FIRE_POOL];
static gsso89_bullet_voice or_bullet_storage[OR_BULLET_POOL];
static gsso89_grenade_voice or_grenade_storage[OR_GRENADE_POOL];
static gsso89_rocket_voice or_rocket_storage[OR_ROCKET_POOL];
static or_s16 or_pcm[OR_MAX_FRAMES * 2U];
static or_s16 or_base[OR_MAX_FRAMES * 2U];

static or_s16 or_sat(or_s32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (or_s16)v;
}

static or_u32 or_ms(or_u32 value)
{
    return (value * OR_RATE) / 1000U;
}

static int or_write_u16(FILE *f, or_u16 value)
{
    if (fputc((int)(value & 255U), f) == EOF) return 0;
    if (fputc((int)((value >> 8) & 255U), f) == EOF) return 0;
    return 1;
}

static int or_write_u32(FILE *f, or_u32 value)
{
    if (fputc((int)(value & 255U), f) == EOF) return 0;
    if (fputc((int)((value >> 8) & 255U), f) == EOF) return 0;
    if (fputc((int)((value >> 16) & 255U), f) == EOF) return 0;
    if (fputc((int)((value >> 24) & 255U), f) == EOF) return 0;
    return 1;
}

static void or_normalize_quiet(or_s16 *pcm, or_u32 frames, or_s16 target)
{
    or_u32 i;
    or_s32 peak;
    or_s32 value;
    or_s32 scale_q15;
    peak = 1;
    for (i = 0U; i < frames * 2U; ++i) {
        value = pcm[i];
        if (value < 0) value = -value;
        if (value > peak) peak = value;
    }
    if (peak >= target) return;
    scale_q15 = ((or_s32)target * 32767) / peak;
    if (scale_q15 > 98301) scale_q15 = 98301;
    for (i = 0U; i < frames * 2U; ++i)
        pcm[i] = or_sat(((or_s32)pcm[i] * scale_q15) >> 15);
}

static int or_write_wav(const char *path, const or_s16 *pcm, or_u32 frames)
{
    FILE *f;
    or_u32 i;
    or_u32 bytes;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    bytes = frames * 4U;
    if (fwrite("RIFF", 1U, 4U, f) != 4U ||
        !or_write_u32(f, 36U + bytes) ||
        fwrite("WAVEfmt ", 1U, 8U, f) != 8U ||
        !or_write_u32(f, 16U) || !or_write_u16(f, 1U) ||
        !or_write_u16(f, 2U) || !or_write_u32(f, OR_RATE) ||
        !or_write_u32(f, OR_RATE * 4U) || !or_write_u16(f, 4U) ||
        !or_write_u16(f, 16U) || fwrite("data", 1U, 4U, f) != 4U ||
        !or_write_u32(f, bytes)) {
        fclose(f);
        return 0;
    }
    for (i = 0U; i < frames * 2U; ++i)
        if (!or_write_u16(f, (or_u16)pcm[i])) {
            fclose(f);
            return 0;
        }
    return fclose(f) == 0;
}

static int or_read_wav(const char *path, or_s16 *pcm, or_u32 cap,
                       or_u32 *out_frames)
{
    FILE *f;
    unsigned char h[44];
    or_u32 bytes;
    or_u32 frames;
    or_u32 i;
    int a;
    int b;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    if (fread(h, 1U, 44U, f) != 44U) {
        fclose(f);
        return 0;
    }
    if (memcmp(h, "RIFF", 4U) != 0 || memcmp(h + 8, "WAVE", 4U) != 0) {
        fclose(f);
        return 0;
    }
    bytes = (or_u32)h[40] | ((or_u32)h[41] << 8) |
            ((or_u32)h[42] << 16) | ((or_u32)h[43] << 24);
    frames = bytes / 4U;
    if (frames > cap) frames = cap;
    for (i = 0U; i < frames * 2U; ++i) {
        a = fgetc(f);
        b = fgetc(f);
        if (a == EOF || b == EOF) {
            frames = i / 2U;
            break;
        }
        pcm[i] = (or_s16)((or_u16)a | ((or_u16)b << 8));
    }
    fclose(f);
    *out_frames = frames;
    return 1;
}

static void or_fire_event(gsse89_context *fire, gwv89_context *handler,
                          or_s16 pan, or_u16 distance, or_s16 gain,
                          or_u32 duration, or_u32 seed, or_u32 key)
{
    gsse89_fire_params p;
    gsse89_fire_defaults(&p, GSSE89_FIRE_ROLE_FLAMETHROWER);
    p.pan_q15 = pan;
    p.distance_q15 = distance;
    p.gain_q15 = gain;
    p.duration_ms = duration;
    p.instance_key = key;
    p.instance_limit = 2U;
    (void)gsse89_play_fire(fire, handler, &p, seed, 0);
}

static void or_bullet_event(gsso89_context *ord, gwv89_context *handler,
                            int preset, or_s16 pan, or_u16 distance,
                            or_s16 gain, or_u32 seed, or_u32 key)
{
    gsso89_bullet_params p;
    gsso89_bullet_defaults(&p);
    p.preset_id = preset;
    p.common.pan_q15 = pan;
    p.common.distance_q15 = distance;
    p.common.gain_q15 = gain;
    p.common.instance_key = key;
    (void)gsso89_play_bullet(ord, handler, &p, seed, 0);
}

static void or_grenade_event(gsso89_context *ord, gwv89_context *handler,
                             int preset, or_s16 pan, or_u16 distance,
                             or_s16 gain, or_u16 intensity,
                             or_u32 seed, or_u32 key)
{
    gsso89_grenade_params p;
    gsso89_grenade_defaults(&p);
    p.preset_id = preset;
    p.intensity_q15 = (gv89_s16)intensity;
    p.common.pan_q15 = pan;
    p.common.distance_q15 = distance;
    p.common.gain_q15 = gain;
    p.common.instance_key = key;
    (void)gsso89_play_grenade(ord, handler, &p, seed, 0);
}

static void or_rocket_event(gsso89_context *ord, gwv89_context *handler,
                            or_u16 preset, or_s16 pan, or_u16 distance,
                            or_s16 gain, or_u16 velocity,
                            or_u32 seed, or_u32 key)
{
    gsso89_rocket_params p;
    gsso89_rocket_defaults(&p);
    p.preset_id = preset;
    p.velocity_q15 = velocity;
    p.common.pan_q15 = pan;
    p.common.distance_q15 = distance;
    p.common.gain_q15 = gain;
    p.common.instance_key = key;
    (void)gsso89_play_rocket(ord, handler, &p, seed, 0);
}

static int or_render(const char *path, int mode, or_u32 frames,
                     const or_s16 *base, or_u32 base_frames,
                     gv89_stats *out_stats)
{
    gwv89_context handler;
    gsse89_context fire;
    gsso89_context ord;
    or_u32 i;
    or_s16 l;
    or_s16 r;
    or_s32 ml;
    or_s32 mr;
    if (frames > OR_MAX_FRAMES) return 0;
    if (gwv89_init_ex(&handler, or_voice_storage, OR_LOGICAL,
                      OR_PHYSICAL, OR_RATE) != GV89_OK) return 0;
    if (!gsse89_init(&fire, 0, 0U, or_fire_storage, OR_FIRE_POOL,
                     OR_RATE)) return 0;
    if (!gsso89_init(&ord, or_bullet_storage, OR_BULLET_POOL,
                     or_grenade_storage, OR_GRENADE_POOL,
                     or_rocket_storage, OR_ROCKET_POOL, OR_RATE)) return 0;
    gv89_set_master(&handler.voices, 28500, 30000, 5U);
    gv89_set_virtualization(&handler.voices, 1400U, 2100U, 96U);
    for (i = 0U; i < frames; ++i) {
        if (mode == 0) {
            if (i == or_ms(250U)) or_fire_event(&fire, &handler, -10000, 2800U,
                                                 24000, 1800U, 1001U, 5001U);
            if (i == or_ms(2050U)) or_fire_event(&fire, &handler, 11000, 5200U,
                                                  19500, 1250U, 2089U, 5001U);
        } else if (mode == 1) {
            if ((i % or_ms(720U)) == or_ms(120U) && i < or_ms(6100U)) {
                or_u32 n;
                n = i / or_ms(720U);
                or_bullet_event(&ord, &handler, (int)(n % GBA89_PRESET_COUNT),
                                (or_s16)(-26000 + (or_s16)n * 7000),
                                (or_u16)(1200U + n * 1100U), 25000,
                                3000U + n, 5100U + n);
            }
        } else if (mode == 2) {
            if (i == or_ms(300U)) or_grenade_event(&ord, &handler,
                WS_GGB89_M67_OPEN, -16000, 1800U, 27500, 30000U, 4001U, 5201U);
            if (i == or_ms(2800U)) or_grenade_event(&ord, &handler,
                WS_GGB89_40MM_HE_OPEN, 13000, 4200U, 26000, 29200U, 4002U, 5202U);
            if (i == or_ms(5400U)) or_grenade_event(&ord, &handler,
                WS_GGB89_INDOOR_CONFINED, -3000, 3000U, 25200, 28600U, 4003U, 5203U);
            if (i == or_ms(8200U)) or_grenade_event(&ord, &handler,
                WS_GGB89_DISTANT, 19000, 17000U, 24500, 28000U, 4004U, 5204U);
        } else if (mode == 3) {
            if (i == or_ms(300U)) or_rocket_event(&ord, &handler,
                WSRB89_PRESET_HEAVY_IMPACT, -17000, 2200U, 27800, 31000U, 6001U, 5301U);
            if (i == or_ms(3400U)) or_rocket_event(&ord, &handler,
                WSRB89_PRESET_CONCRETE_PAAS, 15000, 4800U, 26500, 30000U, 6002U, 5302U);
            if (i == or_ms(6900U)) or_rocket_event(&ord, &handler,
                WSRB89_PRESET_AIRBURST, -6000, 9000U, 25200, 29200U, 6003U, 5303U);
            if (i == or_ms(10400U)) or_rocket_event(&ord, &handler,
                WSRB89_PRESET_DISTANT_PAAS, 19000, 19000U, 24000, 28500U, 6004U, 5304U);
        } else {
            if (i == or_ms(550U)) or_bullet_event(&ord, &handler,
                GBA89_PRESET_CLOSE_RIFLE_PASS, -23000, 1700U, 24500, 7001U, 5401U);
            if (i == or_ms(1350U)) or_bullet_event(&ord, &handler,
                GBA89_PRESET_SUPERSONIC_SNAP, 22000, 900U, 25500, 7002U, 5402U);
            if (i == or_ms(2450U)) or_grenade_event(&ord, &handler,
                WS_GGB89_M67_OPEN, -14000, 2800U, 25000, 29200U, 7003U, 5403U);
            if (i == or_ms(4450U)) or_bullet_event(&ord, &handler,
                GBA89_PRESET_HEAVY_ROUND_PASS, 8000, 1800U, 24800, 7004U, 5404U);
            if (i == or_ms(5800U)) or_fire_event(&fire, &handler, -11000, 4200U,
                                                 22000, 1850U, 7005U, 5405U);
            if (i == or_ms(7500U)) or_fire_event(&fire, &handler, 12000, 6500U,
                                                 18200, 1200U, 7999U, 5405U);
            if (i == or_ms(8900U)) or_grenade_event(&ord, &handler,
                WS_GGB89_40MM_HEDP_HARD, 15000, 6200U, 24800, 28500U, 7006U, 5406U);
            if (i == or_ms(11200U)) or_rocket_event(&ord, &handler,
                WSRB89_PRESET_COMPACT_RPG, -17000, 4300U, 25800, 30000U, 7007U, 5407U);
            if (i == or_ms(13300U)) or_bullet_event(&ord, &handler,
                GBA89_PRESET_DISTANT_CRACK, 20000, 16000U, 23500, 7008U, 5408U);
        }
        l = 0;
        r = 0;
        gwv89_process_stereo_sample(&handler, &l, &r);
        if (base != 0 && i < base_frames) {
            ml = ((or_s32)base[i * 2U] * 28500) >> 15;
            mr = ((or_s32)base[i * 2U + 1U] * 28500) >> 15;
            ml += ((or_s32)l * 24500) >> 15;
            mr += ((or_s32)r * 24500) >> 15;
        } else {
            ml = ((or_s32)l * 30000) >> 15;
            mr = ((or_s32)r * 30000) >> 15;
        }
        or_pcm[i * 2U] = or_sat(ml);
        or_pcm[i * 2U + 1U] = or_sat(mr);
    }
    if (base == 0) {
        if (mode == 0) or_normalize_quiet(or_pcm, frames, 19000);
        else if (mode == 1) or_normalize_quiet(or_pcm, frames, 21000);
        else if (mode == 2) or_normalize_quiet(or_pcm, frames, 23500);
        else if (mode == 3) or_normalize_quiet(or_pcm, frames, 25000);
        else or_normalize_quiet(or_pcm, frames, 23000);
    }
    if (out_stats != 0) gwv89_get_stats(&handler, out_stats);
    return or_write_wav(path, or_pcm, frames);
}

static void or_print_stats(const char *name, const gv89_stats *stats)
{
    printf("%s starts=%lu steals=%lu rejects=%lu peak_logical=%lu peak_physical=%lu limiter=%lu\n",
           name, (unsigned long)stats->starts, (unsigned long)stats->steals,
           (unsigned long)stats->rejects, (unsigned long)stats->peak_logical,
           (unsigned long)stats->peak_physical,
           (unsigned long)stats->limiter_hits);
}

int main(void)
{
    gv89_stats stats;
    or_u32 base_frames;
    if (!or_render("audio/06_flamethrower_corrected_dual.wav", 0,
                   or_ms(4300U), 0, 0U, &stats)) return 1;
    or_print_stats("flamethrower", &stats);
    if (!or_render("audio/07_gbulletair89_isolated_catalog.wav", 1,
                   or_ms(6500U), 0, 0U, &stats)) return 2;
    or_print_stats("bulletair", &stats);
    if (!or_render("audio/08_grenadeblast89_isolated_catalog.wav", 2,
                   or_ms(11000U), 0, 0U, &stats)) return 3;
    or_print_stats("grenade", &stats);
    if (!or_render("audio/09_rocketblast89_isolated_catalog.wav", 3,
                   or_ms(15000U), 0, 0U, &stats)) return 4;
    or_print_stats("rocket", &stats);
    if (!or_render("audio/10_ordnance_integrated_scene.wav", 4,
                   or_ms(16000U), 0, 0U, &stats)) return 5;
    or_print_stats("ordnance", &stats);
    base_frames = 0U;
    if (!or_read_wav("audio/04_full_firefight_plus_gtinkle_gfire.wav",
                     or_base, OR_MAX_FRAMES, &base_frames)) return 6;
    if (!or_render("audio/11_full_weapon_engine_plus_ordnance.wav", 4,
                   base_frames, or_base, base_frames, &stats)) return 7;
    or_print_stats("full-engine", &stats);
    printf("provider bytes: bullet=%lu grenade=%lu rocket=%lu context=%lu\n",
           (unsigned long)gsso89_bullet_voice_bytes(),
           (unsigned long)gsso89_grenade_voice_bytes(),
           (unsigned long)gsso89_rocket_voice_bytes(),
           (unsigned long)gsso89_context_bytes());
    return 0;
}

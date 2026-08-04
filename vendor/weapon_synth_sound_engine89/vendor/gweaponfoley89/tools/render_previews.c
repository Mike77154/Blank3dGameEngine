#include <stdio.h>
#include "gweaponfoley89.h"

#define RENDER_SAMPLES 22050UL
#define GAP_SAMPLES     5292UL

static gwf89_s16 render_buffer[RENDER_SAMPLES];
static gwf89_s16 dry_buffer[RENDER_SAMPLES];
static gwf89_s16 room_buffer[RENDER_SAMPLES];
static gwf89_s16 zero_buffer[GAP_SAMPLES];

static void put_u16_le(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
}

static void put_u32_le(FILE *f, unsigned long v)
{
    fputc((int)(v & 255UL), f);
    fputc((int)((v >> 8) & 255UL), f);
    fputc((int)((v >> 16) & 255UL), f);
    fputc((int)((v >> 24) & 255UL), f);
}

static int write_wav_header(FILE *f, unsigned long samples)
{
    unsigned long data_bytes;
    data_bytes = samples * 2UL;
    if (fwrite("RIFF", 1U, 4U, f) != 4U) return 0;
    put_u32_le(f, 36UL + data_bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, f) != 8U) return 0;
    put_u32_le(f, 16UL);
    put_u16_le(f, 1UL);
    put_u16_le(f, 1UL);
    put_u32_le(f, GWF89_SAMPLE_RATE);
    put_u32_le(f, (unsigned long)GWF89_SAMPLE_RATE * 2UL);
    put_u16_le(f, 2UL);
    put_u16_le(f, 16UL);
    if (fwrite("data", 1U, 4U, f) != 4U) return 0;
    put_u32_le(f, data_bytes);
    return 1;
}

static int write_pcm(FILE *f, const gwf89_s16 *samples,
                     unsigned long count)
{
    unsigned long i;
    unsigned short u;
    for (i = 0UL; i < count; ++i) {
        u = (unsigned short)samples[i];
        fputc((int)(u & 255U), f);
        fputc((int)((u >> 8) & 255U), f);
        if (ferror(f)) return 0;
    }
    return 1;
}

static int write_wav(const char *path, const gwf89_s16 *samples,
                     unsigned long count)
{
    FILE *f;
    f = fopen(path, "wb");
    if (!f) return 0;
    if (!write_wav_header(f, count) || !write_pcm(f, samples, count)) {
        fclose(f);
        return 0;
    }
    if (fclose(f) != 0) return 0;
    return 1;
}

static void render_to_buffers(gwf89_preset_id id, gwf89_u32 seed,
                              unsigned char variant, gwf89_speed speed)
{
    gwf89_context ctx;
    unsigned long i;

    gwf89_init(&ctx, seed);
    gwf89_trigger_ex(&ctx, id, seed, variant, speed);
    for (i = 0UL; i < RENDER_SAMPLES; ++i) {
        render_buffer[i] = gwf89_process_sample_stems(
            &ctx, &dry_buffer[i], &room_buffer[i]);
    }
}

static int render_one(const char *path, gwf89_preset_id id,
                      gwf89_u32 seed, unsigned char variant,
                      gwf89_speed speed)
{
    render_to_buffers(id, seed, variant, speed);
    return write_wav(path, render_buffer, RENDER_SAMPLES);
}

static int render_stems(const char *mix_path, const char *dry_path,
                        const char *room_path, gwf89_preset_id id,
                        gwf89_u32 seed, unsigned char variant,
                        gwf89_speed speed)
{
    render_to_buffers(id, seed, variant, speed);
    if (!write_wav(mix_path, render_buffer, RENDER_SAMPLES)) return 0;
    if (!write_wav(dry_path, dry_buffer, RENDER_SAMPLES)) return 0;
    if (!write_wav(room_path, room_buffer, RENDER_SAMPLES)) return 0;
    return 1;
}

static int render_catalog(const char *path)
{
    gwf89_context ctx;
    FILE *f;
    unsigned long i;
    unsigned long total;
    int id;
    gwf89_u32 seed;

    total = (RENDER_SAMPLES + GAP_SAMPLES) *
            (unsigned long)GWF89_PRESET_COUNT;
    f = fopen(path, "wb");
    if (!f) return 0;
    if (!write_wav_header(f, total)) {
        fclose(f);
        return 0;
    }

    for (id = 0; id < (int)GWF89_PRESET_COUNT; ++id) {
        seed = 0x61A7C001UL + (gwf89_u32)id * 0x10203UL;
        gwf89_init(&ctx, seed);
        gwf89_trigger_ex(&ctx, (gwf89_preset_id)id, seed, 1U,
                         GWF89_SPEED_NORMAL);
        for (i = 0UL; i < RENDER_SAMPLES; ++i)
            render_buffer[i] = gwf89_process_sample(&ctx);
        if (!write_pcm(f, render_buffer, RENDER_SAMPLES) ||
            !write_pcm(f, zero_buffer, GAP_SAMPLES)) {
            fclose(f);
            return 0;
        }
    }

    if (fclose(f) != 0) return 0;
    return 1;
}

int main(void)
{
    static const char *paths[GWF89_PRESET_COUNT] = {
        "previews_real_reference/01_pistol_empty_real_reference.wav",
        "previews_real_reference/02_pistol_handling_real_reference.wav",
        "previews_real_reference/03_magnum_empty_real_reference.wav",
        "previews_real_reference/04_magnum_latch_real_reference.wav",
        "previews_real_reference/05_sniper_empty_real_reference.wav",
        "previews_real_reference/06_sniper_bolt_dry_real_reference.wav",
        "previews_real_reference/07_smg_empty_real_reference.wav",
        "previews_real_reference/08_smg_selector_real_reference.wav",
        "previews_real_reference/09_launcher_empty_real_reference.wav",
        "previews_real_reference/10_launcher_latch_real_reference.wav",
        "previews_real_reference/11_shotgun_empty_real_reference.wav",
        "previews_real_reference/12_shotgun_safety_real_reference.wav"
    };
    static const char *variant_paths[3] = {
        "variants_real_reference/06_sniper_bolt_variant_0_tight.wav",
        "variants_real_reference/06_sniper_bolt_variant_1_neutral.wav",
        "variants_real_reference/06_sniper_bolt_variant_2_heavy.wav"
    };
    int id;
    int variant;
    gwf89_u32 seed;

    for (id = 0; id < (int)GWF89_PRESET_COUNT; ++id) {
        seed = 0x61A7C001UL + (gwf89_u32)id * 0x10203UL;
        if (!render_one(paths[id], (gwf89_preset_id)id, seed, 1U,
                        GWF89_SPEED_NORMAL)) {
            fprintf(stderr, "failed: %s\n", paths[id]);
            return 1;
        }
    }

    if (!render_catalog(
        "previews_real_reference/00_catalog_all_presets_real_reference.wav")) {
        fprintf(stderr, "failed: catalog\n");
        return 1;
    }

    seed = 0x61A7C001UL +
           (gwf89_u32)GWF89_SNIPER_BOLT_DRY * 0x10203UL;
    for (variant = 0; variant < 3; ++variant) {
        if (!render_one(variant_paths[variant], GWF89_SNIPER_BOLT_DRY,
                        seed, (unsigned char)variant,
                        GWF89_SPEED_NORMAL)) {
            fprintf(stderr, "failed: variant\n");
            return 1;
        }
    }

    if (!render_one("speeds_real_reference/06_sniper_bolt_slow.wav",
                    GWF89_SNIPER_BOLT_DRY, seed, 1U,
                    GWF89_SPEED_SLOW) ||
        !render_one("speeds_real_reference/06_sniper_bolt_normal.wav",
                    GWF89_SNIPER_BOLT_DRY, seed, 1U,
                    GWF89_SPEED_NORMAL) ||
        !render_one("speeds_real_reference/06_sniper_bolt_fast.wav",
                    GWF89_SNIPER_BOLT_DRY, seed, 1U,
                    GWF89_SPEED_FAST)) {
        fprintf(stderr, "failed: speed set\n");
        return 1;
    }

    seed = 0x61A7C001UL +
           (gwf89_u32)GWF89_LAUNCHER_LATCH * 0x10203UL;
    if (!render_one("speeds_real_reference/10_launcher_latch_slow.wav",
                    GWF89_LAUNCHER_LATCH, seed, 1U,
                    GWF89_SPEED_SLOW) ||
        !render_one("speeds_real_reference/10_launcher_latch_fast.wav",
                    GWF89_LAUNCHER_LATCH, seed, 1U,
                    GWF89_SPEED_FAST)) {
        fprintf(stderr, "failed: launcher speed set\n");
        return 1;
    }

    seed = 0x61A7C001UL +
           (gwf89_u32)GWF89_SNIPER_BOLT_DRY * 0x10203UL;
    if (!render_stems(
        "stems_real_reference/06_sniper_bolt_mix.wav",
        "stems_real_reference/06_sniper_bolt_dry.wav",
        "stems_real_reference/06_sniper_bolt_room.wav",
        GWF89_SNIPER_BOLT_DRY, seed, 1U, GWF89_SPEED_NORMAL)) {
        fprintf(stderr, "failed: stems\n");
        return 1;
    }

    return 0;
}

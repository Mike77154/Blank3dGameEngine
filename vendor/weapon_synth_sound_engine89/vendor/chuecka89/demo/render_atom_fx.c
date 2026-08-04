#include <stdio.h>
#include "chuecka89.h"
#include "chuecka89_wav.h"

#define FX_RATE 44100U
#define FX_ATOM_MAX_FRAMES (FX_RATE * 1U)
#define FX_MONTAGE_MAX_FRAMES (FX_RATE * 8U)
#define FX_SEED 0x12EC7A89U
#define FX_GAP_FRAMES (FX_RATE / 4U)

static ch89_i16 g_atom[FX_ATOM_MAX_FRAMES];
static ch89_i16 g_montage[FX_MONTAGE_MAX_FRAMES];


static ch89_i32 fx_abs32(ch89_i32 value)
{
    return (value < 0) ? -value : value;
}

static ch89_i16 fx_clip16(ch89_i32 value)
{
    if (value > 32767) return (ch89_i16)32767;
    if (value < -32768) return (ch89_i16)-32768;
    return (ch89_i16)value;
}

static void fx_zero(ch89_i16 *samples, ch89_u32 count)
{
    ch89_u32 i;
    for (i = 0U; i < count; ++i) samples[i] = 0;
}

static int fx_render(
    ch89_context *ctx,
    const ch89_gesture *gesture,
    const char *path,
    ch89_u32 *out_frames
)
{
    ch89_u32 frames;
    ch89_result r;

    frames = ch89_required_frames(gesture, FX_RATE);
    if (frames > FX_ATOM_MAX_FRAMES) return 0;
    ch89_reset(ctx, FX_SEED);
    r = ch89_render(ctx, gesture, g_atom, FX_ATOM_MAX_FRAMES, &frames);
    if (r != CH89_OK) return 0;
    if (!ch89_write_wav_mono16(path, g_atom, frames, FX_RATE)) return 0;
    if (out_frames != 0) *out_frames = frames;
    return 1;
}

static int fx_append(
    ch89_u32 *cursor,
    const ch89_i16 *samples,
    ch89_u32 frames,
    ch89_u32 gap
)
{
    ch89_u32 i;
    ch89_i32 peak;
    ch89_i32 magnitude;
    ch89_i32 scaled;
    const ch89_i32 target_peak = 12000;

    if (*cursor + frames + gap > FX_MONTAGE_MAX_FRAMES) return 0;
    peak = 1;
    for (i = 0U; i < frames; ++i) {
        magnitude = fx_abs32((ch89_i32)samples[i]);
        if (magnitude > peak) peak = magnitude;
    }
    for (i = 0U; i < frames; ++i) {
        scaled = ((ch89_i32)samples[i] * target_peak) / peak;
        g_montage[*cursor + i] = fx_clip16(scaled);
    }
    *cursor += frames;
    for (i = 0U; i < gap; ++i) {
        g_montage[*cursor + i] = 0;
    }
    *cursor += gap;
    return 1;
}

static int fx_render_and_append(
    ch89_context *ctx,
    const ch89_gesture *gesture,
    const char *path,
    ch89_u32 *cursor
)
{
    ch89_u32 frames;
    if (!fx_render(ctx, gesture, path, &frames)) return 0;
    if (!fx_append(cursor, g_atom, frames, FX_GAP_FRAMES)) return 0;
    return 1;
}

int main(void)
{
    ch89_context ctx;
    ch89_gesture base;
    ch89_gesture g;
    ch89_u32 stage_cursor;
    ch89_u32 strength_cursor;
    ch89_result r;

    r = ch89_init(&ctx, FX_RATE, FX_SEED);
    if (r != CH89_OK) {
        fprintf(stderr, "ch89_init failed: %d\n", (int)r);
        return 1;
    }

    r = ch89_make_preset(
        CH89_PRESET_ATOM_SHEKT,
        1U,
        CH89_Q15_ONE,
        &base
    );
    if (r != CH89_OK) {
        fprintf(stderr, "atom preset failed: %d\n", (int)r);
        return 2;
    }

    /* The approved SimSynth-style SH tail remains exactly 12 ms. */
    base.strokes[0].sh.envelope.release_ms = 12U;

    fx_zero(g_montage, FX_MONTAGE_MAX_FRAMES);
    stage_cursor = 0U;

    g = base;
    ch89_set_effect_flags(&g, CH89_EFFECT_NONE);
    if (!fx_render_and_append(&ctx, &g,
        "wav/20_atom_fx_00_dry.wav", &stage_cursor)) return 3;

    g = base;
    ch89_set_effect_flags(&g, CH89_EFFECT_DISTORTION);
    if (!fx_render_and_append(&ctx, &g,
        "wav/21_atom_fx_01_distortion.wav", &stage_cursor)) return 4;

    g = base;
    ch89_set_effect_flags(&g,
        CH89_EFFECT_DISTORTION | CH89_EFFECT_EQ);
    if (!fx_render_and_append(&ctx, &g,
        "wav/22_atom_fx_02_distortion_eq.wav", &stage_cursor)) return 5;

    g = base;
    ch89_set_effect_flags(&g,
        CH89_EFFECT_DISTORTION | CH89_EFFECT_EQ | CH89_EFFECT_CHORUS);
    if (!fx_render_and_append(&ctx, &g,
        "wav/23_atom_fx_03_distortion_eq_chorus.wav", &stage_cursor)) return 6;

    g = base;
    ch89_set_effect_flags(&g, CH89_EFFECT_ALL);
    if (!fx_render_and_append(&ctx, &g,
        "wav/24_atom_fx_04_full_subtle.wav", &stage_cursor)) return 7;

    if (!ch89_write_wav_mono16(
        "wav/25_atom_fx_stage_montage.wav",
        g_montage,
        stage_cursor,
        FX_RATE
    )) return 8;

    fx_zero(g_montage, FX_MONTAGE_MAX_FRAMES);
    strength_cursor = 0U;

    g = base;
    ch89_set_effect_flags(&g, CH89_EFFECT_ALL);
    if (!fx_render_and_append(&ctx, &g,
        "wav/26_atom_fx_strength_subtle.wav", &strength_cursor)) return 9;

    g = base;
    ch89_set_effect_flags(&g, CH89_EFFECT_ALL);
    g.strokes[0].sh.distortion_q15 = 18500;
    g.strokes[0].eckt.distortion_q15 = 23500;
    g.chorus_depth_samples = 24U;
    g.chorus_wet_q15 = 4300;
    g.reverb_wet_q15 = 680;
    g.output_gain_q15 = 28500;
    if (!fx_render_and_append(&ctx, &g,
        "wav/27_atom_fx_strength_medium.wav", &strength_cursor)) return 10;

    g = base;
    ch89_set_effect_flags(&g, CH89_EFFECT_ALL);
    g.strokes[0].sh.distortion_q15 = 23500;
    g.strokes[0].eckt.distortion_q15 = 28500;
    g.chorus_depth_samples = 32U;
    g.chorus_wet_q15 = 7200;
    g.reverb_wet_q15 = 1350;
    g.output_gain_q15 = 26000;
    if (!fx_render_and_append(&ctx, &g,
        "wav/28_atom_fx_strength_pushed.wav", &strength_cursor)) return 11;

    if (!ch89_write_wav_mono16(
        "wav/29_atom_fx_strength_montage.wav",
        g_montage,
        strength_cursor,
        FX_RATE
    )) return 12;

    printf("Rendered atom effect stages and strength tests.\n");
    return 0;
}

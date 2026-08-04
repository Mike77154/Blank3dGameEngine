#include <stdio.h>
#include "wsound_rocketblast89.h"

#define DEMO_SAMPLE_RATE 44100U
#define DEMO_SECONDS 4U
#define DEMO_CHUNK 512U

typedef struct demo_wav_s {
    FILE *file;
    wsrb89_u32 frames;
} demo_wav;

static wsrb89_workspace demo_workspace;
static wsrb89_context demo_context;
static wsrb89_s16 demo_left[DEMO_CHUNK];
static wsrb89_s16 demo_right[DEMO_CHUNK];

static const char *demo_names[WSRB89_PRESET_COUNT] = {
    "heavy_impact",
    "concrete_paas",
    "metal_strike",
    "airburst",
    "indoor_bunker",
    "distant_paas",
    "compact_rpg"
};

static void demo_put_u16(FILE *file, wsrb89_u16 value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void demo_put_u32(FILE *file, wsrb89_u32 value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
    fputc((int)((value >> 16) & 255U), file);
    fputc((int)((value >> 24) & 255U), file);
}

static int demo_wav_open(demo_wav *wav, const char *filename,
                         wsrb89_u32 frames)
{
    wsrb89_u32 data_bytes;
    wsrb89_u32 byte_rate;
    wav->file = fopen(filename, "wb");
    if (wav->file == (FILE *)0) {
        return 0;
    }
    wav->frames = frames;
    data_bytes = frames * 4U;
    byte_rate = DEMO_SAMPLE_RATE * 4U;
    fwrite("RIFF", 1U, 4U, wav->file);
    demo_put_u32(wav->file, 36U + data_bytes);
    fwrite("WAVE", 1U, 4U, wav->file);
    fwrite("fmt ", 1U, 4U, wav->file);
    demo_put_u32(wav->file, 16U);
    demo_put_u16(wav->file, 1U);
    demo_put_u16(wav->file, 2U);
    demo_put_u32(wav->file, DEMO_SAMPLE_RATE);
    demo_put_u32(wav->file, byte_rate);
    demo_put_u16(wav->file, 4U);
    demo_put_u16(wav->file, 16U);
    fwrite("data", 1U, 4U, wav->file);
    demo_put_u32(wav->file, data_bytes);
    return 1;
}

static void demo_wav_write(demo_wav *wav, const wsrb89_s16 *left,
                           const wsrb89_s16 *right, wsrb89_u32 frames)
{
    wsrb89_u32 i;
    i = 0U;
    while (i < frames) {
        demo_put_u16(wav->file, (wsrb89_u16)left[i]);
        demo_put_u16(wav->file, (wsrb89_u16)right[i]);
        i++;
    }
}

static void demo_wav_close(demo_wav *wav)
{
    if (wav->file != (FILE *)0) {
        fclose(wav->file);
        wav->file = (FILE *)0;
    }
}

static int demo_render_preset(wsrb89_u16 preset_id)
{
    char filename[128];
    demo_wav wav;
    wsrb89_params params;
    wsrb89_u32 total_frames;
    wsrb89_u32 done;
    wsrb89_u32 count;

    sprintf(filename, "preview/wsrb89_%s.wav", demo_names[preset_id]);
    total_frames = DEMO_SAMPLE_RATE * DEMO_SECONDS;
    if (!demo_wav_open(&wav, filename, total_frames)) {
        fprintf(stderr, "could not open %s\n", filename);
        return 0;
    }

    wsrb89_init(&demo_context, &demo_workspace, DEMO_SAMPLE_RATE,
                 0x12345678U + preset_id * 0x10203U);
    wsrb89_get_preset(&params, preset_id);
    wsrb89_set_params(&demo_context, &params);
    wsrb89_trigger(&demo_context, 32767U);

    done = 0U;
    while (done < total_frames) {
        count = total_frames - done;
        if (count > DEMO_CHUNK) {
            count = DEMO_CHUNK;
        }
        wsrb89_render_stereo(&demo_context, demo_left, demo_right, count);
        demo_wav_write(&wav, demo_left, demo_right, count);
        done += count;
    }
    demo_wav_close(&wav);
    printf("wrote %s\n", filename);
    return 1;
}

static int demo_render_montage(void)
{
    demo_wav wav;
    wsrb89_params params;
    wsrb89_u32 segment_frames;
    wsrb89_u32 gap_frames;
    wsrb89_u32 total_frames;
    wsrb89_u32 done;
    wsrb89_u32 count;
    wsrb89_u32 i;
    wsrb89_u16 preset;

    segment_frames = DEMO_SAMPLE_RATE * 3U;
    gap_frames = DEMO_SAMPLE_RATE / 4U;
    total_frames = (segment_frames + gap_frames) * WSRB89_PRESET_COUNT;
    if (!demo_wav_open(&wav, "preview/wsrb89_all_presets.wav", total_frames)) {
        return 0;
    }

    preset = 0U;
    while (preset < WSRB89_PRESET_COUNT) {
        wsrb89_init(&demo_context, &demo_workspace, DEMO_SAMPLE_RATE,
                     0xBADC0DEU + preset * 0x10001U);
        wsrb89_get_preset(&params, preset);
        wsrb89_set_params(&demo_context, &params);
        wsrb89_trigger(&demo_context, 32767U);
        done = 0U;
        while (done < segment_frames) {
            count = segment_frames - done;
            if (count > DEMO_CHUNK) {
                count = DEMO_CHUNK;
            }
            wsrb89_render_stereo(&demo_context, demo_left, demo_right, count);
            demo_wav_write(&wav, demo_left, demo_right, count);
            done += count;
        }
        done = 0U;
        while (done < gap_frames) {
            count = gap_frames - done;
            if (count > DEMO_CHUNK) {
                count = DEMO_CHUNK;
            }
            i = 0U;
            while (i < count) {
                demo_left[i] = 0;
                demo_right[i] = 0;
                i++;
            }
            demo_wav_write(&wav, demo_left, demo_right, count);
            done += count;
        }
        preset++;
    }
    demo_wav_close(&wav);
    printf("wrote preview/wsrb89_all_presets.wav\n");
    return 1;
}


static void demo_write_silence(demo_wav *wav, wsrb89_u32 frames)
{
    wsrb89_u32 done;
    wsrb89_u32 count;
    wsrb89_u32 i;
    done = 0U;
    while (done < frames) {
        count = frames - done;
        if (count > DEMO_CHUNK) {
            count = DEMO_CHUNK;
        }
        i = 0U;
        while (i < count) {
            demo_left[i] = 0;
            demo_right[i] = 0;
            i++;
        }
        demo_wav_write(wav, demo_left, demo_right, count);
        done += count;
    }
}

static void demo_render_segment(demo_wav *wav, const wsrb89_params *params,
                                wsrb89_u32 seed, wsrb89_u32 frames)
{
    wsrb89_u32 done;
    wsrb89_u32 count;
    wsrb89_init(&demo_context, &demo_workspace, DEMO_SAMPLE_RATE, seed);
    wsrb89_set_params(&demo_context, params);
    wsrb89_trigger(&demo_context, 32767U);
    done = 0U;
    while (done < frames) {
        count = frames - done;
        if (count > DEMO_CHUNK) {
            count = DEMO_CHUNK;
        }
        wsrb89_render_stereo(&demo_context, demo_left, demo_right, count);
        demo_wav_write(wav, demo_left, demo_right, count);
        done += count;
    }
}

static int demo_render_noise_stack_comparison(void)
{
    demo_wav wav;
    wsrb89_params five_noise;
    wsrb89_params seven_noise;
    wsrb89_u32 segment_frames;
    wsrb89_u32 gap_frames;
    wsrb89_u32 total_frames;
    wsrb89_u32 seed;

    segment_frames = DEMO_SAMPLE_RATE * DEMO_SECONDS;
    gap_frames = DEMO_SAMPLE_RATE / 2U;
    total_frames = segment_frames + gap_frames + segment_frames;
    if (!demo_wav_open(&wav,
            "preview/wsrb89_ab_5noise_vs_7noise_rumble_crackle.wav",
            total_frames)) {
        return 0;
    }

    wsrb89_get_preset(&seven_noise, WSRB89_PRESET_HEAVY_IMPACT);
    five_noise = seven_noise;
    five_noise.noise_level_q15[WSRB89_NOISE_RUMBLE] = 0;
    five_noise.noise_level_q15[WSRB89_NOISE_CRACKLE] = 0;
    five_noise.motion_lfo_depth_q15 = 0U;
    seed = 0x51525354U;

    demo_render_segment(&wav, &five_noise, seed, segment_frames);
    demo_write_silence(&wav, gap_frames);
    demo_render_segment(&wav, &seven_noise, seed, segment_frames);
    demo_wav_close(&wav);
    printf("wrote preview/wsrb89_ab_5noise_vs_7noise_rumble_crackle.wav\n");
    return 1;
}

static int demo_render_rumble_crackle_layers(void)
{
    demo_wav wav;
    wsrb89_params old_stack;
    wsrb89_params rumble_only;
    wsrb89_params crackle_only;
    wsrb89_params full;
    wsrb89_u32 segment_frames;
    wsrb89_u32 gap_frames;
    wsrb89_u32 total_frames;
    wsrb89_u32 seed;
    wsrb89_u16 i;

    segment_frames = DEMO_SAMPLE_RATE * 3U;
    gap_frames = DEMO_SAMPLE_RATE / 3U;
    total_frames = (segment_frames * 4U) + (gap_frames * 3U);
    if (!demo_wav_open(&wav,
            "preview/wsrb89_rumble_crackle_layers.wav", total_frames)) {
        return 0;
    }

    wsrb89_get_preset(&full, WSRB89_PRESET_HEAVY_IMPACT);
    old_stack = full;
    old_stack.noise_level_q15[WSRB89_NOISE_RUMBLE] = 0;
    old_stack.noise_level_q15[WSRB89_NOISE_CRACKLE] = 0;
    old_stack.motion_lfo_depth_q15 = 0U;

    rumble_only = full;
    crackle_only = full;
    i = 0U;
    while (i < WSRB89_NOISE_OSCILLATORS) {
        rumble_only.noise_level_q15[i] = 0;
        crackle_only.noise_level_q15[i] = 0;
        i++;
    }
    rumble_only.noise_level_q15[WSRB89_NOISE_RUMBLE] = 30000;
    rumble_only.sine_level_q15 = 0;
    rumble_only.saw_level_q15 = 0;
    rumble_only.shock_level_q15 = 0;
    rumble_only.reverb_mix_q15 = 9000U;
    rumble_only.output_gain_q12 = 7000U;

    crackle_only.noise_level_q15[WSRB89_NOISE_CRACKLE] = 30000;
    crackle_only.sine_level_q15 = 0;
    crackle_only.saw_level_q15 = 0;
    crackle_only.shock_level_q15 = 0;
    crackle_only.reverb_mix_q15 = 6500U;
    crackle_only.output_gain_q12 = 7600U;

    seed = 0x71727374U;
    demo_render_segment(&wav, &old_stack, seed, segment_frames);
    demo_write_silence(&wav, gap_frames);
    demo_render_segment(&wav, &rumble_only, seed, segment_frames);
    demo_write_silence(&wav, gap_frames);
    demo_render_segment(&wav, &crackle_only, seed, segment_frames);
    demo_write_silence(&wav, gap_frames);
    demo_render_segment(&wav, &full, seed, segment_frames);
    demo_wav_close(&wav);
    printf("wrote preview/wsrb89_rumble_crackle_layers.wav\n");
    return 1;
}

int main(void)
{
    wsrb89_u16 preset;
    preset = 0U;
    while (preset < WSRB89_PRESET_COUNT) {
        if (!demo_render_preset(preset)) {
            return 1;
        }
        preset++;
    }
    if (!demo_render_montage()) {
        return 1;
    }
    if (!demo_render_noise_stack_comparison()) {
        return 1;
    }
    if (!demo_render_rumble_crackle_layers()) {
        return 1;
    }
    printf("workspace bytes: %u\n", (unsigned int)wsrb89_workspace_bytes());
    printf("context bytes: %u\n", (unsigned int)wsrb89_context_bytes());
    return 0;
}

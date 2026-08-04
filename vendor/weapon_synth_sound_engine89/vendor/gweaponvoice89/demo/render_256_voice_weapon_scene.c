#include <stdio.h>
#include <string.h>
#include "gweaponvoice89.h"

#define SAMPLE_RATE 44100U
#define LOGICAL_VOICES 256U
#define PHYSICAL_VOICES 64U
#define EVENT_COUNT 512U
#define TOTAL_FRAMES (SAMPLE_RATE * 6U)
#define BLOCK_FRAMES 64U

typedef struct demo_source_s {
    gv89_u32 pos;
    gv89_u32 length;
    gv89_u32 phase;
    gv89_u32 step;
    gv89_u32 lfsr;
    gv89_s32 lowpass;
    gv89_s16 amplitude;
    gv89_u8 kind;
} demo_source;

typedef struct demo_event_s {
    demo_source source;
    gv89_handle handle;
    gv89_u32 start_frame;
    gwv89_event_class event_class;
    gv89_u8 started;
} demo_event;

static demo_event events[EVENT_COUNT];
static gv89_voice voice_storage[LOGICAL_VOICES];
static gv89_s16 block_buffer[BLOCK_FRAMES * 2U];

static gv89_s32 demo_noise(demo_source *source)
{
    source->lfsr = source->lfsr * 1664525U + 1013904223U;
    return (gv89_s32)((source->lfsr >> 16) & 65535U) - 32768;
}

static gv89_s16 demo_process(void *user)
{
    demo_source *source;
    gv89_s32 env;
    gv89_s32 fast_env;
    gv89_s32 noise;
    gv89_s32 square;
    gv89_s32 sample;
    gv89_u32 remain;
    source = (demo_source *)user;
    if (source->pos >= source->length) return 0;
    remain = source->length - source->pos;
    env = (gv89_s32)((remain * 32767U) / source->length);
    noise = demo_noise(source);
    square = (source->phase & 0x80000000U) ? 32767 : -32767;
    sample = 0;

    if (source->kind == 0U) {
        fast_env = source->pos < 900U
            ? (gv89_s32)(((900U - source->pos) * 32767U) / 900U) : 0;
        sample = ((noise * fast_env) >> 16)
               + (((square * env) >> 15) >> 2);
    } else if (source->kind == 1U) {
        fast_env = source->pos < 1800U
            ? (gv89_s32)(((1800U - source->pos) * 32767U) / 1800U)
            : (env >> 3);
        sample = (noise * fast_env) >> 16;
    } else if (source->kind == 2U) {
        sample = ((square * env) >> 15)
               + (((noise * env) >> 15) >> 3);
        source->step += 1700U;
    } else if (source->kind == 3U) {
        source->lowpass += (noise - source->lowpass) >> 4;
        sample = ((source->lowpass * env) >> 15)
               + (((square * env) >> 15) >> 3);
    } else {
        source->lowpass += (noise - source->lowpass) >> 6;
        sample = (source->lowpass * env) >> 15;
    }

    source->phase += source->step;
    source->pos++;
    sample = (sample * source->amplitude) >> 15;
    if (sample > 32767) sample = 32767;
    if (sample < -32768) sample = -32768;
    return (gv89_s16)sample;
}

static int demo_active(const void *user)
{
    const demo_source *source;
    source = (const demo_source *)user;
    return source->pos < source->length;
}

static void demo_stop(void *user)
{
    demo_source *source;
    source = (demo_source *)user;
    source->pos = source->length;
}

static void demo_advance(void *user, gv89_u32 frames)
{
    demo_source *source;
    gv89_u32 left;
    source = (demo_source *)user;
    if (source->pos >= source->length) return;
    left = source->length - source->pos;
    if (frames > left) frames = left;
    source->phase += source->step * frames;
    source->lfsr = source->lfsr * 1664525U + 1013904223U + frames;
    source->pos += frames;
}

static void demo_restart(void *user)
{
    demo_source *source;
    source = (demo_source *)user;
    source->pos = 0U;
    source->lowpass = 0;
}

static gv89_u16 demo_level(const void *user)
{
    const demo_source *source;
    gv89_u32 remain;
    if (user == 0) return 0U;
    source = (const demo_source *)user;
    if (source->pos >= source->length) return 0U;
    remain = source->length - source->pos;
    return (gv89_u16)((remain * 32767U) / source->length);
}

static void write_u16(FILE *file, unsigned int value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
}

static void write_u32(FILE *file, gv89_u32 value)
{
    fputc((int)(value & 255U), file);
    fputc((int)((value >> 8) & 255U), file);
    fputc((int)((value >> 16) & 255U), file);
    fputc((int)((value >> 24) & 255U), file);
}

static int write_wav_header(FILE *file, gv89_u32 frames)
{
    gv89_u32 data_bytes;
    data_bytes = frames * 4U;
    if (fwrite("RIFF", 1U, 4U, file) != 4U) return 0;
    write_u32(file, 36U + data_bytes);
    if (fwrite("WAVEfmt ", 1U, 8U, file) != 8U) return 0;
    write_u32(file, 16U);
    write_u16(file, 1U);
    write_u16(file, 2U);
    write_u32(file, SAMPLE_RATE);
    write_u32(file, SAMPLE_RATE * 4U);
    write_u16(file, 4U);
    write_u16(file, 16U);
    if (fwrite("data", 1U, 4U, file) != 4U) return 0;
    write_u32(file, data_bytes);
    return 1;
}

static gwv89_event_class initial_class(gv89_u16 index)
{
    if (index < 80U) return GWV89_EVENT_REPORT;
    if (index < 160U) return GWV89_EVENT_IMPACT;
    if (index < 208U) return GWV89_EVENT_RICOCHET;
    if (index < 232U) return GWV89_EVENT_EXPLOSION;
    return GWV89_EVENT_AMBIENCE;
}

static gwv89_event_class reinforcement_class(gv89_u16 local_index)
{
    if (local_index < 64U) return GWV89_EVENT_REPORT;
    if (local_index < 112U) return GWV89_EVENT_IMPACT;
    return GWV89_EVENT_EXPLOSION;
}

static gv89_u8 kind_from_class(gwv89_event_class event_class)
{
    if (event_class == GWV89_EVENT_REPORT) return 0U;
    if (event_class == GWV89_EVENT_IMPACT) return 1U;
    if (event_class == GWV89_EVENT_RICOCHET) return 2U;
    if (event_class == GWV89_EVENT_EXPLOSION) return 3U;
    return 4U;
}

static gv89_u32 length_from_class(gwv89_event_class event_class,
                                  gv89_u16 index)
{
    gv89_u32 base;
    if (event_class == GWV89_EVENT_REPORT) base = 36000U;
    else if (event_class == GWV89_EVENT_IMPACT) base = 30000U;
    else if (event_class == GWV89_EVENT_RICOCHET) base = 50000U;
    else if (event_class == GWV89_EVENT_EXPLOSION) base = 110000U;
    else base = TOTAL_FRAMES;
    return base + (gv89_u32)(index & 15U) * 700U;
}

static gv89_s16 amplitude_from_class(gwv89_event_class event_class)
{
    if (event_class == GWV89_EVENT_REPORT) return 2600;
    if (event_class == GWV89_EVENT_IMPACT) return 1700;
    if (event_class == GWV89_EVENT_RICOCHET) return 1200;
    if (event_class == GWV89_EVENT_EXPLOSION) return 1900;
    return 700;
}

static void setup_events(void)
{
    gv89_u16 i;
    gv89_u16 local;
    gwv89_event_class event_class;
    for (i = 0U; i < EVENT_COUNT; ++i) {
        if (i < 256U) {
            event_class = initial_class(i);
            events[i].start_frame = 0U;
        } else if (i < 384U) {
            local = (gv89_u16)(i - 256U);
            event_class = reinforcement_class(local);
            events[i].start_frame = 32768U;
        } else {
            local = (gv89_u16)(i - 384U);
            event_class = reinforcement_class(local);
            events[i].start_frame = 98304U;
        }
        events[i].event_class = event_class;
        events[i].started = 0U;
        events[i].handle.index = GV89_INVALID_INDEX;
        events[i].handle.generation = 0U;
        events[i].source.pos = 0U;
        events[i].source.length = length_from_class(event_class, i);
        events[i].source.phase = (gv89_u32)i * 2654435761U;
        events[i].source.step = 350000U + (gv89_u32)(i & 63U) * 45000U;
        events[i].source.lfsr = 0xA341316CU ^ ((gv89_u32)i * 747796405U);
        events[i].source.lowpass = 0;
        events[i].source.amplitude = amplitude_from_class(event_class);
        events[i].source.kind = kind_from_class(event_class);
    }
}

static gv89_result start_wave(gwv89_context *ctx,
                               gv89_u16 first,
                               gv89_u16 count,
                               gv89_u8 reinforcement)
{
    gv89_provider_ex provider;
    gwv89_event_desc desc;
    gv89_result result;
    gv89_u16 i;
    gv89_u16 index;
    provider.base.process_mono = demo_process;
    provider.base.is_active = demo_active;
    provider.base.stop = demo_stop;
    provider.advance_frames = demo_advance;
    provider.restart = demo_restart;
    provider.estimated_level_q15 = demo_level;
    provider.physical_state_changed = 0;
    gwv89_begin_batch(ctx);
    for (i = 0U; i < count; ++i) {
        index = (gv89_u16)(first + i);
        provider.base.user = &events[index].source;
        gwv89_event_default(&desc, events[index].event_class);
        desc.screen_x_q15 = (gv89_s16)(((gv89_s32)(index & 31U) * 2048)
                              - 31744);
        desc.distance_q15 = (gv89_u16)((index * 131U) & 24575U);
        desc.occlusion_q15 = (gv89_u16)(24576U + ((index * 41U) & 8191U));
        desc.focus_q15 = 32767U;
        desc.instance_key = (gv89_u32)(events[index].event_class + 1)
                          * 1000U + (index & 31U);
        if (reinforcement) {
            desc.priority_bias = 90;
            desc.allow_protected_steal = 1U;
            desc.allow_higher_priority_steal = 1U;
            desc.distance_q15 = (gv89_u16)((index * 29U) & 8191U);
        }
        result = gwv89_play_ex(ctx, &desc, &provider, &events[index].handle);
        if (result != GV89_OK) {
            gwv89_end_batch(ctx);
            return result;
        }
        events[index].started = 1U;
    }
    return gwv89_end_batch(ctx);
}

static void animate_spatial(gwv89_context *ctx, gv89_u32 frame)
{
    gv89_u16 i;
    gv89_u16 distance;
    gv89_s16 pan;
    for (i = 0U; i < EVENT_COUNT; i = (gv89_u16)(i + 8U)) {
        if (!events[i].started) continue;
        distance = (gv89_u16)(((frame >> 3) + (gv89_u32)i * 733U)
                   & 32767U);
        pan = (gv89_s16)((((gv89_s32)((frame >> 7) + i * 997U)
              & 65535) - 32768));
        gwv89_set_event_spatial(ctx, events[i].handle, pan, distance,
                                30000U, 32767U);
    }
}

int main(int argc, char **argv)
{
    const char *path;
    FILE *file;
    gwv89_context ctx;
    gv89_stats stats;
    gv89_result result;
    gv89_u32 frame;
    gv89_u32 chunk;
    gv89_u32 written;
    gv89_u32 i;
    gv89_s16 sample;

    path = argc > 1 ? argv[1] : "audio/weapon_voice_256logical_64physical_stress.wav";
    memset(&ctx, 0, sizeof(ctx));
    memset(voice_storage, 0, sizeof(voice_storage));
    setup_events();
    result = gwv89_init_ex(&ctx, voice_storage, LOGICAL_VOICES,
                           PHYSICAL_VOICES, SAMPLE_RATE);
    if (result != GV89_OK) return 1;
    gv89_set_master(&ctx.voices, 23500, 30000, 9U);
    gv89_set_virtualization(&ctx.voices, 64U, 1400U, 128U);

    file = fopen(path, "wb");
    if (file == 0) return 2;
    if (!write_wav_header(file, TOTAL_FRAMES)) {
        fclose(file);
        return 3;
    }

    result = start_wave(&ctx, 0U, 256U, 0U);
    if (result != GV89_OK) {
        fclose(file);
        return 4;
    }

    frame = 0U;
    while (frame < TOTAL_FRAMES) {
        if (frame == 32768U) {
            result = start_wave(&ctx, 256U, 128U, 1U);
            if (result != GV89_OK) {
                fclose(file);
                return 5;
            }
        }
        if (frame == 98304U) {
            result = start_wave(&ctx, 384U, 128U, 1U);
            if (result != GV89_OK) {
                fclose(file);
                return 6;
            }
        }
        if ((frame & 2047U) == 0U) animate_spatial(&ctx, frame);
        chunk = TOTAL_FRAMES - frame;
        if (chunk > BLOCK_FRAMES) chunk = BLOCK_FRAMES;
        memset(block_buffer, 0, sizeof(block_buffer));
        gwv89_render_stereo(&ctx, block_buffer, chunk, 0);
        written = 0U;
        for (i = 0U; i < chunk * 2U; ++i) {
            sample = block_buffer[i];
            write_u16(file, (unsigned int)(gv89_u16)sample);
            written++;
        }
        if (written != chunk * 2U) {
            fclose(file);
            return 7;
        }
        frame += chunk;
    }
    fclose(file);
    gwv89_get_stats(&ctx, &stats);
    printf("gweaponvoice89 v2.0 stress demo\n");
    printf("logical capacity: %u\n", (unsigned)LOGICAL_VOICES);
    printf("physical budget: %u\n", (unsigned)PHYSICAL_VOICES);
    printf("starts: %lu\n", (unsigned long)stats.starts);
    printf("steals: %lu\n", (unsigned long)stats.steals);
    printf("group steals: %lu\n", (unsigned long)stats.group_steals);
    printf("peak logical: %lu\n", (unsigned long)stats.peak_logical);
    printf("peak physical: %lu\n", (unsigned long)stats.peak_physical);
    printf("promotions: %lu\n", (unsigned long)stats.promotions);
    printf("demotions: %lu\n", (unsigned long)stats.demotions);
    printf("virtual kills: %lu\n", (unsigned long)stats.virtual_kills);
    printf("virtual timeouts: %lu\n", (unsigned long)stats.virtual_timeouts);
    printf("limiter hits: %lu\n", (unsigned long)stats.limiter_hits);
    printf("rebalance passes: %lu\n", (unsigned long)stats.rebalance_passes);
    return 0;
}

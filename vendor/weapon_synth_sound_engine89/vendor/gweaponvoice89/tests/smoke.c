#include <stdio.h>
#include <string.h>
#include "gweaponvoice89.h"

#define LOGICAL_VOICES 256
#define SOURCES 320
#define FRAMES 2048

typedef struct test_source_s {
    gv89_u32 pos;
    gv89_u32 length;
    gv89_u32 phase;
    gv89_u32 step;
    gv89_s16 amp;
} test_source;

static gv89_s16 source_process(void *user)
{
    test_source *source;
    gv89_s32 env;
    gv89_s32 sample;
    source = (test_source *)user;
    if (source->pos >= source->length) return 0;
    env = (gv89_s32)(((source->length - source->pos) * 32767U)
          / source->length);
    sample = (source->phase & 0x80000000U) ? source->amp : -source->amp;
    source->phase += source->step;
    source->pos++;
    return (gv89_s16)((sample * env) >> 15);
}

static int source_active(const void *user)
{
    const test_source *source;
    source = (const test_source *)user;
    return source->pos < source->length;
}

static void source_stop(void *user)
{
    test_source *source;
    source = (test_source *)user;
    source->pos = source->length;
}

static void source_advance(void *user, gv89_u32 frames)
{
    test_source *source;
    gv89_u32 left;
    source = (test_source *)user;
    left = source->length - source->pos;
    if (frames > left) frames = left;
    source->phase += source->step * frames;
    source->pos += frames;
}

static gv89_u16 source_level(const void *user)
{
    const test_source *source;
    gv89_u32 remain;
    source = (const test_source *)user;
    if (source->pos >= source->length) return 0U;
    remain = source->length - source->pos;
    return (gv89_u16)((remain * 32767U) / source->length);
}

static gwv89_event_class class_for_index(gv89_u16 i)
{
    if (i < 80U) return GWV89_EVENT_REPORT;
    if (i < 160U) return GWV89_EVENT_IMPACT;
    if (i < 208U) return GWV89_EVENT_RICOCHET;
    if (i < 232U) return GWV89_EVENT_EXPLOSION;
    return GWV89_EVENT_AMBIENCE;
}

static gv89_u16 physical_in_group(const gwv89_context *ctx, gv89_u16 group_id)
{
    gv89_u16 i;
    gv89_u16 count;
    count = 0U;
    for (i = 0U; i < ctx->voices.capacity; ++i) {
        if (ctx->voices.voices[i].active
            && ctx->voices.voices[i].physical
            && ctx->voices.voices[i].group_id == group_id) count++;
    }
    return count;
}

int main(void)
{
    gwv89_context ctx;
    gv89_voice voices[LOGICAL_VOICES];
    test_source sources[SOURCES];
    gv89_provider_ex provider;
    gwv89_event_desc event;
    gv89_handle handle;
    gv89_s16 mix[FRAMES * 2];
    gv89_stats stats;
    gv89_result result;
    gv89_u16 i;

    memset(&ctx, 0, sizeof(ctx));
    memset(voices, 0, sizeof(voices));
    memset(sources, 0, sizeof(sources));
    memset(mix, 0, sizeof(mix));
    result = gwv89_init_ex(&ctx, voices, LOGICAL_VOICES, 64U, 44100U);
    if (result != GV89_OK) return 1;

    provider.base.process_mono = source_process;
    provider.base.is_active = source_active;
    provider.base.stop = source_stop;
    provider.advance_frames = source_advance;
    provider.restart = 0;
    provider.estimated_level_q15 = source_level;
    provider.physical_state_changed = 0;

    gwv89_begin_batch(&ctx);
    for (i = 0U; i < LOGICAL_VOICES; ++i) {
        sources[i].pos = 0U;
        sources[i].length = 44100U;
        sources[i].phase = (gv89_u32)i * 1234567U;
        sources[i].step = 700000U + (gv89_u32)i * 11003U;
        sources[i].amp = 1500;
        provider.base.user = &sources[i];
        gwv89_event_default(&event, class_for_index(i));
        event.screen_x_q15 = (gv89_s16)(((gv89_s32)(i & 31U) * 2048)
                              - 31744);
        event.distance_q15 = (gv89_u16)((i * 127U) & 32767U);
        result = gwv89_play_ex(&ctx, &event, &provider, &handle);
        if (result != GV89_OK) return 2;
    }
    if (gwv89_end_batch(&ctx) != GV89_OK) return 3;
    if (gwv89_active_count(&ctx) != 256U) return 4;
    if (gwv89_physical_count(&ctx) != 64U) return 5;
    if (gwv89_virtual_count(&ctx) != 192U) return 6;
    if (physical_in_group(&ctx, GWV89_GROUP_REPORT) < 24U) return 13;
    if (physical_in_group(&ctx, GWV89_GROUP_IMPACT) < 12U) return 14;
    if (physical_in_group(&ctx, GWV89_GROUP_RICOCHET) < 4U) return 15;
    if (physical_in_group(&ctx, GWV89_GROUP_EXPLOSION) < 8U) return 16;

    gwv89_begin_batch(&ctx);
    for (i = 256U; i < SOURCES; ++i) {
        sources[i].pos = 0U;
        sources[i].length = 22050U;
        sources[i].phase = (gv89_u32)i * 7654321U;
        sources[i].step = 2000000U + (gv89_u32)i * 9011U;
        sources[i].amp = 1800;
        provider.base.user = &sources[i];
        gwv89_event_default(&event, GWV89_EVENT_REPORT);
        event.priority_bias = 80;
        event.distance_q15 = (gv89_u16)((i * 53U) & 8191U);
        event.allow_protected_steal = 1U;
        result = gwv89_play_ex(&ctx, &event, &provider, &handle);
        if (result != GV89_OK) return 7;
    }
    if (gwv89_end_batch(&ctx) != GV89_OK) return 8;
    if (gwv89_active_count(&ctx) != 256U) return 9;
    gwv89_get_stats(&ctx, &stats);
    if (stats.rebalance_passes > 4U) return 17;
    gwv89_render_stereo(&ctx, mix, FRAMES, 0);
    gwv89_get_stats(&ctx, &stats);
    if (stats.steals == 0U) return 10;
    if (stats.peak_logical != 256U) return 11;
    if (stats.peak_physical != 64U) return 12;
    printf("PASS logical=%u physical=%u virtual=%u steals=%lu voice_bytes=%lu context_bytes=%lu\n",
           (unsigned)gwv89_active_count(&ctx),
           (unsigned)gwv89_physical_count(&ctx),
           (unsigned)gwv89_virtual_count(&ctx),
           (unsigned long)stats.steals,
           (unsigned long)gv89_voice_bytes(),
           (unsigned long)gwv89_context_bytes());
    return 0;
}

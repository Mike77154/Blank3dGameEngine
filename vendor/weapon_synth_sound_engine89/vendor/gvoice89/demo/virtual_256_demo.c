#include <stdio.h>
#include <string.h>
#include "gvoice89.h"

#define DEMO_VOICES 256U
#define DEMO_FRAMES 4096U

typedef struct demo_source_s {
    gv89_u32 pos;
    gv89_u32 length;
    gv89_u32 phase;
    gv89_u32 step;
} demo_source;

static gv89_voice voice_storage[DEMO_VOICES];
static demo_source sources[DEMO_VOICES];
static gv89_s16 mix_buffer[DEMO_FRAMES * 2U];

static gv89_s16 process_source(void *user)
{
    demo_source *source;
    gv89_s32 sample;
    gv89_s32 env;
    source = (demo_source *)user;
    if (source->pos >= source->length) return 0;
    sample = (source->phase & 0x80000000U) ? 1800 : -1800;
    env = (gv89_s32)(((source->length - source->pos) * 32767U)
          / source->length);
    source->phase += source->step;
    source->pos++;
    return (gv89_s16)((sample * env) >> 15);
}

static int source_active(const void *user)
{
    const demo_source *source;
    source = (const demo_source *)user;
    return source->pos < source->length;
}

static void stop_source(void *user)
{
    demo_source *source;
    source = (demo_source *)user;
    source->pos = source->length;
}

static void advance_source(void *user, gv89_u32 frames)
{
    demo_source *source;
    gv89_u32 left;
    source = (demo_source *)user;
    left = source->length - source->pos;
    if (frames > left) frames = left;
    source->phase += source->step * frames;
    source->pos += frames;
}

static gv89_u16 source_level(const void *user)
{
    const demo_source *source;
    source = (const demo_source *)user;
    if (source->pos >= source->length) return 0U;
    return (gv89_u16)(((source->length - source->pos) * 32767U)
           / source->length);
}

int main(void)
{
    gv89_context context;
    gv89_provider_ex provider;
    gv89_request request;
    gv89_voice_params params;
    gv89_handle handle;
    gv89_stats stats;
    gv89_u16 i;

    memset(&context, 0, sizeof(context));
    memset(voice_storage, 0, sizeof(voice_storage));
    memset(sources, 0, sizeof(sources));
    memset(mix_buffer, 0, sizeof(mix_buffer));
    if (gv89_init(&context, voice_storage, DEMO_VOICES, 44100U) != GV89_OK) {
        return 1;
    }
    gv89_set_physical_limit(&context, 64U);
    gv89_request_default(&request);
    gv89_voice_params_default(&params);
    request.group_id = 1U;
    request.virtual_behavior = GV89_VIRTUAL_ADVANCE;
    gv89_set_group_rule(&context, 1U, DEMO_VOICES, 8U,
                         GV89_STEAL_PRIORITY_AUDIBILITY);

    provider.base.process_mono = process_source;
    provider.base.is_active = source_active;
    provider.base.stop = stop_source;
    provider.advance_frames = advance_source;
    provider.restart = 0;
    provider.estimated_level_q15 = source_level;
    provider.physical_state_changed = 0;

    gv89_begin_batch(&context);
    for (i = 0U; i < DEMO_VOICES; ++i) {
        sources[i].pos = 0U;
        sources[i].length = 44100U;
        sources[i].phase = (gv89_u32)i * 2654435761U;
        sources[i].step = 500000U + (gv89_u32)i * 19001U;
        provider.base.user = &sources[i];
        request.priority = (gv89_u16)(100U + (i & 63U));
        params.audibility_q15 = (gv89_u16)(4096U
            + ((gv89_u32)i * 113U) % 28671U);
        params.pan_q15 = (gv89_s16)(((gv89_s32)(i & 31U) * 2048)
                         - 31744);
        if (gv89_start_ex(&context, &request, &provider,
                          &params, &handle) != GV89_OK) return 2;
    }
    if (gv89_end_batch(&context) != GV89_OK) return 3;
    gv89_render_stereo(&context, mix_buffer, DEMO_FRAMES, 0);
    gv89_get_stats(&context, &stats);
    printf("logical=%u physical=%u virtual=%u promotions=%lu demotions=%lu\n",
           (unsigned)gv89_active_count(&context),
           (unsigned)gv89_physical_count(&context),
           (unsigned)gv89_virtual_count(&context),
           (unsigned long)stats.promotions,
           (unsigned long)stats.demotions);
    printf("voice_bytes=%lu context_bytes=%lu manager_total_256=%lu\n",
           (unsigned long)gv89_voice_bytes(),
           (unsigned long)gv89_context_bytes(),
           (unsigned long)(gv89_context_bytes()
             + gv89_voice_bytes() * DEMO_VOICES));
    return 0;
}

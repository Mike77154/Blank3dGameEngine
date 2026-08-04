#include "wsoundaction89.h"

typedef struct wa_def_s { wsound89_u16 ms; wsound89_u16 code; wsound89_u16 energy; } wa_def;

static const wa_def wa_pistol[] = { { 4,1,19000 }, { 11,2,23000 }, { 24,3,18000 }, { 39,4,17000 }, { 57,6,24000 }, { 0,0,0 } };
static const wa_def wa_machine[] = { { 2,2,21000 }, { 8,3,16000 }, { 15,4,16500 }, { 24,6,22000 }, { 0,0,0 } };
static const wa_def wa_rifle[] = { { 3,2,24000 }, { 12,3,18500 }, { 23,4,19000 }, { 36,6,26000 }, { 0,0,0 } };
static const wa_def wa_revolver[] = { { 1,1,23000 }, { 19,7,19000 }, { 0,0,0 } };
static const wa_def wa_pump[] = { { 1,8,18000 }, { 188,2,29000 }, { 201,3,21000 }, { 252,5,24000 }, { 414,4,20000 }, { 438,9,30000 } };
static const wa_def wa_semi[] = { { 4,2,25000 }, { 17,3,19000 }, { 34,5,21000 }, { 59,6,27000 }, { 0,0,0 } };

static const wa_def *wa_table(wsound89_u16 type)
{
    if (type == 0U) return wa_pistol;
    if (type == 1U) return wa_machine;
    if (type == 2U) return wa_rifle;
    if (type == 3U) return wa_revolver;
    if (type == 4U) return wa_pump;
    return wa_semi;
}

static wsound89_u16 wa_count(wsound89_u16 type)
{
    if (type == 3U) return 2U;
    if (type == 1U) return 4U;
    if (type == 4U) return 6U;
    return 5U;
}

static wsound89_u32 wa_event_frame(const wsoundaction89_context *ctx, const wa_def *def)
{
    wsound89_u32 base;
    if (ctx->speed_q16 == 0U) return 0U;
    base = ((wsound89_u32)def->ms * ctx->sample_rate) / 1000U;
    return (base * 65536U) / ctx->speed_q16;
}

wsound89_result wsoundaction89_init(wsoundaction89_context *ctx, wsound89_u32 sample_rate)
{
    if (ctx == 0 || sample_rate < 8000U) return WSOUND89_EINVAL;
    ctx->sample_rate = sample_rate;
    ctx->frame = 0U;
    ctx->speed_q16 = 65536U;
    ctx->type = 0U;
    ctx->next_event = 0U;
    ctx->active = 0U;
    return WSOUND89_OK;
}

wsound89_result wsoundaction89_trigger(wsoundaction89_context *ctx, wsoundaction89_type type,
                                       wsound89_u32 speed_q16)
{
    if (ctx == 0 || (wsound89_u32)type > (wsound89_u32)WSOUNDACTION89_SEMI_SHOTGUN || speed_q16 == 0U) return WSOUND89_EINVAL;
    ctx->frame = 0U;
    ctx->speed_q16 = speed_q16;
    ctx->type = (wsound89_u16)type;
    ctx->next_event = 0U;
    ctx->active = 1U;
    return WSOUND89_OK;
}

wsound89_result wsoundaction89_advance(wsoundaction89_context *ctx, wsound89_u32 frames,
                                       wsoundaction89_event *events, wsound89_u16 capacity,
                                       wsound89_u16 *written)
{
    const wa_def *table;
    wsound89_u16 count;
    wsound89_u16 n;
    wsound89_u32 end_frame;
    wsound89_u32 event_frame;
    if (ctx == 0 || written == 0) return WSOUND89_EINVAL;
    if (capacity != 0U && events == 0) return WSOUND89_EINVAL;
    *written = 0U;
    if (ctx->active == 0U) return WSOUND89_OK;
    table = wa_table(ctx->type);
    count = wa_count(ctx->type);
    end_frame = ctx->frame + frames;
    n = 0U;
    while (ctx->next_event < count) {
        event_frame = wa_event_frame(ctx, &table[ctx->next_event]);
        if (event_frame >= end_frame) break;
        if (event_frame >= ctx->frame) {
            if (n >= capacity) return WSOUND89_ECAPACITY;
            events[n].frame_offset = event_frame - ctx->frame;
            events[n].code = table[ctx->next_event].code;
            events[n].energy_q15 = table[ctx->next_event].energy;
            n++;
        }
        ctx->next_event++;
    }
    ctx->frame = end_frame;
    if (ctx->next_event >= count) ctx->active = 0U;
    *written = n;
    return WSOUND89_OK;
}

int wsoundaction89_is_active(const wsoundaction89_context *ctx)
{
    return ctx != 0 && ctx->active != 0U;
}

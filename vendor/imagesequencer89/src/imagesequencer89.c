#include "imagesequencer89.h"
#include <string.h>

#define IS89_ERR_NONE 0
#define IS89_ERR_ARGUMENT 1
#define IS89_ERR_CAPACITY 2
#define IS89_ERR_NOT_FOUND 3
#define IS89_ERR_LAYOUT 4

static void is89_zero(void *ptr, unsigned int size)
{
    unsigned char *p;
    unsigned int i;
    if (!ptr) return;
    p = (unsigned char *)ptr;
    for (i = 0U; i < size; ++i) p[i] = 0U;
}

static int is89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U || !src) return 0;
    i = 0U;
    while (src[i] != '\0' && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    if (src[i] != '\0') return 0;
    dst[i] = '\0';
    return 1;
}

static int is89_valid_sequence(const ImageSequencer89 *ctx, is89_id id)
{
    return ctx && id < ctx->sequence_count && ctx->sequences[id].used;
}

static int is89_valid_player(const ImageSequencer89 *ctx, is89_id id)
{
    return ctx && id < IS89_MAX_PLAYERS && ctx->players[id].used;
}

void is89_init(ImageSequencer89 *ctx)
{
    if (!ctx) return;
    is89_zero(ctx, (unsigned int)sizeof(*ctx));
}

is89_id is89_find_sequence(const ImageSequencer89 *ctx, const char *name)
{
    is89_id i;
    if (!ctx || !name) return IS89_INVALID_ID;
    for (i = 0U; i < ctx->sequence_count; ++i) {
        if (ctx->sequences[i].used && strcmp(ctx->sequences[i].name, name) == 0) return i;
    }
    return IS89_INVALID_ID;
}

is89_id is89_sequence_begin(ImageSequencer89 *ctx, const char *name)
{
    is89_id id;
    if (!ctx || !name || name[0] == '\0') return IS89_INVALID_ID;
    id = is89_find_sequence(ctx, name);
    if (id != IS89_INVALID_ID) return id;
    if (ctx->sequence_count >= IS89_MAX_SEQUENCES) {
        ctx->last_error = IS89_ERR_CAPACITY;
        return IS89_INVALID_ID;
    }
    id = ctx->sequence_count++;
    is89_zero(&ctx->sequences[id], (unsigned int)sizeof(ctx->sequences[id]));
    if (!is89_copy(ctx->sequences[id].name, IS89_NAME_CAP, name)) {
        --ctx->sequence_count;
        ctx->last_error = IS89_ERR_ARGUMENT;
        return IS89_INVALID_ID;
    }
    ctx->sequences[id].first_frame = ctx->frame_count;
    ctx->sequences[id].loop_mode = IS89_LOOP_NONE;
    ctx->sequences[id].used = 1U;
    return id;
}

int is89_sequence_set_loop(ImageSequencer89 *ctx, is89_id sequence_id, int loop_mode)
{
    if (!is89_valid_sequence(ctx, sequence_id)) return 0;
    if (loop_mode < IS89_LOOP_NONE || loop_mode > IS89_LOOP_HOLD) return 0;
    ctx->sequences[sequence_id].loop_mode = (is89_u8)loop_mode;
    return 1;
}

static is89_id is89_add_frame_common(ImageSequencer89 *ctx, is89_id sequence_id,
                                     const char *request, is89_u32 duration_ms)
{
    IS89_Sequence *seq;
    IS89_Frame *f;
    is89_id id;
    if (!is89_valid_sequence(ctx, sequence_id) || !request || request[0] == '\0') return IS89_INVALID_ID;
    seq = &ctx->sequences[sequence_id];
    if ((is89_u32)seq->first_frame + (is89_u32)seq->frame_count != (is89_u32)ctx->frame_count) {
        ctx->last_error = IS89_ERR_LAYOUT;
        return IS89_INVALID_ID;
    }
    if (ctx->frame_count >= IS89_MAX_FRAMES || seq->frame_count == 0xFFFFU) {
        ctx->last_error = IS89_ERR_CAPACITY;
        return IS89_INVALID_ID;
    }
    id = ctx->frame_count++;
    f = &ctx->frames[id];
    is89_zero(f, (unsigned int)sizeof(*f));
    if (!is89_copy(f->image_request, IS89_REQUEST_CAP, request)) {
        --ctx->frame_count;
        ctx->last_error = IS89_ERR_ARGUMENT;
        return IS89_INVALID_ID;
    }
    f->duration_ms = duration_ms ? duration_ms : 1U;
    f->scale_x_q16 = IS89_Q16_ONE;
    f->scale_y_q16 = IS89_Q16_ONE;
    ++seq->frame_count;
    return id;
}

is89_id is89_sequence_add_frame(ImageSequencer89 *ctx, is89_id sequence_id,
                                const char *image_request, is89_u32 duration_ms)
{
    return is89_add_frame_common(ctx, sequence_id, image_request, duration_ms);
}

is89_id is89_sequence_add_frame_rect(ImageSequencer89 *ctx, is89_id sequence_id,
                                     const char *image_request,
                                     is89_s32 x, is89_s32 y, is89_s32 w, is89_s32 h,
                                     is89_u32 duration_ms)
{
    is89_id id;
    id = is89_add_frame_common(ctx, sequence_id, image_request, duration_ms);
    if (id == IS89_INVALID_ID) return id;
    ctx->frames[id].source_x = x;
    ctx->frames[id].source_y = y;
    ctx->frames[id].source_w = w;
    ctx->frames[id].source_h = h;
    if (w > 0 && h > 0) ctx->frames[id].flags |= IS89_FRAME_SOURCE_RECT;
    return id;
}

int is89_frame_set_transform(ImageSequencer89 *ctx, is89_id frame_id,
                             is89_s32 offset_x_q16, is89_s32 offset_y_q16,
                             is89_s32 scale_x_q16, is89_s32 scale_y_q16)
{
    if (!ctx || frame_id >= ctx->frame_count) return 0;
    ctx->frames[frame_id].offset_x_q16 = offset_x_q16;
    ctx->frames[frame_id].offset_y_q16 = offset_y_q16;
    ctx->frames[frame_id].scale_x_q16 = scale_x_q16;
    ctx->frames[frame_id].scale_y_q16 = scale_y_q16;
    return 1;
}

int is89_frame_set_flags(ImageSequencer89 *ctx, is89_id frame_id, is89_u32 flags)
{
    if (!ctx || frame_id >= ctx->frame_count) return 0;
    ctx->frames[frame_id].flags = flags;
    return 1;
}

int is89_frame_set_user_tag(ImageSequencer89 *ctx, is89_id frame_id, is89_u32 user_tag)
{
    if (!ctx || frame_id >= ctx->frame_count) return 0;
    ctx->frames[frame_id].user_tag = user_tag;
    return 1;
}

is89_id is89_player_create(ImageSequencer89 *ctx)
{
    is89_id i;
    IS89_Player *p;
    if (!ctx) return IS89_INVALID_ID;
    for (i = 0U; i < IS89_MAX_PLAYERS; ++i) {
        if (!ctx->players[i].used) {
            p = &ctx->players[i];
            is89_zero(p, (unsigned int)sizeof(*p));
            p->used = 1U;
            p->sequence_id = IS89_INVALID_ID;
            p->speed_q16 = IS89_Q16_ONE;
            p->direction = 1;
            if (i >= ctx->player_count) ctx->player_count = (is89_id)(i + 1U);
            return i;
        }
    }
    ctx->last_error = IS89_ERR_CAPACITY;
    return IS89_INVALID_ID;
}

void is89_player_destroy(ImageSequencer89 *ctx, is89_id player_id)
{
    if (!is89_valid_player(ctx, player_id)) return;
    is89_zero(&ctx->players[player_id], (unsigned int)sizeof(ctx->players[player_id]));
}

int is89_player_play(ImageSequencer89 *ctx, is89_id player_id, is89_id sequence_id)
{
    IS89_Player *p;
    IS89_Sequence *s;
    if (!is89_valid_player(ctx, player_id) || !is89_valid_sequence(ctx, sequence_id)) return 0;
    s = &ctx->sequences[sequence_id];
    if (s->frame_count == 0U) return 0;
    p = &ctx->players[player_id];
    p->sequence_id = sequence_id;
    p->elapsed_ms = 0U;
    p->finished = 0U;
    p->playing = 1U;
    if (s->loop_mode == IS89_LOOP_REVERSE) {
        p->frame_pos = (is89_id)(s->frame_count - 1U);
        p->direction = -1;
    } else {
        p->frame_pos = 0U;
        p->direction = 1;
    }
    return 1;
}

void is89_player_stop(ImageSequencer89 *ctx, is89_id player_id)
{
    if (!is89_valid_player(ctx, player_id)) return;
    ctx->players[player_id].playing = 0U;
}

void is89_player_reset(ImageSequencer89 *ctx, is89_id player_id)
{
    IS89_Player *p;
    if (!is89_valid_player(ctx, player_id)) return;
    p = &ctx->players[player_id];
    p->frame_pos = 0U;
    p->elapsed_ms = 0U;
    p->direction = 1;
    p->finished = 0U;
}

void is89_player_set_speed_q16(ImageSequencer89 *ctx, is89_id player_id, is89_s32 speed_q16)
{
    if (!is89_valid_player(ctx, player_id)) return;
    if (speed_q16 > 16 * IS89_Q16_ONE) speed_q16 = 16 * IS89_Q16_ONE;
    if (speed_q16 < -16 * IS89_Q16_ONE) speed_q16 = -16 * IS89_Q16_ONE;
    ctx->players[player_id].speed_q16 = speed_q16;
}

static is89_u32 is89_scale_delta(is89_u32 delta_ms, is89_s32 speed_q16)
{
    is89_u32 speed;
    is89_u32 whole;
    is89_u32 frac;
    is89_u32 out;
    if (speed_q16 == 0) return 0U;
    if (delta_ms > 60000U) delta_ms = 60000U;
    speed = (is89_u32)(speed_q16 < 0 ? -speed_q16 : speed_q16);
    if (speed > 16U * (is89_u32)IS89_Q16_ONE) speed = 16U * (is89_u32)IS89_Q16_ONE;
    whole = speed >> 16;
    frac = speed & 0xFFFFU;
    out = delta_ms * whole;
    out += (delta_ms * frac + 32768U) >> 16;
    return out;
}

static void is89_advance_one(ImageSequencer89 *ctx, IS89_Player *p)
{
    const IS89_Sequence *s;
    int next;
    if (!ctx || !p || p->sequence_id == IS89_INVALID_ID) return;
    s = &ctx->sequences[p->sequence_id];
    next = (int)p->frame_pos + (int)p->direction;
    if (next >= 0 && next < (int)s->frame_count) {
        p->frame_pos = (is89_id)next;
        return;
    }
    if (s->loop_mode == IS89_LOOP_FORWARD) {
        p->frame_pos = 0U;
        p->direction = 1;
    } else if (s->loop_mode == IS89_LOOP_REVERSE) {
        p->frame_pos = (is89_id)(s->frame_count - 1U);
        p->direction = -1;
    } else if (s->loop_mode == IS89_LOOP_PINGPONG) {
        if (s->frame_count <= 1U) {
            p->frame_pos = 0U;
        } else if (p->direction > 0) {
            p->direction = -1;
            p->frame_pos = (is89_id)(s->frame_count - 2U);
        } else {
            p->direction = 1;
            p->frame_pos = 1U;
        }
    } else if (s->loop_mode == IS89_LOOP_HOLD) {
        p->frame_pos = (is89_id)(s->frame_count - 1U);
        p->playing = 0U;
        p->finished = 1U;
    } else {
        p->frame_pos = (is89_id)(p->direction > 0 ? s->frame_count - 1U : 0U);
        p->playing = 0U;
        p->finished = 1U;
    }
}

void is89_player_step(ImageSequencer89 *ctx, is89_id player_id, is89_u32 delta_ms)
{
    IS89_Player *p;
    const IS89_Sequence *s;
    const IS89_Frame *f;
    is89_u32 scaled;
    unsigned int guard;
    if (!is89_valid_player(ctx, player_id)) return;
    p = &ctx->players[player_id];
    if (!p->playing || p->sequence_id == IS89_INVALID_ID || p->speed_q16 == 0) return;
    s = &ctx->sequences[p->sequence_id];
    if (p->speed_q16 < 0) p->direction = -1;
    scaled = is89_scale_delta(delta_ms, p->speed_q16);
    p->elapsed_ms += scaled;
    guard = 0U;
    while (p->playing && guard < 65535U) {
        f = &ctx->frames[(is89_id)(s->first_frame + p->frame_pos)];
        if (p->elapsed_ms < f->duration_ms) break;
        p->elapsed_ms -= f->duration_ms;
        is89_advance_one(ctx, p);
        ++guard;
    }
}

const IS89_Frame *is89_player_current_frame(const ImageSequencer89 *ctx, is89_id player_id)
{
    const IS89_Player *p;
    const IS89_Sequence *s;
    if (!is89_valid_player(ctx, player_id)) return (const IS89_Frame *)0;
    p = &ctx->players[player_id];
    if (p->sequence_id == IS89_INVALID_ID || !is89_valid_sequence(ctx, p->sequence_id)) return (const IS89_Frame *)0;
    s = &ctx->sequences[p->sequence_id];
    if (p->frame_pos >= s->frame_count) return (const IS89_Frame *)0;
    return &ctx->frames[(is89_id)(s->first_frame + p->frame_pos)];
}

int is89_player_finished(const ImageSequencer89 *ctx, is89_id player_id)
{
    if (!is89_valid_player(ctx, player_id)) return 1;
    return ctx->players[player_id].finished ? 1 : 0;
}

const char *is89_error_string(int code)
{
    if (code == IS89_ERR_NONE) return "ok";
    if (code == IS89_ERR_ARGUMENT) return "invalid argument";
    if (code == IS89_ERR_CAPACITY) return "capacity exhausted";
    if (code == IS89_ERR_NOT_FOUND) return "not found";
    if (code == IS89_ERR_LAYOUT) return "sequence frames must remain contiguous";
    return "unknown error";
}

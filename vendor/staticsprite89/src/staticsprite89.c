#include "staticsprite89.h"

static void ss89_zero(void *ptr, unsigned int size)
{
    unsigned char *p;
    unsigned int i;
    if (!ptr) return;
    p = (unsigned char *)ptr;
    for (i = 0U; i < size; ++i) p[i] = 0U;
}

static int ss89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U || !src) return 0;
    i = 0U;
    while (src[i] != '\0') {
        if (i + 1U >= cap) {
            dst[0] = '\0';
            return 0;
        }
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
    return 1;
}

void ss89_init(SS89_StaticSprite *sprite)
{
    if (!sprite) return;
    ss89_zero(sprite, (unsigned int)sizeof(*sprite));
    sprite->scale_q16[0] = SS89_Q16_ONE;
    sprite->scale_q16[1] = SS89_Q16_ONE;
    sprite->tint_rgba[0] = 255U;
    sprite->tint_rgba[1] = 255U;
    sprite->tint_rgba[2] = 255U;
    sprite->tint_rgba[3] = 255U;
    sprite->visible = 1U;
}

int ss89_set_image(SS89_StaticSprite *sprite, const char *request)
{
    if (!sprite || !request || request[0] == '\0') {
        if (sprite) sprite->last_error = SS89_ERR_ARGUMENT;
        return 0;
    }
    if (!ss89_copy(sprite->image_request, SS89_REQUEST_CAP, request)) {
        sprite->last_error = SS89_ERR_REQUEST_TOO_LONG;
        return 0;
    }
    sprite->last_error = SS89_ERR_NONE;
    return 1;
}

void ss89_set_source_rect(SS89_StaticSprite *sprite,
                          ss89_s32 x, ss89_s32 y, ss89_s32 w, ss89_s32 h)
{
    if (!sprite) return;
    sprite->source.x = x;
    sprite->source.y = y;
    sprite->source.w = w;
    sprite->source.h = h;
    sprite->source_enabled = (ss89_u8)((w > 0 && h > 0) ? 1 : 0);
}

void ss89_clear_source_rect(SS89_StaticSprite *sprite)
{
    if (!sprite) return;
    ss89_zero(&sprite->source, (unsigned int)sizeof(sprite->source));
    sprite->source_enabled = 0U;
}

void ss89_set_position_q16(SS89_StaticSprite *sprite,
                           ss89_s32 x, ss89_s32 y, ss89_s32 z)
{
    if (!sprite) return;
    sprite->position_q16[0] = x;
    sprite->position_q16[1] = y;
    sprite->position_q16[2] = z;
}

void ss89_set_pivot_q16(SS89_StaticSprite *sprite, ss89_s32 x, ss89_s32 y)
{
    if (!sprite) return;
    sprite->pivot_q16[0] = x;
    sprite->pivot_q16[1] = y;
}

void ss89_set_scale_q16(SS89_StaticSprite *sprite, ss89_s32 x, ss89_s32 y)
{
    if (!sprite) return;
    sprite->scale_q16[0] = x;
    sprite->scale_q16[1] = y;
}

void ss89_set_tint_rgba8(SS89_StaticSprite *sprite,
                         ss89_u8 r, ss89_u8 g, ss89_u8 b, ss89_u8 a)
{
    if (!sprite) return;
    sprite->tint_rgba[0] = r;
    sprite->tint_rgba[1] = g;
    sprite->tint_rgba[2] = b;
    sprite->tint_rgba[3] = a;
}

void ss89_set_visible(SS89_StaticSprite *sprite, int visible)
{
    if (!sprite) return;
    sprite->visible = (ss89_u8)(visible ? 1 : 0);
}

void ss89_set_flags(SS89_StaticSprite *sprite, ss89_u32 flags)
{
    if (!sprite) return;
    sprite->flags = flags;
}

void ss89_set_user_tag(SS89_StaticSprite *sprite, ss89_u32 user_tag)
{
    if (!sprite) return;
    sprite->user_tag = user_tag;
}

int ss89_sample(const SS89_StaticSprite *sprite, SS89_Sample *out_sample)
{
    unsigned int i;
    if (!sprite || !out_sample || sprite->image_request[0] == '\0') return 0;
    out_sample->image_request = sprite->image_request;
    out_sample->source = sprite->source;
    for (i = 0U; i < 3U; ++i) out_sample->position_q16[i] = sprite->position_q16[i];
    for (i = 0U; i < 2U; ++i) {
        out_sample->pivot_q16[i] = sprite->pivot_q16[i];
        out_sample->scale_q16[i] = sprite->scale_q16[i];
    }
    for (i = 0U; i < 4U; ++i) out_sample->tint_rgba[i] = sprite->tint_rgba[i];
    out_sample->flags = sprite->flags;
    out_sample->user_tag = sprite->user_tag;
    out_sample->source_enabled = sprite->source_enabled;
    out_sample->visible = sprite->visible;
    return 1;
}

const char *ss89_error_string(int code)
{
    if (code == SS89_ERR_NONE) return "ok";
    if (code == SS89_ERR_ARGUMENT) return "invalid argument";
    if (code == SS89_ERR_REQUEST_TOO_LONG) return "image request too long";
    return "unknown error";
}

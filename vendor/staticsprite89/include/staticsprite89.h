#ifndef STATICSPRITE89_H
#define STATICSPRITE89_H

#include <limits.h>

#if UINT_MAX != 0xFFFFFFFFU
#error StaticSprite89 requires a 32-bit unsigned int target
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SS89_VERSION_MAJOR 0
#define SS89_VERSION_MINOR 1
#define SS89_VERSION_PATCH 0

#ifndef SS89_REQUEST_CAP
#define SS89_REQUEST_CAP 260
#endif

#define SS89_Q16_ONE 65536
#define SS89_FLAG_FLIP_X 0x00000001U
#define SS89_FLAG_FLIP_Y 0x00000002U
#define SS89_FLAG_USER0  0x00010000U

#define SS89_ERR_NONE 0
#define SS89_ERR_ARGUMENT 1
#define SS89_ERR_REQUEST_TOO_LONG 2

typedef signed int ss89_s32;
typedef unsigned int ss89_u32;
typedef unsigned char ss89_u8;

typedef struct SS89_Rect_s {
    ss89_s32 x;
    ss89_s32 y;
    ss89_s32 w;
    ss89_s32 h;
} SS89_Rect;

typedef struct SS89_StaticSprite_s {
    char image_request[SS89_REQUEST_CAP];
    SS89_Rect source;
    ss89_s32 position_q16[3];
    ss89_s32 pivot_q16[2];
    ss89_s32 scale_q16[2];
    ss89_u8 tint_rgba[4];
    ss89_u32 flags;
    ss89_u32 user_tag;
    ss89_u8 source_enabled;
    ss89_u8 visible;
    int last_error;
} SS89_StaticSprite;

typedef struct SS89_Sample_s {
    const char *image_request;
    SS89_Rect source;
    ss89_s32 position_q16[3];
    ss89_s32 pivot_q16[2];
    ss89_s32 scale_q16[2];
    ss89_u8 tint_rgba[4];
    ss89_u32 flags;
    ss89_u32 user_tag;
    ss89_u8 source_enabled;
    ss89_u8 visible;
} SS89_Sample;

void ss89_init(SS89_StaticSprite *sprite);
int ss89_set_image(SS89_StaticSprite *sprite, const char *request);
void ss89_set_source_rect(SS89_StaticSprite *sprite,
                          ss89_s32 x, ss89_s32 y, ss89_s32 w, ss89_s32 h);
void ss89_clear_source_rect(SS89_StaticSprite *sprite);
void ss89_set_position_q16(SS89_StaticSprite *sprite,
                           ss89_s32 x, ss89_s32 y, ss89_s32 z);
void ss89_set_pivot_q16(SS89_StaticSprite *sprite, ss89_s32 x, ss89_s32 y);
void ss89_set_scale_q16(SS89_StaticSprite *sprite, ss89_s32 x, ss89_s32 y);
void ss89_set_tint_rgba8(SS89_StaticSprite *sprite,
                         ss89_u8 r, ss89_u8 g, ss89_u8 b, ss89_u8 a);
void ss89_set_visible(SS89_StaticSprite *sprite, int visible);
void ss89_set_flags(SS89_StaticSprite *sprite, ss89_u32 flags);
void ss89_set_user_tag(SS89_StaticSprite *sprite, ss89_u32 user_tag);
int ss89_sample(const SS89_StaticSprite *sprite, SS89_Sample *out_sample);
const char *ss89_error_string(int code);

#ifdef __cplusplus
}
#endif

#endif

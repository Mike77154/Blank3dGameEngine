#ifndef RENLIST89_H
#define RENLIST89_H

#include <limits.h>

#if UINT_MAX != 0xFFFFFFFFU
#error RenList89 requires a 32-bit unsigned int target
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RL89_VERSION_MAJOR 0
#define RL89_VERSION_MINOR 1
#define RL89_VERSION_PATCH 0

#ifndef RL89_MAX_ANIMATIONS
#define RL89_MAX_ANIMATIONS 128
#endif
#ifndef RL89_MAX_FRAMES
#define RL89_MAX_FRAMES 2048
#endif
#ifndef RL89_MAX_PROPERTIES
#define RL89_MAX_PROPERTIES 1024
#endif
#ifndef RL89_NAME_CAP
#define RL89_NAME_CAP 64
#endif
#ifndef RL89_REQUEST_CAP
#define RL89_REQUEST_CAP 260
#endif
#ifndef RL89_KEY_CAP
#define RL89_KEY_CAP 48
#endif
#ifndef RL89_VALUE_CAP
#define RL89_VALUE_CAP 160
#endif

#define RL89_INVALID_ID 0xFFFFU
#define RL89_Q16_ONE 65536

#define RL89_LOOP_NONE 0
#define RL89_LOOP_FORWARD 1
#define RL89_LOOP_REVERSE 2
#define RL89_LOOP_PINGPONG 3
#define RL89_LOOP_HOLD 4

typedef unsigned short rl89_id;
typedef unsigned int rl89_u32;
typedef signed int rl89_s32;
typedef unsigned char rl89_u8;

typedef struct RL89_Frame_s {
    char request[RL89_REQUEST_CAP];
    rl89_u32 duration_ms;
    rl89_s32 offset_x_q16;
    rl89_s32 offset_y_q16;
    rl89_s32 scale_x_q16;
    rl89_s32 scale_y_q16;
    rl89_u32 flags;
    rl89_u32 user_tag;
} RL89_Frame;

typedef struct RL89_Property_s {
    char key[RL89_KEY_CAP];
    char value[RL89_VALUE_CAP];
} RL89_Property;

typedef struct RL89_Animation_s {
    char asset_name[RL89_NAME_CAP];
    char clip_name[RL89_NAME_CAP];
    rl89_id first_frame;
    rl89_id frame_count;
    rl89_id first_property;
    rl89_id property_count;
    rl89_u32 default_frame_ms;
    rl89_u8 loop_mode;
    rl89_u8 used;
} RL89_Animation;

typedef struct RenList89_s {
    RL89_Animation animations[RL89_MAX_ANIMATIONS];
    RL89_Frame frames[RL89_MAX_FRAMES];
    RL89_Property properties[RL89_MAX_PROPERTIES];
    rl89_id animation_count;
    rl89_id frame_count;
    rl89_id property_count;
    unsigned int error_line;
    int last_error;
} RenList89;

void rl89_init(RenList89 *doc);
int rl89_parse(RenList89 *doc, const char *text, rl89_u32 text_size);
rl89_id rl89_find_animation(const RenList89 *doc,
                            const char *asset_name, const char *clip_name);
const RL89_Animation *rl89_animation(const RenList89 *doc, rl89_id animation_id);
const RL89_Frame *rl89_frame(const RenList89 *doc, rl89_id animation_id, rl89_id frame_pos);
const char *rl89_property(const RenList89 *doc, rl89_id animation_id, const char *key);
const char *rl89_error_string(int code);

#ifdef __cplusplus
}
#endif

#endif

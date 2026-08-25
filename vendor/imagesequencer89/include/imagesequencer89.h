#ifndef IMAGESEQUENCER89_H
#define IMAGESEQUENCER89_H

#include <limits.h>

#if UINT_MAX != 0xFFFFFFFFU
#error ImageSequencer89 requires a 32-bit unsigned int target
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IS89_VERSION_MAJOR 0
#define IS89_VERSION_MINOR 1
#define IS89_VERSION_PATCH 0

#ifndef IS89_MAX_SEQUENCES
#define IS89_MAX_SEQUENCES 128
#endif
#ifndef IS89_MAX_FRAMES
#define IS89_MAX_FRAMES 2048
#endif
#ifndef IS89_MAX_PLAYERS
#define IS89_MAX_PLAYERS 128
#endif
#ifndef IS89_NAME_CAP
#define IS89_NAME_CAP 64
#endif
#ifndef IS89_REQUEST_CAP
#define IS89_REQUEST_CAP 260
#endif

#define IS89_Q16_ONE 65536
#define IS89_INVALID_ID 0xFFFFU

#define IS89_LOOP_NONE 0
#define IS89_LOOP_FORWARD 1
#define IS89_LOOP_REVERSE 2
#define IS89_LOOP_PINGPONG 3
#define IS89_LOOP_HOLD 4

#define IS89_FRAME_SOURCE_RECT 0x00000001U
#define IS89_FRAME_FLIP_X      0x00000002U
#define IS89_FRAME_FLIP_Y      0x00000004U

typedef unsigned short is89_id;
typedef unsigned int is89_u32;
typedef signed int is89_s32;
typedef unsigned char is89_u8;

typedef struct IS89_Frame_s {
    char image_request[IS89_REQUEST_CAP];
    is89_s32 source_x;
    is89_s32 source_y;
    is89_s32 source_w;
    is89_s32 source_h;
    is89_s32 offset_x_q16;
    is89_s32 offset_y_q16;
    is89_s32 scale_x_q16;
    is89_s32 scale_y_q16;
    is89_u32 duration_ms;
    is89_u32 flags;
    is89_u32 user_tag;
} IS89_Frame;

typedef struct IS89_Sequence_s {
    char name[IS89_NAME_CAP];
    is89_id first_frame;
    is89_id frame_count;
    is89_u8 loop_mode;
    is89_u8 used;
} IS89_Sequence;

typedef struct IS89_Player_s {
    is89_id sequence_id;
    is89_id frame_pos;
    is89_u32 elapsed_ms;
    is89_s32 speed_q16;
    signed char direction;
    is89_u8 playing;
    is89_u8 finished;
    is89_u8 used;
} IS89_Player;

typedef struct ImageSequencer89_s {
    IS89_Sequence sequences[IS89_MAX_SEQUENCES];
    IS89_Frame frames[IS89_MAX_FRAMES];
    IS89_Player players[IS89_MAX_PLAYERS];
    is89_id sequence_count;
    is89_id frame_count;
    is89_id player_count;
    int last_error;
} ImageSequencer89;

void is89_init(ImageSequencer89 *ctx);
is89_id is89_sequence_begin(ImageSequencer89 *ctx, const char *name);
int is89_sequence_set_loop(ImageSequencer89 *ctx, is89_id sequence_id, int loop_mode);
is89_id is89_sequence_add_frame(ImageSequencer89 *ctx, is89_id sequence_id,
                                const char *image_request, is89_u32 duration_ms);
is89_id is89_sequence_add_frame_rect(ImageSequencer89 *ctx, is89_id sequence_id,
                                     const char *image_request,
                                     is89_s32 x, is89_s32 y, is89_s32 w, is89_s32 h,
                                     is89_u32 duration_ms);
int is89_frame_set_transform(ImageSequencer89 *ctx, is89_id frame_id,
                             is89_s32 offset_x_q16, is89_s32 offset_y_q16,
                             is89_s32 scale_x_q16, is89_s32 scale_y_q16);
int is89_frame_set_flags(ImageSequencer89 *ctx, is89_id frame_id, is89_u32 flags);
int is89_frame_set_user_tag(ImageSequencer89 *ctx, is89_id frame_id, is89_u32 user_tag);
is89_id is89_find_sequence(const ImageSequencer89 *ctx, const char *name);

is89_id is89_player_create(ImageSequencer89 *ctx);
void is89_player_destroy(ImageSequencer89 *ctx, is89_id player_id);
int is89_player_play(ImageSequencer89 *ctx, is89_id player_id, is89_id sequence_id);
void is89_player_stop(ImageSequencer89 *ctx, is89_id player_id);
void is89_player_reset(ImageSequencer89 *ctx, is89_id player_id);
void is89_player_set_speed_q16(ImageSequencer89 *ctx, is89_id player_id, is89_s32 speed_q16);
void is89_player_step(ImageSequencer89 *ctx, is89_id player_id, is89_u32 delta_ms);
const IS89_Frame *is89_player_current_frame(const ImageSequencer89 *ctx, is89_id player_id);
int is89_player_finished(const ImageSequencer89 *ctx, is89_id player_id);
const char *is89_error_string(int code);

#ifdef __cplusplus
}
#endif

#endif

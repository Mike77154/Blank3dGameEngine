#ifndef SPRITEASSET89_H
#define SPRITEASSET89_H

#include <limits.h>

#if UINT_MAX != 0xFFFFFFFFU
#error SpriteAsset89 requires a 32-bit unsigned int target
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SA89_VERSION_MAJOR 0
#define SA89_VERSION_MINOR 1
#define SA89_VERSION_PATCH 2

#ifndef SA89_MAX_SOURCES
#define SA89_MAX_SOURCES 256
#endif
#ifndef SA89_MAX_ASSETS
#define SA89_MAX_ASSETS 128
#endif
#ifndef SA89_MAX_FRAMES
#define SA89_MAX_FRAMES 2048
#endif
#ifndef SA89_MAX_CLIPS
#define SA89_MAX_CLIPS 512
#endif
#ifndef SA89_MAX_PLAYERS
#define SA89_MAX_PLAYERS 256
#endif
#ifndef SA89_NAME_CAP
#define SA89_NAME_CAP 64
#endif
#ifndef SA89_PATH_CAP
#define SA89_PATH_CAP 260
#endif

#define SA89_INVALID_ID 0xFFFFU
#define SA89_Q16_ONE 65536

#define SA89_PROVIDER_ERROR   (-1)
#define SA89_PROVIDER_DECLINE 0
#define SA89_PROVIDER_HANDLED 1

#define SA89_LOOP_NONE     0
#define SA89_LOOP_FORWARD  1
#define SA89_LOOP_REVERSE  2
#define SA89_LOOP_PINGPONG 3

#define SA89_DRAW_FLIP_X 1U
#define SA89_DRAW_FLIP_Y 2U

typedef unsigned short sa89_id;
typedef unsigned int sa89_u32;
typedef signed int sa89_s32;
typedef unsigned char sa89_u8;

typedef struct SA89_ImageView_s {
    const sa89_u8 *pixels;
    sa89_u32 width;
    sa89_u32 height;
    sa89_u32 stride;
} SA89_ImageView;

typedef int (*sa89_image_acquire_fn)(void *user,
                                     const char *path,
                                     sa89_u32 *out_handle,
                                     sa89_u32 *out_width,
                                     sa89_u32 *out_height,
                                     sa89_u32 *out_frame_count);
typedef int (*sa89_image_get_frame_fn)(void *user,
                                       sa89_u32 handle,
                                       sa89_u32 frame_index,
                                       SA89_ImageView *out_view);
typedef void (*sa89_image_release_fn)(void *user, sa89_u32 handle);

typedef struct SA89_ImageProvider_s {
    sa89_image_acquire_fn acquire;
    sa89_image_get_frame_fn get_frame;
    sa89_image_release_fn release;
    void *user;
} SA89_ImageProvider;

typedef int (*sa89_render_draw_fn)(void *user,
                                   const SA89_ImageView *view,
                                   sa89_s32 src_x,
                                   sa89_s32 src_y,
                                   sa89_s32 src_w,
                                   sa89_s32 src_h,
                                   sa89_s32 dst_x,
                                   sa89_s32 dst_y,
                                   sa89_u32 flags);

typedef struct SA89_RenderProvider_s {
    sa89_render_draw_fn draw;
    void *user;
} SA89_RenderProvider;

typedef struct SA89_Source_s {
    char path[SA89_PATH_CAP];
    sa89_u32 image_handle;
    sa89_u32 width;
    sa89_u32 height;
    sa89_u32 embedded_frames;
    sa89_u8 acquired;
    sa89_u8 used;
} SA89_Source;

typedef struct SA89_Frame_s {
    sa89_id source_id;
    sa89_u32 source_frame;
    sa89_s32 x;
    sa89_s32 y;
    sa89_s32 w;
    sa89_s32 h;
    sa89_s32 offset_x;
    sa89_s32 offset_y;
    sa89_u32 duration_ms;
} SA89_Frame;

typedef struct SA89_Clip_s {
    char name[SA89_NAME_CAP];
    sa89_id asset_id;
    sa89_id first_frame;
    sa89_id frame_count;
    sa89_u8 loop_mode;
    sa89_u8 used;
} SA89_Clip;

typedef struct SA89_Asset_s {
    char name[SA89_NAME_CAP];
    sa89_s32 origin_x;
    sa89_s32 origin_y;
    sa89_id first_clip;
    sa89_id clip_count;
    sa89_u8 used;
} SA89_Asset;

typedef struct SA89_Player_s {
    sa89_id asset_id;
    sa89_id clip_id;
    sa89_id frame_pos;
    sa89_u32 elapsed_ms;
    sa89_s32 speed_q16;
    sa89_s32 x;
    sa89_s32 y;
    sa89_u32 draw_flags;
    signed char direction;
    sa89_u8 playing;
    sa89_u8 visible;
    sa89_u8 used;
} SA89_Player;

typedef struct SpriteAsset89_s {
    SA89_Source sources[SA89_MAX_SOURCES];
    SA89_Asset assets[SA89_MAX_ASSETS];
    SA89_Frame frames[SA89_MAX_FRAMES];
    SA89_Clip clips[SA89_MAX_CLIPS];
    SA89_Player players[SA89_MAX_PLAYERS];
    sa89_id source_count;
    sa89_id asset_count;
    sa89_id frame_count;
    sa89_id clip_count;
    sa89_id player_count;
    SA89_ImageProvider image_provider;
    SA89_RenderProvider render_provider;
    int last_error;
} SpriteAsset89;

void sa89_init(SpriteAsset89 *ctx);
void sa89_reset(SpriteAsset89 *ctx);
void sa89_set_image_provider(SpriteAsset89 *ctx, const SA89_ImageProvider *provider);
void sa89_set_render_provider(SpriteAsset89 *ctx, const SA89_RenderProvider *provider);

sa89_id sa89_add_source(SpriteAsset89 *ctx, const char *path);
sa89_id sa89_find_source(const SpriteAsset89 *ctx, const char *path);
sa89_id sa89_add_asset(SpriteAsset89 *ctx, const char *name, sa89_s32 origin_x, sa89_s32 origin_y);
sa89_id sa89_find_asset(const SpriteAsset89 *ctx, const char *name);
sa89_id sa89_add_frame(SpriteAsset89 *ctx, sa89_id source_id, sa89_u32 source_frame,
                       sa89_s32 x, sa89_s32 y, sa89_s32 w, sa89_s32 h,
                       sa89_s32 offset_x, sa89_s32 offset_y, sa89_u32 duration_ms);
sa89_id sa89_add_clip(SpriteAsset89 *ctx, sa89_id asset_id, const char *name,
                      sa89_id first_frame, sa89_id frame_count, int loop_mode);
sa89_id sa89_find_clip(const SpriteAsset89 *ctx, sa89_id asset_id, const char *name);
int sa89_define_static(SpriteAsset89 *ctx, const char *asset_name, const char *path,
                       sa89_u32 duration_ms);
int sa89_define_grid(SpriteAsset89 *ctx, const char *asset_name, const char *clip_name,
                     const char *path, sa89_s32 frame_w, sa89_s32 frame_h,
                     sa89_id frame_count, sa89_id columns, sa89_u32 duration_ms,
                     int loop_mode);
int sa89_define_gamemaker_strip(SpriteAsset89 *ctx, const char *asset_name, const char *clip_name,
                                const char *path, sa89_id frame_count,
                                sa89_u32 duration_ms, int loop_mode);

sa89_id sa89_player_create(SpriteAsset89 *ctx);
void sa89_player_destroy(SpriteAsset89 *ctx, sa89_id player_id);
int sa89_player_set_asset(SpriteAsset89 *ctx, sa89_id player_id, sa89_id asset_id);
int sa89_player_play(SpriteAsset89 *ctx, sa89_id player_id, sa89_id asset_id, const char *clip_name);
void sa89_player_stop(SpriteAsset89 *ctx, sa89_id player_id);
void sa89_player_show(SpriteAsset89 *ctx, sa89_id player_id, int visible);
void sa89_player_set_position(SpriteAsset89 *ctx, sa89_id player_id, sa89_s32 x, sa89_s32 y);
void sa89_player_set_speed_q16(SpriteAsset89 *ctx, sa89_id player_id, sa89_s32 speed_q16);
int sa89_player_set_frame(SpriteAsset89 *ctx, sa89_id player_id, sa89_id frame_pos);
void sa89_player_step(SpriteAsset89 *ctx, sa89_id player_id, sa89_u32 delta_ms);
int sa89_player_render(SpriteAsset89 *ctx, sa89_id player_id);

const SA89_Frame *sa89_player_current_frame(const SpriteAsset89 *ctx, sa89_id player_id);
const char *sa89_error_string(int code);

#ifdef __cplusplus
}
#endif

#endif

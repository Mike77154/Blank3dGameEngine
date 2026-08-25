#ifndef GMSPRITESTRIP89_H
#define GMSPRITESTRIP89_H

#include "../vendor/SpriteAsset89_v0.1.2/include/spriteasset89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GMSS89_VERSION_MAJOR 0
#define GMSS89_VERSION_MINOR 1
#define GMSS89_VERSION_PATCH 0

#define GMSS89_INVALID_ID SA89_INVALID_ID
#define GMSS89_Q16_ONE SA89_Q16_ONE

#define GMSS89_LOOP_NONE SA89_LOOP_NONE
#define GMSS89_LOOP_FORWARD SA89_LOOP_FORWARD
#define GMSS89_LOOP_REVERSE SA89_LOOP_REVERSE
#define GMSS89_LOOP_PINGPONG SA89_LOOP_PINGPONG

#define GMSS89_DRAW_FLIP_X SA89_DRAW_FLIP_X
#define GMSS89_DRAW_FLIP_Y SA89_DRAW_FLIP_Y

typedef SpriteAsset89 GMSpritestrip89;
typedef sa89_id gmss89_id;
typedef sa89_u32 gmss89_u32;
typedef sa89_s32 gmss89_s32;
typedef sa89_u8 gmss89_u8;
typedef SA89_ImageView GMSS89_ImageView;
typedef SA89_ImageProvider GMSS89_ImageProvider;
typedef SA89_RenderProvider GMSS89_RenderProvider;

void gmss89_init(GMSpritestrip89 *ctx);
void gmss89_reset(GMSpritestrip89 *ctx);
void gmss89_set_image_provider(GMSpritestrip89 *ctx, const GMSS89_ImageProvider *provider);
void gmss89_set_render_provider(GMSpritestrip89 *ctx, const GMSS89_RenderProvider *provider);

/* Explicit strip definition: path points to a horizontal strip image, frame_count is the number of subimages. */
int gmss89_define_strip(GMSpritestrip89 *ctx,
                        const char *asset_name,
                        const char *clip_name,
                        const char *path,
                        gmss89_id frame_count,
                        gmss89_u32 duration_ms,
                        int loop_mode);

/* Auto strip definition: derives frame_count from a GameMaker-style _stripN suffix in either path or asset_name. */
int gmss89_define_strip_auto(GMSpritestrip89 *ctx,
                             const char *asset_name,
                             const char *clip_name,
                             const char *path,
                             gmss89_u32 duration_ms,
                             int loop_mode);

/* Convenience: derive cleaned asset name and frame_count from a GameMaker-style filename such as spr_x_walk_strip14.png. */
int gmss89_define_strip_from_path(GMSpritestrip89 *ctx,
                                  const char *path,
                                  const char *clip_name,
                                  gmss89_u32 duration_ms,
                                  int loop_mode);

/* Utilities for GameMaker-like naming and metadata. */
int gmss89_parse_strip_count(const char *text, gmss89_id *out_count);
int gmss89_make_clean_name(const char *text, char *out_name, unsigned int out_cap);
gmss89_id gmss89_asset_subimage_count(const GMSpritestrip89 *ctx,
                                      const char *asset_name,
                                      const char *clip_name);

/* Player façade with GameMaker-ish semantics. */
gmss89_id gmss89_player_create(GMSpritestrip89 *ctx);
void gmss89_player_destroy(GMSpritestrip89 *ctx, gmss89_id player_id);
int gmss89_player_play(GMSpritestrip89 *ctx, gmss89_id player_id,
                       const char *asset_name, const char *clip_name);
void gmss89_player_set_position(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_s32 x, gmss89_s32 y);
void gmss89_player_show(GMSpritestrip89 *ctx, gmss89_id player_id, int visible);
void gmss89_player_set_image_speed_q16(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_s32 speed_q16);
int gmss89_player_set_subimage(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_id subimage);
gmss89_id gmss89_player_get_subimage(const GMSpritestrip89 *ctx, gmss89_id player_id);
void gmss89_player_step(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_u32 delta_ms);
int gmss89_player_render(GMSpritestrip89 *ctx, gmss89_id player_id);

#ifdef __cplusplus
}
#endif

#endif

#ifndef SPRITEPLANE89_H
#define SPRITEPLANE89_H

/* spriteplane89 v2 - C89 fixed-point 3D sprite planes / billboards.
   CC0/public-domain style. No malloc/free/realloc, no heap, no float/double.
   Renderer agnostic: emits vertices/indices/material packets/point commands.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define SP89_VERSION_MAJOR 2
#define SP89_VERSION_MINOR 0

#ifndef SP89_MAX_SPRITES
#define SP89_MAX_SPRITES 256
#endif

#ifndef SP89_MAX_ATLAS_FRAMES
#define SP89_MAX_ATLAS_FRAMES 1024
#endif

#ifndef SP89_MAX_EMIT_VERTS
#define SP89_MAX_EMIT_VERTS 8192
#endif

#ifndef SP89_MAX_EMIT_INDICES
#define SP89_MAX_EMIT_INDICES 16384
#endif

#ifndef SP89_MAX_EMIT_PACKETS
#define SP89_MAX_EMIT_PACKETS 2048
#endif

#ifndef SP89_MAX_VECTOR_CMDS
#define SP89_MAX_VECTOR_CMDS 1024
#endif

#ifndef SP89_MAX_TEXT_CMDS
#define SP89_MAX_TEXT_CMDS 128
#endif

#ifndef SP89_MAX_TEXT_CHARS
#define SP89_MAX_TEXT_CHARS 96
#endif

#ifndef SP89_MAX_FONTS
#define SP89_MAX_FONTS 8
#endif

#ifndef SP89_MAX_GLYPHS_PER_FONT
#define SP89_MAX_GLYPHS_PER_FONT 128
#endif

#ifndef SP89_MAX_PANEL_CMDS
#define SP89_MAX_PANEL_CMDS 128
#endif

#ifndef SP89_MAX_SHADOW_CMDS
#define SP89_MAX_SHADOW_CMDS 128
#endif

#ifndef SP89_MAX_POINT_CMDS
#define SP89_MAX_POINT_CMDS 1024
#endif

#define SP89_FX_SHIFT 16
#define SP89_FX_ONE   65536
#define SP89_FX_HALF  32768
#define SP89_FX_QUARTER 16384

#define SP89_OK 0
#define SP89_ERR_FULL -1
#define SP89_ERR_BADARG -2
#define SP89_ERR_RANGE -3
#define SP89_ERR_CULLED -4

/* Sprite / vertex flags */
#define SP89_FLAG_VISIBLE        0x0001u
#define SP89_FLAG_ADDITIVE       0x0002u
#define SP89_FLAG_ALPHA_BLEND    0x0004u
#define SP89_FLAG_ALPHA_TEST     0x0008u
#define SP89_FLAG_DEPTH_TEST     0x0010u
#define SP89_FLAG_DEPTH_WRITE    0x0020u
#define SP89_FLAG_NO_SORT        0x0040u
#define SP89_FLAG_FLIP_U         0x0080u
#define SP89_FLAG_FLIP_V         0x0100u
#define SP89_FLAG_VECTOR_LAYER   0x0200u
#define SP89_FLAG_RASTER_LAYER   0x0400u
#define SP89_FLAG_OCCLUSION_BIAS 0x0800u
#define SP89_FLAG_TEXT_LAYER     0x1000u
#define SP89_FLAG_PANEL_LAYER    0x2000u
#define SP89_FLAG_SHADOW_LAYER   0x4000u
#define SP89_FLAG_NO_RASTER      0x8000u

/* Extended flags kept outside the 16-bit vertex flags. */
#define SP89_XFLAG_POINT_COMMAND   0x00000001u
#define SP89_XFLAG_SOFT_PARTICLE   0x00000002u
#define SP89_XFLAG_DISTANCE_FADE   0x00000004u
#define SP89_XFLAG_CLAMP_TO_GROUND 0x00000008u
#define SP89_XFLAG_HOOK_LOCKED     0x00000010u

/* Billboard modes */
#define SP89_BILLBOARD_FIXED          0 /* uses sprite axis_right / axis_up */
#define SP89_BILLBOARD_VIEW_ALIGNED   1 /* camera right/up; view-plane aligned */
#define SP89_BILLBOARD_CAMERA_FACING  2 /* normal points to camera position */
#define SP89_BILLBOARD_Y_AXIS         3 /* rotates around world Y; Doom/tree style */
#define SP89_BILLBOARD_AXIS_LOCKED    4 /* rotates around custom locked axis */
#define SP89_BILLBOARD_SCREEN_FIXED   5 /* camera right/up, no world-roll if you pass stable cam */
#define SP89_BILLBOARD_VELOCITY       6 /* long sprite aligned to velocity vector */
#define SP89_BILLBOARD_SURFACE        7 /* uses surface_right/surface_up */
#define SP89_BILLBOARD_CROSSED_Y      8 /* two crossed quads, cheap grass/fire/tree volume */

/* Frame selection modes */
#define SP89_FRAME_STATIC       0
#define SP89_FRAME_ANIM_LOOP    1
#define SP89_FRAME_ANIM_ONCE    2
#define SP89_FRAME_VIEW_8       3
#define SP89_FRAME_VIEW_16      4
#define SP89_FRAME_ANIM_VIEW_8  5
#define SP89_FRAME_ANIM_VIEW_16 6

/* Anchor modes */
#define SP89_ANCHOR_CENTER        0
#define SP89_ANCHOR_BOTTOM_CENTER 1
#define SP89_ANCHOR_TOP_CENTER    2
#define SP89_ANCHOR_FEET_90       3 /* bottom center with small forward foot bias */
#define SP89_ANCHOR_CUSTOM        4
#define SP89_ANCHOR_FRAME_PIVOT   5

/* Render queues */
#define SP89_QUEUE_OPAQUE       0
#define SP89_QUEUE_ALPHA_TEST   1
#define SP89_QUEUE_TRANSPARENT  2
#define SP89_QUEUE_ADDITIVE     3
#define SP89_QUEUE_MULTIPLY     4
#define SP89_QUEUE_OVERLAY      5

/* Blend hints */
#define SP89_BLEND_NONE         0
#define SP89_BLEND_ALPHA        1
#define SP89_BLEND_PREMULT      2
#define SP89_BLEND_ADDITIVE     3
#define SP89_BLEND_SOFT_ADD     4
#define SP89_BLEND_MULTIPLY     5

/* Output primitive/material families */
#define SP89_MAT_RASTER 0
#define SP89_MAT_VECTOR 1
#define SP89_MAT_TEXT   2
#define SP89_MAT_PANEL  3
#define SP89_MAT_SHADOW 4

/* Vector commands */
#define SP89_VCMD_RECT   1
#define SP89_VCMD_LINE   2
#define SP89_VCMD_TRI    3
#define SP89_VCMD_CROSS  4
#define SP89_VCMD_CIRCLE 5

/* Text alignment */
#define SP89_TEXT_LEFT   0
#define SP89_TEXT_CENTER 1
#define SP89_TEXT_RIGHT  2

/* 16.16 fixed-point number */
typedef int sprpl89_fx;
typedef unsigned char sprpl89_u8;
typedef unsigned short sprpl89_u16;
typedef unsigned int sprpl89_u32;

typedef struct sprpl89_vec2_s {
    sprpl89_fx x;
    sprpl89_fx y;
} sprpl89_vec2;

typedef struct sprpl89_vec3_s {
    sprpl89_fx x;
    sprpl89_fx y;
    sprpl89_fx z;
} sprpl89_vec3;

typedef struct sprpl89_color_s {
    sprpl89_u8 r, g, b, a;
} sprpl89_color;

typedef struct sprpl89_atlas_frame_s {
    sprpl89_fx u0, v0, u1, v1;       /* normalized atlas UV in 16.16 */
    sprpl89_fx pivot_x, pivot_y;      /* 0..1 */
    sprpl89_fx logical_w, logical_h;  /* size hint, optional */
    sprpl89_u16 page_id;
    sprpl89_u16 user_id;
} sprpl89_atlas_frame;

typedef struct sprpl89_camera_s {
    sprpl89_vec3 pos;
    sprpl89_vec3 right;
    sprpl89_vec3 up;
    sprpl89_vec3 forward;
    sprpl89_vec3 world_up;
} sprpl89_camera;

typedef struct sprpl89_sprite_s {
    sprpl89_u16 active;
    sprpl89_u16 flags;
    sprpl89_u32 xflags;
    sprpl89_u16 billboard_mode;
    sprpl89_u16 frame_mode;
    sprpl89_u16 anchor_mode;
    sprpl89_u16 render_queue;
    sprpl89_u16 blend_mode;
    sprpl89_u16 sort_layer;
    sprpl89_u16 sort_order;

    sprpl89_vec3 pos;
    sprpl89_vec3 axis_right;
    sprpl89_vec3 axis_up;
    sprpl89_vec3 axis_lock;
    sprpl89_vec3 velocity;
    sprpl89_vec3 surface_right;
    sprpl89_vec3 surface_up;

    sprpl89_fx width;
    sprpl89_fx height;
    sprpl89_fx scale_x;
    sprpl89_fx scale_y;
    sprpl89_fx offset_x;
    sprpl89_fx offset_y;
    sprpl89_fx offset_z;
    sprpl89_fx depth_bias;

    sprpl89_fx cull_distance;       /* 0 = no distance cull */
    sprpl89_fx fade_near_distance;  /* optional hook/render hint */
    sprpl89_fx fade_far_distance;   /* optional hook/render hint */

    /* Facing of the entity in yaw units [0..65535], for Doom-like 8/16 views. */
    sprpl89_u16 yaw_u16;
    sprpl89_u16 base_frame;
    sprpl89_u16 frame_count;
    sprpl89_u16 view_count;       /* 1,8,16 */
    sprpl89_u16 anim_fps_fx8;     /* frames per second in 8.8 fixed */
    sprpl89_u16 anim_phase_fx8;
    sprpl89_u16 texture_page;
    sprpl89_u16 material_id;
    sprpl89_color color;

    sprpl89_fx custom_anchor_x; /* 0..1 */
    sprpl89_fx custom_anchor_y; /* 0..1 */

    sprpl89_u32 user0;
    sprpl89_u32 user1;
} sprpl89_sprite;

typedef struct sprpl89_vertex_s {
    sprpl89_vec3 pos;
    sprpl89_fx u, v;
    sprpl89_color color;
    sprpl89_u16 material_id;
    sprpl89_u16 page_id;
    sprpl89_u16 flags;
    sprpl89_u16 render_queue;
    sprpl89_u16 blend_mode;
} sprpl89_vertex;

typedef struct sprpl89_index_s {
    unsigned short a, b, c;
} sprpl89_tri;

typedef struct sprpl89_packet_s {
    int first_tri;
    int tri_count;
    sprpl89_u16 material_id;
    sprpl89_u16 page_id;
    sprpl89_u16 flags;
    sprpl89_u16 render_queue;
    sprpl89_u16 blend_mode;
    sprpl89_u16 owner_sprite;
    sprpl89_u16 primitive_family;
} sprpl89_packet;

typedef struct sprpl89_point_cmd_s {
    sprpl89_u16 active;
    sprpl89_vec3 pos;
    sprpl89_fx size_x;
    sprpl89_fx size_y;
    sprpl89_fx u0, v0, u1, v1;
    sprpl89_color color;
    sprpl89_u16 material_id;
    sprpl89_u16 page_id;
    sprpl89_u16 flags;
    sprpl89_u16 render_queue;
    sprpl89_u16 blend_mode;
    sprpl89_u16 owner_sprite;
} sprpl89_point_cmd;

typedef struct sprpl89_emit_s {
    sprpl89_vertex verts[SP89_MAX_EMIT_VERTS];
    sprpl89_tri tris[SP89_MAX_EMIT_INDICES / 3];
    sprpl89_packet packets[SP89_MAX_EMIT_PACKETS];
    sprpl89_point_cmd points[SP89_MAX_POINT_CMDS];
    int vert_count;
    int tri_count;
    int packet_count;
    int point_count;
    int dropped_sprites;
    int dropped_vector_cmds;
    int dropped_text_cmds;
    int dropped_panel_cmds;
    int dropped_shadow_cmds;
    int dropped_point_cmds;
    int culled_sprites;
} sprpl89_emit;

typedef struct sprpl89_sort_item_s {
    sprpl89_u16 sprite_index;
    int depth_key; /* larger = farther */
    sprpl89_u16 material_id;
    sprpl89_u16 texture_page;
    sprpl89_u16 render_queue;
    sprpl89_u16 sort_layer;
    sprpl89_u16 sort_order;
} sprpl89_sort_item;

typedef struct sprpl89_vector_cmd_s {
    sprpl89_u16 active;
    sprpl89_u16 sprite_index;
    sprpl89_u16 kind;
    sprpl89_u16 flags;
    sprpl89_fx x0, y0, x1, y1, x2, y2;
    sprpl89_fx thickness;
    sprpl89_color color;
    sprpl89_u16 material_id;
} sprpl89_vector_cmd;

typedef struct sprpl89_glyph_s {
    sprpl89_u16 active;
    sprpl89_u16 codepoint;
    sprpl89_u16 frame_id;
    sprpl89_fx advance_x;
    sprpl89_fx bearing_x;
    sprpl89_fx bearing_y;
    sprpl89_fx width;
    sprpl89_fx height;
} sprpl89_glyph;

typedef struct sprpl89_font_s {
    sprpl89_u16 active;
    sprpl89_u16 line_height;
    sprpl89_u16 space_advance;
    sprpl89_u16 missing_glyph;
    sprpl89_u16 material_id;
    sprpl89_glyph glyphs[SP89_MAX_GLYPHS_PER_FONT];
} sprpl89_font;

typedef struct sprpl89_text_cmd_s {
    sprpl89_u16 active;
    sprpl89_u16 sprite_index;
    sprpl89_u16 font_id;
    sprpl89_u16 flags;
    sprpl89_fx x;
    sprpl89_fx y;
    sprpl89_fx scale_x;
    sprpl89_fx scale_y;
    sprpl89_fx max_width;
    sprpl89_u16 align;
    sprpl89_color color;
    sprpl89_u16 material_id;
    char text[SP89_MAX_TEXT_CHARS];
} sprpl89_text_cmd;

typedef struct sprpl89_panel_cmd_s {
    sprpl89_u16 active;
    sprpl89_u16 sprite_index;
    sprpl89_u16 frame_id;
    sprpl89_u16 flags;
    sprpl89_fx x0, y0, x1, y1;
    sprpl89_fx border_l, border_t, border_r, border_b;
    sprpl89_color color;
    sprpl89_u16 material_id;
} sprpl89_panel_cmd;

typedef struct sprpl89_shadow_cmd_s {
    sprpl89_u16 active;
    sprpl89_u16 sprite_index;
    sprpl89_u16 frame_id;
    sprpl89_u16 flags;
    sprpl89_fx radius_x;
    sprpl89_fx radius_z;
    sprpl89_fx y_offset;
    sprpl89_color color;
    sprpl89_u16 material_id;
} sprpl89_shadow_cmd;

struct sprpl89_ctx_s;

typedef int (*sprpl89_hook_cull_sprite_fn)(struct sprpl89_ctx_s *ctx, int sprite_index, const sprpl89_sprite *sprite, void *user);
typedef int (*sprpl89_hook_resolve_frame_fn)(struct sprpl89_ctx_s *ctx, int sprite_index, const sprpl89_sprite *sprite, sprpl89_u16 default_frame, void *user);
typedef void (*sprpl89_hook_mutate_sprite_fn)(struct sprpl89_ctx_s *ctx, int sprite_index, sprpl89_sprite *sprite_copy, void *user);
typedef void (*sprpl89_hook_post_emit_fn)(struct sprpl89_ctx_s *ctx, int sprite_index, int first_vert, int vert_count, int first_tri, int tri_count, void *user);
typedef sprpl89_fx (*sprpl89_hook_depth_fade_fn)(struct sprpl89_ctx_s *ctx, int sprite_index, sprpl89_vec3 world_pos, sprpl89_fx sprite_depth, void *user);

typedef struct sprpl89_hooks_s {
    void *user;
    sprpl89_hook_cull_sprite_fn cull_sprite;
    sprpl89_hook_resolve_frame_fn resolve_frame;
    sprpl89_hook_mutate_sprite_fn mutate_sprite;
    sprpl89_hook_post_emit_fn post_emit;
    sprpl89_hook_depth_fade_fn depth_fade;
} sprpl89_hooks;

typedef struct sprpl89_pick_hit_s {
    int hit;
    int sprite_index;
    sprpl89_fx t;
    sprpl89_fx local_x;
    sprpl89_fx local_y;
    sprpl89_fx uv_x;
    sprpl89_fx uv_y;
    sprpl89_vec3 world_pos;
} sprpl89_pick_hit;

typedef struct sprpl89_ctx_s {
    sprpl89_sprite sprites[SP89_MAX_SPRITES];
    sprpl89_atlas_frame frames[SP89_MAX_ATLAS_FRAMES];
    sprpl89_sort_item sort_items[SP89_MAX_SPRITES];
    sprpl89_vector_cmd vector_cmds[SP89_MAX_VECTOR_CMDS];
    sprpl89_text_cmd text_cmds[SP89_MAX_TEXT_CMDS];
    sprpl89_panel_cmd panel_cmds[SP89_MAX_PANEL_CMDS];
    sprpl89_shadow_cmd shadow_cmds[SP89_MAX_SHADOW_CMDS];
    sprpl89_font fonts[SP89_MAX_FONTS];
    int sprite_count;
    int frame_count;
    int vector_count;
    int text_count;
    int panel_count;
    int shadow_count;
    int font_count;
    sprpl89_camera camera;
    sprpl89_hooks hooks;
    sprpl89_u32 tick_ms;
} sprpl89_ctx;

/* Fixed helpers */
sprpl89_fx sprpl89_fx_from_int(int v);
int sprpl89_fx_to_int(sprpl89_fx v);
sprpl89_fx sprpl89_fx_mul(sprpl89_fx a, sprpl89_fx b);
sprpl89_fx sprpl89_fx_div(sprpl89_fx a, sprpl89_fx b);
sprpl89_fx sprpl89_fx_abs(sprpl89_fx a);
sprpl89_fx sprpl89_fx_sqrt(sprpl89_fx v);
sprpl89_fx sprpl89_fx_clamp(sprpl89_fx v, sprpl89_fx lo, sprpl89_fx hi);

sprpl89_vec3 sprpl89_v3(sprpl89_fx x, sprpl89_fx y, sprpl89_fx z);
sprpl89_vec3 sprpl89_v3_add(sprpl89_vec3 a, sprpl89_vec3 b);
sprpl89_vec3 sprpl89_v3_sub(sprpl89_vec3 a, sprpl89_vec3 b);
sprpl89_vec3 sprpl89_v3_scale(sprpl89_vec3 a, sprpl89_fx s);
sprpl89_fx sprpl89_v3_dot(sprpl89_vec3 a, sprpl89_vec3 b);
sprpl89_vec3 sprpl89_v3_cross(sprpl89_vec3 a, sprpl89_vec3 b);
sprpl89_vec3 sprpl89_v3_normalize(sprpl89_vec3 a, sprpl89_vec3 fallback);

sprpl89_color sprpl89_rgba(int r, int g, int b, int a);
sprpl89_color sprpl89_rgba_mod_alpha(sprpl89_color c, sprpl89_fx alpha_fx);

void sprpl89_init(sprpl89_ctx *ctx);
void sprpl89_set_camera(sprpl89_ctx *ctx, const sprpl89_camera *cam);
void sprpl89_set_tick_ms(sprpl89_ctx *ctx, sprpl89_u32 tick_ms);
void sprpl89_set_hooks(sprpl89_ctx *ctx, const sprpl89_hooks *hooks);
void sprpl89_clear_hooks(sprpl89_ctx *ctx);

int sprpl89_add_frame(sprpl89_ctx *ctx, const sprpl89_atlas_frame *frame);
int sprpl89_add_frame_px(sprpl89_ctx *ctx, int tex_w, int tex_h, int x, int y, int w, int h, int page_id, int user_id);

int sprpl89_spawn_sprite(sprpl89_ctx *ctx, const sprpl89_sprite *sprite);
sprpl89_sprite *sprpl89_get_sprite(sprpl89_ctx *ctx, int id);
void sprpl89_kill_sprite(sprpl89_ctx *ctx, int id);
void sprpl89_clear_sprites(sprpl89_ctx *ctx);
void sprpl89_clear_vectors(sprpl89_ctx *ctx);
void sprpl89_clear_text(sprpl89_ctx *ctx);
void sprpl89_clear_panels(sprpl89_ctx *ctx);
void sprpl89_clear_shadows(sprpl89_ctx *ctx);

int sprpl89_add_vector_rect(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_color color);
int sprpl89_add_vector_line(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_fx thickness, sprpl89_color color);
int sprpl89_add_vector_tri(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_fx x2, sprpl89_fx y2, sprpl89_color color);
int sprpl89_add_vector_circle(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx cx, sprpl89_fx cy, sprpl89_fx radius, sprpl89_color color);

int sprpl89_add_font(sprpl89_ctx *ctx, int line_height_px, int space_advance_px, int material_id);
int sprpl89_font_set_glyph(sprpl89_ctx *ctx, int font_id, int codepoint, int frame_id, int advance_px, int bearing_x_px, int bearing_y_px, int w_px, int h_px);
int sprpl89_font_set_ascii_grid(sprpl89_ctx *ctx, int font_id, int tex_w, int tex_h, int page_id, int first_code, int cols, int rows, int cell_w, int cell_h, int advance_px);
int sprpl89_add_text(sprpl89_ctx *ctx, int sprite_index, int font_id, const char *text, sprpl89_fx x, sprpl89_fx y, sprpl89_fx sx, sprpl89_fx sy, sprpl89_color color, int align);

int sprpl89_add_panel_9slice(sprpl89_ctx *ctx, int sprite_index, int frame_id, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_fx border, sprpl89_color color);
int sprpl89_add_shadow_blob(sprpl89_ctx *ctx, int sprite_index, int frame_id, sprpl89_fx radius_x, sprpl89_fx radius_z, sprpl89_fx y_offset, sprpl89_color color);

/* Builds far-to-near order. You may use this order or your renderer's own batching/sort. */
int sprpl89_build_sort(sprpl89_ctx *ctx);
void sprpl89_sort_for_render(sprpl89_ctx *ctx, int count);
void sprpl89_sort_far_to_near(sprpl89_ctx *ctx, int count); /* v1 compatible alias */

/* Emits raster/vector/text/panel/shadow geometry to out. */
int sprpl89_emit_begin(sprpl89_emit *out);
int sprpl89_emit_sprite(sprpl89_ctx *ctx, int sprite_index, sprpl89_emit *out);
int sprpl89_emit_all_sorted(sprpl89_ctx *ctx, sprpl89_emit *out);

/* Picking / interaction: ray direction should be normalized in the same fixed space if possible. */
int sprpl89_pick_sprite_ray(sprpl89_ctx *ctx, int sprite_index, sprpl89_vec3 ray_origin, sprpl89_vec3 ray_dir, sprpl89_pick_hit *hit);
int sprpl89_pick_all_ray(sprpl89_ctx *ctx, sprpl89_vec3 ray_origin, sprpl89_vec3 ray_dir, sprpl89_pick_hit *hit);

/* Utility for Doom-like orientation frames. Returns 0..7 or 0..15. */
int sprpl89_select_view_index(sprpl89_vec3 sprite_pos, sprpl89_u16 sprite_yaw_u16, sprpl89_vec3 camera_pos, int view_count);
sprpl89_u16 sprpl89_frame_for_sprite(sprpl89_ctx *ctx, const sprpl89_sprite *spr);

#ifdef __cplusplus
}
#endif

#endif /* SPRITEPLANE89_H */

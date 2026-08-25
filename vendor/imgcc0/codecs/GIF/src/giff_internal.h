#ifndef GIFF_INTERNAL_H
#define GIFF_INTERNAL_H

#include <string.h>
#include "giff/giff.h"

#define GIFF_OCTREE_INVALID 0xFFFFu

typedef struct giff_oct_node {
    giff_u16 child[8];
    giff_u16 next_reducible;
    giff_u16 palette_index;
    giff_u8 level;
    giff_u8 is_leaf;
    giff_u32 count;
    giff_u32 rsum;
    giff_u32 gsum;
    giff_u32 bsum;
} giff_oct_node;

typedef struct giff_ws_cursor {
    giff_u8* base;
    giff_u32 size;
    giff_u32 used;
} giff_ws_cursor;

giff_u32 giff_align_up(giff_u32 value, giff_u32 align);
void giff_mem_zero(void* ptr, giff_u32 size);
void giff_mem_copy(void* dst, const void* src, giff_u32 size);
void giff_mem_move(void* dst, const void* src, giff_u32 size);

giff_result giff_ws_take(
    giff_ws_cursor* ws,
    giff_u32 bytes,
    giff_u32 align,
    void** out_ptr
);

giff_result giff_encoder_write_bytes(
    giff_encoder* enc,
    const giff_u8* src,
    giff_u32 size
);

void giff_lzw_dec_reset(giff_decoder* dec);
void giff_lzw_enc_reset(giff_encoder* enc);

giff_result giff_lzw_enc_begin_image(giff_encoder* enc, giff_u8 min_code_size);
giff_result giff_lzw_enc_emit_index(giff_encoder* enc, giff_u8 index);
giff_result giff_lzw_enc_end_image(giff_encoder* enc);
giff_result giff_lzw_enc_force_clear(giff_encoder* enc);
giff_result giff_lzw_enc_soft_break(giff_encoder* enc);

giff_result giff_palette_find_nearest(
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 transparent_index,
    giff_u8 has_transparent,
    giff_s32 r,
    giff_s32 g,
    giff_s32 b,
    giff_u8* out_index,
    giff_rgb8* out_color
);

giff_result giff_palette_map_rgb(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 transparent_index,
    giff_u8 has_transparent,
    giff_s32 r,
    giff_s32 g,
    giff_s32 b,
    giff_u8 use_octree_direct,
    giff_u8* out_index,
    giff_rgb8* out_color
);

giff_result giff_palette_map_rgba(
    giff_encoder* enc,
    const giff_rgb8* palette,
    giff_u16 palette_entries,
    giff_u8 transparent_index,
    giff_u8 has_transparent,
    const giff_rgba8* px,
    giff_u16 x,
    giff_u16 y,
    giff_u8 dither_mode,
    giff_u8 use_octree_direct,
    giff_u8* out_index
);

giff_result giff_palette_build_from_rgba(
    giff_encoder* enc,
    const giff_rgba8* pixels,
    giff_u16 width,
    giff_u16 height,
    giff_u32 stride,
    giff_rgb8* out_palette,
    giff_u16* out_entries,
    giff_u16 max_entries,
    giff_u8 reserve_transparent,
    giff_u8* out_transparent_index,
    giff_u8* out_has_transparent
);

#endif

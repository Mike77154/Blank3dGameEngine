#include "spriteplane89.h"

/* spriteplane89 v2 implementation: C89 only, fixed point only. */

sprpl89_fx sprpl89_fx_clamp(sprpl89_fx v, sprpl89_fx lo, sprpl89_fx hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

sprpl89_fx sprpl89_fx_from_int(int v) { return (sprpl89_fx)(v << SP89_FX_SHIFT); }
int sprpl89_fx_to_int(sprpl89_fx v) { return (int)(v >> SP89_FX_SHIFT); }
sprpl89_fx sprpl89_fx_abs(sprpl89_fx a) { return a < 0 ? -a : a; }

sprpl89_fx sprpl89_fx_mul(sprpl89_fx a, sprpl89_fx b)
{
    /* Portable 32-bit fixed multiply: avoids long long/float/double. */
    return (sprpl89_fx)((a >> 8) * (b >> 8));
}

sprpl89_fx sprpl89_fx_div(sprpl89_fx a, sprpl89_fx b)
{
    sprpl89_fx sign;
    unsigned int ua, ub, q, r;
    int i;
    if (b == 0) return 0;
    sign = 1;
    if (a < 0) { a = -a; sign = -sign; }
    if (b < 0) { b = -b; sign = -sign; }
    ua = (unsigned int)a;
    ub = (unsigned int)b;
    q = 0;
    r = 0;
    for (i = 31; i >= 0; --i) {
        r = (r << 1) | ((ua >> i) & 1u);
        if (r >= ub) { r -= ub; q |= (1u << i); }
    }
    for (i = 0; i < 16; ++i) {
        r <<= 1;
        q <<= 1;
        if (r >= ub) { r -= ub; q |= 1u; }
    }
    if (q > 0x7fffffffu) q = 0x7fffffffu;
    return sign < 0 ? -(sprpl89_fx)q : (sprpl89_fx)q;
}

sprpl89_fx sprpl89_fx_sqrt(sprpl89_fx v)
{
    unsigned int x;
    unsigned int bit;
    unsigned int res;
    if (v <= 0) return 0;
    x = (unsigned int)v;
    bit = 1u << 30;
    res = 0;
    while (bit > x) bit >>= 2;
    while (bit != 0) {
        if (x >= res + bit) {
            x -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (sprpl89_fx)(res << 8);
}

sprpl89_vec3 sprpl89_v3(sprpl89_fx x, sprpl89_fx y, sprpl89_fx z)
{
    sprpl89_vec3 r;
    r.x = x; r.y = y; r.z = z;
    return r;
}

sprpl89_vec3 sprpl89_v3_add(sprpl89_vec3 a, sprpl89_vec3 b)
{
    return sprpl89_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

sprpl89_vec3 sprpl89_v3_sub(sprpl89_vec3 a, sprpl89_vec3 b)
{
    return sprpl89_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

sprpl89_vec3 sprpl89_v3_scale(sprpl89_vec3 a, sprpl89_fx s)
{
    return sprpl89_v3(sprpl89_fx_mul(a.x, s), sprpl89_fx_mul(a.y, s), sprpl89_fx_mul(a.z, s));
}

sprpl89_fx sprpl89_v3_dot(sprpl89_vec3 a, sprpl89_vec3 b)
{
    return sprpl89_fx_mul(a.x, b.x) + sprpl89_fx_mul(a.y, b.y) + sprpl89_fx_mul(a.z, b.z);
}

sprpl89_vec3 sprpl89_v3_cross(sprpl89_vec3 a, sprpl89_vec3 b)
{
    sprpl89_vec3 r;
    r.x = sprpl89_fx_mul(a.y, b.z) - sprpl89_fx_mul(a.z, b.y);
    r.y = sprpl89_fx_mul(a.z, b.x) - sprpl89_fx_mul(a.x, b.z);
    r.z = sprpl89_fx_mul(a.x, b.y) - sprpl89_fx_mul(a.y, b.x);
    return r;
}

sprpl89_vec3 sprpl89_v3_normalize(sprpl89_vec3 a, sprpl89_vec3 fallback)
{
    sprpl89_fx len2;
    sprpl89_fx len;
    len2 = sprpl89_v3_dot(a, a);
    if (len2 <= 16) return fallback;
    len = sprpl89_fx_sqrt(len2);
    if (len <= 0) return fallback;
    return sprpl89_v3(sprpl89_fx_div(a.x, len), sprpl89_fx_div(a.y, len), sprpl89_fx_div(a.z, len));
}

sprpl89_color sprpl89_rgba(int r, int g, int b, int a)
{
    sprpl89_color c;
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    c.r = (sprpl89_u8)r; c.g = (sprpl89_u8)g; c.b = (sprpl89_u8)b; c.a = (sprpl89_u8)a;
    return c;
}

sprpl89_color sprpl89_rgba_mod_alpha(sprpl89_color c, sprpl89_fx alpha_fx)
{
    int a;
    alpha_fx = sprpl89_fx_clamp(alpha_fx, 0, SP89_FX_ONE);
    a = (int)((c.a * alpha_fx) >> SP89_FX_SHIFT);
    c.a = (sprpl89_u8)a;
    return c;
}

void sprpl89_init(sprpl89_ctx *ctx)
{
    int i, j;
    if (!ctx) return;
    for (i = 0; i < SP89_MAX_SPRITES; ++i) ctx->sprites[i].active = 0;
    for (i = 0; i < SP89_MAX_ATLAS_FRAMES; ++i) ctx->frames[i].page_id = 0;
    for (i = 0; i < SP89_MAX_VECTOR_CMDS; ++i) ctx->vector_cmds[i].active = 0;
    for (i = 0; i < SP89_MAX_TEXT_CMDS; ++i) ctx->text_cmds[i].active = 0;
    for (i = 0; i < SP89_MAX_PANEL_CMDS; ++i) ctx->panel_cmds[i].active = 0;
    for (i = 0; i < SP89_MAX_SHADOW_CMDS; ++i) ctx->shadow_cmds[i].active = 0;
    for (i = 0; i < SP89_MAX_FONTS; ++i) {
        ctx->fonts[i].active = 0;
        for (j = 0; j < SP89_MAX_GLYPHS_PER_FONT; ++j) ctx->fonts[i].glyphs[j].active = 0;
    }
    ctx->sprite_count = 0;
    ctx->frame_count = 0;
    ctx->vector_count = 0;
    ctx->text_count = 0;
    ctx->panel_count = 0;
    ctx->shadow_count = 0;
    ctx->font_count = 0;
    ctx->tick_ms = 0;
    ctx->hooks.user = 0;
    ctx->hooks.cull_sprite = 0;
    ctx->hooks.resolve_frame = 0;
    ctx->hooks.mutate_sprite = 0;
    ctx->hooks.post_emit = 0;
    ctx->hooks.depth_fade = 0;
    ctx->camera.pos = sprpl89_v3(0, 0, 0);
    ctx->camera.right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    ctx->camera.up = sprpl89_v3(0, SP89_FX_ONE, 0);
    ctx->camera.forward = sprpl89_v3(0, 0, SP89_FX_ONE);
    ctx->camera.world_up = sprpl89_v3(0, SP89_FX_ONE, 0);
}

void sprpl89_set_camera(sprpl89_ctx *ctx, const sprpl89_camera *cam)
{
    if (!ctx || !cam) return;
    ctx->camera = *cam;
    ctx->camera.right = sprpl89_v3_normalize(ctx->camera.right, sprpl89_v3(SP89_FX_ONE,0,0));
    ctx->camera.up = sprpl89_v3_normalize(ctx->camera.up, sprpl89_v3(0,SP89_FX_ONE,0));
    ctx->camera.forward = sprpl89_v3_normalize(ctx->camera.forward, sprpl89_v3(0,0,SP89_FX_ONE));
    ctx->camera.world_up = sprpl89_v3_normalize(ctx->camera.world_up, sprpl89_v3(0,SP89_FX_ONE,0));
}

void sprpl89_set_tick_ms(sprpl89_ctx *ctx, sprpl89_u32 tick_ms)
{
    if (!ctx) return;
    ctx->tick_ms = tick_ms;
}

void sprpl89_set_hooks(sprpl89_ctx *ctx, const sprpl89_hooks *hooks)
{
    if (!ctx || !hooks) return;
    ctx->hooks = *hooks;
}

void sprpl89_clear_hooks(sprpl89_ctx *ctx)
{
    if (!ctx) return;
    ctx->hooks.user = 0;
    ctx->hooks.cull_sprite = 0;
    ctx->hooks.resolve_frame = 0;
    ctx->hooks.mutate_sprite = 0;
    ctx->hooks.post_emit = 0;
    ctx->hooks.depth_fade = 0;
}

int sprpl89_add_frame(sprpl89_ctx *ctx, const sprpl89_atlas_frame *frame)
{
    int id;
    if (!ctx || !frame) return SP89_ERR_BADARG;
    if (ctx->frame_count >= SP89_MAX_ATLAS_FRAMES) return SP89_ERR_FULL;
    id = ctx->frame_count++;
    ctx->frames[id] = *frame;
    return id;
}

int sprpl89_add_frame_px(sprpl89_ctx *ctx, int tex_w, int tex_h, int x, int y, int w, int h, int page_id, int user_id)
{
    sprpl89_atlas_frame f;
    if (!ctx || tex_w <= 0 || tex_h <= 0 || w <= 0 || h <= 0) return SP89_ERR_BADARG;
    f.u0 = sprpl89_fx_div(sprpl89_fx_from_int(x), sprpl89_fx_from_int(tex_w));
    f.v0 = sprpl89_fx_div(sprpl89_fx_from_int(y), sprpl89_fx_from_int(tex_h));
    f.u1 = sprpl89_fx_div(sprpl89_fx_from_int(x + w), sprpl89_fx_from_int(tex_w));
    f.v1 = sprpl89_fx_div(sprpl89_fx_from_int(y + h), sprpl89_fx_from_int(tex_h));
    f.pivot_x = SP89_FX_HALF;
    f.pivot_y = SP89_FX_HALF;
    f.logical_w = sprpl89_fx_from_int(w);
    f.logical_h = sprpl89_fx_from_int(h);
    f.page_id = (sprpl89_u16)page_id;
    f.user_id = (sprpl89_u16)user_id;
    return sprpl89_add_frame(ctx, &f);
}

static void sprpl89_defaults_for_sprite(sprpl89_sprite *s)
{
    if (!s) return;
    if (s->scale_x == 0) s->scale_x = SP89_FX_ONE;
    if (s->scale_y == 0) s->scale_y = SP89_FX_ONE;
    if (s->width == 0) s->width = SP89_FX_ONE;
    if (s->height == 0) s->height = SP89_FX_ONE;
    if (s->view_count == 0) s->view_count = 1;
    if (s->blend_mode == 0 && (s->flags & SP89_FLAG_ADDITIVE)) s->blend_mode = SP89_BLEND_ADDITIVE;
    if (s->blend_mode == 0 && (s->flags & SP89_FLAG_ALPHA_BLEND)) s->blend_mode = SP89_BLEND_ALPHA;
    if (s->render_queue == 0 && (s->flags & SP89_FLAG_ALPHA_TEST)) s->render_queue = SP89_QUEUE_ALPHA_TEST;
    if (s->render_queue == 0 && (s->flags & (SP89_FLAG_ALPHA_BLEND | SP89_FLAG_ADDITIVE))) s->render_queue = SP89_QUEUE_TRANSPARENT;
    if (s->flags & SP89_FLAG_ADDITIVE) s->render_queue = SP89_QUEUE_ADDITIVE;
    if (!(s->flags & (SP89_FLAG_ALPHA_BLEND|SP89_FLAG_ALPHA_TEST|SP89_FLAG_ADDITIVE))) s->flags |= SP89_FLAG_ALPHA_BLEND;
    s->flags |= SP89_FLAG_VISIBLE;
}

int sprpl89_spawn_sprite(sprpl89_ctx *ctx, const sprpl89_sprite *sprite)
{
    int i;
    sprpl89_sprite s;
    if (!ctx || !sprite) return SP89_ERR_BADARG;
    for (i = 0; i < SP89_MAX_SPRITES; ++i) {
        if (!ctx->sprites[i].active) {
            s = *sprite;
            s.active = 1;
            sprpl89_defaults_for_sprite(&s);
            ctx->sprites[i] = s;
            if (i >= ctx->sprite_count) ctx->sprite_count = i + 1;
            return i;
        }
    }
    return SP89_ERR_FULL;
}

sprpl89_sprite *sprpl89_get_sprite(sprpl89_ctx *ctx, int id)
{
    if (!ctx || id < 0 || id >= SP89_MAX_SPRITES) return 0;
    if (!ctx->sprites[id].active) return 0;
    return &ctx->sprites[id];
}

void sprpl89_kill_sprite(sprpl89_ctx *ctx, int id)
{
    if (!ctx || id < 0 || id >= SP89_MAX_SPRITES) return;
    ctx->sprites[id].active = 0;
}

void sprpl89_clear_sprites(sprpl89_ctx *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < SP89_MAX_SPRITES; ++i) ctx->sprites[i].active = 0;
    ctx->sprite_count = 0;
}

void sprpl89_clear_vectors(sprpl89_ctx *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < SP89_MAX_VECTOR_CMDS; ++i) ctx->vector_cmds[i].active = 0;
    ctx->vector_count = 0;
}

void sprpl89_clear_text(sprpl89_ctx *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < SP89_MAX_TEXT_CMDS; ++i) ctx->text_cmds[i].active = 0;
    ctx->text_count = 0;
}

void sprpl89_clear_panels(sprpl89_ctx *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < SP89_MAX_PANEL_CMDS; ++i) ctx->panel_cmds[i].active = 0;
    ctx->panel_count = 0;
}

void sprpl89_clear_shadows(sprpl89_ctx *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < SP89_MAX_SHADOW_CMDS; ++i) ctx->shadow_cmds[i].active = 0;
    ctx->shadow_count = 0;
}

static int sprpl89_add_vector(sprpl89_ctx *ctx, const sprpl89_vector_cmd *cmd)
{
    int i;
    if (!ctx || !cmd) return SP89_ERR_BADARG;
    if (cmd->sprite_index >= SP89_MAX_SPRITES) return SP89_ERR_RANGE;
    for (i = 0; i < SP89_MAX_VECTOR_CMDS; ++i) {
        if (!ctx->vector_cmds[i].active) {
            ctx->vector_cmds[i] = *cmd;
            ctx->vector_cmds[i].active = 1;
            if (i >= ctx->vector_count) ctx->vector_count = i + 1;
            return i;
        }
    }
    return SP89_ERR_FULL;
}

int sprpl89_add_vector_rect(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_color color)
{
    sprpl89_vector_cmd c;
    c.active = 1; c.sprite_index = (sprpl89_u16)sprite_index; c.kind = SP89_VCMD_RECT; c.flags = 0;
    c.x0 = x0; c.y0 = y0; c.x1 = x1; c.y1 = y1; c.x2 = 0; c.y2 = 0; c.thickness = 0;
    c.color = color; c.material_id = 0;
    return sprpl89_add_vector(ctx, &c);
}

int sprpl89_add_vector_line(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_fx thickness, sprpl89_color color)
{
    sprpl89_vector_cmd c;
    c.active = 1; c.sprite_index = (sprpl89_u16)sprite_index; c.kind = SP89_VCMD_LINE; c.flags = 0;
    c.x0 = x0; c.y0 = y0; c.x1 = x1; c.y1 = y1; c.x2 = 0; c.y2 = 0; c.thickness = thickness;
    c.color = color; c.material_id = 0;
    return sprpl89_add_vector(ctx, &c);
}

int sprpl89_add_vector_tri(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_fx x2, sprpl89_fx y2, sprpl89_color color)
{
    sprpl89_vector_cmd c;
    c.active = 1; c.sprite_index = (sprpl89_u16)sprite_index; c.kind = SP89_VCMD_TRI; c.flags = 0;
    c.x0 = x0; c.y0 = y0; c.x1 = x1; c.y1 = y1; c.x2 = x2; c.y2 = y2; c.thickness = 0;
    c.color = color; c.material_id = 0;
    return sprpl89_add_vector(ctx, &c);
}

int sprpl89_add_vector_circle(sprpl89_ctx *ctx, int sprite_index, sprpl89_fx cx, sprpl89_fx cy, sprpl89_fx radius, sprpl89_color color)
{
    sprpl89_vector_cmd c;
    c.active = 1; c.sprite_index = (sprpl89_u16)sprite_index; c.kind = SP89_VCMD_CIRCLE; c.flags = 0;
    c.x0 = cx; c.y0 = cy; c.x1 = radius; c.y1 = 0; c.x2 = 0; c.y2 = 0; c.thickness = 0;
    c.color = color; c.material_id = 0;
    return sprpl89_add_vector(ctx, &c);
}

int sprpl89_add_font(sprpl89_ctx *ctx, int line_height_px, int space_advance_px, int material_id)
{
    int i, j;
    if (!ctx || line_height_px <= 0) return SP89_ERR_BADARG;
    for (i = 0; i < SP89_MAX_FONTS; ++i) {
        if (!ctx->fonts[i].active) {
            ctx->fonts[i].active = 1;
            ctx->fonts[i].line_height = (sprpl89_u16)line_height_px;
            ctx->fonts[i].space_advance = (sprpl89_u16)space_advance_px;
            ctx->fonts[i].missing_glyph = 0;
            ctx->fonts[i].material_id = (sprpl89_u16)material_id;
            for (j = 0; j < SP89_MAX_GLYPHS_PER_FONT; ++j) ctx->fonts[i].glyphs[j].active = 0;
            if (i >= ctx->font_count) ctx->font_count = i + 1;
            return i;
        }
    }
    return SP89_ERR_FULL;
}

static sprpl89_glyph *sprpl89_find_glyph(sprpl89_font *font, int codepoint)
{
    int i;
    if (!font || !font->active) return 0;
    for (i = 0; i < SP89_MAX_GLYPHS_PER_FONT; ++i) {
        if (font->glyphs[i].active && font->glyphs[i].codepoint == (sprpl89_u16)codepoint) return &font->glyphs[i];
    }
    return 0;
}

int sprpl89_font_set_glyph(sprpl89_ctx *ctx, int font_id, int codepoint, int frame_id, int advance_px, int bearing_x_px, int bearing_y_px, int w_px, int h_px)
{
    int i;
    sprpl89_font *font;
    sprpl89_glyph *g;
    if (!ctx || font_id < 0 || font_id >= SP89_MAX_FONTS || !ctx->fonts[font_id].active) return SP89_ERR_BADARG;
    if (frame_id < 0 || frame_id >= ctx->frame_count) return SP89_ERR_RANGE;
    font = &ctx->fonts[font_id];
    g = sprpl89_find_glyph(font, codepoint);
    if (!g) {
        for (i = 0; i < SP89_MAX_GLYPHS_PER_FONT; ++i) {
            if (!font->glyphs[i].active) { g = &font->glyphs[i]; break; }
        }
    }
    if (!g) return SP89_ERR_FULL;
    g->active = 1;
    g->codepoint = (sprpl89_u16)codepoint;
    g->frame_id = (sprpl89_u16)frame_id;
    g->advance_x = sprpl89_fx_from_int(advance_px);
    g->bearing_x = sprpl89_fx_from_int(bearing_x_px);
    g->bearing_y = sprpl89_fx_from_int(bearing_y_px);
    g->width = sprpl89_fx_from_int(w_px <= 0 ? advance_px : w_px);
    g->height = sprpl89_fx_from_int(h_px <= 0 ? (int)font->line_height : h_px);
    return SP89_OK;
}

int sprpl89_font_set_ascii_grid(sprpl89_ctx *ctx, int font_id, int tex_w, int tex_h, int page_id, int first_code, int cols, int rows, int cell_w, int cell_h, int advance_px)
{
    int r, c, code, frame_id, e;
    if (!ctx || cols <= 0 || rows <= 0 || cell_w <= 0 || cell_h <= 0) return SP89_ERR_BADARG;
    code = first_code;
    for (r = 0; r < rows; ++r) {
        for (c = 0; c < cols; ++c) {
            frame_id = sprpl89_add_frame_px(ctx, tex_w, tex_h, c * cell_w, r * cell_h, cell_w, cell_h, page_id, code);
            if (frame_id < 0) return frame_id;
            e = sprpl89_font_set_glyph(ctx, font_id, code, frame_id, advance_px, 0, 0, cell_w, cell_h);
            if (e != SP89_OK) return e;
            ++code;
        }
    }
    return SP89_OK;
}

int sprpl89_add_text(sprpl89_ctx *ctx, int sprite_index, int font_id, const char *text, sprpl89_fx x, sprpl89_fx y, sprpl89_fx sx, sprpl89_fx sy, sprpl89_color color, int align)
{
    int i, j;
    sprpl89_text_cmd c;
    if (!ctx || !text || font_id < 0 || font_id >= SP89_MAX_FONTS || !ctx->fonts[font_id].active) return SP89_ERR_BADARG;
    if (sprite_index < 0 || sprite_index >= SP89_MAX_SPRITES) return SP89_ERR_RANGE;
    for (i = 0; i < SP89_MAX_TEXT_CMDS; ++i) {
        if (!ctx->text_cmds[i].active) {
            c.active = 1;
            c.sprite_index = (sprpl89_u16)sprite_index;
            c.font_id = (sprpl89_u16)font_id;
            c.flags = 0;
            c.x = x;
            c.y = y;
            c.scale_x = sx == 0 ? SP89_FX_ONE : sx;
            c.scale_y = sy == 0 ? SP89_FX_ONE : sy;
            c.max_width = 0;
            c.align = (sprpl89_u16)align;
            c.color = color;
            c.material_id = ctx->fonts[font_id].material_id;
            for (j = 0; j < SP89_MAX_TEXT_CHARS; ++j) {
                c.text[j] = text[j];
                if (text[j] == '\0') break;
            }
            c.text[SP89_MAX_TEXT_CHARS - 1] = '\0';
            ctx->text_cmds[i] = c;
            if (i >= ctx->text_count) ctx->text_count = i + 1;
            return i;
        }
    }
    return SP89_ERR_FULL;
}

int sprpl89_add_panel_9slice(sprpl89_ctx *ctx, int sprite_index, int frame_id, sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1, sprpl89_fx border, sprpl89_color color)
{
    int i;
    sprpl89_panel_cmd c;
    if (!ctx || frame_id < 0 || frame_id >= ctx->frame_count) return SP89_ERR_BADARG;
    if (sprite_index < 0 || sprite_index >= SP89_MAX_SPRITES) return SP89_ERR_RANGE;
    for (i = 0; i < SP89_MAX_PANEL_CMDS; ++i) {
        if (!ctx->panel_cmds[i].active) {
            c.active = 1;
            c.sprite_index = (sprpl89_u16)sprite_index;
            c.frame_id = (sprpl89_u16)frame_id;
            c.flags = 0;
            c.x0 = x0; c.y0 = y0; c.x1 = x1; c.y1 = y1;
            c.border_l = border; c.border_t = border; c.border_r = border; c.border_b = border;
            c.color = color;
            c.material_id = 0;
            ctx->panel_cmds[i] = c;
            if (i >= ctx->panel_count) ctx->panel_count = i + 1;
            return i;
        }
    }
    return SP89_ERR_FULL;
}

int sprpl89_add_shadow_blob(sprpl89_ctx *ctx, int sprite_index, int frame_id, sprpl89_fx radius_x, sprpl89_fx radius_z, sprpl89_fx y_offset, sprpl89_color color)
{
    int i;
    sprpl89_shadow_cmd c;
    if (!ctx || frame_id < 0 || frame_id >= ctx->frame_count) return SP89_ERR_BADARG;
    if (sprite_index < 0 || sprite_index >= SP89_MAX_SPRITES) return SP89_ERR_RANGE;
    for (i = 0; i < SP89_MAX_SHADOW_CMDS; ++i) {
        if (!ctx->shadow_cmds[i].active) {
            c.active = 1;
            c.sprite_index = (sprpl89_u16)sprite_index;
            c.frame_id = (sprpl89_u16)frame_id;
            c.flags = 0;
            c.radius_x = radius_x;
            c.radius_z = radius_z;
            c.y_offset = y_offset;
            c.color = color;
            c.material_id = 0;
            ctx->shadow_cmds[i] = c;
            if (i >= ctx->shadow_count) ctx->shadow_count = i + 1;
            return i;
        }
    }
    return SP89_ERR_FULL;
}

static sprpl89_u16 sprpl89_atan2_u16_approx(sprpl89_fx y, sprpl89_fx x)
{
    sprpl89_fx ax, ay;
    sprpl89_fx r;
    int angle;
    ax = sprpl89_fx_abs(x);
    ay = sprpl89_fx_abs(y);
    if (x == 0 && y == 0) return 0;
    if (ax >= ay) {
        r = sprpl89_fx_div(ay, ax == 0 ? 1 : ax);
        angle = (int)((r * 8192) >> SP89_FX_SHIFT);
    } else {
        r = sprpl89_fx_div(ax, ay == 0 ? 1 : ay);
        angle = 16384 - (int)((r * 8192) >> SP89_FX_SHIFT);
    }
    if (x >= 0 && y >= 0) return (sprpl89_u16)angle;
    if (x < 0 && y >= 0) return (sprpl89_u16)(32768 - angle);
    if (x < 0 && y < 0) return (sprpl89_u16)(32768 + angle);
    return (sprpl89_u16)(65536 - angle);
}

int sprpl89_select_view_index(sprpl89_vec3 sprite_pos, sprpl89_u16 sprite_yaw_u16, sprpl89_vec3 camera_pos, int view_count)
{
    sprpl89_vec3 to_cam;
    sprpl89_u16 cam_angle;
    unsigned int rel;
    unsigned int sector;
    unsigned int step;
    if (view_count != 16) view_count = 8;
    to_cam = sprpl89_v3_sub(camera_pos, sprite_pos);
    cam_angle = sprpl89_atan2_u16_approx(to_cam.x, to_cam.z);
    rel = (unsigned int)((cam_angle - sprite_yaw_u16) & 65535u);
    step = 65536u / (unsigned int)view_count;
    sector = (rel + (step / 2u)) / step;
    sector %= (unsigned int)view_count;
    return (int)sector;
}

sprpl89_u16 sprpl89_frame_for_sprite(sprpl89_ctx *ctx, const sprpl89_sprite *spr)
{
    sprpl89_u32 anim_index;
    sprpl89_u16 view_index;
    sprpl89_u16 frames_per_view;
    sprpl89_u32 fps;
    sprpl89_u32 t;
    sprpl89_u16 default_frame;
    if (!ctx || !spr) return 0;
    view_index = 0;
    frames_per_view = spr->frame_count == 0 ? 1 : spr->frame_count;

    if (spr->frame_mode == SP89_FRAME_VIEW_8 || spr->frame_mode == SP89_FRAME_ANIM_VIEW_8) {
        view_index = (sprpl89_u16)sprpl89_select_view_index(spr->pos, spr->yaw_u16, ctx->camera.pos, 8);
    } else if (spr->frame_mode == SP89_FRAME_VIEW_16 || spr->frame_mode == SP89_FRAME_ANIM_VIEW_16) {
        view_index = (sprpl89_u16)sprpl89_select_view_index(spr->pos, spr->yaw_u16, ctx->camera.pos, 16);
    }

    anim_index = 0;
    if (spr->frame_mode == SP89_FRAME_ANIM_LOOP || spr->frame_mode == SP89_FRAME_ANIM_ONCE ||
        spr->frame_mode == SP89_FRAME_ANIM_VIEW_8 || spr->frame_mode == SP89_FRAME_ANIM_VIEW_16) {
        fps = spr->anim_fps_fx8;
        if (fps == 0) fps = 8u << 8;
        t = ctx->tick_ms + (sprpl89_u32)spr->anim_phase_fx8;
        anim_index = ((t * fps) / 1000u) >> 8;
        if (spr->frame_mode == SP89_FRAME_ANIM_ONCE) {
            if (anim_index >= frames_per_view) anim_index = frames_per_view - 1;
        } else {
            anim_index %= frames_per_view;
        }
    }

    default_frame = (sprpl89_u16)(spr->base_frame + (view_index * frames_per_view) + (sprpl89_u16)anim_index);
    return default_frame;
}

static void sprpl89_basis_for_sprite(sprpl89_ctx *ctx, const sprpl89_sprite *spr, sprpl89_vec3 *out_right, sprpl89_vec3 *out_up)
{
    sprpl89_vec3 to_cam;
    sprpl89_vec3 right;
    sprpl89_vec3 up;
    sprpl89_vec3 normal;
    sprpl89_vec3 lock_axis;
    sprpl89_vec3 fallback_right;
    sprpl89_vec3 fallback_up;

    fallback_right = ctx->camera.right;
    fallback_up = ctx->camera.up;
    right = fallback_right;
    up = fallback_up;

    if (spr->billboard_mode == SP89_BILLBOARD_FIXED) {
        right = sprpl89_v3_normalize(spr->axis_right, fallback_right);
        up = sprpl89_v3_normalize(spr->axis_up, fallback_up);
    } else if (spr->billboard_mode == SP89_BILLBOARD_VIEW_ALIGNED || spr->billboard_mode == SP89_BILLBOARD_SCREEN_FIXED) {
        right = ctx->camera.right;
        up = ctx->camera.up;
    } else if (spr->billboard_mode == SP89_BILLBOARD_Y_AXIS || spr->billboard_mode == SP89_BILLBOARD_CROSSED_Y) {
        up = ctx->camera.world_up;
        to_cam = sprpl89_v3_sub(ctx->camera.pos, spr->pos);
        to_cam.y = 0;
        normal = sprpl89_v3_normalize(to_cam, ctx->camera.forward);
        right = sprpl89_v3_cross(up, normal);
        right = sprpl89_v3_normalize(right, ctx->camera.right);
    } else if (spr->billboard_mode == SP89_BILLBOARD_AXIS_LOCKED) {
        lock_axis = sprpl89_v3_normalize(spr->axis_lock, ctx->camera.world_up);
        to_cam = sprpl89_v3_sub(ctx->camera.pos, spr->pos);
        normal = sprpl89_v3_normalize(to_cam, ctx->camera.forward);
        right = sprpl89_v3_cross(lock_axis, normal);
        right = sprpl89_v3_normalize(right, ctx->camera.right);
        up = lock_axis;
    } else if (spr->billboard_mode == SP89_BILLBOARD_VELOCITY) {
        right = sprpl89_v3_normalize(spr->velocity, ctx->camera.right);
        normal = ctx->camera.forward;
        up = sprpl89_v3_cross(normal, right);
        up = sprpl89_v3_normalize(up, ctx->camera.up);
    } else if (spr->billboard_mode == SP89_BILLBOARD_SURFACE) {
        right = sprpl89_v3_normalize(spr->surface_right, spr->axis_right);
        up = sprpl89_v3_normalize(spr->surface_up, spr->axis_up);
    } else {
        to_cam = sprpl89_v3_sub(ctx->camera.pos, spr->pos);
        normal = sprpl89_v3_normalize(to_cam, ctx->camera.forward);
        right = sprpl89_v3_cross(ctx->camera.world_up, normal);
        right = sprpl89_v3_normalize(right, ctx->camera.right);
        up = sprpl89_v3_cross(normal, right);
        up = sprpl89_v3_normalize(up, ctx->camera.up);
    }

    *out_right = right;
    *out_up = up;
}

static int sprpl89_sprite_distance2_to_camera(sprpl89_ctx *ctx, const sprpl89_sprite *spr)
{
    sprpl89_vec3 d;
    d = sprpl89_v3_sub(spr->pos, ctx->camera.pos);
    return (int)sprpl89_v3_dot(d, d);
}

static int sprpl89_should_cull(sprpl89_ctx *ctx, int sprite_index, const sprpl89_sprite *spr)
{
    sprpl89_fx d2;
    sprpl89_fx max2;
    if (!ctx || !spr) return 1;
    if (ctx->hooks.cull_sprite) {
        if (ctx->hooks.cull_sprite(ctx, sprite_index, spr, ctx->hooks.user)) return 1;
    }
    if (spr->cull_distance > 0) {
        d2 = (sprpl89_fx)sprpl89_sprite_distance2_to_camera(ctx, spr);
        max2 = sprpl89_fx_mul(spr->cull_distance, spr->cull_distance);
        if (d2 > max2) return 1;
    }
    return 0;
}

int sprpl89_build_sort(sprpl89_ctx *ctx)
{
    int i, n;
    sprpl89_fx key;
    if (!ctx) return 0;
    n = 0;
    for (i = 0; i < ctx->sprite_count; ++i) {
        if (ctx->sprites[i].active && (ctx->sprites[i].flags & SP89_FLAG_VISIBLE)) {
            if (sprpl89_should_cull(ctx, i, &ctx->sprites[i])) continue;
            key = (sprpl89_fx)sprpl89_sprite_distance2_to_camera(ctx, &ctx->sprites[i]);
            ctx->sort_items[n].sprite_index = (sprpl89_u16)i;
            ctx->sort_items[n].depth_key = key;
            ctx->sort_items[n].material_id = ctx->sprites[i].material_id;
            ctx->sort_items[n].texture_page = ctx->sprites[i].texture_page;
            ctx->sort_items[n].render_queue = ctx->sprites[i].render_queue;
            ctx->sort_items[n].sort_layer = ctx->sprites[i].sort_layer;
            ctx->sort_items[n].sort_order = ctx->sprites[i].sort_order;
            ++n;
        }
    }
    return n;
}

void sprpl89_sort_for_render(sprpl89_ctx *ctx, int count)
{
    int i, j;
    sprpl89_sort_item key;
    int move;
    if (!ctx) return;
    if (count > SP89_MAX_SPRITES) count = SP89_MAX_SPRITES;
    for (i = 1; i < count; ++i) {
        key = ctx->sort_items[i];
        j = i - 1;
        while (j >= 0) {
            move = 0;
            if (ctx->sort_items[j].render_queue > key.render_queue) move = 1;
            else if (ctx->sort_items[j].render_queue == key.render_queue && ctx->sort_items[j].sort_layer > key.sort_layer) move = 1;
            else if (ctx->sort_items[j].render_queue == key.render_queue && ctx->sort_items[j].sort_layer == key.sort_layer && ctx->sort_items[j].sort_order > key.sort_order) move = 1;
            else if (ctx->sort_items[j].render_queue == key.render_queue && ctx->sort_items[j].sort_layer == key.sort_layer && ctx->sort_items[j].sort_order == key.sort_order) {
                if (key.render_queue >= SP89_QUEUE_TRANSPARENT) {
                    if (ctx->sort_items[j].depth_key < key.depth_key) move = 1;
                } else {
                    if (ctx->sort_items[j].depth_key > key.depth_key) move = 1;
                }
            }
            if (!move) break;
            ctx->sort_items[j + 1] = ctx->sort_items[j];
            --j;
        }
        ctx->sort_items[j + 1] = key;
    }
}

void sprpl89_sort_far_to_near(sprpl89_ctx *ctx, int count)
{
    int i, j;
    sprpl89_sort_item key;
    if (!ctx) return;
    if (count > SP89_MAX_SPRITES) count = SP89_MAX_SPRITES;
    for (i = 1; i < count; ++i) {
        key = ctx->sort_items[i];
        j = i - 1;
        while (j >= 0 && ctx->sort_items[j].depth_key < key.depth_key) {
            ctx->sort_items[j + 1] = ctx->sort_items[j];
            --j;
        }
        ctx->sort_items[j + 1] = key;
    }
}

int sprpl89_emit_begin(sprpl89_emit *out)
{
    if (!out) return SP89_ERR_BADARG;
    out->vert_count = 0;
    out->tri_count = 0;
    out->packet_count = 0;
    out->point_count = 0;
    out->dropped_sprites = 0;
    out->dropped_vector_cmds = 0;
    out->dropped_text_cmds = 0;
    out->dropped_panel_cmds = 0;
    out->dropped_shadow_cmds = 0;
    out->dropped_point_cmds = 0;
    out->culled_sprites = 0;
    return SP89_OK;
}

static int sprpl89_emit_packet(sprpl89_emit *out, int first_tri, int tri_count, sprpl89_u16 material_id, sprpl89_u16 page_id, sprpl89_u16 flags, sprpl89_u16 render_queue, sprpl89_u16 blend_mode, sprpl89_u16 owner_sprite, sprpl89_u16 family)
{
    int p;
    if (!out) return SP89_ERR_BADARG;
    if (out->packet_count >= SP89_MAX_EMIT_PACKETS) return SP89_ERR_FULL;
    p = out->packet_count++;
    out->packets[p].first_tri = first_tri;
    out->packets[p].tri_count = tri_count;
    out->packets[p].material_id = material_id;
    out->packets[p].page_id = page_id;
    out->packets[p].flags = flags;
    out->packets[p].render_queue = render_queue;
    out->packets[p].blend_mode = blend_mode;
    out->packets[p].owner_sprite = owner_sprite;
    out->packets[p].primitive_family = family;
    return SP89_OK;
}

static int sprpl89_emit_quad_ex(sprpl89_emit *out, sprpl89_vec3 p0, sprpl89_vec3 p1, sprpl89_vec3 p2, sprpl89_vec3 p3,
                          sprpl89_fx u0, sprpl89_fx v0, sprpl89_fx u1, sprpl89_fx v1,
                          sprpl89_color color, sprpl89_u16 material_id, sprpl89_u16 page_id, sprpl89_u16 flags,
                          sprpl89_u16 render_queue, sprpl89_u16 blend_mode, sprpl89_u16 owner_sprite, sprpl89_u16 family)
{
    int base;
    int first_tri;
    sprpl89_fx tu0, tu1, tv0, tv1;
    sprpl89_fx tmp;
    if (!out) return SP89_ERR_BADARG;
    if (out->vert_count + 4 > SP89_MAX_EMIT_VERTS || out->tri_count + 2 > (SP89_MAX_EMIT_INDICES / 3)) return SP89_ERR_FULL;
    base = out->vert_count;
    first_tri = out->tri_count;
    tu0 = u0; tu1 = u1; tv0 = v0; tv1 = v1;
    if (flags & SP89_FLAG_FLIP_U) { tmp = tu0; tu0 = tu1; tu1 = tmp; }
    if (flags & SP89_FLAG_FLIP_V) { tmp = tv0; tv0 = tv1; tv1 = tmp; }

    out->verts[base + 0].pos = p0; out->verts[base + 0].u = tu0; out->verts[base + 0].v = tv1;
    out->verts[base + 1].pos = p1; out->verts[base + 1].u = tu1; out->verts[base + 1].v = tv1;
    out->verts[base + 2].pos = p2; out->verts[base + 2].u = tu1; out->verts[base + 2].v = tv0;
    out->verts[base + 3].pos = p3; out->verts[base + 3].u = tu0; out->verts[base + 3].v = tv0;

    out->verts[base + 0].color = color; out->verts[base + 1].color = color;
    out->verts[base + 2].color = color; out->verts[base + 3].color = color;
    out->verts[base + 0].material_id = material_id; out->verts[base + 1].material_id = material_id;
    out->verts[base + 2].material_id = material_id; out->verts[base + 3].material_id = material_id;
    out->verts[base + 0].page_id = page_id; out->verts[base + 1].page_id = page_id;
    out->verts[base + 2].page_id = page_id; out->verts[base + 3].page_id = page_id;
    out->verts[base + 0].flags = flags; out->verts[base + 1].flags = flags;
    out->verts[base + 2].flags = flags; out->verts[base + 3].flags = flags;
    out->verts[base + 0].render_queue = render_queue; out->verts[base + 1].render_queue = render_queue;
    out->verts[base + 2].render_queue = render_queue; out->verts[base + 3].render_queue = render_queue;
    out->verts[base + 0].blend_mode = blend_mode; out->verts[base + 1].blend_mode = blend_mode;
    out->verts[base + 2].blend_mode = blend_mode; out->verts[base + 3].blend_mode = blend_mode;

    out->tris[out->tri_count].a = (unsigned short)(base + 0);
    out->tris[out->tri_count].b = (unsigned short)(base + 1);
    out->tris[out->tri_count].c = (unsigned short)(base + 2);
    ++out->tri_count;
    out->tris[out->tri_count].a = (unsigned short)(base + 0);
    out->tris[out->tri_count].b = (unsigned short)(base + 2);
    out->tris[out->tri_count].c = (unsigned short)(base + 3);
    ++out->tri_count;
    out->vert_count += 4;
    return sprpl89_emit_packet(out, first_tri, 2, material_id, page_id, flags, render_queue, blend_mode, owner_sprite, family);
}

static int sprpl89_emit_tri_ex(sprpl89_emit *out, sprpl89_vec3 p0, sprpl89_vec3 p1, sprpl89_vec3 p2, sprpl89_color color, sprpl89_u16 material_id, sprpl89_u16 page_id, sprpl89_u16 flags, sprpl89_u16 render_queue, sprpl89_u16 blend_mode, sprpl89_u16 owner_sprite, sprpl89_u16 family)
{
    int base;
    int first_tri;
    if (!out) return SP89_ERR_BADARG;
    if (out->vert_count + 3 > SP89_MAX_EMIT_VERTS || out->tri_count + 1 > (SP89_MAX_EMIT_INDICES / 3)) return SP89_ERR_FULL;
    base = out->vert_count;
    first_tri = out->tri_count;
    out->verts[base + 0].pos = p0;
    out->verts[base + 1].pos = p1;
    out->verts[base + 2].pos = p2;
    out->verts[base + 0].u = 0; out->verts[base + 0].v = 0;
    out->verts[base + 1].u = SP89_FX_ONE; out->verts[base + 1].v = 0;
    out->verts[base + 2].u = SP89_FX_HALF; out->verts[base + 2].v = SP89_FX_ONE;
    out->verts[base + 0].color = color; out->verts[base + 1].color = color; out->verts[base + 2].color = color;
    out->verts[base + 0].material_id = material_id; out->verts[base + 1].material_id = material_id; out->verts[base + 2].material_id = material_id;
    out->verts[base + 0].page_id = page_id; out->verts[base + 1].page_id = page_id; out->verts[base + 2].page_id = page_id;
    out->verts[base + 0].flags = flags; out->verts[base + 1].flags = flags; out->verts[base + 2].flags = flags;
    out->verts[base + 0].render_queue = render_queue; out->verts[base + 1].render_queue = render_queue; out->verts[base + 2].render_queue = render_queue;
    out->verts[base + 0].blend_mode = blend_mode; out->verts[base + 1].blend_mode = blend_mode; out->verts[base + 2].blend_mode = blend_mode;
    out->tris[out->tri_count].a = (unsigned short)(base + 0);
    out->tris[out->tri_count].b = (unsigned short)(base + 1);
    out->tris[out->tri_count].c = (unsigned short)(base + 2);
    ++out->tri_count;
    out->vert_count += 3;
    return sprpl89_emit_packet(out, first_tri, 1, material_id, page_id, flags, render_queue, blend_mode, owner_sprite, family);
}

static int sprpl89_emit_point_cmd(sprpl89_emit *out, const sprpl89_sprite *spr, sprpl89_u16 frame_id, const sprpl89_atlas_frame *f, sprpl89_u16 owner)
{
    int p;
    if (!out || !spr || !f) return SP89_ERR_BADARG;
    if (out->point_count >= SP89_MAX_POINT_CMDS) return SP89_ERR_FULL;
    p = out->point_count++;
    out->points[p].active = 1;
    out->points[p].pos = spr->pos;
    out->points[p].size_x = sprpl89_fx_mul(spr->width, spr->scale_x);
    out->points[p].size_y = sprpl89_fx_mul(spr->height, spr->scale_y);
    out->points[p].u0 = f->u0; out->points[p].v0 = f->v0; out->points[p].u1 = f->u1; out->points[p].v1 = f->v1;
    out->points[p].color = spr->color;
    out->points[p].material_id = spr->material_id;
    out->points[p].page_id = f->page_id;
    out->points[p].flags = (sprpl89_u16)(spr->flags | SP89_FLAG_RASTER_LAYER);
    out->points[p].render_queue = spr->render_queue;
    out->points[p].blend_mode = spr->blend_mode;
    out->points[p].owner_sprite = owner;
    (void)frame_id;
    return SP89_OK;
}

static int sprpl89_local_to_world_quad(sprpl89_ctx *ctx, const sprpl89_sprite *spr, sprpl89_vec3 right, sprpl89_vec3 up,
                                    sprpl89_fx lx0, sprpl89_fx ly0, sprpl89_fx lx1, sprpl89_fx ly1,
                                    sprpl89_vec3 *p0, sprpl89_vec3 *p1, sprpl89_vec3 *p2, sprpl89_vec3 *p3)
{
    sprpl89_vec3 center;
    center = spr->pos;
    center.x += spr->offset_x;
    center.y += spr->offset_y;
    center.z += spr->offset_z;
    if (spr->depth_bias != 0) center = sprpl89_v3_add(center, sprpl89_v3_scale(ctx->camera.forward, spr->depth_bias));

    *p0 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, lx0), sprpl89_v3_scale(up, ly0)));
    *p1 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, lx1), sprpl89_v3_scale(up, ly0)));
    *p2 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, lx1), sprpl89_v3_scale(up, ly1)));
    *p3 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, lx0), sprpl89_v3_scale(up, ly1)));
    return SP89_OK;
}

static int sprpl89_emit_local_quad(sprpl89_ctx *ctx, sprpl89_emit *out, const sprpl89_sprite *spr, sprpl89_vec3 right, sprpl89_vec3 up,
                                sprpl89_fx x0, sprpl89_fx y0, sprpl89_fx x1, sprpl89_fx y1,
                                sprpl89_fx u0, sprpl89_fx v0, sprpl89_fx u1, sprpl89_fx v1,
                                sprpl89_color color, sprpl89_u16 material_id, sprpl89_u16 page_id, sprpl89_u16 flags,
                                sprpl89_u16 queue, sprpl89_u16 blend, sprpl89_u16 owner, sprpl89_u16 family)
{
    sprpl89_vec3 p0, p1, p2, p3;
    sprpl89_local_to_world_quad(ctx, spr, right, up, x0, y0, x1, y1, &p0, &p1, &p2, &p3);
    return sprpl89_emit_quad_ex(out, p0, p1, p2, p3, u0, v0, u1, v1, color, material_id, page_id, flags, queue, blend, owner, family);
}

static int sprpl89_emit_vector_cmd(sprpl89_ctx *ctx, sprpl89_emit *out, const sprpl89_sprite *spr, int owner, const sprpl89_vector_cmd *cmd, sprpl89_vec3 right, sprpl89_vec3 up)
{
    sprpl89_vec3 p0, p1, p2, p3;
    sprpl89_fx hx, hy;
    sprpl89_fx dx, dy, len;
    sprpl89_fx nx, ny;
    sprpl89_vec3 center;
    sprpl89_u16 flags;
    int i;
    static const int cx[12] = { 65536, 56756, 32768, 0, -32768, -56756, -65536, -56756, -32768, 0, 32768, 56756 };
    static const int cy[12] = { 0, 32768, 56756, 65536, 56756, 32768, 0, -32768, -56756, -65536, -56756, -32768 };
    flags = (sprpl89_u16)(spr->flags | SP89_FLAG_VECTOR_LAYER);

    if (cmd->kind == SP89_VCMD_RECT) {
        return sprpl89_emit_local_quad(ctx, out, spr, right, up, cmd->x0, cmd->y0, cmd->x1, cmd->y1,
            0, 0, SP89_FX_ONE, SP89_FX_ONE, cmd->color, cmd->material_id, 0, flags, spr->render_queue, spr->blend_mode, (sprpl89_u16)owner, SP89_MAT_VECTOR);
    } else if (cmd->kind == SP89_VCMD_CROSS) {
        sprpl89_add_vector_line(ctx, owner, cmd->x0 - cmd->x1, cmd->y0, cmd->x0 + cmd->x1, cmd->y0, cmd->thickness, cmd->color);
        sprpl89_add_vector_line(ctx, owner, cmd->x0, cmd->y0 - cmd->x1, cmd->x0, cmd->y0 + cmd->x1, cmd->thickness, cmd->color);
        return SP89_OK;
    } else if (cmd->kind == SP89_VCMD_LINE) {
        dx = cmd->x1 - cmd->x0;
        dy = cmd->y1 - cmd->y0;
        len = sprpl89_fx_sqrt(sprpl89_fx_mul(dx, dx) + sprpl89_fx_mul(dy, dy));
        if (len <= 0) return SP89_OK;
        nx = -sprpl89_fx_div(dy, len);
        ny = sprpl89_fx_div(dx, len);
        hx = sprpl89_fx_mul(nx, cmd->thickness / 2);
        hy = sprpl89_fx_mul(ny, cmd->thickness / 2);
        center = spr->pos; center.x += spr->offset_x; center.y += spr->offset_y; center.z += spr->offset_z;
        p0 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x0 + hx), sprpl89_v3_scale(up, cmd->y0 + hy)));
        p1 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x1 + hx), sprpl89_v3_scale(up, cmd->y1 + hy)));
        p2 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x1 - hx), sprpl89_v3_scale(up, cmd->y1 - hy)));
        p3 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x0 - hx), sprpl89_v3_scale(up, cmd->y0 - hy)));
        return sprpl89_emit_quad_ex(out, p0, p1, p2, p3, 0, 0, SP89_FX_ONE, SP89_FX_ONE, cmd->color, cmd->material_id, 0, flags, spr->render_queue, spr->blend_mode, (sprpl89_u16)owner, SP89_MAT_VECTOR);
    } else if (cmd->kind == SP89_VCMD_TRI) {
        center = spr->pos; center.x += spr->offset_x; center.y += spr->offset_y; center.z += spr->offset_z;
        p0 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x0), sprpl89_v3_scale(up, cmd->y0)));
        p1 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x1), sprpl89_v3_scale(up, cmd->y1)));
        p2 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x2), sprpl89_v3_scale(up, cmd->y2)));
        return sprpl89_emit_tri_ex(out, p0, p1, p2, cmd->color, cmd->material_id, 0, flags, spr->render_queue, spr->blend_mode, (sprpl89_u16)owner, SP89_MAT_VECTOR);
    } else if (cmd->kind == SP89_VCMD_CIRCLE) {
        center = spr->pos; center.x += spr->offset_x; center.y += spr->offset_y; center.z += spr->offset_z;
        for (i = 0; i < 12; ++i) {
            p0 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x0), sprpl89_v3_scale(up, cmd->y0)));
            p1 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x0 + sprpl89_fx_mul(cmd->x1, (sprpl89_fx)cx[i])), sprpl89_v3_scale(up, cmd->y0 + sprpl89_fx_mul(cmd->x1, (sprpl89_fx)cy[i]))));
            p2 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, cmd->x0 + sprpl89_fx_mul(cmd->x1, (sprpl89_fx)cx[(i + 1) % 12])), sprpl89_v3_scale(up, cmd->y0 + sprpl89_fx_mul(cmd->x1, (sprpl89_fx)cy[(i + 1) % 12]))));
            if (sprpl89_emit_tri_ex(out, p0, p1, p2, cmd->color, cmd->material_id, 0, flags, spr->render_queue, spr->blend_mode, (sprpl89_u16)owner, SP89_MAT_VECTOR) != SP89_OK) return SP89_ERR_FULL;
        }
    }
    return SP89_OK;
}

static sprpl89_fx sprpl89_text_width(sprpl89_ctx *ctx, const sprpl89_text_cmd *cmd)
{
    int i;
    sprpl89_fx x;
    sprpl89_font *font;
    sprpl89_glyph *g;
    unsigned char ch;
    if (!ctx || !cmd) return 0;
    font = &ctx->fonts[cmd->font_id];
    x = 0;
    for (i = 0; i < SP89_MAX_TEXT_CHARS && cmd->text[i] != '\0'; ++i) {
        ch = (unsigned char)cmd->text[i];
        if (ch == '\n') break;
        if (ch == ' ') { x += sprpl89_fx_mul(sprpl89_fx_from_int(font->space_advance), cmd->scale_x); continue; }
        g = sprpl89_find_glyph(font, (int)ch);
        if (!g) g = sprpl89_find_glyph(font, (int)font->missing_glyph);
        if (g) x += sprpl89_fx_mul(g->advance_x, cmd->scale_x);
    }
    return x;
}

static int sprpl89_emit_text_cmd(sprpl89_ctx *ctx, sprpl89_emit *out, const sprpl89_sprite *spr, int owner, const sprpl89_text_cmd *cmd, sprpl89_vec3 right, sprpl89_vec3 up)
{
    int i;
    sprpl89_font *font;
    sprpl89_glyph *g;
    sprpl89_atlas_frame *f;
    sprpl89_fx pen_x, pen_y, start_x, tw;
    sprpl89_fx gx0, gy0, gx1, gy1;
    unsigned char ch;
    sprpl89_u16 flags;
    if (!ctx || !out || !spr || !cmd) return SP89_ERR_BADARG;
    if (cmd->font_id >= SP89_MAX_FONTS || !ctx->fonts[cmd->font_id].active) return SP89_ERR_BADARG;
    font = &ctx->fonts[cmd->font_id];
    flags = (sprpl89_u16)(spr->flags | SP89_FLAG_TEXT_LAYER);
    tw = sprpl89_text_width(ctx, cmd);
    start_x = cmd->x;
    if (cmd->align == SP89_TEXT_CENTER) start_x -= tw / 2;
    else if (cmd->align == SP89_TEXT_RIGHT) start_x -= tw;
    pen_x = start_x;
    pen_y = cmd->y;
    for (i = 0; i < SP89_MAX_TEXT_CHARS && cmd->text[i] != '\0'; ++i) {
        ch = (unsigned char)cmd->text[i];
        if (ch == '\n') {
            pen_x = start_x;
            pen_y -= sprpl89_fx_mul(sprpl89_fx_from_int(font->line_height), cmd->scale_y);
            continue;
        }
        if (ch == ' ') {
            pen_x += sprpl89_fx_mul(sprpl89_fx_from_int(font->space_advance), cmd->scale_x);
            continue;
        }
        g = sprpl89_find_glyph(font, (int)ch);
        if (!g) g = sprpl89_find_glyph(font, (int)font->missing_glyph);
        if (!g || g->frame_id >= ctx->frame_count) continue;
        f = &ctx->frames[g->frame_id];
        gx0 = pen_x + sprpl89_fx_mul(g->bearing_x, cmd->scale_x);
        gy0 = pen_y + sprpl89_fx_mul(g->bearing_y, cmd->scale_y);
        gx1 = gx0 + sprpl89_fx_mul(g->width, cmd->scale_x);
        gy1 = gy0 + sprpl89_fx_mul(g->height, cmd->scale_y);
        if (sprpl89_emit_local_quad(ctx, out, spr, right, up, gx0, gy0, gx1, gy1, f->u0, f->v0, f->u1, f->v1, cmd->color, cmd->material_id, f->page_id, flags, spr->render_queue, spr->blend_mode, (sprpl89_u16)owner, SP89_MAT_TEXT) != SP89_OK) return SP89_ERR_FULL;
        pen_x += sprpl89_fx_mul(g->advance_x, cmd->scale_x);
    }
    return SP89_OK;
}

static int sprpl89_emit_panel_cmd(sprpl89_ctx *ctx, sprpl89_emit *out, const sprpl89_sprite *spr, int owner, const sprpl89_panel_cmd *cmd, sprpl89_vec3 right, sprpl89_vec3 up)
{
    sprpl89_atlas_frame *f;
    sprpl89_fx xs[4], ys[4], us[4], vs[4];
    int ix, iy;
    sprpl89_u16 flags;
    if (!ctx || !out || !spr || !cmd || cmd->frame_id >= ctx->frame_count) return SP89_ERR_BADARG;
    f = &ctx->frames[cmd->frame_id];
    flags = (sprpl89_u16)(spr->flags | SP89_FLAG_PANEL_LAYER);
    xs[0] = cmd->x0; xs[1] = cmd->x0 + cmd->border_l; xs[2] = cmd->x1 - cmd->border_r; xs[3] = cmd->x1;
    ys[0] = cmd->y0; ys[1] = cmd->y0 + cmd->border_b; ys[2] = cmd->y1 - cmd->border_t; ys[3] = cmd->y1;
    us[0] = f->u0; us[1] = f->u0 + (f->u1 - f->u0) / 3; us[2] = f->u0 + ((f->u1 - f->u0) * 2) / 3; us[3] = f->u1;
    vs[0] = f->v0; vs[1] = f->v0 + (f->v1 - f->v0) / 3; vs[2] = f->v0 + ((f->v1 - f->v0) * 2) / 3; vs[3] = f->v1;
    for (iy = 0; iy < 3; ++iy) {
        for (ix = 0; ix < 3; ++ix) {
            if (sprpl89_emit_local_quad(ctx, out, spr, right, up, xs[ix], ys[iy], xs[ix + 1], ys[iy + 1], us[ix], vs[iy], us[ix + 1], vs[iy + 1], cmd->color, cmd->material_id, f->page_id, flags, spr->render_queue, spr->blend_mode, (sprpl89_u16)owner, SP89_MAT_PANEL) != SP89_OK) return SP89_ERR_FULL;
        }
    }
    return SP89_OK;
}

static int sprpl89_emit_shadow_cmd(sprpl89_ctx *ctx, sprpl89_emit *out, const sprpl89_sprite *spr, int owner, const sprpl89_shadow_cmd *cmd)
{
    sprpl89_atlas_frame *f;
    sprpl89_vec3 p0, p1, p2, p3;
    sprpl89_vec3 center;
    sprpl89_vec3 right;
    sprpl89_vec3 forward;
    sprpl89_u16 flags;
    if (!ctx || !out || !spr || !cmd || cmd->frame_id >= ctx->frame_count) return SP89_ERR_BADARG;
    f = &ctx->frames[cmd->frame_id];
    flags = (sprpl89_u16)(SP89_FLAG_ALPHA_BLEND | SP89_FLAG_DEPTH_TEST | SP89_FLAG_SHADOW_LAYER);
    center = spr->pos;
    center.y += cmd->y_offset;
    right = sprpl89_v3(SP89_FX_ONE, 0, 0);
    forward = sprpl89_v3(0, 0, SP89_FX_ONE);
    p0 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, -cmd->radius_x), sprpl89_v3_scale(forward, -cmd->radius_z)));
    p1 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right,  cmd->radius_x), sprpl89_v3_scale(forward, -cmd->radius_z)));
    p2 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right,  cmd->radius_x), sprpl89_v3_scale(forward,  cmd->radius_z)));
    p3 = sprpl89_v3_add(center, sprpl89_v3_add(sprpl89_v3_scale(right, -cmd->radius_x), sprpl89_v3_scale(forward,  cmd->radius_z)));
    return sprpl89_emit_quad_ex(out, p0, p1, p2, p3, f->u0, f->v0, f->u1, f->v1, cmd->color, cmd->material_id, f->page_id, flags, SP89_QUEUE_MULTIPLY, SP89_BLEND_MULTIPLY, (sprpl89_u16)owner, SP89_MAT_SHADOW);
}

int sprpl89_emit_sprite(sprpl89_ctx *ctx, int sprite_index, sprpl89_emit *out)
{
    sprpl89_sprite original;
    sprpl89_sprite local_spr;
    sprpl89_atlas_frame f;
    sprpl89_vec3 right, up, right2;
    sprpl89_vec3 p0, p1, p2, p3;
    sprpl89_fx w, h;
    sprpl89_fx ax, ay;
    sprpl89_fx lx0, lx1, ly0, ly1;
    sprpl89_fx fade;
    int frame_id;
    int i;
    int err;
    int first_vert, first_tri;
    if (!ctx || !out) return SP89_ERR_BADARG;
    if (sprite_index < 0 || sprite_index >= SP89_MAX_SPRITES) return SP89_ERR_RANGE;
    if (!ctx->sprites[sprite_index].active || !(ctx->sprites[sprite_index].flags & SP89_FLAG_VISIBLE)) return SP89_OK;
    original = ctx->sprites[sprite_index];
    if (sprpl89_should_cull(ctx, sprite_index, &original)) {
        out->culled_sprites++;
        return SP89_ERR_CULLED;
    }
    local_spr = original;
    if (ctx->hooks.mutate_sprite) ctx->hooks.mutate_sprite(ctx, sprite_index, &local_spr, ctx->hooks.user);

    frame_id = (int)sprpl89_frame_for_sprite(ctx, &local_spr);
    if (ctx->hooks.resolve_frame) {
        int hf = ctx->hooks.resolve_frame(ctx, sprite_index, &local_spr, (sprpl89_u16)frame_id, ctx->hooks.user);
        if (hf >= 0) frame_id = hf;
    }
    if (frame_id < 0 || frame_id >= ctx->frame_count) {
        f.u0 = 0; f.v0 = 0; f.u1 = SP89_FX_ONE; f.v1 = SP89_FX_ONE;
        f.pivot_x = SP89_FX_HALF; f.pivot_y = SP89_FX_HALF;
        f.logical_w = local_spr.width; f.logical_h = local_spr.height;
        f.page_id = local_spr.texture_page; f.user_id = 0;
    } else {
        f = ctx->frames[frame_id];
    }

    if (local_spr.xflags & SP89_XFLAG_DISTANCE_FADE) {
        int d2 = sprpl89_sprite_distance2_to_camera(ctx, &local_spr);
        (void)d2;
    }

    if (ctx->hooks.depth_fade) {
        fade = ctx->hooks.depth_fade(ctx, sprite_index, local_spr.pos, (sprpl89_fx)sprpl89_sprite_distance2_to_camera(ctx, &local_spr), ctx->hooks.user);
        local_spr.color = sprpl89_rgba_mod_alpha(local_spr.color, fade);
    }

    if (local_spr.xflags & SP89_XFLAG_POINT_COMMAND) {
        err = sprpl89_emit_point_cmd(out, &local_spr, (sprpl89_u16)frame_id, &f, (sprpl89_u16)sprite_index);
        if (err != SP89_OK) { out->dropped_point_cmds++; return err; }
        return SP89_OK;
    }

    sprpl89_basis_for_sprite(ctx, &local_spr, &right, &up);
    w = sprpl89_fx_mul(local_spr.width, local_spr.scale_x);
    h = sprpl89_fx_mul(local_spr.height, local_spr.scale_y);

    if (local_spr.anchor_mode == SP89_ANCHOR_CENTER) {
        ax = SP89_FX_HALF; ay = SP89_FX_HALF;
    } else if (local_spr.anchor_mode == SP89_ANCHOR_BOTTOM_CENTER || local_spr.anchor_mode == SP89_ANCHOR_FEET_90) {
        ax = SP89_FX_HALF; ay = 0;
    } else if (local_spr.anchor_mode == SP89_ANCHOR_TOP_CENTER) {
        ax = SP89_FX_HALF; ay = SP89_FX_ONE;
    } else if (local_spr.anchor_mode == SP89_ANCHOR_CUSTOM) {
        ax = sprpl89_fx_clamp(local_spr.custom_anchor_x, 0, SP89_FX_ONE);
        ay = sprpl89_fx_clamp(local_spr.custom_anchor_y, 0, SP89_FX_ONE);
    } else {
        ax = f.pivot_x; ay = f.pivot_y;
    }

    lx0 = -sprpl89_fx_mul(w, ax);
    lx1 = lx0 + w;
    ly0 = -sprpl89_fx_mul(h, ay);
    ly1 = ly0 + h;
    if (local_spr.anchor_mode == SP89_ANCHOR_FEET_90) {
        ly0 += sprpl89_fx_from_int(1) / 64;
        ly1 += sprpl89_fx_from_int(1) / 64;
    }

    first_vert = out->vert_count;
    first_tri = out->tri_count;

    if (!(local_spr.flags & SP89_FLAG_NO_RASTER)) {
        err = sprpl89_local_to_world_quad(ctx, &local_spr, right, up, lx0, ly0, lx1, ly1, &p0, &p1, &p2, &p3);
        if (err != SP89_OK) return err;
        err = sprpl89_emit_quad_ex(out, p0, p1, p2, p3, f.u0, f.v0, f.u1, f.v1, local_spr.color, local_spr.material_id, f.page_id, (sprpl89_u16)(local_spr.flags | SP89_FLAG_RASTER_LAYER), local_spr.render_queue, local_spr.blend_mode, (sprpl89_u16)sprite_index, SP89_MAT_RASTER);
        if (err != SP89_OK) {
            out->dropped_sprites++;
            return err;
        }
        if (local_spr.billboard_mode == SP89_BILLBOARD_CROSSED_Y) {
            right2 = sprpl89_v3_cross(up, right);
            right2 = sprpl89_v3_normalize(right2, ctx->camera.forward);
            err = sprpl89_local_to_world_quad(ctx, &local_spr, right2, up, lx0, ly0, lx1, ly1, &p0, &p1, &p2, &p3);
            if (err == SP89_OK) err = sprpl89_emit_quad_ex(out, p0, p1, p2, p3, f.u0, f.v0, f.u1, f.v1, local_spr.color, local_spr.material_id, f.page_id, (sprpl89_u16)(local_spr.flags | SP89_FLAG_RASTER_LAYER), local_spr.render_queue, local_spr.blend_mode, (sprpl89_u16)sprite_index, SP89_MAT_RASTER);
            if (err != SP89_OK) out->dropped_sprites++;
        }
    }

    for (i = 0; i < ctx->panel_count; ++i) {
        if (ctx->panel_cmds[i].active && ctx->panel_cmds[i].sprite_index == (sprpl89_u16)sprite_index) {
            err = sprpl89_emit_panel_cmd(ctx, out, &local_spr, sprite_index, &ctx->panel_cmds[i], right, up);
            if (err != SP89_OK) out->dropped_panel_cmds++;
        }
    }

    for (i = 0; i < ctx->vector_count; ++i) {
        if (ctx->vector_cmds[i].active && ctx->vector_cmds[i].sprite_index == (sprpl89_u16)sprite_index) {
            err = sprpl89_emit_vector_cmd(ctx, out, &local_spr, sprite_index, &ctx->vector_cmds[i], right, up);
            if (err != SP89_OK) out->dropped_vector_cmds++;
        }
    }

    for (i = 0; i < ctx->text_count; ++i) {
        if (ctx->text_cmds[i].active && ctx->text_cmds[i].sprite_index == (sprpl89_u16)sprite_index) {
            err = sprpl89_emit_text_cmd(ctx, out, &local_spr, sprite_index, &ctx->text_cmds[i], right, up);
            if (err != SP89_OK) out->dropped_text_cmds++;
        }
    }

    for (i = 0; i < ctx->shadow_count; ++i) {
        if (ctx->shadow_cmds[i].active && ctx->shadow_cmds[i].sprite_index == (sprpl89_u16)sprite_index) {
            err = sprpl89_emit_shadow_cmd(ctx, out, &local_spr, sprite_index, &ctx->shadow_cmds[i]);
            if (err != SP89_OK) out->dropped_shadow_cmds++;
        }
    }

    if (ctx->hooks.post_emit) {
        ctx->hooks.post_emit(ctx, sprite_index, first_vert, out->vert_count - first_vert, first_tri, out->tri_count - first_tri, ctx->hooks.user);
    }
    return SP89_OK;
}

int sprpl89_emit_all_sorted(sprpl89_ctx *ctx, sprpl89_emit *out)
{
    int count, i, err;
    if (!ctx || !out) return SP89_ERR_BADARG;
    count = sprpl89_build_sort(ctx);
    sprpl89_sort_for_render(ctx, count);
    for (i = 0; i < count; ++i) {
        err = sprpl89_emit_sprite(ctx, (int)ctx->sort_items[i].sprite_index, out);
        if (err != SP89_OK && err != SP89_ERR_FULL && err != SP89_ERR_CULLED) return err;
    }
    return SP89_OK;
}

int sprpl89_pick_sprite_ray(sprpl89_ctx *ctx, int sprite_index, sprpl89_vec3 ray_origin, sprpl89_vec3 ray_dir, sprpl89_pick_hit *hit)
{
    sprpl89_sprite *spr;
    sprpl89_atlas_frame f;
    sprpl89_vec3 right, up, normal, center, rel, hp;
    sprpl89_fx denom, t, lx, ly, w, h, ax, ay;
    int frame_id;
    if (!ctx || !hit) return SP89_ERR_BADARG;
    hit->hit = 0;
    spr = sprpl89_get_sprite(ctx, sprite_index);
    if (!spr || !(spr->flags & SP89_FLAG_VISIBLE)) return SP89_OK;
    frame_id = (int)sprpl89_frame_for_sprite(ctx, spr);
    if (frame_id >= 0 && frame_id < ctx->frame_count) f = ctx->frames[frame_id];
    else { f.pivot_x = SP89_FX_HALF; f.pivot_y = SP89_FX_HALF; f.u0 = 0; f.v0 = 0; f.u1 = SP89_FX_ONE; f.v1 = SP89_FX_ONE; f.page_id = 0; f.user_id = 0; f.logical_w = 0; f.logical_h = 0; }
    sprpl89_basis_for_sprite(ctx, spr, &right, &up);
    normal = sprpl89_v3_cross(right, up);
    normal = sprpl89_v3_normalize(normal, ctx->camera.forward);
    denom = sprpl89_v3_dot(normal, ray_dir);
    if (sprpl89_fx_abs(denom) < 16) return SP89_OK;
    center = spr->pos;
    center.x += spr->offset_x; center.y += spr->offset_y; center.z += spr->offset_z;
    t = sprpl89_fx_div(sprpl89_v3_dot(normal, sprpl89_v3_sub(center, ray_origin)), denom);
    if (t < 0) return SP89_OK;
    hp = sprpl89_v3_add(ray_origin, sprpl89_v3_scale(ray_dir, t));
    rel = sprpl89_v3_sub(hp, center);
    lx = sprpl89_v3_dot(rel, right);
    ly = sprpl89_v3_dot(rel, up);
    w = sprpl89_fx_mul(spr->width, spr->scale_x);
    h = sprpl89_fx_mul(spr->height, spr->scale_y);
    if (spr->anchor_mode == SP89_ANCHOR_CENTER) { ax = SP89_FX_HALF; ay = SP89_FX_HALF; }
    else if (spr->anchor_mode == SP89_ANCHOR_BOTTOM_CENTER || spr->anchor_mode == SP89_ANCHOR_FEET_90) { ax = SP89_FX_HALF; ay = 0; }
    else if (spr->anchor_mode == SP89_ANCHOR_TOP_CENTER) { ax = SP89_FX_HALF; ay = SP89_FX_ONE; }
    else if (spr->anchor_mode == SP89_ANCHOR_CUSTOM) { ax = spr->custom_anchor_x; ay = spr->custom_anchor_y; }
    else { ax = f.pivot_x; ay = f.pivot_y; }
    lx += sprpl89_fx_mul(w, ax);
    ly += sprpl89_fx_mul(h, ay);
    if (lx < 0 || ly < 0 || lx > w || ly > h) return SP89_OK;
    hit->hit = 1;
    hit->sprite_index = sprite_index;
    hit->t = t;
    hit->local_x = lx;
    hit->local_y = ly;
    hit->uv_x = w == 0 ? 0 : sprpl89_fx_div(lx, w);
    hit->uv_y = h == 0 ? 0 : sprpl89_fx_div(ly, h);
    hit->world_pos = hp;
    return SP89_OK;
}

int sprpl89_pick_all_ray(sprpl89_ctx *ctx, sprpl89_vec3 ray_origin, sprpl89_vec3 ray_dir, sprpl89_pick_hit *hit)
{
    int i;
    sprpl89_pick_hit tmp;
    sprpl89_fx best_t;
    if (!ctx || !hit) return SP89_ERR_BADARG;
    hit->hit = 0;
    best_t = 0x7fffffff;
    for (i = 0; i < ctx->sprite_count; ++i) {
        if (sprpl89_pick_sprite_ray(ctx, i, ray_origin, ray_dir, &tmp) == SP89_OK && tmp.hit) {
            if (!hit->hit || tmp.t < best_t) {
                *hit = tmp;
                best_t = tmp.t;
            }
        }
    }
    return SP89_OK;
}

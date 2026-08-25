#include "blank3d_projectile_sprite89.h"

#include "../vendor/staticsprite89/include/staticsprite89.h"
#include "../vendor/imagesequencer89/include/imagesequencer89.h"
#include "../vendor/renlist89/include/renlist89.h"
#include "../vendor/renlist89/adapters/imagesequencer89/renlist89_imagesequencer89.h"
#include "../vendor/tilecell89/include/tilecell89.h"
#include "../vendor/gmspritestrip89/include/gmspritestrip89.h"

#include <stdio.h>
#include <string.h>

#define B3D_PSPR89_TEXT_CAP (128U * 1024U)

/* Large parser/vendor contexts are reusable load-time scratch. They are not
   copied per weapon or projectile and therefore keep runtime memory bounded. */
static ImageSequencer89 b3d_pspr89_seq_scratch;
static RenList89 b3d_pspr89_ren_scratch;
static TileCell89 b3d_pspr89_tile_scratch;
static GMSpritestrip89 b3d_pspr89_gm_scratch;
static char b3d_pspr89_text[B3D_PSPR89_TEXT_CAP];

static void b3d_pspr89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    if (!src) src = "";
    i = 0U;
    while (src[i] && i + 1U < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void b3d_pspr89_set_status(Blank3DProjectileSpriteClip89 *clip,
                                  const char *text)
{
    if (!clip) return;
    b3d_pspr89_copy(clip->status, B3D_PSPR89_STATUS_CAP, text);
}

static int b3d_pspr89_read_text(const char *path, unsigned int *out_size)
{
    FILE *file;
    size_t got;
    int extra;
    if (out_size) *out_size = 0U;
    if (!path || !path[0]) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    got = fread(b3d_pspr89_text, 1U, (size_t)(B3D_PSPR89_TEXT_CAP - 1U), file);
    extra = fgetc(file);
    fclose(file);
    if (extra != EOF) return 0;
    b3d_pspr89_text[got] = '\0';
    if (out_size) *out_size = (unsigned int)got;
    return 1;
}

static int b3d_pspr89_resolve_data(const Blank3DProjectileSpriteRuntime89 *runtime,
                                   const char *request,
                                   char *out_path, unsigned int out_cap)
{
    if (!runtime || !request || !request[0] || !out_path || out_cap == 0U)
        return 0;
    if (runtime->routes &&
        ar89_resolve_request(runtime->routes, AR89_KIND_DATA, request,
                             out_path, (ar89_u32)out_cap)) return 1;
    b3d_pspr89_copy(out_path, out_cap, request);
    return 1;
}

static Blank3DProjectileSpriteClip89 *b3d_pspr89_find(
    Blank3DProjectileSpriteRuntime89 *runtime, int weapon_id)
{
    int i;
    if (!runtime || weapon_id <= 0) return 0;
    for (i = 0; i < B3D_PSPR89_CACHE_CAP; ++i)
        if (runtime->clips[i].used && runtime->clips[i].weapon_id == weapon_id)
            return &runtime->clips[i];
    return 0;
}

static Blank3DProjectileSpriteClip89 *b3d_pspr89_alloc(
    Blank3DProjectileSpriteRuntime89 *runtime, int weapon_id)
{
    int i;
    Blank3DProjectileSpriteClip89 *clip;
    clip = b3d_pspr89_find(runtime, weapon_id);
    if (clip) return clip;
    if (!runtime) return 0;
    for (i = 0; i < B3D_PSPR89_CACHE_CAP; ++i) {
        if (runtime->clips[i].used) continue;
        clip = &runtime->clips[i];
        memset(clip, 0, sizeof(*clip));
        clip->used = 1;
        clip->weapon_id = weapon_id;
        return clip;
    }
    return 0;
}

static int b3d_pspr89_copy_is89(Blank3DProjectileSpriteClip89 *clip,
                                const ImageSequencer89 *seqs,
                                is89_id sequence_id)
{
    const IS89_Sequence *sequence;
    unsigned int i;
    unsigned int total;
    if (!clip || !seqs || sequence_id == IS89_INVALID_ID ||
        sequence_id >= seqs->sequence_count) return 0;
    sequence = &seqs->sequences[sequence_id];
    if (!sequence->used || sequence->frame_count == 0U ||
        sequence->frame_count > B3D_PSPR89_FRAME_CAP) return 0;
    total = 0U;
    for (i = 0U; i < (unsigned int)sequence->frame_count; ++i) {
        const IS89_Frame *src;
        Blank3DProjectileSpriteFrame89 *dst;
        unsigned int index;
        index = (unsigned int)sequence->first_frame + i;
        if (index >= (unsigned int)seqs->frame_count) return 0;
        src = &seqs->frames[index];
        dst = &clip->frames[i];
        memset(dst, 0, sizeof(*dst));
        b3d_pspr89_copy(dst->image_request,
                        (unsigned int)sizeof(dst->image_request),
                        src->image_request);
        dst->source_enabled = (src->flags & IS89_FRAME_SOURCE_RECT) ? 1 : 0;
        dst->source_x = (int)src->source_x;
        dst->source_y = (int)src->source_y;
        dst->source_w = (int)src->source_w;
        dst->source_h = (int)src->source_h;
        dst->scale_x_q16 = (long)src->scale_x_q16;
        dst->scale_y_q16 = (long)src->scale_y_q16;
        dst->duration_ms = src->duration_ms ? src->duration_ms : 1U;
        dst->flags =
            ((src->flags & IS89_FRAME_FLIP_X) ? B3D_PSPR89_FLIP_X : 0U) |
            ((src->flags & IS89_FRAME_FLIP_Y) ? B3D_PSPR89_FLIP_Y : 0U);
        if (0xFFFFFFFFU - total < dst->duration_ms) total = 0xFFFFFFFFU;
        else total += dst->duration_ms;
    }
    clip->frame_count = sequence->frame_count;
    clip->loop_mode = (int)sequence->loop_mode;
    clip->total_ms = total ? total : 1U;
    return 1;
}

static int b3d_pspr89_compile_static(Blank3DProjectileSpriteClip89 *clip,
                                     const Blank3DWeaponModules *modules)
{
    SS89_StaticSprite sprite;
    SS89_Sample sample;
    Blank3DProjectileSpriteFrame89 *frame;
    if (!clip || !modules || !modules->projectile_visual_image[0]) return 0;
    ss89_init(&sprite);
    if (!ss89_set_image(&sprite, modules->projectile_visual_image) ||
        !ss89_sample(&sprite, &sample)) return 0;
    frame = &clip->frames[0];
    memset(frame, 0, sizeof(*frame));
    b3d_pspr89_copy(frame->image_request,
                    (unsigned int)sizeof(frame->image_request),
                    sample.image_request);
    frame->source_enabled = sample.source_enabled ? 1 : 0;
    frame->source_x = (int)sample.source.x;
    frame->source_y = (int)sample.source.y;
    frame->source_w = (int)sample.source.w;
    frame->source_h = (int)sample.source.h;
    frame->scale_x_q16 = sample.scale_q16[0];
    frame->scale_y_q16 = sample.scale_q16[1];
    frame->duration_ms = modules->projectile_visual_frame_ms
        ? modules->projectile_visual_frame_ms : 1000U;
    frame->flags =
        ((sample.flags & SS89_FLAG_FLIP_X) ? B3D_PSPR89_FLIP_X : 0U) |
        ((sample.flags & SS89_FLAG_FLIP_Y) ? B3D_PSPR89_FLIP_Y : 0U);
    clip->frame_count = 1U;
    clip->loop_mode = GWM89_PROJECTILE_SPRITE_LOOP_HOLD;
    clip->total_ms = frame->duration_ms;
    return 1;
}

static int b3d_pspr89_token(const char **cursor, char *out, unsigned int cap)
{
    const char *p;
    unsigned int n;
    char quote;
    if (!cursor || !*cursor || !out || cap == 0U) return 0;
    p = *cursor;
    while (*p == ' ' || *p == '\t') ++p;
    if (!*p || *p == '#' || *p == ';') { out[0] = '\0'; *cursor = p; return 0; }
    n = 0U;
    quote = 0;
    if (*p == '"' || *p == '\'') { quote = *p; ++p; }
    while (*p) {
        if (quote) {
            if (*p == quote) { ++p; break; }
        } else if (*p == ' ' || *p == '\t' || *p == '#' || *p == ';') break;
        if (n + 1U < cap) out[n++] = *p;
        ++p;
    }
    out[n] = '\0';
    while (*p == ' ' || *p == '\t') ++p;
    *cursor = p;
    return n > 0U;
}

static unsigned int b3d_pspr89_uint(const char *text, unsigned int fallback)
{
    unsigned int value;
    unsigned int i;
    if (!text || !text[0]) return fallback;
    value = 0U;
    i = 0U;
    while (text[i]) {
        if (text[i] < '0' || text[i] > '9') return fallback;
        if (value > 429496729U) return fallback;
        value = value * 10U + (unsigned int)(text[i] - '0');
        ++i;
    }
    return value;
}

static int b3d_pspr89_int(const char *text, int fallback)
{
    int sign;
    unsigned int value;
    if (!text || !text[0]) return fallback;
    sign = 1;
    if (*text == '-') { sign = -1; ++text; }
    else if (*text == '+') ++text;
    value = b3d_pspr89_uint(text, 0xFFFFFFFFU);
    if (value == 0xFFFFFFFFU || value > 2147483647U) return fallback;
    return sign < 0 ? -(int)value : (int)value;
}

static int b3d_pspr89_loop_word(const char *text, int fallback)
{
    if (!text) return fallback;
    if (!strcmp(text, "none") || !strcmp(text, "once")) return IS89_LOOP_NONE;
    if (!strcmp(text, "loop") || !strcmp(text, "forward")) return IS89_LOOP_FORWARD;
    if (!strcmp(text, "reverse")) return IS89_LOOP_REVERSE;
    if (!strcmp(text, "pingpong") || !strcmp(text, "ping-pong")) return IS89_LOOP_PINGPONG;
    if (!strcmp(text, "hold") || !strcmp(text, "hold_last")) return IS89_LOOP_HOLD;
    return fallback;
}

/* Minimal loose-image sequence recipe. RenList89 remains the richer DSL.\n\n   sequence flame\n   loop forward\n   frame "fire_001" 33\n   frame "fire_002" 33\n   rect "sheet" 0 0 64 64 40\n*/
static int b3d_pspr89_parse_sequence(const char *text,
                                     ImageSequencer89 *seqs,
                                     is89_id *out_sequence)
{
    const char *p;
    char line[512];
    unsigned int ln;
    is89_id seq;
    int have_seq;
    if (!text || !seqs) return 0;
    is89_init(seqs);
    seq = IS89_INVALID_ID;
    have_seq = 0;
    p = text;
    while (*p) {
        const char *cur;
        char cmd[48];
        char a[260];
        char b[48];
        char c[48];
        char d[48];
        char e[48];
        char f[48];
        ln = 0U;
        while (*p && *p != '\n' && *p != '\r') {
            if (ln + 1U < (unsigned int)sizeof(line)) line[ln++] = *p;
            ++p;
        }
        line[ln] = '\0';
        while (*p == '\n' || *p == '\r') ++p;
        cur = line;
        if (!b3d_pspr89_token(&cur, cmd, sizeof(cmd))) continue;
        if (!strcmp(cmd, "sequence")) {
            if (!b3d_pspr89_token(&cur, a, sizeof(a))) b3d_pspr89_copy(a, sizeof(a), "default");
            seq = is89_sequence_begin(seqs, a);
            if (seq == IS89_INVALID_ID) return 0;
            have_seq = 1;
        } else if (!strcmp(cmd, "loop")) {
            if (!have_seq) {
                seq = is89_sequence_begin(seqs, "default");
                if (seq == IS89_INVALID_ID) return 0;
                have_seq = 1;
            }
            if (!b3d_pspr89_token(&cur, a, sizeof(a))) return 0;
            if (!is89_sequence_set_loop(seqs, seq,
                    b3d_pspr89_loop_word(a, IS89_LOOP_FORWARD))) return 0;
        } else if (!strcmp(cmd, "frame")) {
            unsigned int ms;
            if (!have_seq) {
                seq = is89_sequence_begin(seqs, "default");
                if (seq == IS89_INVALID_ID) return 0;
                have_seq = 1;
            }
            if (!b3d_pspr89_token(&cur, a, sizeof(a))) return 0;
            ms = 33U;
            if (b3d_pspr89_token(&cur, b, sizeof(b))) ms = b3d_pspr89_uint(b, 33U);
            if (is89_sequence_add_frame(seqs, seq, a, ms) == IS89_INVALID_ID) return 0;
        } else if (!strcmp(cmd, "rect")) {
            unsigned int ms;
            if (!have_seq) {
                seq = is89_sequence_begin(seqs, "default");
                if (seq == IS89_INVALID_ID) return 0;
                have_seq = 1;
            }
            if (!b3d_pspr89_token(&cur, a, sizeof(a)) ||
                !b3d_pspr89_token(&cur, b, sizeof(b)) ||
                !b3d_pspr89_token(&cur, c, sizeof(c)) ||
                !b3d_pspr89_token(&cur, d, sizeof(d)) ||
                !b3d_pspr89_token(&cur, e, sizeof(e))) return 0;
            ms = 33U;
            if (b3d_pspr89_token(&cur, f, sizeof(f))) ms = b3d_pspr89_uint(f, 33U);
            if (is89_sequence_add_frame_rect(seqs, seq, a,
                    (is89_s32)b3d_pspr89_int(b, 0),
                    (is89_s32)b3d_pspr89_int(c, 0),
                    (is89_s32)b3d_pspr89_int(d, 0),
                    (is89_s32)b3d_pspr89_int(e, 0), ms) == IS89_INVALID_ID) return 0;
        }
    }
    if (!have_seq || seq == IS89_INVALID_ID) return 0;
    if (out_sequence) *out_sequence = seq;
    return 1;
}

static int b3d_pspr89_compile_sequence(Blank3DProjectileSpriteRuntime89 *runtime,
                                       Blank3DProjectileSpriteClip89 *clip,
                                       const Blank3DWeaponModules *modules)
{
    char path[AR89_PATH_CAP];
    unsigned int size;
    is89_id seq;
    (void)size;
    if (!runtime || !clip || !modules || !modules->projectile_visual_recipe[0]) return 0;
    if (!b3d_pspr89_resolve_data(runtime, modules->projectile_visual_recipe,
                                 path, sizeof(path)) ||
        !b3d_pspr89_read_text(path, &size) ||
        !b3d_pspr89_parse_sequence(b3d_pspr89_text,
                                   &b3d_pspr89_seq_scratch, &seq)) return 0;
    if (modules->projectile_sprite_loop >= GWM89_PROJECTILE_SPRITE_LOOP_NONE &&
        modules->projectile_sprite_loop <= GWM89_PROJECTILE_SPRITE_LOOP_HOLD)
        (void)is89_sequence_set_loop(&b3d_pspr89_seq_scratch, seq,
                                     modules->projectile_sprite_loop);
    return b3d_pspr89_copy_is89(clip, &b3d_pspr89_seq_scratch, seq);
}

static int b3d_pspr89_compile_renlist(Blank3DProjectileSpriteRuntime89 *runtime,
                                      Blank3DProjectileSpriteClip89 *clip,
                                      const Blank3DWeaponModules *modules)
{
    char path[AR89_PATH_CAP];
    unsigned int size;
    rl89_id anim;
    is89_id seq;
    const char *asset_name;
    const char *clip_name;
    if (!runtime || !clip || !modules || !modules->projectile_visual_recipe[0]) return 0;
    if (!b3d_pspr89_resolve_data(runtime, modules->projectile_visual_recipe,
                                 path, sizeof(path)) ||
        !b3d_pspr89_read_text(path, &size)) return 0;
    rl89_init(&b3d_pspr89_ren_scratch);
    if (!rl89_parse(&b3d_pspr89_ren_scratch, b3d_pspr89_text, size)) return 0;
    asset_name = modules->projectile_visual_animation[0]
        ? modules->projectile_visual_animation : 0;
    clip_name = modules->projectile_visual_clip[0]
        ? modules->projectile_visual_clip : "default";
    if (asset_name) anim = rl89_find_animation(&b3d_pspr89_ren_scratch,
                                               asset_name, clip_name);
    else anim = b3d_pspr89_ren_scratch.animation_count
        ? 0U : RL89_INVALID_ID;
    if (anim == RL89_INVALID_ID) return 0;
    is89_init(&b3d_pspr89_seq_scratch);
    if (!rl89_to_imagesequencer89(&b3d_pspr89_ren_scratch, anim,
                                   &b3d_pspr89_seq_scratch, &seq)) return 0;
    return b3d_pspr89_copy_is89(clip, &b3d_pspr89_seq_scratch, seq);
}

static int b3d_pspr89_image_dimensions(Blank3DProjectileSpriteRuntime89 *runtime,
                                       const char *request,
                                       int *out_id,
                                       unsigned int *out_w,
                                       unsigned int *out_h)
{
    int id;
    const Blank3DImageAsset *asset;
    if (!runtime || !runtime->images || !request || !request[0]) return 0;
    id = blank3d_image_assets_resolve_request(runtime->images, request);
    if (id <= 0 || !blank3d_image_assets_load(runtime->images, id)) return 0;
    asset = blank3d_image_assets_get(runtime->images, id);
    if (!asset || !asset->loaded || asset->width == 0U || asset->height == 0U) return 0;
    if (out_id) *out_id = id;
    if (out_w) *out_w = asset->width;
    if (out_h) *out_h = asset->height;
    return 1;
}

static int b3d_pspr89_compile_tilecell(Blank3DProjectileSpriteRuntime89 *runtime,
                                       Blank3DProjectileSpriteClip89 *clip,
                                       const Blank3DWeaponModules *modules)
{
    unsigned int image_w;
    unsigned int image_h;
    unsigned int count;
    unsigned int i;
    unsigned int start_index;
    tc89_id atlas;
    is89_id seq;
    if (!runtime || !clip || !modules || !modules->projectile_visual_image[0] ||
        modules->projectile_visual_frame_width == 0U ||
        modules->projectile_visual_frame_height == 0U) return 0;
    if (!b3d_pspr89_image_dimensions(runtime, modules->projectile_visual_image,
                                     0, &image_w, &image_h)) return 0;
    tc89_init(&b3d_pspr89_tile_scratch);
    atlas = tc89_add_atlas(&b3d_pspr89_tile_scratch, "projectile",
        modules->projectile_visual_image, image_w, image_h,
        modules->projectile_visual_frame_width,
        modules->projectile_visual_frame_height,
        modules->projectile_visual_margin_x,
        modules->projectile_visual_margin_y,
        modules->projectile_visual_spacing_x,
        modules->projectile_visual_spacing_y);
    if (atlas == TC89_INVALID_ID) return 0;
    count = modules->projectile_visual_frame_count;
    if (count == 0U) count = b3d_pspr89_tile_scratch.atlases[atlas].columns *
                             b3d_pspr89_tile_scratch.atlases[atlas].rows;
    if (count == 0U || count > B3D_PSPR89_FRAME_CAP) return 0;
    is89_init(&b3d_pspr89_seq_scratch);
    seq = is89_sequence_begin(&b3d_pspr89_seq_scratch, "projectile");
    if (seq == IS89_INVALID_ID) return 0;
    start_index = modules->projectile_visual_start_y *
                  b3d_pspr89_tile_scratch.atlases[atlas].columns +
                  modules->projectile_visual_start_x;
    for (i = 0U; i < count; ++i) {
        TC89_Rect rect;
        int ok;
        if (modules->projectile_visual_step_x == 1 &&
            modules->projectile_visual_step_y == 0) {
            ok = tc89_atlas_rect_index(&b3d_pspr89_tile_scratch, atlas,
                                       start_index + i, &rect);
        } else {
            int x;
            int y;
            x = (int)modules->projectile_visual_start_x +
                (int)i * (int)modules->projectile_visual_step_x;
            y = (int)modules->projectile_visual_start_y +
                (int)i * (int)modules->projectile_visual_step_y;
            ok = x >= 0 && y >= 0 &&
                 tc89_atlas_rect_xy(&b3d_pspr89_tile_scratch, atlas,
                                    (tc89_u32)x, (tc89_u32)y, &rect);
        }
        if (!ok || is89_sequence_add_frame_rect(&b3d_pspr89_seq_scratch, seq,
                modules->projectile_visual_image,
                rect.x, rect.y, rect.w, rect.h,
                modules->projectile_visual_frame_ms
                    ? modules->projectile_visual_frame_ms : 33U) == IS89_INVALID_ID)
            return 0;
    }
    if (!is89_sequence_set_loop(&b3d_pspr89_seq_scratch, seq,
                                 modules->projectile_sprite_loop)) return 0;
    return b3d_pspr89_copy_is89(clip, &b3d_pspr89_seq_scratch, seq);
}

static int b3d_pspr89_gm_acquire(void *user, const char *path,
                                  gmss89_u32 *out_handle,
                                  gmss89_u32 *out_width,
                                  gmss89_u32 *out_height,
                                  gmss89_u32 *out_frames)
{
    Blank3DProjectileSpriteRuntime89 *runtime;
    int image_id;
    unsigned int width;
    unsigned int height;
    runtime = (Blank3DProjectileSpriteRuntime89 *)user;
    if (!runtime || !out_handle || !out_width || !out_height || !out_frames)
        return SA89_PROVIDER_ERROR;
    if (!b3d_pspr89_image_dimensions(runtime, path, &image_id, &width, &height))
        return SA89_PROVIDER_ERROR;
    *out_handle = (gmss89_u32)image_id;
    *out_width = (gmss89_u32)width;
    *out_height = (gmss89_u32)height;
    *out_frames = 1U;
    return SA89_PROVIDER_HANDLED;
}

static int b3d_pspr89_gm_get_frame(void *user, gmss89_u32 handle,
                                    gmss89_u32 frame_index,
                                    GMSS89_ImageView *out_view)
{
    Blank3DProjectileSpriteRuntime89 *runtime;
    const Blank3DImageAsset *asset;
    runtime = (Blank3DProjectileSpriteRuntime89 *)user;
    if (!runtime || !runtime->images || !out_view || frame_index != 0U)
        return SA89_PROVIDER_ERROR;
    asset = blank3d_image_assets_get(runtime->images, (int)handle);
    if (!asset || !asset->loaded) return SA89_PROVIDER_ERROR;
    out_view->pixels = 0;
    out_view->width = asset->width;
    out_view->height = asset->height;
    out_view->stride = asset->width * 4U;
    return SA89_PROVIDER_HANDLED;
}

static void b3d_pspr89_gm_release(void *user, gmss89_u32 handle)
{
    (void)user;
    (void)handle;
}

static int b3d_pspr89_compile_gmstrip(Blank3DProjectileSpriteRuntime89 *runtime,
                                      Blank3DProjectileSpriteClip89 *clip,
                                      const Blank3DWeaponModules *modules)
{
    GMSS89_ImageProvider provider;
    int image_id;
    const Blank3DImageAsset *image;
    char clean[SA89_NAME_CAP];
    gmss89_id asset_id;
    gmss89_id clip_id;
    unsigned int i;
    unsigned int total;
    const char *clip_name;
    if (!runtime || !clip || !modules || !modules->projectile_visual_image[0]) return 0;
    image_id = blank3d_image_assets_resolve_request(runtime->images,
                                                     modules->projectile_visual_image);
    if (image_id <= 0 || !blank3d_image_assets_load(runtime->images, image_id)) return 0;
    image = blank3d_image_assets_get(runtime->images, image_id);
    if (!image || !image->loaded) return 0;
    gmss89_init(&b3d_pspr89_gm_scratch);
    memset(&provider, 0, sizeof(provider));
    provider.acquire = b3d_pspr89_gm_acquire;
    provider.get_frame = b3d_pspr89_gm_get_frame;
    provider.release = b3d_pspr89_gm_release;
    provider.user = runtime;
    gmss89_set_image_provider(&b3d_pspr89_gm_scratch, &provider);
    if (!gmss89_make_clean_name(image->path, clean, sizeof(clean)))
        b3d_pspr89_copy(clean, sizeof(clean), "projectile");
    clip_name = modules->projectile_visual_clip[0]
        ? modules->projectile_visual_clip : "default";
    if (!gmss89_define_strip_auto(&b3d_pspr89_gm_scratch, clean, clip_name,
                                   image->path,
                                   modules->projectile_visual_frame_ms
                                       ? modules->projectile_visual_frame_ms : 33U,
                                   modules->projectile_sprite_loop)) {
        if (modules->projectile_visual_frame_count == 0U ||
            !gmss89_define_strip(&b3d_pspr89_gm_scratch, clean, clip_name,
                                 image->path,
                                 modules->projectile_visual_frame_count,
                                 modules->projectile_visual_frame_ms
                                     ? modules->projectile_visual_frame_ms : 33U,
                                 modules->projectile_sprite_loop)) return 0;
    }
    asset_id = sa89_find_asset(&b3d_pspr89_gm_scratch, clean);
    if (asset_id == SA89_INVALID_ID) return 0;
    clip_id = sa89_find_clip(&b3d_pspr89_gm_scratch, asset_id, clip_name);
    if (clip_id == SA89_INVALID_ID ||
        b3d_pspr89_gm_scratch.clips[clip_id].frame_count == 0U ||
        b3d_pspr89_gm_scratch.clips[clip_id].frame_count > B3D_PSPR89_FRAME_CAP)
        return 0;
    total = 0U;
    for (i = 0U; i < b3d_pspr89_gm_scratch.clips[clip_id].frame_count; ++i) {
        const SA89_Frame *src;
        const SA89_Source *source;
        Blank3DProjectileSpriteFrame89 *dst;
        unsigned int idx;
        idx = (unsigned int)b3d_pspr89_gm_scratch.clips[clip_id].first_frame + i;
        if (idx >= b3d_pspr89_gm_scratch.frame_count) return 0;
        src = &b3d_pspr89_gm_scratch.frames[idx];
        if (src->source_id >= b3d_pspr89_gm_scratch.source_count) return 0;
        source = &b3d_pspr89_gm_scratch.sources[src->source_id];
        dst = &clip->frames[i];
        memset(dst, 0, sizeof(*dst));
        b3d_pspr89_copy(dst->image_request, sizeof(dst->image_request), source->path);
        dst->source_enabled = 1;
        dst->source_x = (int)src->x;
        dst->source_y = (int)src->y;
        dst->source_w = (int)src->w;
        dst->source_h = (int)src->h;
        dst->scale_x_q16 = 65536L;
        dst->scale_y_q16 = 65536L;
        dst->duration_ms = src->duration_ms ? src->duration_ms : 1U;
        total += dst->duration_ms;
    }
    clip->frame_count = b3d_pspr89_gm_scratch.clips[clip_id].frame_count;
    clip->loop_mode = b3d_pspr89_gm_scratch.clips[clip_id].loop_mode;
    clip->total_ms = total ? total : 1U;
    return 1;
}

void blank3d_projectile_sprite89_init(Blank3DProjectileSpriteRuntime89 *runtime,
                                      Blank3DImageAssets *images,
                                      AssetRoute89 *routes)
{
    if (!runtime) return;
    memset(runtime, 0, sizeof(*runtime));
    runtime->images = images;
    runtime->routes = routes;
}

void blank3d_projectile_sprite89_reset(Blank3DProjectileSpriteRuntime89 *runtime)
{
    Blank3DImageAssets *images;
    AssetRoute89 *routes;
    if (!runtime) return;
    images = runtime->images;
    routes = runtime->routes;
    memset(runtime, 0, sizeof(*runtime));
    runtime->images = images;
    runtime->routes = routes;
}

int blank3d_projectile_sprite89_prepare(Blank3DProjectileSpriteRuntime89 *runtime,
                                        const Blank3DWeaponModules *modules)
{
    Blank3DProjectileSpriteClip89 *clip;
    int ok;
    if (!runtime || !modules || modules->weapon_id <= 0 ||
        !modules->projectile_visual_billboard ||
        modules->projectile_sprite_mode == GWM89_PROJECTILE_SPRITE_NONE)
        return 0;
    clip = b3d_pspr89_find(runtime, modules->weapon_id);
    if (clip) return clip->valid;
    clip = b3d_pspr89_alloc(runtime, modules->weapon_id);
    if (!clip) return 0;
    clip->source_mode = modules->projectile_sprite_mode;
    ok = 0;
    if (modules->projectile_sprite_mode == GWM89_PROJECTILE_SPRITE_STATIC)
        ok = b3d_pspr89_compile_static(clip, modules);
    else if (modules->projectile_sprite_mode == GWM89_PROJECTILE_SPRITE_SEQUENCE)
        ok = b3d_pspr89_compile_sequence(runtime, clip, modules);
    else if (modules->projectile_sprite_mode == GWM89_PROJECTILE_SPRITE_RENLIST)
        ok = b3d_pspr89_compile_renlist(runtime, clip, modules);
    else if (modules->projectile_sprite_mode == GWM89_PROJECTILE_SPRITE_TILECELL)
        ok = b3d_pspr89_compile_tilecell(runtime, clip, modules);
    else if (modules->projectile_sprite_mode == GWM89_PROJECTILE_SPRITE_GMSTRIP)
        ok = b3d_pspr89_compile_gmstrip(runtime, clip, modules);
    clip->valid = ok ? 1 : 0;
    if (ok) b3d_pspr89_set_status(clip, "projectile sprite recipe ready");
    else b3d_pspr89_set_status(clip, "projectile sprite recipe failed; mesh fallback active");
    return clip->valid;
}

static unsigned int b3d_pspr89_phase_ms(const Blank3DProjectileSpriteClip89 *clip,
                                        unsigned int frame_offset)
{
    unsigned int i;
    unsigned int total;
    if (!clip || clip->frame_count == 0U) return 0U;
    frame_offset %= (unsigned int)clip->frame_count;
    total = 0U;
    for (i = 0U; i < frame_offset; ++i) total += clip->frames[i].duration_ms;
    return total;
}

static unsigned int b3d_pspr89_forward_index(const Blank3DProjectileSpriteClip89 *clip,
                                              unsigned int t)
{
    unsigned int i;
    unsigned int accum;
    if (!clip || clip->frame_count == 0U) return 0U;
    accum = 0U;
    for (i = 0U; i < (unsigned int)clip->frame_count; ++i) {
        unsigned int d;
        d = clip->frames[i].duration_ms ? clip->frames[i].duration_ms : 1U;
        if (t < accum + d) return i;
        accum += d;
    }
    return (unsigned int)clip->frame_count - 1U;
}

static unsigned int b3d_pspr89_index_at(const Blank3DProjectileSpriteClip89 *clip,
                                        unsigned int age_ms)
{
    unsigned int t;
    unsigned int total;
    unsigned int index;
    if (!clip || clip->frame_count == 0U) return 0U;
    if (clip->frame_count == 1U) return 0U;
    total = clip->total_ms ? clip->total_ms : 1U;
    if (clip->loop_mode == IS89_LOOP_FORWARD) {
        t = age_ms % total;
        return b3d_pspr89_forward_index(clip, t);
    }
    if (clip->loop_mode == IS89_LOOP_REVERSE) {
        t = age_ms % total;
        index = b3d_pspr89_forward_index(clip, t);
        return (unsigned int)clip->frame_count - 1U - index;
    }
    if (clip->loop_mode == IS89_LOOP_PINGPONG) {
        unsigned int edge_first;
        unsigned int edge_last;
        unsigned int cycle;
        edge_first = clip->frames[0].duration_ms ? clip->frames[0].duration_ms : 1U;
        edge_last = clip->frames[clip->frame_count - 1U].duration_ms
            ? clip->frames[clip->frame_count - 1U].duration_ms : 1U;
        cycle = total * 2U;
        if (cycle > edge_first) cycle -= edge_first;
        if (cycle > edge_last) cycle -= edge_last;
        if (cycle == 0U) cycle = 1U;
        t = age_ms % cycle;
        if (t < total) return b3d_pspr89_forward_index(clip, t);
        t -= total;
        if (clip->frame_count <= 2U) return 0U;
        index = (unsigned int)clip->frame_count - 2U;
        while (index > 0U) {
            unsigned int d;
            d = clip->frames[index].duration_ms ? clip->frames[index].duration_ms : 1U;
            if (t < d) return index;
            t -= d;
            --index;
        }
        return 0U;
    }
    if (age_ms >= total)
        return (unsigned int)clip->frame_count - 1U;
    return b3d_pspr89_forward_index(clip, age_ms);
}

int blank3d_projectile_sprite89_sample(Blank3DProjectileSpriteRuntime89 *runtime,
                                       const Blank3DWeaponModules *modules,
                                       unsigned int age_ms,
                                       int projectile_slot,
                                       Blank3DProjectileSpriteSample89 *out_sample)
{
    Blank3DProjectileSpriteClip89 *clip;
    Blank3DProjectileSpriteFrame89 *frame;
    unsigned int phase_frames;
    unsigned int phase_time;
    unsigned int index;
    int image_id;
    if (!out_sample) return 0;
    memset(out_sample, 0, sizeof(*out_sample));
    if (!runtime || !modules || !blank3d_projectile_sprite89_prepare(runtime, modules))
        return 0;
    clip = b3d_pspr89_find(runtime, modules->weapon_id);
    if (!clip || !clip->valid || clip->frame_count == 0U) return 0;
    phase_frames = projectile_slot < 0 ? 0U :
        ((unsigned int)projectile_slot * (unsigned int)modules->projectile_visual_phase_step);
    phase_time = b3d_pspr89_phase_ms(clip, phase_frames);
    index = b3d_pspr89_index_at(clip, age_ms + phase_time);
    if (index >= clip->frame_count) return 0;
    frame = &clip->frames[index];
    image_id = blank3d_image_assets_resolve_request(runtime->images,
                                                     frame->image_request);
    if (image_id <= 0) return 0;
    out_sample->valid = 1;
    out_sample->image_id = image_id;
    out_sample->source_enabled = frame->source_enabled;
    out_sample->source_x = frame->source_x;
    out_sample->source_y = frame->source_y;
    out_sample->source_w = frame->source_w;
    out_sample->source_h = frame->source_h;
    out_sample->scale_x_q16 = frame->scale_x_q16 ? frame->scale_x_q16 : 65536L;
    out_sample->scale_y_q16 = frame->scale_y_q16 ? frame->scale_y_q16 : 65536L;
    out_sample->frame_index = index;
    out_sample->flags = frame->flags;
    return 1;
}

const char *blank3d_projectile_sprite89_status(
    const Blank3DProjectileSpriteRuntime89 *runtime, int weapon_id)
{
    int i;
    if (!runtime) return "projectile sprite runtime unavailable";
    for (i = 0; i < B3D_PSPR89_CACHE_CAP; ++i)
        if (runtime->clips[i].used && runtime->clips[i].weapon_id == weapon_id)
            return runtime->clips[i].status;
    return "projectile sprite recipe not prepared";
}

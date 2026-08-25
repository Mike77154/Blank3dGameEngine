#include "../include/gmspritestrip89.h"

static int gmss89_is_digit(char c) { return c >= '0' && c <= '9'; }
static unsigned int gmss89_strlen(const char *s) { unsigned int n = 0U; while (s && s[n]) ++n; return n; }
static void gmss89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i = 0U;
    if (!dst || cap == 0U) return;
    if (!src) { dst[0] = '\0'; return; }
    while (i + 1U < cap && src[i]) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}
static const char *gmss89_basename(const char *path)
{
    const char *p = path;
    const char *last = path;
    if (!path) return "";
    while (*p) {
        if (*p == '/' || *p == '\\') last = p + 1;
        ++p;
    }
    return last;
}
static void gmss89_remove_extension(const char *in, char *out, unsigned int cap)
{
    unsigned int i = 0U;
    unsigned int last_dot = 0U;
    unsigned int seen_dot = 0U;
    if (!out || cap == 0U) return;
    out[0] = '\0';
    if (!in) return;
    while (in[i] && i + 1U < cap) {
        out[i] = in[i];
        if (in[i] == '.') { last_dot = i; seen_dot = 1U; }
        ++i;
    }
    out[i] = '\0';
    if (seen_dot) out[last_dot] = '\0';
}
static int gmss89_find_strip_marker(const char *text, unsigned int *start_idx, unsigned int *digits_idx)
{
    unsigned int len;
    unsigned int i;
    if (!text) return 0;
    len = gmss89_strlen(text);
    if (len < 7U) return 0;
    for (i = 0U; i + 6U < len; ++i) {
        if (text[i] == '_' && text[i+1] == 's' && text[i+2] == 't' && text[i+3] == 'r' &&
            text[i+4] == 'i' && text[i+5] == 'p' && gmss89_is_digit(text[i+6])) {
            if (start_idx) *start_idx = i;
            if (digits_idx) *digits_idx = i + 6U;
            return 1;
        }
    }
    return 0;
}
int gmss89_parse_strip_count(const char *text, gmss89_id *out_count)
{
    unsigned int digits_idx = 0U;
    unsigned int start_idx = 0U;
    unsigned int value = 0U;
    if (!text || !out_count) return 0;
    if (!gmss89_find_strip_marker(text, &start_idx, &digits_idx)) return 0;
    while (text[digits_idx] && gmss89_is_digit(text[digits_idx])) {
        value = value * 10U + (unsigned int)(text[digits_idx] - '0');
        if (value > 0xFFFFU) return 0;
        ++digits_idx;
    }
    if (value == 0U) return 0;
    *out_count = (gmss89_id)value;
    return 1;
}
int gmss89_make_clean_name(const char *text, char *out_name, unsigned int out_cap)
{
    char stem[SA89_PATH_CAP];
    unsigned int marker = 0U;
    unsigned int digits = 0U;
    unsigned int i;
    const char *base;
    if (!text || !out_name || out_cap == 0U) return 0;
    base = gmss89_basename(text);
    gmss89_remove_extension(base, stem, (unsigned int)sizeof(stem));
    if (!gmss89_find_strip_marker(stem, &marker, &digits)) {
        gmss89_copy(out_name, out_cap, stem);
        return 1;
    }
    i = 0U;
    while (i < marker && i + 1U < out_cap) {
        out_name[i] = stem[i];
        ++i;
    }
    out_name[i] = '\0';
    return i > 0U;
}
void gmss89_init(GMSpritestrip89 *ctx) { sa89_init(ctx); }
void gmss89_reset(GMSpritestrip89 *ctx) { sa89_reset(ctx); }
void gmss89_set_image_provider(GMSpritestrip89 *ctx, const GMSS89_ImageProvider *provider) { sa89_set_image_provider(ctx, provider); }
void gmss89_set_render_provider(GMSpritestrip89 *ctx, const GMSS89_RenderProvider *provider) { sa89_set_render_provider(ctx, provider); }
int gmss89_define_strip(GMSpritestrip89 *ctx,
                        const char *asset_name,
                        const char *clip_name,
                        const char *path,
                        gmss89_id frame_count,
                        gmss89_u32 duration_ms,
                        int loop_mode)
{
    return sa89_define_gamemaker_strip(ctx, asset_name, clip_name, path, frame_count, duration_ms, loop_mode);
}
int gmss89_define_strip_auto(GMSpritestrip89 *ctx,
                             const char *asset_name,
                             const char *clip_name,
                             const char *path,
                             gmss89_u32 duration_ms,
                             int loop_mode)
{
    gmss89_id n = 0U;
    if (!gmss89_parse_strip_count(path, &n) && !gmss89_parse_strip_count(asset_name, &n)) return 0;
    return gmss89_define_strip(ctx, asset_name, clip_name, path, n, duration_ms, loop_mode);
}
int gmss89_define_strip_from_path(GMSpritestrip89 *ctx,
                                  const char *path,
                                  const char *clip_name,
                                  gmss89_u32 duration_ms,
                                  int loop_mode)
{
    char clean[SA89_NAME_CAP];
    gmss89_id n = 0U;
    if (!gmss89_make_clean_name(path, clean, (unsigned int)sizeof(clean))) return 0;
    if (!gmss89_parse_strip_count(path, &n)) return 0;
    return gmss89_define_strip(ctx, clean, clip_name ? clip_name : "default", path, n, duration_ms, loop_mode);
}
gmss89_id gmss89_asset_subimage_count(const GMSpritestrip89 *ctx,
                                      const char *asset_name,
                                      const char *clip_name)
{
    gmss89_id a;
    gmss89_id c;
    if (!ctx || !asset_name) return GMSS89_INVALID_ID;
    a = sa89_find_asset(ctx, asset_name);
    if (a == SA89_INVALID_ID) return SA89_INVALID_ID;
    c = sa89_find_clip(ctx, a, clip_name ? clip_name : "default");
    if (c == SA89_INVALID_ID) return SA89_INVALID_ID;
    return ctx->clips[c].frame_count;
}
gmss89_id gmss89_player_create(GMSpritestrip89 *ctx) { return sa89_player_create(ctx); }
void gmss89_player_destroy(GMSpritestrip89 *ctx, gmss89_id player_id) { sa89_player_destroy(ctx, player_id); }
int gmss89_player_play(GMSpritestrip89 *ctx, gmss89_id player_id,
                       const char *asset_name, const char *clip_name)
{
    gmss89_id a;
    if (!ctx || !asset_name) return 0;
    a = sa89_find_asset(ctx, asset_name);
    if (a == SA89_INVALID_ID) return 0;
    return sa89_player_play(ctx, player_id, a, clip_name ? clip_name : "default");
}
void gmss89_player_set_position(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_s32 x, gmss89_s32 y) { sa89_player_set_position(ctx, player_id, x, y); }
void gmss89_player_show(GMSpritestrip89 *ctx, gmss89_id player_id, int visible) { sa89_player_show(ctx, player_id, visible); }
void gmss89_player_set_image_speed_q16(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_s32 speed_q16) { sa89_player_set_speed_q16(ctx, player_id, speed_q16); }
int gmss89_player_set_subimage(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_id subimage) { return sa89_player_set_frame(ctx, player_id, subimage); }
gmss89_id gmss89_player_get_subimage(const GMSpritestrip89 *ctx, gmss89_id player_id)
{
    if (!ctx || player_id >= ctx->player_count || !ctx->players[player_id].used) return GMSS89_INVALID_ID;
    return ctx->players[player_id].frame_pos;
}
void gmss89_player_step(GMSpritestrip89 *ctx, gmss89_id player_id, gmss89_u32 delta_ms) { sa89_player_step(ctx, player_id, delta_ms); }
int gmss89_player_render(GMSpritestrip89 *ctx, gmss89_id player_id) { return sa89_player_render(ctx, player_id); }

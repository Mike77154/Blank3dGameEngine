#include "spriteasset89.h"
#include <string.h>

#define SA89_ERR_NONE 0
#define SA89_ERR_ARGUMENT 1
#define SA89_ERR_CAPACITY 2
#define SA89_ERR_NOT_FOUND 3
#define SA89_ERR_PROVIDER 4
#define SA89_ERR_STATE 5

static void sa89_zero(void *p, unsigned int n)
{
    unsigned char *b;
    unsigned int i;
    b = (unsigned char *)p;
    for (i = 0U; i < n; ++i) b[i] = 0U;
}

static void sa89_copy(char *dst, unsigned int cap, const char *src)
{
    unsigned int i;
    if (!dst || cap == 0U) return;
    i = 0U;
    if (src) {
        while (src[i] != '\0' && i + 1U < cap) {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

static int sa89_valid_player(const SpriteAsset89 *ctx, sa89_id id)
{
    return ctx && id < SA89_MAX_PLAYERS && ctx->players[id].used;
}

static int sa89_valid_asset(const SpriteAsset89 *ctx, sa89_id id)
{
    return ctx && id < SA89_MAX_ASSETS && ctx->assets[id].used;
}

static int sa89_valid_clip(const SpriteAsset89 *ctx, sa89_id id)
{
    return ctx && id < SA89_MAX_CLIPS && ctx->clips[id].used;
}

static int sa89_source_acquire(SpriteAsset89 *ctx, sa89_id source_id);

void sa89_init(SpriteAsset89 *ctx)
{
    if (!ctx) return;
    sa89_zero(ctx, (unsigned int)sizeof(*ctx));
}

void sa89_reset(SpriteAsset89 *ctx)
{
    sa89_id i;
    SA89_ImageProvider ip;
    SA89_RenderProvider rp;
    if (!ctx) return;
    ip = ctx->image_provider;
    rp = ctx->render_provider;
    if (ip.release) {
        for (i = 0U; i < ctx->source_count; ++i) {
            if (ctx->sources[i].used && ctx->sources[i].acquired) {
                ip.release(ip.user, ctx->sources[i].image_handle);
            }
        }
    }
    sa89_zero(ctx, (unsigned int)sizeof(*ctx));
    ctx->image_provider = ip;
    ctx->render_provider = rp;
}

void sa89_set_image_provider(SpriteAsset89 *ctx, const SA89_ImageProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->image_provider = *provider;
    else sa89_zero(&ctx->image_provider, (unsigned int)sizeof(ctx->image_provider));
}

void sa89_set_render_provider(SpriteAsset89 *ctx, const SA89_RenderProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->render_provider = *provider;
    else sa89_zero(&ctx->render_provider, (unsigned int)sizeof(ctx->render_provider));
}

sa89_id sa89_find_source(const SpriteAsset89 *ctx, const char *path)
{
    sa89_id i;
    if (!ctx || !path) return SA89_INVALID_ID;
    for (i = 0U; i < ctx->source_count; ++i) {
        if (ctx->sources[i].used && strcmp(ctx->sources[i].path, path) == 0) return i;
    }
    return SA89_INVALID_ID;
}

sa89_id sa89_add_source(SpriteAsset89 *ctx, const char *path)
{
    sa89_id id;
    if (!ctx || !path || !path[0]) return SA89_INVALID_ID;
    id = sa89_find_source(ctx, path);
    if (id != SA89_INVALID_ID) return id;
    if (ctx->source_count >= SA89_MAX_SOURCES) {
        ctx->last_error = SA89_ERR_CAPACITY;
        return SA89_INVALID_ID;
    }
    id = ctx->source_count++;
    sa89_zero(&ctx->sources[id], (unsigned int)sizeof(ctx->sources[id]));
    sa89_copy(ctx->sources[id].path, SA89_PATH_CAP, path);
    ctx->sources[id].used = 1U;
    return id;
}

sa89_id sa89_find_asset(const SpriteAsset89 *ctx, const char *name)
{
    sa89_id i;
    if (!ctx || !name) return SA89_INVALID_ID;
    for (i = 0U; i < ctx->asset_count; ++i) {
        if (ctx->assets[i].used && strcmp(ctx->assets[i].name, name) == 0) return i;
    }
    return SA89_INVALID_ID;
}

sa89_id sa89_add_asset(SpriteAsset89 *ctx, const char *name, sa89_s32 origin_x, sa89_s32 origin_y)
{
    sa89_id id;
    if (!ctx || !name || !name[0]) return SA89_INVALID_ID;
    id = sa89_find_asset(ctx, name);
    if (id != SA89_INVALID_ID) return id;
    if (ctx->asset_count >= SA89_MAX_ASSETS) {
        ctx->last_error = SA89_ERR_CAPACITY;
        return SA89_INVALID_ID;
    }
    id = ctx->asset_count++;
    sa89_zero(&ctx->assets[id], (unsigned int)sizeof(ctx->assets[id]));
    sa89_copy(ctx->assets[id].name, SA89_NAME_CAP, name);
    ctx->assets[id].origin_x = origin_x;
    ctx->assets[id].origin_y = origin_y;
    ctx->assets[id].first_clip = SA89_INVALID_ID;
    ctx->assets[id].used = 1U;
    return id;
}

sa89_id sa89_add_frame(SpriteAsset89 *ctx, sa89_id source_id, sa89_u32 source_frame,
                       sa89_s32 x, sa89_s32 y, sa89_s32 w, sa89_s32 h,
                       sa89_s32 offset_x, sa89_s32 offset_y, sa89_u32 duration_ms)
{
    sa89_id id;
    SA89_Frame *f;
    if (!ctx || source_id >= ctx->source_count || !ctx->sources[source_id].used) return SA89_INVALID_ID;
    if (ctx->frame_count >= SA89_MAX_FRAMES) {
        ctx->last_error = SA89_ERR_CAPACITY;
        return SA89_INVALID_ID;
    }
    id = ctx->frame_count++;
    f = &ctx->frames[id];
    f->source_id = source_id;
    f->source_frame = source_frame;
    f->x = x; f->y = y; f->w = w; f->h = h;
    f->offset_x = offset_x; f->offset_y = offset_y;
    f->duration_ms = duration_ms ? duration_ms : 1U;
    return id;
}

sa89_id sa89_add_clip(SpriteAsset89 *ctx, sa89_id asset_id, const char *name,
                      sa89_id first_frame, sa89_id frame_count, int loop_mode)
{
    sa89_id id;
    SA89_Asset *a;
    if (!ctx || !sa89_valid_asset(ctx, asset_id) || !name || !name[0] || frame_count == 0U) return SA89_INVALID_ID;
    if ((unsigned int)first_frame + (unsigned int)frame_count > (unsigned int)ctx->frame_count) return SA89_INVALID_ID;
    if (ctx->clip_count >= SA89_MAX_CLIPS) {
        ctx->last_error = SA89_ERR_CAPACITY;
        return SA89_INVALID_ID;
    }
    id = ctx->clip_count++;
    sa89_zero(&ctx->clips[id], (unsigned int)sizeof(ctx->clips[id]));
    sa89_copy(ctx->clips[id].name, SA89_NAME_CAP, name);
    ctx->clips[id].asset_id = asset_id;
    ctx->clips[id].first_frame = first_frame;
    ctx->clips[id].frame_count = frame_count;
    ctx->clips[id].loop_mode = (sa89_u8)loop_mode;
    ctx->clips[id].used = 1U;
    a = &ctx->assets[asset_id];
    if (a->clip_count == 0U) a->first_clip = id;
    a->clip_count++;
    return id;
}

sa89_id sa89_find_clip(const SpriteAsset89 *ctx, sa89_id asset_id, const char *name)
{
    sa89_id i;
    if (!ctx || !name) return SA89_INVALID_ID;
    for (i = 0U; i < ctx->clip_count; ++i) {
        if (ctx->clips[i].used && ctx->clips[i].asset_id == asset_id && strcmp(ctx->clips[i].name, name) == 0) return i;
    }
    return SA89_INVALID_ID;
}

int sa89_define_static(SpriteAsset89 *ctx, const char *asset_name, const char *path,
                       sa89_u32 duration_ms)
{
    sa89_id s;
    sa89_id a;
    sa89_id f;
    sa89_id c;
    s = sa89_add_source(ctx, path);
    if (s == SA89_INVALID_ID) return 0;
    a = sa89_add_asset(ctx, asset_name, 0, 0);
    if (a == SA89_INVALID_ID) return 0;
    f = sa89_add_frame(ctx, s, 0U, 0, 0, 0, 0, 0, 0, duration_ms ? duration_ms : 1000U);
    if (f == SA89_INVALID_ID) return 0;
    c = sa89_add_clip(ctx, a, "default", f, 1U, SA89_LOOP_NONE);
    return c != SA89_INVALID_ID;
}

int sa89_define_grid(SpriteAsset89 *ctx, const char *asset_name, const char *clip_name,
                     const char *path, sa89_s32 frame_w, sa89_s32 frame_h,
                     sa89_id frame_count, sa89_id columns, sa89_u32 duration_ms,
                     int loop_mode)
{
    sa89_id source;
    sa89_id asset;
    sa89_id first;
    sa89_id f;
    sa89_id i;
    sa89_s32 x;
    sa89_s32 y;
    if (!ctx || !asset_name || !clip_name || !path || frame_w <= 0 || frame_h <= 0 || frame_count == 0U) return 0;
    if (columns == 0U) columns = frame_count;
    source = sa89_add_source(ctx, path);
    asset = sa89_add_asset(ctx, asset_name, 0, 0);
    if (source == SA89_INVALID_ID || asset == SA89_INVALID_ID) return 0;
    first = ctx->frame_count;
    for (i = 0U; i < frame_count; ++i) {
        x = (sa89_s32)(i % columns) * frame_w;
        y = (sa89_s32)(i / columns) * frame_h;
        f = sa89_add_frame(ctx, source, 0U, x, y, frame_w, frame_h, 0, 0, duration_ms ? duration_ms : 100U);
        if (f == SA89_INVALID_ID) return 0;
    }
    return sa89_add_clip(ctx, asset, clip_name, first, frame_count, loop_mode) != SA89_INVALID_ID;
}

int sa89_define_gamemaker_strip(SpriteAsset89 *ctx, const char *asset_name, const char *clip_name,
                                const char *path, sa89_id frame_count,
                                sa89_u32 duration_ms, int loop_mode)
{
    sa89_id source;
    sa89_s32 frame_w;
    sa89_s32 frame_h;
    SA89_Source *s;
    if (!ctx || !asset_name || !asset_name[0] || !clip_name || !clip_name[0] ||
        !path || !path[0] || frame_count == 0U) {
        if (ctx) ctx->last_error = SA89_ERR_ARGUMENT;
        return 0;
    }
    if ((unsigned int)ctx->frame_count + (unsigned int)frame_count > (unsigned int)SA89_MAX_FRAMES ||
        ctx->asset_count >= SA89_MAX_ASSETS || ctx->clip_count >= SA89_MAX_CLIPS) {
        ctx->last_error = SA89_ERR_CAPACITY;
        return 0;
    }
    source = sa89_add_source(ctx, path);
    if (source == SA89_INVALID_ID) return 0;
    if (!sa89_source_acquire(ctx, source)) return 0;
    s = &ctx->sources[source];
    if (s->width == 0U || s->height == 0U || (s->width % (sa89_u32)frame_count) != 0U) {
        ctx->last_error = SA89_ERR_STATE;
        return 0;
    }
    if (s->width / (sa89_u32)frame_count > 0x7FFFFFFFU || s->height > 0x7FFFFFFFU) {
        ctx->last_error = SA89_ERR_STATE;
        return 0;
    }
    frame_w = (sa89_s32)(s->width / (sa89_u32)frame_count);
    frame_h = (sa89_s32)s->height;
    return sa89_define_grid(ctx, asset_name, clip_name, path,
                            frame_w, frame_h, frame_count, frame_count,
                            duration_ms, loop_mode);
}


sa89_id sa89_player_create(SpriteAsset89 *ctx)
{
    sa89_id i;
    SA89_Player *p;
    if (!ctx) return SA89_INVALID_ID;
    for (i = 0U; i < SA89_MAX_PLAYERS; ++i) {
        if (!ctx->players[i].used) {
            p = &ctx->players[i];
            sa89_zero(p, (unsigned int)sizeof(*p));
            p->asset_id = SA89_INVALID_ID;
            p->clip_id = SA89_INVALID_ID;
            p->speed_q16 = SA89_Q16_ONE;
            p->direction = 1;
            p->visible = 1U;
            p->used = 1U;
            if (i >= ctx->player_count) ctx->player_count = (sa89_id)(i + 1U);
            return i;
        }
    }
    ctx->last_error = SA89_ERR_CAPACITY;
    return SA89_INVALID_ID;
}

void sa89_player_destroy(SpriteAsset89 *ctx, sa89_id player_id)
{
    if (!sa89_valid_player(ctx, player_id)) return;
    sa89_zero(&ctx->players[player_id], (unsigned int)sizeof(ctx->players[player_id]));
}

int sa89_player_set_asset(SpriteAsset89 *ctx, sa89_id player_id, sa89_id asset_id)
{
    SA89_Player *p;
    SA89_Asset *a;
    if (!sa89_valid_player(ctx, player_id) || !sa89_valid_asset(ctx, asset_id)) return 0;
    p = &ctx->players[player_id];
    a = &ctx->assets[asset_id];
    p->asset_id = asset_id;
    p->clip_id = a->clip_count ? a->first_clip : SA89_INVALID_ID;
    p->frame_pos = 0U;
    p->elapsed_ms = 0U;
    return 1;
}

int sa89_player_play(SpriteAsset89 *ctx, sa89_id player_id, sa89_id asset_id, const char *clip_name)
{
    sa89_id clip_id;
    SA89_Player *p;
    if (!sa89_valid_player(ctx, player_id) || !sa89_valid_asset(ctx, asset_id)) return 0;
    if (clip_name && clip_name[0]) clip_id = sa89_find_clip(ctx, asset_id, clip_name);
    else clip_id = ctx->assets[asset_id].first_clip;
    if (!sa89_valid_clip(ctx, clip_id)) return 0;
    p = &ctx->players[player_id];
    p->asset_id = asset_id;
    p->clip_id = clip_id;
    p->elapsed_ms = 0U;
    p->playing = 1U;
    p->visible = 1U;
    p->direction = (ctx->clips[clip_id].loop_mode == SA89_LOOP_REVERSE) ? -1 : 1;
    p->frame_pos = (p->direction < 0) ? (sa89_id)(ctx->clips[clip_id].frame_count - 1U) : 0U;
    return 1;
}

void sa89_player_stop(SpriteAsset89 *ctx, sa89_id player_id)
{
    if (sa89_valid_player(ctx, player_id)) ctx->players[player_id].playing = 0U;
}

void sa89_player_show(SpriteAsset89 *ctx, sa89_id player_id, int visible)
{
    if (sa89_valid_player(ctx, player_id)) ctx->players[player_id].visible = visible ? 1U : 0U;
}

void sa89_player_set_position(SpriteAsset89 *ctx, sa89_id player_id, sa89_s32 x, sa89_s32 y)
{
    if (!sa89_valid_player(ctx, player_id)) return;
    ctx->players[player_id].x = x;
    ctx->players[player_id].y = y;
}

void sa89_player_set_speed_q16(SpriteAsset89 *ctx, sa89_id player_id, sa89_s32 speed_q16)
{
    if (!sa89_valid_player(ctx, player_id)) return;
    ctx->players[player_id].speed_q16 = speed_q16 < 0 ? 0 : speed_q16;
}

int sa89_player_set_frame(SpriteAsset89 *ctx, sa89_id player_id, sa89_id frame_pos)
{
    SA89_Player *p;
    if (!sa89_valid_player(ctx, player_id)) return 0;
    p = &ctx->players[player_id];
    if (!sa89_valid_clip(ctx, p->clip_id) || frame_pos >= ctx->clips[p->clip_id].frame_count) return 0;
    p->frame_pos = frame_pos;
    p->elapsed_ms = 0U;
    return 1;
}

static void sa89_advance_one(SpriteAsset89 *ctx, SA89_Player *p)
{
    SA89_Clip *c;
    if (!ctx || !p || !sa89_valid_clip(ctx, p->clip_id)) return;
    c = &ctx->clips[p->clip_id];
    if (p->direction >= 0) {
        if ((unsigned int)p->frame_pos + 1U < (unsigned int)c->frame_count) {
            p->frame_pos++;
        } else if (c->loop_mode == SA89_LOOP_FORWARD) {
            p->frame_pos = 0U;
        } else if (c->loop_mode == SA89_LOOP_PINGPONG && c->frame_count > 1U) {
            p->direction = -1;
            p->frame_pos--;
        } else {
            p->playing = 0U;
        }
    } else {
        if (p->frame_pos > 0U) {
            p->frame_pos--;
        } else if (c->loop_mode == SA89_LOOP_REVERSE) {
            p->frame_pos = (sa89_id)(c->frame_count - 1U);
        } else if (c->loop_mode == SA89_LOOP_PINGPONG && c->frame_count > 1U) {
            p->direction = 1;
            p->frame_pos++;
        } else {
            p->playing = 0U;
        }
    }
}

const SA89_Frame *sa89_player_current_frame(const SpriteAsset89 *ctx, sa89_id player_id)
{
    const SA89_Player *p;
    const SA89_Clip *c;
    sa89_id frame_id;
    if (!sa89_valid_player(ctx, player_id)) return 0;
    p = &ctx->players[player_id];
    if (!sa89_valid_clip(ctx, p->clip_id)) return 0;
    c = &ctx->clips[p->clip_id];
    if (p->frame_pos >= c->frame_count) return 0;
    frame_id = (sa89_id)(c->first_frame + p->frame_pos);
    if (frame_id >= ctx->frame_count) return 0;
    return &ctx->frames[frame_id];
}

void sa89_player_step(SpriteAsset89 *ctx, sa89_id player_id, sa89_u32 delta_ms)
{
    SA89_Player *p;
    const SA89_Frame *f;
    sa89_u32 scaled;
    sa89_u32 dur;
    unsigned int guard;
    if (!sa89_valid_player(ctx, player_id)) return;
    p = &ctx->players[player_id];
    if (!p->playing || p->speed_q16 <= 0) return;
    scaled = delta_ms * ((sa89_u32)p->speed_q16 >> 16);
    scaled += (delta_ms * ((sa89_u32)p->speed_q16 & 0xFFFFU)) >> 16;
    p->elapsed_ms += scaled;
    guard = 0U;
    while (p->playing && guard++ < SA89_MAX_FRAMES) {
        f = sa89_player_current_frame(ctx, player_id);
        if (!f) break;
        dur = f->duration_ms ? f->duration_ms : 1U;
        if (p->elapsed_ms < dur) break;
        p->elapsed_ms -= dur;
        sa89_advance_one(ctx, p);
    }
}

static int sa89_source_acquire(SpriteAsset89 *ctx, sa89_id source_id)
{
    SA89_Source *s;
    int r;
    if (!ctx || source_id >= ctx->source_count) return 0;
    s = &ctx->sources[source_id];
    if (s->acquired) return 1;
    if (!ctx->image_provider.acquire) return 0;
    r = ctx->image_provider.acquire(ctx->image_provider.user, s->path,
                                    &s->image_handle, &s->width, &s->height,
                                    &s->embedded_frames);
    if (r != SA89_PROVIDER_HANDLED) {
        ctx->last_error = SA89_ERR_PROVIDER;
        return 0;
    }
    s->acquired = 1U;
    return 1;
}

int sa89_player_render(SpriteAsset89 *ctx, sa89_id player_id)
{
    SA89_Player *p;
    const SA89_Frame *f;
    SA89_Source *s;
    SA89_ImageView view;
    sa89_s32 sw;
    sa89_s32 sh;
    int r;
    if (!sa89_valid_player(ctx, player_id)) return 0;
    p = &ctx->players[player_id];
    if (!p->visible) return 1;
    f = sa89_player_current_frame(ctx, player_id);
    if (!f || f->source_id >= ctx->source_count) return 0;
    if (!sa89_source_acquire(ctx, f->source_id)) return 0;
    s = &ctx->sources[f->source_id];
    if (!ctx->image_provider.get_frame || !ctx->render_provider.draw) return 0;
    sa89_zero(&view, (unsigned int)sizeof(view));
    r = ctx->image_provider.get_frame(ctx->image_provider.user, s->image_handle,
                                      f->source_frame, &view);
    if (r != SA89_PROVIDER_HANDLED) return 0;
    sw = f->w > 0 ? f->w : (sa89_s32)view.width;
    sh = f->h > 0 ? f->h : (sa89_s32)view.height;
    r = ctx->render_provider.draw(ctx->render_provider.user, &view,
                                  f->x, f->y, sw, sh,
                                  p->x - ctx->assets[p->asset_id].origin_x + f->offset_x,
                                  p->y - ctx->assets[p->asset_id].origin_y + f->offset_y,
                                  p->draw_flags);
    return r == SA89_PROVIDER_HANDLED;
}

const char *sa89_error_string(int code)
{
    switch (code) {
        case SA89_ERR_NONE: return "ok";
        case SA89_ERR_ARGUMENT: return "argument";
        case SA89_ERR_CAPACITY: return "capacity";
        case SA89_ERR_NOT_FOUND: return "not found";
        case SA89_ERR_PROVIDER: return "provider";
        case SA89_ERR_STATE: return "state";
        default: return "unknown";
    }
}

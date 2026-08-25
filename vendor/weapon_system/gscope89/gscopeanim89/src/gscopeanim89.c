#include "../include/gscopeanim89.h"
#include <string.h>

static void gsa89_copy(char *dst, int cap, const char *src)
{
    int i;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    i = 0;
    while (i + 1 < cap && src[i]) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static int gsa89_prefix(const char *s, const char *prefix)
{
    while (*prefix) {
        if (*s != *prefix) return 0;
        ++s; ++prefix;
    }
    return 1;
}

static short gsa89_mode(const char *s)
{
    if (s && !strcmp(s, "loop")) return GSA89_MODE_LOOP;
    return GSA89_MODE_ONESHOT;
}

static short gsa89_ease(const char *s)
{
    if (!s) return GSA89_EASE_LINEAR;
    if (!strcmp(s, "in_quad")) return GSA89_EASE_IN_QUAD;
    if (!strcmp(s, "out_quad")) return GSA89_EASE_OUT_QUAD;
    if (!strcmp(s, "in_out_quad")) return GSA89_EASE_IN_OUT_QUAD;
    if (!strcmp(s, "step")) return GSA89_EASE_STEP;
    return GSA89_EASE_LINEAR;
}

static short gsa89_blend(const char *s)
{
    if (s && !strcmp(s, "add")) return GSA89_BLEND_ADD;
    if (s && !strcmp(s, "multiply")) return GSA89_BLEND_MULTIPLY;
    return GSA89_BLEND_REPLACE;
}

static short gsa89_property(const char *s)
{
    if (!s) return -1;
    if (!strcmp(s, "scale") || !strcmp(s, "scale_x1000")) return GSA89_PROP_SCALE_X1000;
    if (!strcmp(s, "offset_x") || !strcmp(s, "offset_x_norm")) return GSA89_PROP_OFFSET_X_NORM;
    if (!strcmp(s, "offset_y") || !strcmp(s, "offset_y_norm")) return GSA89_PROP_OFFSET_Y_NORM;
    if (!strcmp(s, "alpha") || !strcmp(s, "alpha_x1000")) return GSA89_PROP_ALPHA_X1000;
    return -1;
}

static short gsa89_part(const char *s)
{
    if (!s || !strcmp(s, "root") || !strcmp(s, "all")) return GSA89_TARGET_ROOT;
    if (gsa89_prefix(s, "part:")) s += 5;
    if (!strcmp(s, "primary")) return GSV89_PART_PRIMARY;
    if (!strcmp(s, "secondary")) return GSV89_PART_SECONDARY;
    if (!strcmp(s, "center")) return GSV89_PART_CENTER;
    if (!strcmp(s, "posts")) return GSV89_PART_POSTS;
    if (!strcmp(s, "ticks")) return GSV89_PART_TICKS;
    if (!strcmp(s, "range")) return GSV89_PART_RANGE;
    if (!strcmp(s, "bdc")) return GSV89_PART_BDC;
    if (!strcmp(s, "wind")) return GSV89_PART_WIND;
    if (!strcmp(s, "lead")) return GSV89_PART_LEAD;
    if (!strcmp(s, "stadia")) return GSV89_PART_STADIA;
    if (!strcmp(s, "labels")) return GSV89_PART_LABELS;
    if (!strcmp(s, "illumination")) return GSV89_PART_ILLUMINATION;
    if (!strcmp(s, "frame")) return GSV89_PART_FRAME;
    if (!strcmp(s, "decoration")) return GSV89_PART_DECORATION;
    if (!strcmp(s, "warning")) return GSV89_PART_WARNING;
    return GSA89_TARGET_ROOT;
}

static short gsa89_find_clip(const gsa89_ctx *ctx, const char *name)
{
    short i;
    for (i = 0; i < ctx->clip_count; ++i)
        if (!strcmp(ctx->clips[i].name, name)) return i;
    return -1;
}

static short gsa89_add_clip(gsa89_ctx *ctx, const char *name)
{
    short id;
    id = gsa89_find_clip(ctx, name);
    if (id >= 0) return id;
    if (ctx->clip_count >= GSA89_MAX_CLIPS) return -1;
    id = ctx->clip_count++;
    memset(&ctx->clips[id], 0, sizeof(ctx->clips[id]));
    gsa89_copy(ctx->clips[id].name, GSA89_MAX_NAME, name);
    gsa89_copy(ctx->clips[id].trigger, GSA89_MAX_TRIGGER, name);
    ctx->clips[id].mode = GSA89_MODE_ONESHOT;
    ctx->clips[id].duration_ms = 1;
    return id;
}

static int gsa89_seen_section(const gri89_doc *doc, short upto, const char *section)
{
    short i;
    for (i = 0; i < upto; ++i)
        if (!strcmp(doc->entries[i].section, section)) return 1;
    return 0;
}

void gsa89_init(gsa89_ctx *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

int gsa89_load_doc(gsa89_ctx *ctx, const gri89_doc *doc)
{
    short i;
    if (!ctx || !doc) return 0;
    gsa89_init(ctx);

    for (i = 0; i < doc->count; ++i) {
        const char *sec;
        const char *name;
        short id;
        if (gsa89_seen_section(doc, i, doc->entries[i].section)) continue;
        sec = doc->entries[i].section;
        if (!gsa89_prefix(sec, "animation.")) continue;
        name = sec + 10;
        if (!name[0]) continue;
        id = gsa89_add_clip(ctx, name);
        if (id < 0) { ctx->last_error = GSA89_ERR_OVERFLOW; return 0; }
        gsa89_copy(ctx->clips[id].trigger, GSA89_MAX_TRIGGER,
                   gri89_get(doc, sec, "trigger", name));
        ctx->clips[id].mode = gsa89_mode(gri89_get(doc, sec, "mode", "oneshot"));
        ctx->clips[id].duration_ms = (unsigned short)gri89_get_long(doc, sec, "duration_ms", 1L);
        if (ctx->clips[id].duration_ms == 0) ctx->clips[id].duration_ms = 1;
    }

    for (i = 0; i < doc->count; ++i) {
        const char *sec;
        const char *clip_name;
        gsa89_channel *ch;
        short cid;
        short prop;
        if (gsa89_seen_section(doc, i, doc->entries[i].section)) continue;
        sec = doc->entries[i].section;
        if (!gsa89_prefix(sec, "channel.")) continue;
        if (ctx->channel_count >= GSA89_MAX_CHANNELS) {
            ctx->last_error = GSA89_ERR_OVERFLOW;
            return 0;
        }
        clip_name = gri89_get(doc, sec, "clip", "");
        cid = gsa89_find_clip(ctx, clip_name);
        prop = gsa89_property(gri89_get(doc, sec, "property", ""));
        if (cid < 0 || prop < 0) {
            ctx->last_error = GSA89_ERR_PARSE;
            return 0;
        }
        ch = &ctx->channels[ctx->channel_count++];
        memset(ch, 0, sizeof(*ch));
        gsa89_copy(ch->name, GSA89_MAX_NAME, sec + 8);
        ch->clip_id = cid;
        ch->target_part = gsa89_part(gri89_get(doc, sec, "target", "root"));
        ch->property = prop;
        ch->from_value = gri89_get_long(doc, sec, "from", prop == GSA89_PROP_SCALE_X1000 || prop == GSA89_PROP_ALPHA_X1000 ? 1000L : 0L);
        ch->to_value = gri89_get_long(doc, sec, "to", ch->from_value);
        ch->start_ms = (unsigned short)gri89_get_long(doc, sec, "start_ms", 0L);
        ch->duration_ms = (unsigned short)gri89_get_long(doc, sec, "duration_ms", 1L);
        if (ch->duration_ms == 0) ch->duration_ms = 1;
        ch->ease = gsa89_ease(gri89_get(doc, sec, "ease", "linear"));
        ch->blend = gsa89_blend(gri89_get(doc, sec, "blend", "replace"));
    }

    gsa89_reset(ctx);
    return 1;
}

int gsa89_load(gsa89_ctx *ctx, const char *path)
{
    gri89_doc doc;
    if (!ctx || !path) return 0;
    gri89_init(&doc);
    if (!gri89_load(&doc, path)) {
        gsa89_init(ctx);
        ctx->last_error = GSA89_ERR_RECIPE;
        return 0;
    }
    return gsa89_load_doc(ctx, &doc);
}

void gsa89_reset(gsa89_ctx *ctx)
{
    short i;
    if (!ctx) return;
    for (i = 0; i < ctx->clip_count; ++i) {
        ctx->clips[i].elapsed_ms = 0;
        ctx->clips[i].active = (!strcmp(ctx->clips[i].trigger, "always")) ? 1 : 0;
    }
}

short gsa89_trigger(gsa89_ctx *ctx, const char *trigger_name)
{
    short i;
    short count;
    if (!ctx || !trigger_name) return 0;
    count = 0;
    for (i = 0; i < ctx->clip_count; ++i) {
        if (!strcmp(ctx->clips[i].trigger, trigger_name)) {
            ctx->clips[i].active = 1;
            ctx->clips[i].elapsed_ms = 0;
            ++count;
        }
    }
    return count;
}

void gsa89_tick(gsa89_ctx *ctx, unsigned short dt_ms)
{
    short i;
    unsigned long t;
    if (!ctx) return;
    for (i = 0; i < ctx->clip_count; ++i) {
        gsa89_clip *c;
        c = &ctx->clips[i];
        if (!c->active) continue;
        t = (unsigned long)c->elapsed_ms + (unsigned long)dt_ms;
        if (c->mode == GSA89_MODE_LOOP) {
            c->elapsed_ms = (unsigned short)(t % (unsigned long)c->duration_ms);
        } else if (t >= (unsigned long)c->duration_ms) {
            c->elapsed_ms = c->duration_ms;
            c->active = 0;
        } else {
            c->elapsed_ms = (unsigned short)t;
        }
    }
}

static long gsa89_ease_t(short ease, long t)
{
    long inv;
    if (t < 0) t = 0;
    if (t > 1000) t = 1000;
    if (ease == GSA89_EASE_IN_QUAD) return (t * t) / 1000L;
    if (ease == GSA89_EASE_OUT_QUAD) {
        inv = 1000L - t;
        return 1000L - (inv * inv) / 1000L;
    }
    if (ease == GSA89_EASE_IN_OUT_QUAD) {
        if (t < 500L) return (2L * t * t) / 1000L;
        inv = 1000L - t;
        return 1000L - (2L * inv * inv) / 1000L;
    }
    if (ease == GSA89_EASE_STEP) return t >= 1000L ? 1000L : 0L;
    return t;
}

static void gsa89_pose_part_identity(gsa89_pose_part *p)
{
    p->scale_x1000 = 1000L;
    p->offset_x_norm = 0L;
    p->offset_y_norm = 0L;
    p->alpha_x1000 = 1000L;
}

static void gsa89_apply_value(gsa89_pose_part *p, short property, long value, short blend)
{
    long *dst;
    long identity;
    if (property == GSA89_PROP_SCALE_X1000) { dst = &p->scale_x1000; identity = 1000L; }
    else if (property == GSA89_PROP_OFFSET_X_NORM) { dst = &p->offset_x_norm; identity = 0L; }
    else if (property == GSA89_PROP_OFFSET_Y_NORM) { dst = &p->offset_y_norm; identity = 0L; }
    else { dst = &p->alpha_x1000; identity = 1000L; }

    if (blend == GSA89_BLEND_ADD) *dst += value - identity;
    else if (blend == GSA89_BLEND_MULTIPLY) *dst = (*dst * value) / (identity ? identity : 1L);
    else *dst = value;
}

void gsa89_sample(const gsa89_ctx *ctx, gsa89_pose *out_pose)
{
    short i;
    if (!out_pose) return;
    gsa89_pose_part_identity(&out_pose->root);
    for (i = 0; i < GSV89_MAX_PARTS; ++i) gsa89_pose_part_identity(&out_pose->part[i]);
    if (!ctx) return;

    for (i = 0; i < ctx->channel_count; ++i) {
        const gsa89_channel *ch;
        const gsa89_clip *clip;
        long local;
        long t;
        long value;
        gsa89_pose_part *target;
        ch = &ctx->channels[i];
        if (ch->clip_id < 0 || ch->clip_id >= ctx->clip_count) continue;
        clip = &ctx->clips[ch->clip_id];
        if (!clip->active) continue;
        if (clip->elapsed_ms < ch->start_ms) continue;
        local = (long)clip->elapsed_ms - (long)ch->start_ms;
        if (local > (long)ch->duration_ms) continue;
        t = (local * 1000L) / (long)ch->duration_ms;
        t = gsa89_ease_t(ch->ease, t);
        value = ch->from_value + ((ch->to_value - ch->from_value) * t) / 1000L;
        target = ch->target_part == GSA89_TARGET_ROOT ? &out_pose->root :
                 ((ch->target_part >= 0 && ch->target_part < GSV89_MAX_PARTS) ? &out_pose->part[ch->target_part] : &out_pose->root);
        gsa89_apply_value(target, ch->property, value, ch->blend);
    }
}

static gsp89_fx gsa89_scale_fx(gsp89_fx v, long scale_x1000)
{
    return (gsp89_fx)((v * scale_x1000) / 1000L);
}

static gsp89_fx gsa89_offset_fx(long norm)
{
    return GSP89_NORM(norm);
}

static void gsa89_transform_shape(const gsa89_pose *pose, const gsv89_shape *src, gsv89_shape *dst)
{
    long scale;
    long ox;
    long oy;
    const gsa89_pose_part *pp;
    *dst = *src;
    pp = (src->part_id >= 0 && src->part_id < GSV89_MAX_PARTS) ? &pose->part[src->part_id] : 0;
    scale = pose->root.scale_x1000;
    ox = pose->root.offset_x_norm;
    oy = pose->root.offset_y_norm;
    if (pp) {
        scale = (scale * pp->scale_x1000) / 1000L;
        ox += pp->offset_x_norm;
        oy += pp->offset_y_norm;
    }
    dst->x0 = gsa89_scale_fx(src->x0, scale) + gsa89_offset_fx(ox);
    dst->y0 = gsa89_scale_fx(src->y0, scale) + gsa89_offset_fx(oy);
    dst->x1 = gsa89_scale_fx(src->x1, scale) + gsa89_offset_fx(ox);
    dst->y1 = gsa89_scale_fx(src->y1, scale) + gsa89_offset_fx(oy);
    dst->x2 = gsa89_scale_fx(src->x2, scale) + gsa89_offset_fx(ox);
    dst->y2 = gsa89_scale_fx(src->y2, scale) + gsa89_offset_fx(oy);
    dst->x3 = gsa89_scale_fx(src->x3, scale) + gsa89_offset_fx(ox);
    dst->y3 = gsa89_scale_fx(src->y3, scale) + gsa89_offset_fx(oy);
    dst->a = gsa89_scale_fx(src->a, scale);
    dst->b = gsa89_scale_fx(src->b, scale);
    dst->c = gsa89_scale_fx(src->c, scale);
    dst->d = gsa89_scale_fx(src->d, scale);
}

short gsa89_apply_shapes(const gsa89_pose *pose,
                          const gsv89_shape *src,
                          short shape_count,
                          gsv89_shape *dst,
                          short dst_capacity)
{
    short i;
    if (!pose || !src || !dst || shape_count < 0 || dst_capacity < shape_count) return 0;
    for (i = 0; i < shape_count; ++i) gsa89_transform_shape(pose, &src[i], &dst[i]);
    return shape_count;
}

short gsa89_apply_alpha(const gsa89_pose *pose, short global_alpha, short part_id)
{
    long a;
    long mul;
    if (!pose) return global_alpha;
    mul = pose->root.alpha_x1000;
    if (part_id >= 0 && part_id < GSV89_MAX_PARTS)
        mul = (mul * pose->part[part_id].alpha_x1000) / 1000L;
    a = ((long)global_alpha * mul) / 1000L;
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (short)a;
}

short gsa89_last_error(const gsa89_ctx *ctx)
{
    return ctx ? ctx->last_error : GSA89_ERR_PARSE;
}

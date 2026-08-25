#include "renlist89.h"
#include <string.h>

#define RL89_ERR_NONE 0
#define RL89_ERR_ARGUMENT 1
#define RL89_ERR_SYNTAX 2
#define RL89_ERR_CAPACITY 3
#define RL89_ERR_LAYOUT 4

static void rl89_zero(void *ptr, unsigned int size)
{
    unsigned char *p;
    unsigned int i;
    if (!ptr) return;
    p = (unsigned char *)ptr;
    for (i = 0U; i < size; ++i) p[i] = 0U;
}

static int rl89_space(int c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static unsigned int rl89_trim_left(const char *s, unsigned int n)
{
    unsigned int i;
    i = 0U;
    while (i < n && rl89_space((unsigned char)s[i])) ++i;
    return i;
}

static unsigned int rl89_trim_right(const char *s, unsigned int n)
{
    while (n > 0U && rl89_space((unsigned char)s[n - 1U])) --n;
    return n;
}

static int rl89_copy_n(char *dst, unsigned int cap, const char *src, unsigned int n)
{
    unsigned int i;
    if (!dst || cap == 0U || !src) return 0;
    if (n + 1U > cap) return 0;
    for (i = 0U; i < n; ++i) dst[i] = src[i];
    dst[n] = '\0';
    return 1;
}

static int rl89_starts(const char *s, unsigned int n, const char *prefix)
{
    unsigned int i;
    i = 0U;
    while (prefix[i] != '\0') {
        if (i >= n || s[i] != prefix[i]) return 0;
        ++i;
    }
    return 1;
}

static int rl89_equal(const char *s, unsigned int n, const char *literal)
{
    unsigned int i;
    if (!s || !literal) return 0;
    i = 0U;
    while (i < n && literal[i] != '\0') {
        if (s[i] != literal[i]) return 0;
        ++i;
    }
    return i == n && literal[i] == '\0';
}

static int rl89_uint(const char *s, unsigned int n, unsigned int *out)
{
    unsigned int i;
    unsigned int v;
    if (!s || !out) return 0;
    i = rl89_trim_left(s, n);
    if (i >= n || s[i] < '0' || s[i] > '9') return 0;
    v = 0U;
    while (i < n && s[i] >= '0' && s[i] <= '9') {
        if (v > 429496729U) return 0;
        v = v * 10U + (unsigned int)(s[i] - '0');
        ++i;
    }
    *out = v;
    return 1;
}

static int rl89_seconds_ms(const char *s, unsigned int n, unsigned int *out)
{
    unsigned int i;
    unsigned int whole;
    unsigned int frac;
    unsigned int scale;
    if (!s || !out) return 0;
    i = rl89_trim_left(s, n);
    if (i >= n || s[i] < '0' || s[i] > '9') return 0;
    whole = 0U;
    while (i < n && s[i] >= '0' && s[i] <= '9') {
        whole = whole * 10U + (unsigned int)(s[i] - '0');
        ++i;
    }
    frac = 0U;
    scale = 1U;
    if (i < n && s[i] == '.') {
        ++i;
        while (i < n && s[i] >= '0' && s[i] <= '9' && scale < 10000U) {
            frac = frac * 10U + (unsigned int)(s[i] - '0');
            scale *= 10U;
            ++i;
        }
    }
    if (whole > 4294967U) return 0;
    *out = whole * 1000U + (frac * 1000U) / scale;
    return 1;
}

static int rl89_parse_header(const char *s, unsigned int n,
                             char *asset, char *clip)
{
    unsigned int p;
    unsigned int a0;
    unsigned int a1;
    unsigned int c0;
    unsigned int c1;
    if (!rl89_starts(s, n, "image ")) return 0;
    p = 6U;
    while (p < n && rl89_space((unsigned char)s[p])) ++p;
    a0 = p;
    while (p < n && !rl89_space((unsigned char)s[p]) && s[p] != ':') ++p;
    a1 = p;
    while (p < n && rl89_space((unsigned char)s[p])) ++p;
    c0 = p;
    while (p < n && s[p] != ':') ++p;
    c1 = p;
    c1 = rl89_trim_right(s + c0, c1 - c0) + c0;
    if (p >= n || s[p] != ':' || a1 <= a0) return 0;
    if (!rl89_copy_n(asset, RL89_NAME_CAP, s + a0, a1 - a0)) return 0;
    if (c1 > c0) {
        if (!rl89_copy_n(clip, RL89_NAME_CAP, s + c0, c1 - c0)) return 0;
    } else {
        if (!rl89_copy_n(clip, RL89_NAME_CAP, "default", 7U)) return 0;
    }
    return 1;
}

static int rl89_add_property(RenList89 *doc, RL89_Animation *a,
                             const char *key, unsigned int key_n,
                             const char *value, unsigned int value_n)
{
    RL89_Property *p;
    if (doc->property_count >= RL89_MAX_PROPERTIES || a->property_count == 0xFFFFU) return 0;
    if ((unsigned int)a->first_property + (unsigned int)a->property_count != (unsigned int)doc->property_count) return 0;
    p = &doc->properties[doc->property_count];
    rl89_zero(p, (unsigned int)sizeof(*p));
    if (!rl89_copy_n(p->key, RL89_KEY_CAP, key, key_n)) return 0;
    if (!rl89_copy_n(p->value, RL89_VALUE_CAP, value, value_n)) return 0;
    ++doc->property_count;
    ++a->property_count;
    return 1;
}

static int rl89_add_frame(RenList89 *doc, RL89_Animation *a,
                          const char *request, unsigned int request_n,
                          unsigned int duration_ms)
{
    RL89_Frame *f;
    if (doc->frame_count >= RL89_MAX_FRAMES || a->frame_count == 0xFFFFU) return 0;
    if ((unsigned int)a->first_frame + (unsigned int)a->frame_count != (unsigned int)doc->frame_count) return 0;
    f = &doc->frames[doc->frame_count];
    rl89_zero(f, (unsigned int)sizeof(*f));
    if (!rl89_copy_n(f->request, RL89_REQUEST_CAP, request, request_n)) return 0;
    f->duration_ms = duration_ms ? duration_ms : 1U;
    f->scale_x_q16 = RL89_Q16_ONE;
    f->scale_y_q16 = RL89_Q16_ONE;
    ++doc->frame_count;
    ++a->frame_count;
    return 1;
}

static int rl89_parse_quoted(const char *s, unsigned int n,
                             unsigned int *out_start, unsigned int *out_len)
{
    unsigned int i;
    unsigned int start;
    i = 0U;
    while (i < n && rl89_space((unsigned char)s[i])) ++i;
    if (i >= n || s[i] != '"') return 0;
    ++i;
    start = i;
    while (i < n && s[i] != '"') ++i;
    if (i >= n) return 0;
    *out_start = start;
    *out_len = i - start;
    return 1;
}

static int rl89_parse_frame_directive(RenList89 *doc, RL89_Animation *a,
                                      const char *s, unsigned int n)
{
    unsigned int q0;
    unsigned int qn;
    unsigned int i;
    unsigned int duration;
    if (!rl89_starts(s, n, "frame ")) return 0;
    if (!rl89_parse_quoted(s + 6U, n - 6U, &q0, &qn)) return -1;
    q0 += 6U;
    i = q0 + qn + 1U;
    while (i < n && s[i] != 'f') ++i;
    duration = a->default_frame_ms;
    if (i + 4U <= n && rl89_starts(s + i, n - i, "for ")) {
        if (!rl89_uint(s + i + 4U, n - i - 4U, &duration)) return -1;
    }
    if (!rl89_add_frame(doc, a, s + q0, qn, duration)) return -1;
    return 1;
}

void rl89_init(RenList89 *doc)
{
    if (!doc) return;
    rl89_zero(doc, (unsigned int)sizeof(*doc));
}

int rl89_parse(RenList89 *doc, const char *text, rl89_u32 text_size)
{
    unsigned int pos;
    unsigned int line;
    RL89_Animation *current;
    if (!doc || !text) return 0;
    rl89_init(doc);
    pos = 0U;
    line = 1U;
    current = (RL89_Animation *)0;
    while (pos < text_size) {
        unsigned int start;
        unsigned int n;
        unsigned int l;
        unsigned int r;
        const char *s;
        start = pos;
        while (pos < text_size && text[pos] != '\n') ++pos;
        n = pos - start;
        if (pos < text_size) ++pos;
        l = rl89_trim_left(text + start, n);
        r = rl89_trim_right(text + start + l, n - l);
        s = text + start + l;
        if (r == 0U || s[0] == '#' || s[0] == ';') {
            ++line;
            continue;
        }
        if (rl89_starts(s, r, "image ")) {
            char asset[RL89_NAME_CAP];
            char clip[RL89_NAME_CAP];
            RL89_Animation *a;
            if (doc->animation_count >= RL89_MAX_ANIMATIONS ||
                !rl89_parse_header(s, r, asset, clip)) goto syntax_fail;
            a = &doc->animations[doc->animation_count++];
            rl89_zero(a, (unsigned int)sizeof(*a));
            if (!rl89_copy_n(a->asset_name, RL89_NAME_CAP, asset, (unsigned int)strlen(asset)) ||
                !rl89_copy_n(a->clip_name, RL89_NAME_CAP, clip, (unsigned int)strlen(clip))) goto syntax_fail;
            a->first_frame = doc->frame_count;
            a->first_property = doc->property_count;
            a->default_frame_ms = 100U;
            a->loop_mode = RL89_LOOP_NONE;
            a->used = 1U;
            current = a;
        } else {
            unsigned int q0;
            unsigned int qn;
            int frame_result;
            if (!current) goto syntax_fail;
            if (rl89_parse_quoted(s, r, &q0, &qn)) {
                if (!rl89_add_frame(doc, current, s + q0, qn, current->default_frame_ms)) goto capacity_fail;
            } else if ((frame_result = rl89_parse_frame_directive(doc, current, s, r)) != 0) {
                if (frame_result < 0) goto syntax_fail;
            } else if (rl89_starts(s, r, "pause ")) {
                unsigned int ms;
                if (current->frame_count == 0U || !rl89_seconds_ms(s + 6U, r - 6U, &ms)) goto syntax_fail;
                doc->frames[(rl89_id)(current->first_frame + current->frame_count - 1U)].duration_ms = ms ? ms : 1U;
            } else if (rl89_starts(s, r, "pause_ms ")) {
                unsigned int ms;
                if (current->frame_count == 0U || !rl89_uint(s + 9U, r - 9U, &ms)) goto syntax_fail;
                doc->frames[(rl89_id)(current->first_frame + current->frame_count - 1U)].duration_ms = ms ? ms : 1U;
            } else if (rl89_starts(s, r, "fps ")) {
                unsigned int fps;
                if (!rl89_uint(s + 4U, r - 4U, &fps) || fps == 0U) goto syntax_fail;
                current->default_frame_ms = (1000U + fps / 2U) / fps;
                if (current->default_frame_ms == 0U) current->default_frame_ms = 1U;
            } else if (rl89_equal(s, r, "repeat") || rl89_equal(s, r, "loop")) {
                current->loop_mode = RL89_LOOP_FORWARD;
            } else if (rl89_equal(s, r, "pingpong") || rl89_equal(s, r, "ping-pong")) {
                current->loop_mode = RL89_LOOP_PINGPONG;
            } else if (rl89_equal(s, r, "reverse")) {
                current->loop_mode = RL89_LOOP_REVERSE;
            } else if (rl89_equal(s, r, "hold") || rl89_equal(s, r, "hold_last")) {
                current->loop_mode = RL89_LOOP_HOLD;
            } else if (rl89_equal(s, r, "once")) {
                current->loop_mode = RL89_LOOP_NONE;
            } else {
                unsigned int k;
                unsigned int v;
                k = 0U;
                while (k < r && !rl89_space((unsigned char)s[k]) && s[k] != '=') ++k;
                if (k == 0U || k >= r) goto syntax_fail;
                v = k;
                while (v < r && (rl89_space((unsigned char)s[v]) || s[v] == '=')) ++v;
                if (v >= r) goto syntax_fail;
                if (!rl89_add_property(doc, current, s, k, s + v, r - v)) goto capacity_fail;
            }
        }
        ++line;
    }
    doc->last_error = RL89_ERR_NONE;
    return doc->animation_count > 0U ? 1 : 0;

syntax_fail:
    doc->last_error = RL89_ERR_SYNTAX;
    doc->error_line = line;
    return 0;
capacity_fail:
    doc->last_error = RL89_ERR_CAPACITY;
    doc->error_line = line;
    return 0;
}

rl89_id rl89_find_animation(const RenList89 *doc,
                            const char *asset_name, const char *clip_name)
{
    rl89_id i;
    const char *clip;
    if (!doc || !asset_name) return RL89_INVALID_ID;
    clip = clip_name ? clip_name : "default";
    for (i = 0U; i < doc->animation_count; ++i) {
        if (doc->animations[i].used &&
            strcmp(doc->animations[i].asset_name, asset_name) == 0 &&
            strcmp(doc->animations[i].clip_name, clip) == 0) return i;
    }
    return RL89_INVALID_ID;
}

const RL89_Animation *rl89_animation(const RenList89 *doc, rl89_id animation_id)
{
    if (!doc || animation_id >= doc->animation_count || !doc->animations[animation_id].used) return (const RL89_Animation *)0;
    return &doc->animations[animation_id];
}

const RL89_Frame *rl89_frame(const RenList89 *doc, rl89_id animation_id, rl89_id frame_pos)
{
    const RL89_Animation *a;
    a = rl89_animation(doc, animation_id);
    if (!a || frame_pos >= a->frame_count) return (const RL89_Frame *)0;
    return &doc->frames[(rl89_id)(a->first_frame + frame_pos)];
}

const char *rl89_property(const RenList89 *doc, rl89_id animation_id, const char *key)
{
    const RL89_Animation *a;
    rl89_id i;
    if (!key) return (const char *)0;
    a = rl89_animation(doc, animation_id);
    if (!a) return (const char *)0;
    for (i = 0U; i < a->property_count; ++i) {
        const RL89_Property *p;
        p = &doc->properties[(rl89_id)(a->first_property + i)];
        if (strcmp(p->key, key) == 0) return p->value;
    }
    return (const char *)0;
}

const char *rl89_error_string(int code)
{
    if (code == RL89_ERR_NONE) return "ok";
    if (code == RL89_ERR_ARGUMENT) return "invalid argument";
    if (code == RL89_ERR_SYNTAX) return "syntax error";
    if (code == RL89_ERR_CAPACITY) return "capacity exhausted";
    if (code == RL89_ERR_LAYOUT) return "non-contiguous document layout";
    return "unknown error";
}

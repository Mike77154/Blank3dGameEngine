/* scanemu.c - implementation (C89)
   SPDX-License-Identifier: MIT
*/
#include "scanemu/scanemu.h"

#include <string.h> /* memset, strncpy, strcmp */
#include <ctype.h>  /* isspace */

/* ------------------------------
   Internal helpers
   ------------------------------ */

static void se_str_copy(char* dst, const char* src, scanemu_i32 cap)
{
    scanemu_i32 i = 0;
    if (!dst || cap <= 0) return;
    if (!src) { dst[0] = '\0'; return; }
    while (i < cap - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static scanemu_bool se_str_eq(const char* a, const char* b)
{
    if (!a || !b) return SCANEMU_FALSE;
    return (strcmp(a, b) == 0) ? SCANEMU_TRUE : SCANEMU_FALSE;
}

static scanemu_i32 se_find_index(const scanemu_ctx* ctx, const char* symbol)
{
    scanemu_i32 i;
    if (!ctx || !symbol) return -1;
    for (i = 0; i < ctx->capacity; ++i) {
        if (ctx->bindings[i].in_use && se_str_eq(ctx->bindings[i].symbol, symbol)) {
            return i;
        }
    }
    return -1;
}

static scanemu_i32 se_find_free(const scanemu_ctx* ctx)
{
    scanemu_i32 i;
    if (!ctx) return -1;
    for (i = 0; i < ctx->capacity; ++i) {
        if (!ctx->bindings[i].in_use) return i;
    }
    return -1;
}

static scanemu_i32 se_ensure_entry(scanemu_ctx* ctx, const char* symbol)
{
    scanemu_i32 idx;
    if (!ctx || !symbol) return -1;

    idx = se_find_index(ctx, symbol);
    if (idx >= 0) return idx;

    idx = se_find_free(ctx);
    if (idx < 0) return -1;

    ctx->bindings[idx].in_use = SCANEMU_TRUE;
    se_str_copy(ctx->bindings[idx].symbol, symbol, (scanemu_i32)SCANEMU_MAX_SYMBOL_LEN);
    memset(&ctx->bindings[idx].token, 0, sizeof(ctx->bindings[idx].token));
    ctx->bindings[idx].token.type = SCANEMU_T_NONE;
    ctx->count += 1;
    return idx;
}

static scanemu_bool se_accepts_kind(scanemu_u32 flags, scanemu_event_kind kind)
{
    scanemu_bool on_press   = (flags & SCANEMU_CAPTURE_ON_PRESS) ? SCANEMU_TRUE : SCANEMU_FALSE;
    scanemu_bool on_release = (flags & SCANEMU_CAPTURE_ON_RELEASE) ? SCANEMU_TRUE : SCANEMU_FALSE;

    if (kind == SCANEMU_EV_PRESS && on_press) return SCANEMU_TRUE;
    if (kind == SCANEMU_EV_RELEASE && on_release) return SCANEMU_TRUE;
    /* MOVE/TEXT handled separately by token type gates */
    return SCANEMU_FALSE;
}

static scanemu_bool se_accepts_token_family(scanemu_u32 flags, scanemu_token_type t)
{
    if (t == SCANEMU_T_KEY) return (flags & SCANEMU_LISTEN_KEYS) ? SCANEMU_TRUE : SCANEMU_FALSE;

    if (t == SCANEMU_T_MOUSE_BUTTON || t == SCANEMU_T_MOUSE_WHEEL)
        return (flags & SCANEMU_LISTEN_MOUSE) ? SCANEMU_TRUE : SCANEMU_FALSE;

    if (t == SCANEMU_T_PAD_BUTTON || t == SCANEMU_T_PAD_AXIS_POS || t == SCANEMU_T_PAD_AXIS_NEG || t == SCANEMU_T_PAD_HAT)
        return (flags & SCANEMU_LISTEN_GAMEPAD) ? SCANEMU_TRUE : SCANEMU_FALSE;

    if (t == SCANEMU_T_TOUCH_GESTURE) return (flags & SCANEMU_LISTEN_TOUCH) ? SCANEMU_TRUE : SCANEMU_FALSE;
    if (t == SCANEMU_T_CUSTOM) return (flags & SCANEMU_LISTEN_CUSTOM) ? SCANEMU_TRUE : SCANEMU_FALSE;

    /* Text capture is signaled via event.kind, not token.type (token.type may be CUSTOM). */
    return SCANEMU_FALSE;
}

static float se_fabs(float x) { return (x < 0.0f) ? -x : x; }

/* For MOVE axis events: convert a generic axis token (code/value) to POS/NEG binding token. */
static scanemu_bool se_capture_axis(scanemu_ctx* ctx, const scanemu_event* ev, scanemu_token* out_binding)
{
    float v;
    if (!ctx || !ev || !out_binding) return SCANEMU_FALSE;

    v = ev->token.value;
    if (se_fabs(v) < ctx->axis_threshold) return SCANEMU_FALSE;

    *out_binding = ev->token;
    if (v >= 0.0f) out_binding->type = SCANEMU_T_PAD_AXIS_POS;
    else           out_binding->type = SCANEMU_T_PAD_AXIS_NEG;

    out_binding->value = ctx->axis_threshold; /* store threshold */
    return SCANEMU_TRUE;
}

/* ------------------------------
   Public API
   ------------------------------ */

void scanemu_init(scanemu_ctx* ctx, scanemu_binding* storage, scanemu_i32 storage_count)
{
    scanemu_i32 i;
    if (!ctx) return;

    memset(ctx, 0, sizeof(*ctx));
    ctx->bindings = storage;
    ctx->capacity = storage_count;
    ctx->count = 0;
    ctx->listening = SCANEMU_FALSE;
    ctx->listen_flags = SCANEMU_LISTEN_ANY;
    ctx->axis_threshold = 0.5f;
    ctx->last_valid = SCANEMU_FALSE;

    if (storage && storage_count > 0) {
        for (i = 0; i < storage_count; ++i) {
            storage[i].in_use = SCANEMU_FALSE;
            storage[i].symbol[0] = '\0';
            memset(&storage[i].token, 0, sizeof(storage[i].token));
            storage[i].token.type = SCANEMU_T_NONE;
        }
    }
}

void scanemu_set_on_bound(scanemu_ctx* ctx, scanemu_on_bound_fn fn, void* user)
{
    if (!ctx) return;
    ctx->on_bound = fn;
    ctx->on_bound_user = user;
}

scanemu_bool scanemu_is_listening(const scanemu_ctx* ctx)
{
    if (!ctx) return SCANEMU_FALSE;
    return ctx->listening;
}

void scanemu_listen(scanemu_ctx* ctx, const char* symbol, scanemu_u32 listen_flags)
{
    if (!ctx || !symbol) return;
    se_str_copy(ctx->listen_symbol, symbol, (scanemu_i32)SCANEMU_MAX_SYMBOL_LEN);
    ctx->listen_flags = listen_flags ? listen_flags : SCANEMU_LISTEN_ANY;
    ctx->listening = SCANEMU_TRUE;
}

void scanemu_cancel(scanemu_ctx* ctx)
{
    if (!ctx) return;
    ctx->listening = SCANEMU_FALSE;
    ctx->listen_symbol[0] = '\0';
}

scanemu_feed_result scanemu_feed(scanemu_ctx* ctx, const scanemu_event* ev)
{
    scanemu_i32 idx;
    scanemu_token binding_token;

    if (!ctx || !ev) return SCANEMU_FEED_IGNORED;
    if (!ctx->listening) return SCANEMU_FEED_IGNORED;

    /* TEXT capture (optional) */
    if (ev->kind == SCANEMU_EV_TEXT) {
        if (!(ctx->listen_flags & SCANEMU_LISTEN_TEXT)) return SCANEMU_FEED_LISTENING;
        /* Store as CUSTOM with extra=0; caller can interpret text separately if desired */
        binding_token = ev->token;
        binding_token.type = SCANEMU_T_CUSTOM;
        /* capture */
        idx = se_ensure_entry(ctx, ctx->listen_symbol);
        if (idx < 0) return SCANEMU_FEED_LISTENING;
        ctx->bindings[idx].token = binding_token;
        ctx->last_valid = SCANEMU_TRUE;
        se_str_copy(ctx->last_symbol, ctx->listen_symbol, (scanemu_i32)SCANEMU_MAX_SYMBOL_LEN);
        ctx->last_token = binding_token;
        ctx->listening = SCANEMU_FALSE;
        if (ctx->on_bound) ctx->on_bound(ctx->on_bound_user, ctx->last_symbol, &ctx->last_token);
        return SCANEMU_FEED_CAPTURED;
    }

    /* Axis capture from MOVE events: callers should emit token.code=axis_id and token.value in [-1..1]. */
    if (ev->kind == SCANEMU_EV_MOVE) {
        if (!se_accepts_token_family(ctx->listen_flags, SCANEMU_T_PAD_AXIS_POS) &&
            !se_accepts_token_family(ctx->listen_flags, SCANEMU_T_PAD_AXIS_NEG)) {
            /* If gamepad listening is off, ignore */
            return SCANEMU_FEED_LISTENING;
        }
        if (!se_capture_axis(ctx, ev, &binding_token)) return SCANEMU_FEED_LISTENING;

        idx = se_ensure_entry(ctx, ctx->listen_symbol);
        if (idx < 0) return SCANEMU_FEED_LISTENING;

        ctx->bindings[idx].token = binding_token;
        ctx->last_valid = SCANEMU_TRUE;
        se_str_copy(ctx->last_symbol, ctx->listen_symbol, (scanemu_i32)SCANEMU_MAX_SYMBOL_LEN);
        ctx->last_token = binding_token;

        ctx->listening = SCANEMU_FALSE;
        if (ctx->on_bound) ctx->on_bound(ctx->on_bound_user, ctx->last_symbol, &ctx->last_token);
        return SCANEMU_FEED_CAPTURED;
    }

    /* Button/key capture: only capture if kind matches policy (press/release). */
    if (!se_accepts_kind(ctx->listen_flags, ev->kind)) return SCANEMU_FEED_LISTENING;

    if (!se_accepts_token_family(ctx->listen_flags, ev->token.type)) return SCANEMU_FEED_LISTENING;

    idx = se_ensure_entry(ctx, ctx->listen_symbol);
    if (idx < 0) return SCANEMU_FEED_LISTENING;

    ctx->bindings[idx].token = ev->token;

    ctx->last_valid = SCANEMU_TRUE;
    se_str_copy(ctx->last_symbol, ctx->listen_symbol, (scanemu_i32)SCANEMU_MAX_SYMBOL_LEN);
    ctx->last_token = ev->token;

    ctx->listening = SCANEMU_FALSE;
    if (ctx->on_bound) ctx->on_bound(ctx->on_bound_user, ctx->last_symbol, &ctx->last_token);

    return SCANEMU_FEED_CAPTURED;
}

scanemu_bool scanemu_has(const scanemu_ctx* ctx, const char* symbol)
{
    return (se_find_index(ctx, symbol) >= 0) ? SCANEMU_TRUE : SCANEMU_FALSE;
}

scanemu_bool scanemu_get(const scanemu_ctx* ctx, const char* symbol, scanemu_token* out_token)
{
    scanemu_i32 idx;
    if (!ctx || !symbol || !out_token) return SCANEMU_FALSE;
    idx = se_find_index(ctx, symbol);
    if (idx < 0) return SCANEMU_FALSE;
    *out_token = ctx->bindings[idx].token;
    return SCANEMU_TRUE;
}

scanemu_bool scanemu_set(scanemu_ctx* ctx, const char* symbol, const scanemu_token* token)
{
    scanemu_i32 idx;
    if (!ctx || !symbol || !token) return SCANEMU_FALSE;
    idx = se_ensure_entry(ctx, symbol);
    if (idx < 0) return SCANEMU_FALSE;
    ctx->bindings[idx].token = *token;
    return SCANEMU_TRUE;
}

scanemu_bool scanemu_clear(scanemu_ctx* ctx, const char* symbol)
{
    scanemu_i32 idx;
    if (!ctx || !symbol) return SCANEMU_FALSE;
    idx = se_find_index(ctx, symbol);
    if (idx < 0) return SCANEMU_FALSE;

    ctx->bindings[idx].in_use = SCANEMU_FALSE;
    ctx->bindings[idx].symbol[0] = '\0';
    memset(&ctx->bindings[idx].token, 0, sizeof(ctx->bindings[idx].token));
    ctx->bindings[idx].token.type = SCANEMU_T_NONE;
    ctx->count -= 1;
    if (ctx->count < 0) ctx->count = 0;
    return SCANEMU_TRUE;
}

scanemu_i32 scanemu_binding_count(const scanemu_ctx* ctx)
{
    if (!ctx) return 0;
    return ctx->count;
}

scanemu_bool scanemu_binding_at(const scanemu_ctx* ctx, scanemu_i32 index,
                                const char** out_symbol, scanemu_token* out_token)
{
    scanemu_i32 i;
    scanemu_i32 n = 0;
    if (!ctx) return SCANEMU_FALSE;

    for (i = 0; i < ctx->capacity; ++i) {
        if (!ctx->bindings[i].in_use) continue;
        if (n == index) {
            if (out_symbol) *out_symbol = ctx->bindings[i].symbol;
            if (out_token) *out_token = ctx->bindings[i].token;
            return SCANEMU_TRUE;
        }
        n++;
    }
    return SCANEMU_FALSE;
}

scanemu_bool scanemu_last_get(const scanemu_ctx* ctx, const char** out_symbol, scanemu_token* out_token)
{
    if (!ctx || !ctx->last_valid) return SCANEMU_FALSE;
    if (out_symbol) *out_symbol = ctx->last_symbol;
    if (out_token) *out_token = ctx->last_token;
    return SCANEMU_TRUE;
}

void scanemu_last_clear(scanemu_ctx* ctx)
{
    if (!ctx) return;
    ctx->last_valid = SCANEMU_FALSE;
    ctx->last_symbol[0] = '\0';
    memset(&ctx->last_token, 0, sizeof(ctx->last_token));
    ctx->last_token.type = SCANEMU_T_NONE;
}

/* ------------------------------
   Token helpers
   ------------------------------ */

scanemu_bool scanemu_token_equal(const scanemu_token* a, const scanemu_token* b)
{
    if (!a || !b) return SCANEMU_FALSE;
    if (a->type != b->type) return SCANEMU_FALSE;
    if (a->device_id != b->device_id) return SCANEMU_FALSE;
    if (a->code != b->code) return SCANEMU_FALSE;
    if (a->extra != b->extra) return SCANEMU_FALSE;

    /* For axis tokens, value matters (threshold). For others, ignore value. */
    if (a->type == SCANEMU_T_PAD_AXIS_POS || a->type == SCANEMU_T_PAD_AXIS_NEG) {
        /* Accept small float differences */
        float dv = a->value - b->value;
        if (dv < 0.0f) dv = -dv;
        if (dv > 0.0001f) return SCANEMU_FALSE;
    }
    return SCANEMU_TRUE;
}

/* Tiny integer/string utilities (avoid snprintf for portability). */
static scanemu_i32 se_append_char(char* out, scanemu_i32 cap, scanemu_i32* io_len, char c)
{
    if (!out || cap <= 0 || !io_len) return 0;
    if (*io_len >= cap - 1) return 0;
    out[*io_len] = c;
    *io_len += 1;
    out[*io_len] = '\0';
    return 1;
}

static scanemu_i32 se_append_str(char* out, scanemu_i32 cap, scanemu_i32* io_len, const char* s)
{
    scanemu_i32 i = 0;
    if (!s) return 1;
    while (s[i]) {
        if (!se_append_char(out, cap, io_len, s[i])) return 0;
        i++;
    }
    return 1;
}

static scanemu_i32 se_append_int(char* out, scanemu_i32 cap, scanemu_i32* io_len, scanemu_i32 v)
{
    char buf[32];
    scanemu_i32 n = 0;
    scanemu_i32 i;
    scanemu_i32 neg = 0;
    if (v < 0) { neg = 1; v = -v; }

    /* build reversed */
    do {
        buf[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v > 0 && n < (scanemu_i32)(sizeof(buf) - 1));

    if (neg) buf[n++] = '-';

    /* append forward */
    for (i = n - 1; i >= 0; --i) {
        if (!se_append_char(out, cap, io_len, buf[i])) return 0;
    }
    return 1;
}

static scanemu_i32 se_append_float_2(char* out, scanemu_i32 cap, scanemu_i32* io_len, float f)
{
    /* very small, stable: prints with 2 decimals, no locale */
    scanemu_i32 iv;
    scanemu_i32 frac;
    scanemu_i32 neg = 0;
    float af = f;

    if (af < 0.0f) { neg = 1; af = -af; }
    iv = (scanemu_i32)af;
    frac = (scanemu_i32)((af - (float)iv) * 100.0f + 0.5f);
    if (frac >= 100) { iv += 1; frac -= 100; }

    if (neg) { if (!se_append_char(out, cap, io_len, '-')) return 0; }
    if (!se_append_int(out, cap, io_len, iv)) return 0;
    if (!se_append_char(out, cap, io_len, '.')) return 0;
    if (frac < 10) { if (!se_append_char(out, cap, io_len, '0')) return 0; }
    if (!se_append_int(out, cap, io_len, frac)) return 0;
    return 1;
}

static scanemu_bool se_starts_with(const char* s, const char* prefix)
{
    scanemu_i32 i = 0;
    if (!s || !prefix) return SCANEMU_FALSE;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return SCANEMU_FALSE;
        i++;
    }
    return SCANEMU_TRUE;
}

static const char* se_skip_ws(const char* s)
{
    while (s && *s && isspace((unsigned char)*s)) s++;
    return s;
}

static scanemu_bool se_parse_int(const char** io_s, scanemu_i32* out)
{
    const char* s;
    scanemu_i32 sign = 1;
    scanemu_i32 v = 0;
    scanemu_bool any = SCANEMU_FALSE;

    if (!io_s || !*io_s || !out) return SCANEMU_FALSE;
    s = se_skip_ws(*io_s);

    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (scanemu_i32)(*s - '0');
        s++;
        any = SCANEMU_TRUE;
    }
    if (!any) return SCANEMU_FALSE;
    *out = v * sign;
    *io_s = s;
    return SCANEMU_TRUE;
}

static scanemu_bool se_parse_float(const char** io_s, float* out_f)
{
    /* minimal float parser: int[.frac] */
    const char* s;
    scanemu_i32 iv;
    scanemu_i32 frac = 0;
    scanemu_i32 frac_div = 1;
    scanemu_i32 sign = 1;

    if (!io_s || !*io_s || !out_f) return SCANEMU_FALSE;
    s = se_skip_ws(*io_s);

    if (*s == '-') { sign = -1; s++; }
    /* integer part */
    {
        scanemu_bool ok;
        const char* tmp = s;
        ok = se_parse_int(&tmp, &iv);
        if (!ok) return SCANEMU_FALSE;
        s = tmp;
    }

    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9' && frac_div < 1000000) {
            frac = frac * 10 + (scanemu_i32)(*s - '0');
            frac_div *= 10;
            s++;
        }
    }

    *out_f = (float)sign * ((float)iv + ((float)frac / (float)frac_div));
    *io_s = s;
    return SCANEMU_TRUE;
}

scanemu_bool scanemu_token_to_string(const scanemu_token* t, char* out, scanemu_i32 out_cap)
{
    scanemu_i32 len = 0;
    if (!t || !out || out_cap <= 0) return SCANEMU_FALSE;
    out[0] = '\0';

    switch (t->type) {
        case SCANEMU_T_KEY:           if (!se_append_str(out, out_cap, &len, "KEY")) return SCANEMU_FALSE; break;
        case SCANEMU_T_MOUSE_BUTTON:  if (!se_append_str(out, out_cap, &len, "MOUSEBTN")) return SCANEMU_FALSE; break;
        case SCANEMU_T_MOUSE_WHEEL:   if (!se_append_str(out, out_cap, &len, "WHEEL")) return SCANEMU_FALSE; break;
        case SCANEMU_T_PAD_BUTTON:    if (!se_append_str(out, out_cap, &len, "PADBTN")) return SCANEMU_FALSE; break;
        case SCANEMU_T_PAD_AXIS_POS:  if (!se_append_str(out, out_cap, &len, "PADAXIS+")) return SCANEMU_FALSE; break;
        case SCANEMU_T_PAD_AXIS_NEG:  if (!se_append_str(out, out_cap, &len, "PADAXIS-")) return SCANEMU_FALSE; break;
        case SCANEMU_T_PAD_HAT:       if (!se_append_str(out, out_cap, &len, "PADHAT")) return SCANEMU_FALSE; break;
        case SCANEMU_T_TOUCH_GESTURE: if (!se_append_str(out, out_cap, &len, "TOUCH")) return SCANEMU_FALSE; break;
        case SCANEMU_T_CUSTOM:        if (!se_append_str(out, out_cap, &len, "CUSTOM")) return SCANEMU_FALSE; break;
        default:                      if (!se_append_str(out, out_cap, &len, "NONE")) return SCANEMU_FALSE; break;
    }

    if (!se_append_char(out, out_cap, &len, ':')) return SCANEMU_FALSE;
    if (!se_append_int(out, out_cap, &len, (scanemu_i32)t->device_id)) return SCANEMU_FALSE;
    if (!se_append_char(out, out_cap, &len, ':')) return SCANEMU_FALSE;
    if (!se_append_int(out, out_cap, &len, (scanemu_i32)t->code)) return SCANEMU_FALSE;

    if (t->type == SCANEMU_T_PAD_AXIS_POS || t->type == SCANEMU_T_PAD_AXIS_NEG) {
        if (!se_append_char(out, out_cap, &len, ':')) return SCANEMU_FALSE;
        if (!se_append_float_2(out, out_cap, &len, t->value)) return SCANEMU_FALSE;
    } else if (t->type == SCANEMU_T_PAD_HAT) {
        if (!se_append_char(out, out_cap, &len, ':')) return SCANEMU_FALSE;
        if (!se_append_int(out, out_cap, &len, (scanemu_i32)t->extra)) return SCANEMU_FALSE;
    }

    return SCANEMU_TRUE;
}

scanemu_bool scanemu_token_from_string(scanemu_token* out_t, const char* s)
{
    const char* p;
    scanemu_i32 device;
    scanemu_i32 code;
    float val;

    if (!out_t || !s) return SCANEMU_FALSE;
    memset(out_t, 0, sizeof(*out_t));
    out_t->type = SCANEMU_T_NONE;

    p = se_skip_ws(s);

    if (se_starts_with(p, "KEY")) out_t->type = SCANEMU_T_KEY;
    else if (se_starts_with(p, "MOUSEBTN")) out_t->type = SCANEMU_T_MOUSE_BUTTON;
    else if (se_starts_with(p, "WHEEL")) out_t->type = SCANEMU_T_MOUSE_WHEEL;
    else if (se_starts_with(p, "PADBTN")) out_t->type = SCANEMU_T_PAD_BUTTON;
    else if (se_starts_with(p, "PADAXIS+")) out_t->type = SCANEMU_T_PAD_AXIS_POS;
    else if (se_starts_with(p, "PADAXIS-")) out_t->type = SCANEMU_T_PAD_AXIS_NEG;
    else if (se_starts_with(p, "PADHAT")) out_t->type = SCANEMU_T_PAD_HAT;
    else if (se_starts_with(p, "TOUCH")) out_t->type = SCANEMU_T_TOUCH_GESTURE;
    else if (se_starts_with(p, "CUSTOM")) out_t->type = SCANEMU_T_CUSTOM;
    else return SCANEMU_FALSE;

    /* advance to first ':' */
    while (*p && *p != ':') p++;
    if (*p != ':') return SCANEMU_FALSE;
    p++;

    if (!se_parse_int(&p, &device)) return SCANEMU_FALSE;
    if (*p != ':') return SCANEMU_FALSE;
    p++;

    if (!se_parse_int(&p, &code)) return SCANEMU_FALSE;

    out_t->device_id = device;
    out_t->code = code;

    if (out_t->type == SCANEMU_T_PAD_AXIS_POS || out_t->type == SCANEMU_T_PAD_AXIS_NEG) {
        if (*p != ':') return SCANEMU_FALSE;
        p++;
        if (!se_parse_float(&p, &val)) return SCANEMU_FALSE;
        out_t->value = val;
    } else if (out_t->type == SCANEMU_T_PAD_HAT) {
        scanemu_i32 extra;
        if (*p != ':') return SCANEMU_FALSE;
        p++;
        if (!se_parse_int(&p, &extra)) return SCANEMU_FALSE;
        out_t->extra = (scanemu_u32)extra;
    }

    return SCANEMU_TRUE;
}

scanemu_bool scanemu_binding_matches_event(const scanemu_token* binding, const scanemu_event* ev)
{
    if (!binding || !ev) return SCANEMU_FALSE;

    /* For button-like bindings, match on PRESS events of same family. */
    if (binding->type == SCANEMU_T_KEY ||
        binding->type == SCANEMU_T_MOUSE_BUTTON ||
        binding->type == SCANEMU_T_PAD_BUTTON ||
        binding->type == SCANEMU_T_PAD_HAT ||
        binding->type == SCANEMU_T_TOUCH_GESTURE ||
        binding->type == SCANEMU_T_CUSTOM) {

        if (ev->kind != SCANEMU_EV_PRESS) return SCANEMU_FALSE;
        return scanemu_token_equal(binding, &ev->token);
    }

    /* For axis bindings, accept MOVE events with sign and threshold. */
    if (binding->type == SCANEMU_T_PAD_AXIS_POS || binding->type == SCANEMU_T_PAD_AXIS_NEG) {
        float v;
        if (ev->kind != SCANEMU_EV_MOVE) return SCANEMU_FALSE;

        /* Expect backend to send a generic axis token (code=value) with type=PAD_AXIS_POS/NEG OR CUSTOM.
           We'll match by device/code and compare signed magnitude. */
        if (binding->device_id != ev->token.device_id) return SCANEMU_FALSE;
        if (binding->code != ev->token.code) return SCANEMU_FALSE;

        v = ev->token.value;
        if (binding->type == SCANEMU_T_PAD_AXIS_POS) {
            if (v < 0.0f) return SCANEMU_FALSE;
            return (v >= binding->value) ? SCANEMU_TRUE : SCANEMU_FALSE;
        } else {
            if (v > 0.0f) return SCANEMU_FALSE;
            return ((-v) >= binding->value) ? SCANEMU_TRUE : SCANEMU_FALSE;
        }
    }

    return SCANEMU_FALSE;
}

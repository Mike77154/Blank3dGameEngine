/* input_hook.c - C89-friendly keyboard hook core */

#include "input_hook.h"

#include <string.h> /* memset, memcpy, strcmp */
#include <ctype.h>  /* tolower */

/* =========================
   Internal bit helpers
   ========================= */

static void ihk_bits_zero(ihk_u8 *dst, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) dst[i] = 0;
}

static void ihk_bits_copy(ihk_u8 *dst, const ihk_u8 *src, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) dst[i] = src[i];
}

static void ihk_bits_andnot(ihk_u8 *out, const ihk_u8 *a, const ihk_u8 *b, size_t n)
{
    /* out = a & ~b */
    size_t i;
    for (i = 0; i < n; ++i) out[i] = (ihk_u8)(a[i] & (ihk_u8)(~b[i]));
}

static void ihk_bits_notand(ihk_u8 *out, const ihk_u8 *a, const ihk_u8 *b, size_t n)
{
    /* out = ~a & b */
    size_t i;
    for (i = 0; i < n; ++i) out[i] = (ihk_u8)((ihk_u8)(~a[i]) & b[i]);
}

static int ihk_bit_test(const ihk_u8 *bits, ihk_u8 usage)
{
    ihk_u8 byte = (ihk_u8)(usage >> 3);
    ihk_u8 mask = (ihk_u8)(1u << (usage & 7u));
    return (bits[byte] & mask) ? 1 : 0;
}

/* =========================
   Binding dispatch
   ========================= */

static void ihk_dispatch_one(input_hook *h, ihk_key key, ihk_event_kind kind)
{
    int i;
    ihk_u8 want_bit = 0;

    if (!h) return;

    /* 1) global callback */
    if (h->on_event) {
        h->on_event(key, kind, h->on_event_user);
    }

    /* 2) binding table */
    switch (kind) {
    case IHK_EVENT_PRESS:   want_bit = IHK_BIND_PRESS; break;
    case IHK_EVENT_RELEASE: want_bit = IHK_BIND_RELEASE; break;
    case IHK_EVENT_HOLD:    want_bit = IHK_BIND_HOLD; break;
    default: return;
    }

    for (i = 0; i < (int)IHK_MAX_BINDINGS; ++i) {
        ihk_binding_entry *b = &h->bindings[i];
        if (!b->in_use) continue;
        if (!b->fn) continue;
        if ((b->mask & want_bit) == 0) continue;

        /* key match: exact or wildcard */
        if (b->key != IHK_KEY_NONE && b->key != key) {
            continue;
        }

        b->fn(key, kind, b->user);
    }
}

static void ihk_dispatch_events(input_hook *h)
{
    /* 256 keys; we walk bytes and bits for pressed/released; hold is optional. */
    ihk_u32 opt;
    int byte_i;

    if (!h) return;

    opt = h->options;

    /* Pressed */
    for (byte_i = 0; byte_i < (int)IHK_KB_BITS_BYTES; ++byte_i) {
        ihk_u8 v = h->kb_pressed[byte_i];
        if (v) {
            int bit;
            for (bit = 0; bit < 8; ++bit) {
                if (v & (ihk_u8)(1u << bit)) {
                    ihk_u8 usage = (ihk_u8)((byte_i << 3) | bit);
                    ihk_key key = IHK_HID_KB(usage);
                    ihk_dispatch_one(h, key, IHK_EVENT_PRESS);
                    if ((opt & IHK_OPT_EMIT_HOLD) && (opt & IHK_OPT_HOLD_INCLUDES_PRESS)) {
                        ihk_dispatch_one(h, key, IHK_EVENT_HOLD);
                    }
                }
            }
        }
    }

    /* Released */
    for (byte_i = 0; byte_i < (int)IHK_KB_BITS_BYTES; ++byte_i) {
        ihk_u8 v = h->kb_released[byte_i];
        if (v) {
            int bit;
            for (bit = 0; bit < 8; ++bit) {
                if (v & (ihk_u8)(1u << bit)) {
                    ihk_u8 usage = (ihk_u8)((byte_i << 3) | bit);
                    ihk_key key = IHK_HID_KB(usage);
                    ihk_dispatch_one(h, key, IHK_EVENT_RELEASE);
                }
            }
        }
    }

    /* Hold (optional) */
    if (opt & IHK_OPT_EMIT_HOLD) {
        for (byte_i = 0; byte_i < (int)IHK_KB_BITS_BYTES; ++byte_i) {
            ihk_u8 v = h->kb_curr[byte_i];
            if (v) {
                int bit;
                for (bit = 0; bit < 8; ++bit) {
                    ihk_u8 m = (ihk_u8)(1u << bit);
                    ihk_u8 usage;
                    ihk_key key;

                    if ((v & m) == 0) continue;

                    usage = (ihk_u8)((byte_i << 3) | bit);
                    key = IHK_HID_KB(usage);

                    /* If HOLD should NOT include press frame, skip keys pressed this frame */
                    if ((opt & IHK_OPT_HOLD_INCLUDES_PRESS) == 0) {
                        if (ihk_bit_test(h->kb_pressed, usage)) {
                            continue;
                        }
                    }

                    ihk_dispatch_one(h, key, IHK_EVENT_HOLD);
                }
            }
        }
    }
}

/* =========================
   Public API
   ========================= */

void input_hook_init(input_hook *h, const ihk_backend *backend)
{
    int i;

    if (!h) return;

    /* zero everything */
    h->backend.user = 0;
    h->backend.poll_keyboard = 0;
    h->backend.shutdown = 0;
    h->backend.capabilities = 0;

    ihk_bits_zero(h->kb_prev, IHK_KB_BITS_BYTES);
    ihk_bits_zero(h->kb_curr, IHK_KB_BITS_BYTES);
    ihk_bits_zero(h->kb_pressed, IHK_KB_BITS_BYTES);
    ihk_bits_zero(h->kb_released, IHK_KB_BITS_BYTES);

    h->frame_index = 0;
    h->options = 0;

    h->on_event = 0;
    h->on_event_user = 0;

    for (i = 0; i < (int)IHK_MAX_BINDINGS; ++i) {
        h->bindings[i].key = IHK_KEY_NONE;
        h->bindings[i].mask = 0;
        h->bindings[i].fn = 0;
        h->bindings[i].user = 0;
        h->bindings[i].in_use = 0;
    }

    if (backend) {
        h->backend = *backend;
    }
}

void input_hook_set_backend(input_hook *h, const ihk_backend *backend)
{
    if (!h) return;
    if (!backend) {
        h->backend.user = 0;
        h->backend.poll_keyboard = 0;
        h->backend.shutdown = 0;
        h->backend.capabilities = 0;
        return;
    }
    h->backend = *backend;
}

void input_hook_shutdown(input_hook *h)
{
    if (!h) return;

    if (h->backend.shutdown) {
        h->backend.shutdown(h->backend.user);
    }

    /* Clear backend pointers to avoid use-after-shutdown */
    h->backend.user = 0;
    h->backend.poll_keyboard = 0;
    h->backend.shutdown = 0;
    h->backend.capabilities = 0;
}

void input_hook_update(input_hook *h)
{
    if (!h) return;

    /* Advance frame */
    h->frame_index += 1;

    /* Shift current -> previous */
    ihk_bits_copy(h->kb_prev, h->kb_curr, IHK_KB_BITS_BYTES);

    /* Poll new state */
    ihk_bits_zero(h->kb_curr, IHK_KB_BITS_BYTES);
    if (h->backend.poll_keyboard) {
        h->backend.poll_keyboard(h->backend.user, h->kb_curr, IHK_KB_BITS_BYTES);
    }

    /* pressed = curr & ~prev */
    ihk_bits_andnot(h->kb_pressed, h->kb_curr, h->kb_prev, IHK_KB_BITS_BYTES);

    /* released = ~curr & prev */
    ihk_bits_notand(h->kb_released, h->kb_curr, h->kb_prev, IHK_KB_BITS_BYTES);

    /* dispatch callbacks */
    ihk_dispatch_events(h);
}

static int ihk_kb_query_bits(const ihk_u8 *bits, ihk_key key)
{
    ihk_u8 page;
    ihk_u8 usage;

    if (!bits) return 0;

    page = IHK_HID_PAGE(key);
    if (page != (ihk_u8)IHK_HID_PAGE_KEYBOARD) {
        return 0;
    }

    usage = IHK_HID_USAGE(key);
    return ihk_bit_test(bits, usage);
}

int input_hook_down(const input_hook *h, ihk_key key)
{
    if (!h) return 0;
    return ihk_kb_query_bits(h->kb_curr, key);
}

int input_hook_pressed(const input_hook *h, ihk_key key)
{
    if (!h) return 0;
    return ihk_kb_query_bits(h->kb_pressed, key);
}

int input_hook_released(const input_hook *h, ihk_key key)
{
    if (!h) return 0;
    return ihk_kb_query_bits(h->kb_released, key);
}

int input_hook_kb_down(const input_hook *h, ihk_u8 usage)
{
    if (!h) return 0;
    return ihk_bit_test(h->kb_curr, usage);
}

int input_hook_kb_pressed(const input_hook *h, ihk_u8 usage)
{
    if (!h) return 0;
    return ihk_bit_test(h->kb_pressed, usage);
}

int input_hook_kb_released(const input_hook *h, ihk_u8 usage)
{
    if (!h) return 0;
    return ihk_bit_test(h->kb_released, usage);
}

int input_hook_down_name(const input_hook *h, const char *name)
{
    ihk_key k = input_hook_key_from_name(name);
    if (k == IHK_KEY_NONE) return 0;
    return input_hook_down(h, k);
}

int input_hook_pressed_name(const input_hook *h, const char *name)
{
    ihk_key k = input_hook_key_from_name(name);
    if (k == IHK_KEY_NONE) return 0;
    return input_hook_pressed(h, k);
}

int input_hook_released_name(const input_hook *h, const char *name)
{
    ihk_key k = input_hook_key_from_name(name);
    if (k == IHK_KEY_NONE) return 0;
    return input_hook_released(h, k);
}

const ihk_u8 *input_hook_bits_curr(const input_hook *h)
{
    if (!h) return 0;
    return h->kb_curr;
}

const ihk_u8 *input_hook_bits_pressed(const input_hook *h)
{
    if (!h) return 0;
    return h->kb_pressed;
}

const ihk_u8 *input_hook_bits_released(const input_hook *h)
{
    if (!h) return 0;
    return h->kb_released;
}

void input_hook_set_options(input_hook *h, ihk_u32 options)
{
    if (!h) return;
    h->options = options;
}

void input_hook_set_event_callback(input_hook *h, ihk_event_fn fn, void *user)
{
    if (!h) return;
    h->on_event = fn;
    h->on_event_user = user;
}

int input_hook_bind(input_hook *h, ihk_key key, ihk_u8 mask, ihk_event_fn fn, void *user)
{
    int i;
    if (!h || !fn || mask == 0) return -1;

    for (i = 0; i < (int)IHK_MAX_BINDINGS; ++i) {
        ihk_binding_entry *b = &h->bindings[i];
        if (!b->in_use) {
            b->key = key;
            b->mask = mask;
            b->fn = fn;
            b->user = user;
            b->in_use = 1;
            return 0;
        }
    }

    return -1;
}

void input_hook_unbind(input_hook *h, ihk_event_fn fn, void *user)
{
    int i;
    if (!h || !fn) return;

    for (i = 0; i < (int)IHK_MAX_BINDINGS; ++i) {
        ihk_binding_entry *b = &h->bindings[i];
        if (b->in_use && b->fn == fn && b->user == user) {
            b->in_use = 0;
            b->key = IHK_KEY_NONE;
            b->mask = 0;
            b->fn = 0;
            b->user = 0;
        }
    }
}

void input_hook_clear_bindings(input_hook *h)
{
    int i;
    if (!h) return;

    for (i = 0; i < (int)IHK_MAX_BINDINGS; ++i) {
        h->bindings[i].in_use = 0;
        h->bindings[i].key = IHK_KEY_NONE;
        h->bindings[i].mask = 0;
        h->bindings[i].fn = 0;
        h->bindings[i].user = 0;
    }
}

/* =========================
   Name helpers
   ========================= */

static int ihk_is_hex_digit(char c)
{
    if (c >= '0' && c <= '9') return 1;
    if (c >= 'a' && c <= 'f') return 1;
    if (c >= 'A' && c <= 'F') return 1;
    return 0;
}

static int ihk_hex_val(char c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return 10 + (int)(c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (int)(c - 'A');
    return 0;
}

static int ihk_parse_hex_byte(const char *s, ihk_u8 *out)
{
    /* expects exactly two hex chars */
    if (!s || !out) return 0;
    if (!ihk_is_hex_digit(s[0]) || !ihk_is_hex_digit(s[1])) return 0;
    *out = (ihk_u8)((ihk_hex_val(s[0]) << 4) | ihk_hex_val(s[1]));
    return 1;
}

static int ihk_parse_u32_dec(const char *s, int *out)
{
    int v = 0;
    int any = 0;
    if (!s || !out) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        any = 1;
        v = (v * 10) + (int)(*s - '0');
        ++s;
    }
    if (!any) return 0;
    *out = v;
    return 1;
}

static int ihk_str_eq(const char *a, const char *b)
{
    return (a && b && strcmp(a, b) == 0) ? 1 : 0;
}

static void ihk_normalize_name(const char *name, char *out, size_t out_sz)
{
    /* lower-case, trim spaces, convert '-' to '_' */
    size_t i = 0;
    size_t j = 0;

    if (!out || out_sz == 0) return;
    out[0] = '\0';

    if (!name) return;

    /* skip leading spaces */
    while (*name && isspace((unsigned char)*name)) {
        ++name;
    }

    while (name[i] && j + 1 < out_sz) {
        char c = name[i];
        if (c == '\0') break;

        /* stop at trailing spaces (we'll trim later) */
        if (c == '\r' || c == '\n') break;

        if (c == '-') c = '_';
        out[j] = (char)tolower((unsigned char)c);
        ++i;
        ++j;
    }

    /* trim trailing spaces */
    while (j > 0 && isspace((unsigned char)out[j - 1])) {
        --j;
    }

    out[j] = '\0';
}

static ihk_u8 ihk_kb_usage_from_letter(char c)
{
    /* HID: A=0x04..Z=0x1D */
    if (c >= 'a' && c <= 'z') {
        return (ihk_u8)(0x04 + (ihk_u8)(c - 'a'));
    }
    return 0;
}

static ihk_u8 ihk_kb_usage_from_digit(char c)
{
    /* HID: '1'=0x1E..'9'=0x26, '0'=0x27 */
    if (c >= '1' && c <= '9') {
        return (ihk_u8)(0x1E + (ihk_u8)(c - '1'));
    }
    if (c == '0') {
        return (ihk_u8)0x27;
    }
    return 0;
}

static ihk_u8 ihk_kb_usage_from_fkey(int n)
{
    /* F1..F12 => 0x3A..0x45
       F13..F24 => 0x68..0x73
    */
    if (n >= 1 && n <= 12) {
        return (ihk_u8)(0x3A + (ihk_u8)(n - 1));
    }
    if (n >= 13 && n <= 24) {
        return (ihk_u8)(0x68 + (ihk_u8)(n - 13));
    }
    return 0;
}

static int ihk_parse_hex_usage_prefix(const char *s, ihk_u8 *out_usage)
{
    /* supports:
       0xNN
       kb_0xNN
       hid:07:NN
    */
    if (!s || !out_usage) return 0;

    if (s[0] == '0' && s[1] == 'x') {
        ihk_u8 v;
        if (ihk_parse_hex_byte(s + 2, &v)) {
            *out_usage = v;
            return 1;
        }
    }

    if (s[0] == 'k' && s[1] == 'b' && s[2] == '_' && s[3] == '0' && s[4] == 'x') {
        ihk_u8 v;
        if (ihk_parse_hex_byte(s + 5, &v)) {
            *out_usage = v;
            return 1;
        }
    }

    if (s[0] == 'h' && s[1] == 'i' && s[2] == 'd' && s[3] == ':' ) {
        /* hid:PP:UU */
        ihk_u8 page, usage;
        if (ihk_parse_hex_byte(s + 4, &page) && s[6] == ':' && ihk_parse_hex_byte(s + 7, &usage)) {
            if (page == (ihk_u8)IHK_HID_PAGE_KEYBOARD) {
                *out_usage = usage;
                return 1;
            }
        }
    }

    return 0;
}

static int ihk_prefix(const char *s, const char *p)
{
    /* returns 1 if s starts with p */
    while (*p) {
        if (*s != *p) return 0;
        ++s;
        ++p;
    }
    return 1;
}

static ihk_u8 ihk_kb_usage_from_kp(const char *s)
{
    /* kp_0..kp_9, kp_add, kp_sub, kp_mul, kp_div, kp_enter, kp_dot */
    if (!s) return 0;

    if (ihk_str_eq(s, "kp_0")) return 0x62;
    if (ihk_str_eq(s, "kp_1")) return 0x59;
    if (ihk_str_eq(s, "kp_2")) return 0x5A;
    if (ihk_str_eq(s, "kp_3")) return 0x5B;
    if (ihk_str_eq(s, "kp_4")) return 0x5C;
    if (ihk_str_eq(s, "kp_5")) return 0x5D;
    if (ihk_str_eq(s, "kp_6")) return 0x5E;
    if (ihk_str_eq(s, "kp_7")) return 0x5F;
    if (ihk_str_eq(s, "kp_8")) return 0x60;
    if (ihk_str_eq(s, "kp_9")) return 0x61;

    if (ihk_str_eq(s, "kp_add")) return 0x57;
    if (ihk_str_eq(s, "kp_sub")) return 0x56;
    if (ihk_str_eq(s, "kp_mul")) return 0x55;
    if (ihk_str_eq(s, "kp_div")) return 0x54;
    if (ihk_str_eq(s, "kp_enter")) return 0x58;
    if (ihk_str_eq(s, "kp_dot") || ihk_str_eq(s, "kp_decimal")) return 0x63;

    return 0;
}

ihk_key input_hook_key_from_name(const char *name)
{
    char buf[64];
    ihk_u8 usage;

    if (!name) return IHK_KEY_NONE;

    ihk_normalize_name(name, buf, sizeof(buf));
    if (buf[0] == '\0') return IHK_KEY_NONE;

    /* Hex forms */
    if (ihk_parse_hex_usage_prefix(buf, &usage)) {
        return IHK_HID_KB(usage);
    }

    /* Single letter */
    if (buf[1] == '\0') {
        usage = ihk_kb_usage_from_letter(buf[0]);
        if (usage) return IHK_HID_KB(usage);
        usage = ihk_kb_usage_from_digit(buf[0]);
        if (usage) return IHK_HID_KB(usage);
    }

    /* Function keys: f1..f24 */
    if (buf[0] == 'f') {
        int n;
        if (ihk_parse_u32_dec(buf + 1, &n)) {
            usage = ihk_kb_usage_from_fkey(n);
            if (usage) return IHK_HID_KB(usage);
        }
    }

    /* Keypad */
    if (ihk_prefix(buf, "kp_")) {
        usage = ihk_kb_usage_from_kp(buf);
        if (usage) return IHK_HID_KB(usage);
    }

    /* Common names */
    if (ihk_str_eq(buf, "none")) return IHK_KEY_NONE;

    if (ihk_str_eq(buf, "enter") || ihk_str_eq(buf, "return")) return IHK_HID_KB(0x28);
    if (ihk_str_eq(buf, "esc") || ihk_str_eq(buf, "escape")) return IHK_HID_KB(0x29);
    if (ihk_str_eq(buf, "backspace") || ihk_str_eq(buf, "bksp")) return IHK_HID_KB(0x2A);
    if (ihk_str_eq(buf, "tab")) return IHK_HID_KB(0x2B);
    if (ihk_str_eq(buf, "space")) return IHK_HID_KB(0x2C);

    /* Punctuation (US-ish names) */
    if (ihk_str_eq(buf, "minus") || ihk_str_eq(buf, "dash")) return IHK_HID_KB(0x2D);
    if (ihk_str_eq(buf, "equal") || ihk_str_eq(buf, "equals")) return IHK_HID_KB(0x2E);
    if (ihk_str_eq(buf, "lbracket") || ihk_str_eq(buf, "leftbracket")) return IHK_HID_KB(0x2F);
    if (ihk_str_eq(buf, "rbracket") || ihk_str_eq(buf, "rightbracket")) return IHK_HID_KB(0x30);
    if (ihk_str_eq(buf, "backslash") || ihk_str_eq(buf, "bslash")) return IHK_HID_KB(0x31);
    if (ihk_str_eq(buf, "hash") || ihk_str_eq(buf, "nonus_hash")) return IHK_HID_KB(0x32);
    if (ihk_str_eq(buf, "semicolon") || ihk_str_eq(buf, "semi")) return IHK_HID_KB(0x33);
    if (ihk_str_eq(buf, "quote") || ihk_str_eq(buf, "apostrophe")) return IHK_HID_KB(0x34);
    if (ihk_str_eq(buf, "grave") || ihk_str_eq(buf, "tilde") || ihk_str_eq(buf, "backtick")) return IHK_HID_KB(0x35);
    if (ihk_str_eq(buf, "comma")) return IHK_HID_KB(0x36);
    if (ihk_str_eq(buf, "period") || ihk_str_eq(buf, "dot")) return IHK_HID_KB(0x37);
    if (ihk_str_eq(buf, "slash") || ihk_str_eq(buf, "forwardslash")) return IHK_HID_KB(0x38);

    /* Locks */
    if (ihk_str_eq(buf, "capslock")) return IHK_HID_KB(0x39);
    if (ihk_str_eq(buf, "numlock")) return IHK_HID_KB(0x53);
    if (ihk_str_eq(buf, "scrolllock")) return IHK_HID_KB(0x47);

    /* Navigation/editing */
    if (ihk_str_eq(buf, "insert")) return IHK_HID_KB(0x49);
    if (ihk_str_eq(buf, "home")) return IHK_HID_KB(0x4A);
    if (ihk_str_eq(buf, "pageup") || ihk_str_eq(buf, "pgup")) return IHK_HID_KB(0x4B);
    if (ihk_str_eq(buf, "delete") || ihk_str_eq(buf, "del")) return IHK_HID_KB(0x4C);
    if (ihk_str_eq(buf, "end")) return IHK_HID_KB(0x4D);
    if (ihk_str_eq(buf, "pagedown") || ihk_str_eq(buf, "pgdn")) return IHK_HID_KB(0x4E);

    /* Arrows */
    if (ihk_str_eq(buf, "right")) return IHK_HID_KB(0x4F);
    if (ihk_str_eq(buf, "left"))  return IHK_HID_KB(0x50);
    if (ihk_str_eq(buf, "down"))  return IHK_HID_KB(0x51);
    if (ihk_str_eq(buf, "up"))    return IHK_HID_KB(0x52);

    /* System / misc */
    if (ihk_str_eq(buf, "printscreen") || ihk_str_eq(buf, "prtsc")) return IHK_HID_KB(0x46);
    if (ihk_str_eq(buf, "pause")) return IHK_HID_KB(0x48);
    if (ihk_str_eq(buf, "menu") || ihk_str_eq(buf, "application") || ihk_str_eq(buf, "app")) return IHK_HID_KB(0x65);

    /* Modifiers */
    if (ihk_str_eq(buf, "lctrl") || ihk_str_eq(buf, "ctrl")) return IHK_HID_KB(0xE0);
    if (ihk_str_eq(buf, "lshift") || ihk_str_eq(buf, "shift")) return IHK_HID_KB(0xE1);
    if (ihk_str_eq(buf, "lalt") || ihk_str_eq(buf, "alt")) return IHK_HID_KB(0xE2);
    if (ihk_str_eq(buf, "lgui") || ihk_str_eq(buf, "gui") || ihk_str_eq(buf, "win") || ihk_str_eq(buf, "cmd") || ihk_str_eq(buf, "meta")) return IHK_HID_KB(0xE3);

    if (ihk_str_eq(buf, "rctrl")) return IHK_HID_KB(0xE4);
    if (ihk_str_eq(buf, "rshift")) return IHK_HID_KB(0xE5);
    if (ihk_str_eq(buf, "ralt") || ihk_str_eq(buf, "altgr")) return IHK_HID_KB(0xE6);
    if (ihk_str_eq(buf, "rgui")) return IHK_HID_KB(0xE7);

    return IHK_KEY_NONE;
}

static const char *ihk_known_name_for_usage(ihk_u8 u)
{
    switch (u) {
    case 0x00: return "none";

    case 0x28: return "enter";
    case 0x29: return "escape";
    case 0x2A: return "backspace";
    case 0x2B: return "tab";
    case 0x2C: return "space";

    case 0x39: return "capslock";

    case 0x46: return "printscreen";
    case 0x47: return "scrolllock";
    case 0x48: return "pause";

    case 0x49: return "insert";
    case 0x4A: return "home";
    case 0x4B: return "pageup";
    case 0x4C: return "delete";
    case 0x4D: return "end";
    case 0x4E: return "pagedown";

    case 0x4F: return "right";
    case 0x50: return "left";
    case 0x51: return "down";
    case 0x52: return "up";

    case 0x53: return "numlock";

    case 0x54: return "kp_div";
    case 0x55: return "kp_mul";
    case 0x56: return "kp_sub";
    case 0x57: return "kp_add";
    case 0x58: return "kp_enter";

    case 0x63: return "kp_dot";

    case 0x65: return "menu";

    case 0xE0: return "lctrl";
    case 0xE1: return "lshift";
    case 0xE2: return "lalt";
    case 0xE3: return "lgui";
    case 0xE4: return "rctrl";
    case 0xE5: return "rshift";
    case 0xE6: return "ralt";
    case 0xE7: return "rgui";

    default:
        return 0;
    }
}

static void ihk_write_hex2(char *dst, ihk_u8 v)
{
    static const char *hex = "0123456789abcdef";
    dst[0] = hex[(v >> 4) & 0x0F];
    dst[1] = hex[v & 0x0F];
}

const char *input_hook_key_name(ihk_key key, char *tmp, size_t tmp_sz)
{
    ihk_u8 page = IHK_HID_PAGE(key);
    ihk_u8 u = IHK_HID_USAGE(key);

    /* Non-keyboard pages: return generic */
    if (page != (ihk_u8)IHK_HID_PAGE_KEYBOARD) {
        if (tmp && tmp_sz >= 10) {
            /* hid:PP:UU */
            tmp[0] = 'h'; tmp[1] = 'i'; tmp[2] = 'd'; tmp[3] = ':';
            ihk_write_hex2(tmp + 4, page);
            tmp[6] = ':';
            ihk_write_hex2(tmp + 7, u);
            tmp[9] = '\0';
            return tmp;
        }
        return "unknown";
    }

    /* Letters */
    if (u >= 0x04 && u <= 0x1D) {
        if (tmp && tmp_sz >= 2) {
            tmp[0] = (char)('a' + (char)(u - 0x04));
            tmp[1] = '\0';
            return tmp;
        }
        return "unknown";
    }

    /* Digits 1..9,0 */
    if (u >= 0x1E && u <= 0x26) {
        if (tmp && tmp_sz >= 2) {
            tmp[0] = (char)('1' + (char)(u - 0x1E));
            tmp[1] = '\0';
            return tmp;
        }
        return "unknown";
    }
    if (u == 0x27) {
        if (tmp && tmp_sz >= 2) {
            tmp[0] = '0';
            tmp[1] = '\0';
            return tmp;
        }
        return "unknown";
    }

    /* Function keys */
    if (u >= 0x3A && u <= 0x45) {
        int n = (int)(u - 0x3A) + 1;
        if (tmp && tmp_sz >= 5) {
            /* "f" + number */
            tmp[0] = 'f';
            if (n >= 10) {
                tmp[1] = (char)('0' + (n / 10));
                tmp[2] = (char)('0' + (n % 10));
                tmp[3] = '\0';
            } else {
                tmp[1] = (char)('0' + n);
                tmp[2] = '\0';
            }
            return tmp;
        }
        return "unknown";
    }

    if (u >= 0x68 && u <= 0x73) {
        int n = (int)(u - 0x68) + 13;
        if (tmp && tmp_sz >= 5) {
            tmp[0] = 'f';
            /* n is 13..24 */
            tmp[1] = (char)('0' + (n / 10));
            tmp[2] = (char)('0' + (n % 10));
            tmp[3] = '\0';
            return tmp;
        }
        return "unknown";
    }

    /* Keypad digits */
    if (u >= 0x59 && u <= 0x61) {
        int n = (int)(u - 0x59) + 1;
        if (tmp && tmp_sz >= 5) {
            tmp[0] = 'k'; tmp[1] = 'p'; tmp[2] = '_';
            tmp[3] = (char)('0' + n);
            tmp[4] = '\0';
            return tmp;
        }
        return "unknown";
    }
    if (u == 0x62) {
        if (tmp && tmp_sz >= 5) {
            tmp[0] = 'k'; tmp[1] = 'p'; tmp[2] = '_'; tmp[3] = '0'; tmp[4] = '\0';
            return tmp;
        }
        return "unknown";
    }

    /* Known switch names */
    {
        const char *known = ihk_known_name_for_usage(u);
        if (known) return known;
    }

    /* Fallback: kb_0xNN */
    if (tmp && tmp_sz >= 8) {
        tmp[0] = 'k'; tmp[1] = 'b'; tmp[2] = '_'; tmp[3] = '0'; tmp[4] = 'x';
        ihk_write_hex2(tmp + 5, u);
        tmp[7] = '\0';
        return tmp;
    }

    return "unknown";
}

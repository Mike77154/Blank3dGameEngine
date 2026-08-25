/* input_hook.c - C89-friendly keyboard hook core */

#include "input_hook.h"

#include <string.h> /* memset, memcpy */

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


ihk_key input_hook_key_from_input_key89(input_key89 key)
{
    unsigned int page;
    unsigned int usage;

    if (!input_keys89_to_hid(key, &page, &usage)) return IHK_KEY_NONE;
    if (page > 0xFFU || usage > 0xFFU) return IHK_KEY_NONE;
    return IHK_HID_MAKE(page, usage);
}

input_key89 input_hook_key_to_input_key89(ihk_key key)
{
    return input_keys89_from_hid((unsigned int)IHK_HID_PAGE(key),
                                 (unsigned int)IHK_HID_USAGE(key));
}

ihk_key input_hook_key_from_name(const char *name)
{
    return input_hook_key_from_input_key89(input_keys89_from_name(name));
}

const char *input_hook_key_name(ihk_key key, char *tmp, size_t tmp_sz)
{
    input_key89 canonical;
    unsigned int cap;

    canonical = input_hook_key_to_input_key89(key);
    cap = (tmp_sz > 0xFFFFFFFFUL) ? 0xFFFFFFFFU : (unsigned int)tmp_sz;
    return input_keys89_name(canonical, tmp, cap);
}

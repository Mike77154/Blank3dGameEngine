/* input_hook.h - C89-friendly, backend-agnostic keyboard hook (USB HID style)

   🎯 Goal
   - One small core that tracks ALL keyboard keys (press/hold/release)
   - No dependency on your scanner, polls, or any event/cond/action system
   - Backends are pluggable (Win32, SDL, Allegro, raylib, etc.)

   🧠 Canonical key codes
   - Uses USB HID Usage IDs (Usage Page 0x07: Keyboard/Keypad)
   - Key code is 16-bit: (page << 8) | usage
     e.g. 'A' => page 0x07, usage 0x04  => 0x0704

   🧱 Usage
     1) Provide a backend poll function that fills a 256-bit keyboard bitset
     2) Call input_hook_update() each frame/tick
     3) Query input_hook_down/pressed/released() OR bind callbacks

   Notes
   - "All keys" depends on your backend. The core supports all 0..255 usages.
   - For SDL2: SDL_Scancode values already match USB HID usages (page 0x07).
*/

#ifndef INPUT_HOOK_H
#define INPUT_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* size_t */
#include "input_keys89.h"

/* =========================
   Tiny fixed-width types (C89-friendly)
   ========================= */

typedef unsigned char  ihk_u8;
typedef unsigned short ihk_u16;
typedef unsigned int   ihk_u32;

/* =========================
   HID key coding
   ========================= */

typedef ihk_u16 ihk_key;

#define IHK_KEY_NONE ((ihk_key)0)

#define IHK_HID_PAGE_KEYBOARD   0x07u

/* Build (page<<8)|usage */
#define IHK_HID_MAKE(page, usage) ((ihk_key)((((ihk_u16)(page)) << 8) | ((ihk_u16)(usage) & 0xFFu)))

/* Convenience for keyboard page */
#define IHK_HID_KB(usage) IHK_HID_MAKE(IHK_HID_PAGE_KEYBOARD, (usage))

#define IHK_HID_PAGE(key)  ((ihk_u8)(((key) >> 8) & 0xFFu))
#define IHK_HID_USAGE(key) ((ihk_u8)((key) & 0xFFu))

/* Keyboard supports 0..255 usages -> 256 bits -> 32 bytes */
#define IHK_KB_USAGE_COUNT 256u
#define IHK_KB_BITS_BYTES  (IHK_KB_USAGE_COUNT / 8u)

/* =========================
   Backend interface
   ========================= */

/* Backend capability flags (optional info) */
#define IHK_CAP_KEYBOARD              0x00000001u
#define IHK_CAP_GLOBAL_CAPTURE        0x00000002u /* works without window focus (if OS allows) */
#define IHK_CAP_LAYOUT_INDEPENDENT    0x00000004u /* uses scancodes/physical positions */

/* Poll callback:
   - Must write the 256-bit keyboard bitset into out_kb_bits.
   - out_bytes will always be IHK_KB_BITS_BYTES (32), but we pass it for safety.

   Bit format:
     usage u => byte = u>>3, bit = 1<<(u&7)
     bit=1 means "DOWN".
*/
typedef void (*ihk_poll_keyboard_fn)(void *user, ihk_u8 *out_kb_bits, size_t out_bytes);

/* Optional shutdown */
typedef void (*ihk_backend_shutdown_fn)(void *user);

typedef struct ihk_backend {
    void *user;
    ihk_poll_keyboard_fn poll_keyboard;
    ihk_backend_shutdown_fn shutdown;
    ihk_u32 capabilities;
} ihk_backend;

/* =========================
   Events / bindings
   ========================= */

/* Event kinds (edge + state) */
typedef enum ihk_event_kind {
    IHK_EVENT_PRESS   = 1,
    IHK_EVENT_RELEASE = 2,
    IHK_EVENT_HOLD    = 3
} ihk_event_kind;

/* Binding mask */
#define IHK_BIND_PRESS    0x01u
#define IHK_BIND_RELEASE  0x02u
#define IHK_BIND_HOLD     0x04u

/* Optional update behavior flags */
#define IHK_OPT_EMIT_HOLD            0x00000001u /* emit HOLD events */
#define IHK_OPT_HOLD_INCLUDES_PRESS  0x00000002u /* HOLD also emitted on the press frame */

/* Callback signature for bindings and global event callback */
typedef void (*ihk_event_fn)(ihk_key key, ihk_event_kind kind, void *user);

#ifndef IHK_MAX_BINDINGS
#define IHK_MAX_BINDINGS 32
#endif

typedef struct ihk_binding_entry {
    ihk_key key;         /* specific key, or IHK_KEY_NONE as wildcard (any key) */
    ihk_u8  mask;        /* IHK_BIND_* bits */
    ihk_event_fn fn;
    void *user;
    ihk_u8 in_use;
} ihk_binding_entry;

/* =========================
   Context
   ========================= */

typedef struct input_hook {
    ihk_backend backend;

    ihk_u8 kb_prev[IHK_KB_BITS_BYTES];
    ihk_u8 kb_curr[IHK_KB_BITS_BYTES];
    ihk_u8 kb_pressed[IHK_KB_BITS_BYTES];
    ihk_u8 kb_released[IHK_KB_BITS_BYTES];

    ihk_u32 frame_index;
    ihk_u32 options;

    /* Global event callback (optional) */
    ihk_event_fn on_event;
    void *on_event_user;

    /* Small binding table (optional) */
    ihk_binding_entry bindings[IHK_MAX_BINDINGS];
} input_hook;

/* =========================
   Core API
   ========================= */

/* Initialize a hook context.
   - backend can be NULL; you can set it later with input_hook_set_backend().
*/
void input_hook_init(input_hook *h, const ihk_backend *backend);

/* Set/replace backend at runtime (optional).
   - Does NOT call old backend shutdown automatically.
   - If you want to shutdown old backend, call input_hook_shutdown() first.
*/
void input_hook_set_backend(input_hook *h, const ihk_backend *backend);

/* Shutdown (calls backend.shutdown if provided) */
void input_hook_shutdown(input_hook *h);

/* Update:
   1) calls backend.poll_keyboard()
   2) computes pressed/released bitsets
   3) dispatches events to on_event + bindings
*/
void input_hook_update(input_hook *h);

/* Query by HID key (page 0x07 recommended) */
int input_hook_down(const input_hook *h, ihk_key key);
int input_hook_pressed(const input_hook *h, ihk_key key);
int input_hook_released(const input_hook *h, ihk_key key);

/* Convenience: query by keyboard usage (0..255) */
int input_hook_kb_down(const input_hook *h, ihk_u8 usage);
int input_hook_kb_pressed(const input_hook *h, ihk_u8 usage);
int input_hook_kb_released(const input_hook *h, ihk_u8 usage);

/* Convenience: query by human name (script-friendly)
   - Internally calls input_hook_key_from_name().
   - For performance, prefer converting once and using input_hook_* with ihk_key.
*/
int input_hook_down_name(const input_hook *h, const char *name);
int input_hook_pressed_name(const input_hook *h, const char *name);
int input_hook_released_name(const input_hook *h, const char *name);

/* Access raw bitsets (advanced)
   - returns pointer to internal 32-byte arrays (do not modify)
*/
const ihk_u8 *input_hook_bits_curr(const input_hook *h);
const ihk_u8 *input_hook_bits_pressed(const input_hook *h);
const ihk_u8 *input_hook_bits_released(const input_hook *h);

/* Options */
void input_hook_set_options(input_hook *h, ihk_u32 options);

/* Global event callback */
void input_hook_set_event_callback(input_hook *h, ihk_event_fn fn, void *user);

/* Bindings (tiny fixed table)
   - key = IHK_KEY_NONE works as wildcard (any key)
*/
int  input_hook_bind(input_hook *h, ihk_key key, ihk_u8 mask, ihk_event_fn fn, void *user);
void input_hook_unbind(input_hook *h, ihk_event_fn fn, void *user);
void input_hook_clear_bindings(input_hook *h);

/* =========================
   Name helpers (script-friendly)
   ========================= */

/* Parse common names (case-insensitive), e.g.:
     "a", "z", "1", "0"
     "enter", "return", "esc", "escape"
     "space", "tab", "backspace", "delete", "insert"
     "up", "down", "left", "right"
     "lshift", "rshift", "shift", "lctrl", "ctrl", "lalt", "alt", "lgui", "win", "cmd"
     "f1".."f24"
     "kp_0".."kp_9", "kp_add", "kp_sub", "kp_mul", "kp_div", "kp_enter", "kp_dot"

   Also supports:
     "0xNN"            -> usage NN on keyboard page
     "kb_0xNN"         -> usage NN on keyboard page
     "hid:PP:UU"       -> page PP hex, usage UU hex
*/
ihk_key input_hook_key_from_name(const char *name);

/* Returns a readable name.
   - For well-known keys: returns a static string.
   - For other keys: writes into tmp (if provided) and returns tmp.
   - If tmp is NULL or too small: returns "unknown".
*/
const char *input_hook_key_name(ihk_key key, char *tmp, size_t tmp_sz);

/* Canonical input_keys89 bridge.  ihk_key and input_key89 intentionally share
 * the same (page << 8) | usage representation, but callers should use these
 * helpers instead of assuming that representation in application code. */
ihk_key input_hook_key_from_input_key89(input_key89 key);
input_key89 input_hook_key_to_input_key89(ihk_key key);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_HOOK_H */

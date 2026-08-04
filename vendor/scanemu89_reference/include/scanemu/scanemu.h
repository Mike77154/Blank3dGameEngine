/* scanemu.h - C89, backend-agnostic capture of symbolic hardware tokens
   SPDX-License-Identifier: MIT
*/
#ifndef SCANEMU_H_INCLUDED
#define SCANEMU_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------
   Build-time configuration
   ------------------------------ */

/* Maximum length (including NUL) of a symbol name, e.g. "jumpbutton". */
#ifndef SCANEMU_MAX_SYMBOL_LEN
#define SCANEMU_MAX_SYMBOL_LEN 48
#endif

/* ------------------------------
   Basic types (C89)
   ------------------------------ */

typedef unsigned long scanemu_u32;
typedef long          scanemu_i32;
typedef int           scanemu_bool;

#ifndef SCANEMU_TRUE
#define SCANEMU_TRUE  1
#define SCANEMU_FALSE 0
#endif

/* ------------------------------
   Token + Event model
   ------------------------------ */

/* The "what" we capture (hardware token). Codes are backend-defined numeric IDs. */
typedef enum scanemu_token_type {
    SCANEMU_T_NONE = 0,

    /* Keyboard-like */
    SCANEMU_T_KEY = 1,

    /* Mouse-like */
    SCANEMU_T_MOUSE_BUTTON = 10,
    SCANEMU_T_MOUSE_WHEEL  = 11,

    /* Gamepad-like */
    SCANEMU_T_PAD_BUTTON   = 20,
    SCANEMU_T_PAD_AXIS_POS = 21, /* + axis, threshold in value */
    SCANEMU_T_PAD_AXIS_NEG = 22, /* - axis, threshold in value */
    SCANEMU_T_PAD_HAT      = 23, /* dpad/hat direction mask in extra */

    /* Touch/gesture (optional) */
    SCANEMU_T_TOUCH_GESTURE = 30,

    /* Escape hatch */
    SCANEMU_T_CUSTOM = 100
} scanemu_token_type;

/* A captured token. Interpretation is intentionally minimal and portable. */
typedef struct scanemu_token {
    scanemu_token_type type;
    scanemu_i32 device_id;  /* e.g. keyboard 0, pad 0..N-1 */
    scanemu_i32 code;       /* e.g. scancode/button/axis/hat id */
    float       value;      /* e.g. axis threshold, wheel step magnitude */
    scanemu_u32 extra;      /* e.g. hat dir mask, modifiers, vendor info */
} scanemu_token;

/* The "how" we heard it (input event). scanemu does NOT implement key states;
   it only listens to discrete events provided by your backend/input lib. */
typedef enum scanemu_event_kind {
    SCANEMU_EV_NONE = 0,
    SCANEMU_EV_PRESS,
    SCANEMU_EV_RELEASE,
    SCANEMU_EV_MOVE,   /* analog motion (axes, wheel) */
    SCANEMU_EV_TEXT    /* optional text input */
} scanemu_event_kind;

typedef struct scanemu_event {
    scanemu_event_kind kind;
    scanemu_token      token;

    /* Optional: for TEXT events, backends may pass ASCII/UTF-8 bytes via extra;
       scanemu itself ignores it unless you use SCANEMU_LISTEN_TEXT. */
    const char*        text; /* may be NULL */
} scanemu_event;

/* ------------------------------
   Binding table + context
   ------------------------------ */

typedef struct scanemu_binding {
    scanemu_bool in_use;
    char         symbol[SCANEMU_MAX_SYMBOL_LEN];
    scanemu_token token;
} scanemu_binding;

typedef void (*scanemu_on_bound_fn)(void* user, const char* symbol, const scanemu_token* token);

/* Listener flags (what kinds of tokens to accept). */
enum {
    SCANEMU_LISTEN_KEYS        = 1u << 0,
    SCANEMU_LISTEN_MOUSE       = 1u << 1,
    SCANEMU_LISTEN_GAMEPAD     = 1u << 2,
    SCANEMU_LISTEN_TOUCH       = 1u << 3,
    SCANEMU_LISTEN_CUSTOM      = 1u << 4,
    SCANEMU_LISTEN_TEXT        = 1u << 5,

    /* Capture policy */
    SCANEMU_CAPTURE_ON_PRESS   = 1u << 16,
    SCANEMU_CAPTURE_ON_RELEASE = 1u << 17,

    /* Convenience */
    SCANEMU_LISTEN_ANY =
        (SCANEMU_LISTEN_KEYS | SCANEMU_LISTEN_MOUSE | SCANEMU_LISTEN_GAMEPAD |
         SCANEMU_LISTEN_TOUCH | SCANEMU_LISTEN_CUSTOM | SCANEMU_LISTEN_TEXT) |
        SCANEMU_CAPTURE_ON_PRESS
};

/* Feed result (what happened after scanemu_feed). */
typedef enum scanemu_feed_result {
    SCANEMU_FEED_IGNORED = 0,   /* not listening or event not accepted */
    SCANEMU_FEED_LISTENING,     /* listening, event processed but not captured */
    SCANEMU_FEED_CAPTURED,      /* captured and stored */
    SCANEMU_FEED_CANCELED       /* canceled (e.g. ESC policy handled by user) */
} scanemu_feed_result;

typedef struct scanemu_ctx {
    scanemu_binding* bindings;
    scanemu_i32      capacity;
    scanemu_i32      count;

    scanemu_bool     listening;
    scanemu_u32      listen_flags;

    /* Threshold for analog capture (axes). Default 0.5. */
    float            axis_threshold;

    /* Currently targeted symbol while listening. */
    char             listen_symbol[SCANEMU_MAX_SYMBOL_LEN];

    /* Last capture (handy for UI). */
    scanemu_bool     last_valid;
    char             last_symbol[SCANEMU_MAX_SYMBOL_LEN];
    scanemu_token    last_token;

    /* Optional callback. */
    scanemu_on_bound_fn on_bound;
    void*              on_bound_user;
} scanemu_ctx;

/* ------------------------------
   API
   ------------------------------ */

/* Initialize scanemu with caller-provided storage (no malloc inside scanemu). */
void scanemu_init(scanemu_ctx* ctx, scanemu_binding* storage, scanemu_i32 storage_count);

/* Optional: set callback invoked upon successful capture. */
void scanemu_set_on_bound(scanemu_ctx* ctx, scanemu_on_bound_fn fn, void* user);

/* Listener control */
scanemu_bool scanemu_is_listening(const scanemu_ctx* ctx);
void scanemu_listen(scanemu_ctx* ctx, const char* symbol, scanemu_u32 listen_flags);
void scanemu_cancel(scanemu_ctx* ctx);

/* Feed events from ANY backend (SDL/Allegro/console SDK/etc). */
scanemu_feed_result scanemu_feed(scanemu_ctx* ctx, const scanemu_event* ev);

/* Binding table ops */
scanemu_bool scanemu_has(const scanemu_ctx* ctx, const char* symbol);
scanemu_bool scanemu_get(const scanemu_ctx* ctx, const char* symbol, scanemu_token* out_token);
scanemu_bool scanemu_set(scanemu_ctx* ctx, const char* symbol, const scanemu_token* token);
scanemu_bool scanemu_clear(scanemu_ctx* ctx, const char* symbol);

/* Iterate (for building an Options menu) */
scanemu_i32  scanemu_binding_count(const scanemu_ctx* ctx);
scanemu_bool scanemu_binding_at(const scanemu_ctx* ctx, scanemu_i32 index,
                                const char** out_symbol, scanemu_token* out_token);

/* Last capture (UI convenience) */
scanemu_bool scanemu_last_get(const scanemu_ctx* ctx, const char** out_symbol, scanemu_token* out_token);
void         scanemu_last_clear(scanemu_ctx* ctx);

/* ------------------------------
   Token helpers (portable)
   ------------------------------ */

/* Compare tokens for identity (exact match; axes compare type/device/code, and threshold value). */
scanemu_bool scanemu_token_equal(const scanemu_token* a, const scanemu_token* b);

/* Serialize/parse tokens into a stable, backend-agnostic string.
   Format examples:
     KEY:0:44
     MOUSEBTN:0:1
     PADBTN:0:0
     PADAXIS+:0:2:0.50
     PADAXIS-:1:3:0.35
     PADHAT:0:0:8
   Returns SCANEMU_TRUE on success. */
scanemu_bool scanemu_token_to_string(const scanemu_token* t, char* out, scanemu_i32 out_cap);
scanemu_bool scanemu_token_from_string(scanemu_token* out_t, const char* s);

/* Helper: decide if a *captured binding token* is "triggered" by a backend event.
   This is optional but useful for writing an Input Mapper. */
scanemu_bool scanemu_binding_matches_event(const scanemu_token* binding, const scanemu_event* ev);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCANEMU_H_INCLUDED */

#ifndef MWINMENUAPP89_H
#define MWINMENUAPP89_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "mmenuengine89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MWINMENUAPP89_KEY_HANDLED 1
#define MWINMENUAPP89_KEY_REQUEST_CLOSE 2

#define MWINMENUAPP89_UI_MOVE 1
#define MWINMENUAPP89_UI_ACTIVATE 2
#define MWINMENUAPP89_UI_SKIP 3

#define MWINMENUAPP89_POINTER_MOVE 1
#define MWINMENUAPP89_POINTER_DOWN 2
#define MWINMENUAPP89_POINTER_UP 3
#define MWINMENUAPP89_POINTER_CANCEL 4

#define MWINMENUAPP89_POINTER_BUTTON_NONE 0
#define MWINMENUAPP89_POINTER_BUTTON_LEFT 1
#define MWINMENUAPP89_POINTER_BUTTON_RIGHT 2
#define MWINMENUAPP89_POINTER_BUTTON_MIDDLE 3

#define MWINMENUAPP89_POINTER_HANDLED 1
#define MWINMENUAPP89_POINTER_REQUEST_CLOSE 2
#define MWINMENUAPP89_POINTER_HAND_CURSOR 4

typedef void (*mwinmenuapp89_cue_fn)(
    void *user,
    int cue_id,
    unsigned long elapsed_ms
);
typedef int (*mwinmenuapp89_command_fn)(
    void *user,
    int command,
    unsigned long elapsed_ms
);
typedef void (*mwinmenuapp89_tick_fn)(
    void *user,
    unsigned long elapsed_ms
);
typedef void (*mwinmenuapp89_shutdown_fn)(void *user);
typedef void (*mwinmenuapp89_ui_event_fn)(
    void *user,
    int event_id,
    unsigned long elapsed_ms
);
typedef int (*mwinmenuapp89_visible_fn)(void *user);
typedef int (*mwinmenuapp89_key_fn)(
    void *user,
    unsigned int key,
    int is_down,
    unsigned long elapsed_ms
);
typedef int (*mwinmenuapp89_pointer_fn)(
    void *user,
    int event_type,
    int x,
    int y,
    int client_width,
    int client_height,
    int button,
    unsigned long elapsed_ms
);
typedef void (*mwinmenuapp89_render_fn)(
    void *user,
    int width,
    int height,
    unsigned long elapsed_ms,
    const mcanvas89_provider *canvas,
    const mtext89_provider *text
);
typedef void (*mwinmenuapp89_prepare_render_fn)(
    void *user,
    int width,
    int height,
    unsigned long elapsed_ms
);
typedef int (*mwinmenuapp89_text_transform_fn)(
    void *user,
    const mcanvas89_rect *source_rect,
    const char *text,
    const mtext89_style *source_style,
    mcanvas89_rect *result_rect,
    mtext89_style *result_style
);

typedef struct mwinmenuapp89_config {
    const char *class_name;
    const char *window_title;
    int client_width;
    int client_height;
    unsigned int timer_id;
    unsigned int timer_ms;
    int escape_closes_window;
    int double_buffer;
    mmenuengine89_state *engine;
    void *user;
    mwinmenuapp89_cue_fn on_cue;
    mwinmenuapp89_command_fn on_command;
    mwinmenuapp89_tick_fn on_tick;
    mwinmenuapp89_shutdown_fn on_shutdown;
    mwinmenuapp89_ui_event_fn on_ui_event;
    mwinmenuapp89_visible_fn is_menu_visible;
    mwinmenuapp89_key_fn on_key;
    mwinmenuapp89_pointer_fn on_pointer;
    mwinmenuapp89_prepare_render_fn prepare_render;
    mwinmenuapp89_text_transform_fn transform_text;
    mwinmenuapp89_render_fn render_underlay;
    mwinmenuapp89_render_fn render_overlay;
} mwinmenuapp89_config;

int mwinmenuapp89_run(
    HINSTANCE instance,
    int show_command,
    const mwinmenuapp89_config *config
);

#ifdef __cplusplus
}
#endif

#endif

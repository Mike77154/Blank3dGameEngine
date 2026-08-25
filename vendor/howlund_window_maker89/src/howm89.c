#include "howm89.h"
#include <string.h>

struct HOWM89_Window {
    int active;
    void *impl;
    int width;
    int height;
    int x;
    int y;
    int visible;
    int fullscreen;
    int resizable;
    int decorated;
    int topmost;
    int minimized;
    int maximized;
    HOWM89_Fix opacity_q16;
    char title[HOWM89_TITLE_CAPACITY];
};

static HOWM89_BackendVTable g_backend;
static void *g_backend_user = 0;
static int g_backend_registered = 0;
static int g_initialized = 0;
static HOWM89_Window g_windows[HOWM89_MAX_WINDOWS];

static HOWM89_Fix howm89_clamp_unit_q16(HOWM89_Fix v)
{
    if (v < 0) return 0;
    if (v > HOWM89_Q16_ONE) return HOWM89_Q16_ONE;
    return v;
}

static void howm89_copy_text(char *dst, int cap, const char *src)
{
    int i;
    if (dst == 0 || cap <= 0) return;
    if (src == 0) src = "";
    i = 0;
    while (i + 1 < cap && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int howm89_text_required(const char *s)
{
    int n;
    if (s == 0) return 1;
    n = 0;
    while (s[n] != '\0') ++n;
    return n + 1;
}

static HOWM89_Result howm89_require_backend(void)
{
    if (!g_backend_registered) return HOWM89_ERROR_BACKEND_NOT_REGISTERED;
    return HOWM89_OK;
}

static HOWM89_Result howm89_ensure_initialized(void)
{
    HOWM89_Result r;
    r = howm89_require_backend();
    if (r != HOWM89_OK) return r;
    if (!g_initialized) return howm89_library_init();
    return HOWM89_OK;
}

static HOWM89_Window *howm89_alloc_window(void)
{
    int i;
    for (i = 0; i < HOWM89_MAX_WINDOWS; ++i) {
        if (!g_windows[i].active) {
            memset(&g_windows[i], 0, sizeof(g_windows[i]));
            g_windows[i].active = 1;
            return &g_windows[i];
        }
    }
    return 0;
}

static void howm89_release_window(HOWM89_Window *window)
{
    if (window == 0) return;
    memset(window, 0, sizeof(*window));
}

static HOWM89_Result howm89_validate_window(HOWM89_Window *window)
{
    if (window == 0 || !window->active) return HOWM89_ERROR_INVALID_ARGUMENT;
    return howm89_ensure_initialized();
}

void howm89_window_desc_defaults(HOWM89_WindowDesc *desc)
{
    if (desc == 0) return;
    desc->title = "Howlund Window";
    desc->width = 640;
    desc->height = 480;
    desc->x = 0;
    desc->y = 0;
    desc->flags = HOWM89_WINDOW_VISIBLE | HOWM89_WINDOW_RESIZABLE | HOWM89_WINDOW_DECORATED;
    desc->opacity_q16 = HOWM89_Q16_ONE;
}

HOWM89_Result howm89_register_backend(const HOWM89_BackendVTable *vtable,
                                      void *backend_user)
{
    if (vtable == 0 || vtable->create_window == 0 || vtable->destroy_window == 0) {
        return HOWM89_ERROR_INVALID_ARGUMENT;
    }
    if (howm89_window_active_count() != 0 || g_initialized) return HOWM89_ERROR_BUSY;
    memset(&g_backend, 0, sizeof(g_backend));
    g_backend = *vtable;
    g_backend_user = backend_user;
    g_backend_registered = 1;
    return HOWM89_OK;
}

HOWM89_Result howm89_unregister_backend(void)
{
    if (howm89_window_active_count() != 0 || g_initialized) return HOWM89_ERROR_BUSY;
    memset(&g_backend, 0, sizeof(g_backend));
    g_backend_user = 0;
    g_backend_registered = 0;
    return HOWM89_OK;
}

HOWM89_Bool howm89_backend_is_registered(void)
{
    return g_backend_registered ? HOWM89_TRUE : HOWM89_FALSE;
}

HOWM89_Result howm89_library_init(void)
{
    HOWM89_Result r;
    r = howm89_require_backend();
    if (r != HOWM89_OK) return r;
    if (g_initialized) return HOWM89_OK;
    if (g_backend.init != 0 && g_backend.init(g_backend_user) != 0) {
        return HOWM89_ERROR_BACKEND_FAILED;
    }
    g_initialized = 1;
    return HOWM89_OK;
}

HOWM89_Bool howm89_library_is_initialized(void)
{
    return g_initialized ? HOWM89_TRUE : HOWM89_FALSE;
}

void howm89_library_shutdown(void)
{
    int i;
    if (!g_initialized) return;
    for (i = 0; i < HOWM89_MAX_WINDOWS; ++i) {
        if (g_windows[i].active) howm89_window_destroy(&g_windows[i]);
    }
    if (g_backend.shutdown != 0) g_backend.shutdown(g_backend_user);
    g_initialized = 0;
}

int howm89_window_capacity(void)
{
    return HOWM89_MAX_WINDOWS;
}

int howm89_window_active_count(void)
{
    int i;
    int n;
    n = 0;
    for (i = 0; i < HOWM89_MAX_WINDOWS; ++i) if (g_windows[i].active) ++n;
    return n;
}

HOWM89_Window *howm89_window_create(const char *title, int width, int height)
{
    HOWM89_WindowDesc desc;
    howm89_window_desc_defaults(&desc);
    desc.title = title;
    desc.width = width;
    desc.height = height;
    return howm89_window_create_ex(&desc);
}

HOWM89_Window *howm89_window_create_ex(const HOWM89_WindowDesc *desc)
{
    HOWM89_Window *w;
    void *impl;
    int out_w;
    int out_h;
    HOWM89_Result r;
    if (desc == 0 || desc->width <= 0 || desc->height <= 0) return 0;
    r = howm89_ensure_initialized();
    if (r != HOWM89_OK) return 0;
    w = howm89_alloc_window();
    if (w == 0) return 0;
    out_w = desc->width;
    out_h = desc->height;
    impl = g_backend.create_window(g_backend_user,
                                   desc->title ? desc->title : "",
                                   desc->width,
                                   desc->height,
                                   &out_w,
                                   &out_h);
    if (impl == 0) {
        howm89_release_window(w);
        return 0;
    }
    w->impl = impl;
    w->width = out_w > 0 ? out_w : desc->width;
    w->height = out_h > 0 ? out_h : desc->height;
    w->x = desc->x;
    w->y = desc->y;
    w->visible = 1;
    w->resizable = 1;
    w->decorated = 1;
    w->opacity_q16 = HOWM89_Q16_ONE;
    howm89_copy_text(w->title, HOWM89_TITLE_CAPACITY, desc->title);

    if ((desc->flags & HOWM89_WINDOW_VISIBLE) == 0U) (void)howm89_window_hide(w);
    if ((desc->flags & HOWM89_WINDOW_RESIZABLE) == 0U) (void)howm89_window_set_resizable(w, HOWM89_FALSE);
    if ((desc->flags & HOWM89_WINDOW_DECORATED) == 0U) (void)howm89_window_set_decorated(w, HOWM89_FALSE);
    if ((desc->flags & HOWM89_WINDOW_TOPMOST) != 0U) (void)howm89_window_set_topmost(w, HOWM89_TRUE);
    if ((desc->flags & HOWM89_WINDOW_FULLSCREEN) != 0U) (void)howm89_window_set_fullscreen(w, HOWM89_TRUE);
    if (desc->x != 0 || desc->y != 0) (void)howm89_window_set_position(w, desc->x, desc->y);
    if (desc->opacity_q16 != HOWM89_Q16_ONE) (void)howm89_window_set_opacity_q16(w, desc->opacity_q16);
    return w;
}

void howm89_window_destroy(HOWM89_Window *window)
{
    if (window == 0 || !window->active) return;
    if (g_backend_registered && g_backend.destroy_window != 0 && window->impl != 0) {
        g_backend.destroy_window(g_backend_user, window->impl);
    }
    howm89_release_window(window);
}

HOWM89_Bool howm89_window_is_valid(const HOWM89_Window *window)
{
    return (window != 0 && window->active) ? HOWM89_TRUE : HOWM89_FALSE;
}

HOWM89_Result howm89_window_set_size(HOWM89_Window *window, int width, int height)
{
    HOWM89_Result r;
    if (width <= 0 || height <= 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_window_size == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_window_size(g_backend_user, window->impl, width, height) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    window->width = width;
    window->height = height;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_get_size(HOWM89_Window *window, int *out_width, int *out_height)
{
    HOWM89_Result r;
    if (out_width == 0 || out_height == 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.get_window_size != 0) {
        if (g_backend.get_window_size(g_backend_user, window->impl, out_width, out_height) != 0) return HOWM89_ERROR_BACKEND_FAILED;
        window->width = *out_width;
        window->height = *out_height;
    } else {
        *out_width = window->width;
        *out_height = window->height;
    }
    return HOWM89_OK;
}

HOWM89_Result howm89_window_set_fullscreen(HOWM89_Window *window, HOWM89_Bool enable_fullscreen)
{
    HOWM89_Result r;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_fullscreen == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_fullscreen(g_backend_user, window->impl, enable_fullscreen ? 1 : 0) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    window->fullscreen = enable_fullscreen ? 1 : 0;
    return HOWM89_OK;
}

#define HOWM89_SIMPLE_WINDOW_CALL(name, member) \
HOWM89_Result name(HOWM89_Window *window) \
{ \
    HOWM89_Result r; \
    r = howm89_validate_window(window); \
    if (r != HOWM89_OK) return r; \
    if (g_backend.member == 0) return HOWM89_ERROR_NOT_SUPPORTED; \
    if (g_backend.member(g_backend_user, window->impl) != 0) return HOWM89_ERROR_BACKEND_FAILED; \
    return HOWM89_OK; \
}

HOWM89_SIMPLE_WINDOW_CALL(howm89_window_minimize, minimize)
HOWM89_SIMPLE_WINDOW_CALL(howm89_window_restore, restore)
HOWM89_SIMPLE_WINDOW_CALL(howm89_window_maximize, maximize)
HOWM89_SIMPLE_WINDOW_CALL(howm89_window_focus, focus)
HOWM89_SIMPLE_WINDOW_CALL(howm89_window_request_attention, request_attention)
HOWM89_SIMPLE_WINDOW_CALL(howm89_window_request_close, request_close)

HOWM89_Result howm89_window_set_opacity_q16(HOWM89_Window *window, HOWM89_Fix alpha_q16)
{
    HOWM89_Result r;
    alpha_q16 = howm89_clamp_unit_q16(alpha_q16);
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_opacity_q16 == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_opacity_q16(g_backend_user, window->impl, alpha_q16) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    window->opacity_q16 = alpha_q16;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_set_component_opacity_q16(HOWM89_Window *window,
                                                       unsigned int component_mask,
                                                       HOWM89_Fix alpha_q16)
{
    HOWM89_Result r;
    alpha_q16 = howm89_clamp_unit_q16(alpha_q16);
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_component_opacity_q16 == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_component_opacity_q16(g_backend_user, window->impl, component_mask, alpha_q16) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_show(HOWM89_Window *window)
{
    HOWM89_Result r;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.show_window == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.show_window(g_backend_user, window->impl, 1) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    window->visible = 1;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_hide(HOWM89_Window *window)
{
    HOWM89_Result r;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.show_window == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.show_window(g_backend_user, window->impl, 0) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    window->visible = 0;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_is_visible(HOWM89_Window *window, HOWM89_Bool *out_visible)
{
    HOWM89_Result r;
    int value;
    if (out_visible == 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    value = window->visible;
    if (g_backend.is_window_visible != 0) {
        if (g_backend.is_window_visible(g_backend_user, window->impl, &value) != 0) return HOWM89_ERROR_BACKEND_FAILED;
        window->visible = value ? 1 : 0;
    }
    *out_visible = value ? HOWM89_TRUE : HOWM89_FALSE;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_set_title(HOWM89_Window *window, const char *title)
{
    HOWM89_Result r;
    if (title == 0) title = "";
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_title == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_title(g_backend_user, window->impl, title) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    howm89_copy_text(window->title, HOWM89_TITLE_CAPACITY, title);
    return HOWM89_OK;
}

HOWM89_Result howm89_window_get_title(HOWM89_Window *window,
                                      char *buffer,
                                      int buffer_size,
                                      int *out_required_size)
{
    HOWM89_Result r;
    int required;
    if (buffer_size < 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.get_title != 0) {
        if (g_backend.get_title(g_backend_user, window->impl, buffer, buffer_size, &required) != 0) return HOWM89_ERROR_BACKEND_FAILED;
        if (out_required_size != 0) *out_required_size = required;
        if (buffer != 0 && buffer_size > 0 && required > buffer_size) return HOWM89_ERROR_INSUFFICIENT_BUFFER;
        return HOWM89_OK;
    }
    required = howm89_text_required(window->title);
    if (out_required_size != 0) *out_required_size = required;
    if (buffer != 0 && buffer_size > 0) howm89_copy_text(buffer, buffer_size, window->title);
    if (buffer != 0 && buffer_size > 0 && required > buffer_size) return HOWM89_ERROR_INSUFFICIENT_BUFFER;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_set_position(HOWM89_Window *window, int x, int y)
{
    HOWM89_Result r;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_position == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_position(g_backend_user, window->impl, x, y) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    window->x = x;
    window->y = y;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_get_position(HOWM89_Window *window, int *out_x, int *out_y)
{
    HOWM89_Result r;
    if (out_x == 0 || out_y == 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.get_position != 0) {
        if (g_backend.get_position(g_backend_user, window->impl, out_x, out_y) != 0) return HOWM89_ERROR_BACKEND_FAILED;
        window->x = *out_x;
        window->y = *out_y;
    } else {
        *out_x = window->x;
        *out_y = window->y;
    }
    return HOWM89_OK;
}

static HOWM89_Result howm89_window_set_bool(HOWM89_Window *window,
                                             HOWM89_Bool value,
                                             int (*fn)(void *, void *, int),
                                             int *cache)
{
    HOWM89_Result r;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (fn == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (fn(g_backend_user, window->impl, value ? 1 : 0) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    *cache = value ? 1 : 0;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_center_on_monitor(HOWM89_Window *window, int monitor_index)
{
    HOWM89_Result r;
    if (monitor_index < 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.center_on_monitor == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.center_on_monitor(g_backend_user, window->impl, monitor_index) != 0)
        return HOWM89_ERROR_BACKEND_FAILED;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_set_resizable(HOWM89_Window *window, HOWM89_Bool resizable)
{
    return howm89_window_set_bool(window, resizable, g_backend.set_resizable, &window->resizable);
}

HOWM89_Result howm89_window_set_decorated(HOWM89_Window *window, HOWM89_Bool decorated)
{
    return howm89_window_set_bool(window, decorated, g_backend.set_decorated, &window->decorated);
}

HOWM89_Result howm89_window_set_topmost(HOWM89_Window *window, HOWM89_Bool topmost)
{
    return howm89_window_set_bool(window, topmost, g_backend.set_topmost, &window->topmost);
}

static HOWM89_Result howm89_window_query_bool(HOWM89_Window *window,
                                               HOWM89_Bool *out_value,
                                               int (*fn)(void *, void *, int *),
                                               int *cache)
{
    HOWM89_Result r;
    int value;
    if (out_value == 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (fn == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    value = 0;
    if (fn(g_backend_user, window->impl, &value) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    *cache = value ? 1 : 0;
    *out_value = value ? HOWM89_TRUE : HOWM89_FALSE;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_is_minimized(HOWM89_Window *window, HOWM89_Bool *out_minimized)
{
    return howm89_window_query_bool(window, out_minimized, g_backend.is_minimized, &window->minimized);
}

HOWM89_Result howm89_window_is_maximized(HOWM89_Window *window, HOWM89_Bool *out_maximized)
{
    return howm89_window_query_bool(window, out_maximized, g_backend.is_maximized, &window->maximized);
}

HOWM89_Result howm89_window_set_size_limits(HOWM89_Window *window,
                                             int min_w,
                                             int min_h,
                                             int max_w,
                                             int max_h)
{
    HOWM89_Result r;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    if (g_backend.set_size_limits == 0) return HOWM89_ERROR_NOT_SUPPORTED;
    if (g_backend.set_size_limits(g_backend_user, window->impl, min_w, min_h, max_w, max_h) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    return HOWM89_OK;
}

HOWM89_Result howm89_window_get_scale_factor_q16(HOWM89_Window *window,
                                                  HOWM89_Fix *out_scale_q16)
{
    HOWM89_Result r;
    HOWM89_Fix scale;
    if (out_scale_q16 == 0) return HOWM89_ERROR_INVALID_ARGUMENT;
    r = howm89_validate_window(window);
    if (r != HOWM89_OK) return r;
    scale = HOWM89_Q16_ONE;
    if (g_backend.get_scale_factor_q16 != 0) {
        if (g_backend.get_scale_factor_q16(g_backend_user, window->impl, &scale) != 0) return HOWM89_ERROR_BACKEND_FAILED;
    }
    if (scale <= 0) scale = HOWM89_Q16_ONE;
    *out_scale_q16 = scale;
    return HOWM89_OK;
}

void *howm89_window_get_native_handle(HOWM89_Window *window)
{
    if (window == 0 || !window->active) return 0;
    if (!g_backend_registered || g_backend.get_native_handle == 0) return 0;
    return g_backend.get_native_handle(g_backend_user, window->impl);
}

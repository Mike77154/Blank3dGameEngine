#ifndef HOWM89_H
#define HOWM89_H

/* Howlund Window Maker 89
 * Platform-neutral window creation/control facade.
 * ISO C89 core. No heap. No floating point. No 64-bit integer API.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HOWM89_MAX_WINDOWS
#define HOWM89_MAX_WINDOWS 16
#endif

#ifndef HOWM89_TITLE_CAPACITY
#define HOWM89_TITLE_CAPACITY 128
#endif

typedef int HOWM89_Bool;
typedef int HOWM89_Fix;

#define HOWM89_FALSE 0
#define HOWM89_TRUE  1
#define HOWM89_Q16_ONE 65536
#define HOWM89_Q16_ZERO 0

typedef enum HOWM89_Result {
    HOWM89_OK = 0,
    HOWM89_ERROR_BACKEND_NOT_REGISTERED = -1,
    HOWM89_ERROR_BACKEND_FAILED = -2,
    HOWM89_ERROR_INVALID_ARGUMENT = -3,
    HOWM89_ERROR_NOT_INITIALIZED = -4,
    HOWM89_ERROR_NOT_SUPPORTED = -5,
    HOWM89_ERROR_CAPACITY = -6,
    HOWM89_ERROR_INSUFFICIENT_BUFFER = -7,
    HOWM89_ERROR_BUSY = -8
} HOWM89_Result;

enum {
    HOWM89_COMPONENT_FRAME      = 1U << 0,
    HOWM89_COMPONENT_TITLEBAR   = 1U << 1,
    HOWM89_COMPONENT_CLIENT     = 1U << 2,
    HOWM89_COMPONENT_BACKGROUND = 1U << 3
};

enum {
    HOWM89_WINDOW_VISIBLE   = 1U << 0,
    HOWM89_WINDOW_RESIZABLE = 1U << 1,
    HOWM89_WINDOW_DECORATED = 1U << 2,
    HOWM89_WINDOW_TOPMOST   = 1U << 3,
    HOWM89_WINDOW_FULLSCREEN = 1U << 4
};

typedef struct HOWM89_Window HOWM89_Window;

typedef struct HOWM89_WindowDesc {
    const char *title;
    int width;
    int height;
    int x;
    int y;
    unsigned int flags;
    HOWM89_Fix opacity_q16;
} HOWM89_WindowDesc;

/* Backend contract. 0 from callbacks means success; nonzero means failure.
 * Every callback receives backend_user so the host can own all backend state.
 * Callbacks may be NULL when unsupported.
 */
typedef struct HOWM89_BackendVTable {
    int  (*init)(void *backend_user);
    void (*shutdown)(void *backend_user);

    void *(*create_window)(void *backend_user,
                           const char *title,
                           int width,
                           int height,
                           int *out_width,
                           int *out_height);
    void (*destroy_window)(void *backend_user, void *impl);

    int (*set_window_size)(void *backend_user, void *impl, int width, int height);
    int (*get_window_size)(void *backend_user, void *impl, int *out_width, int *out_height);

    int (*set_fullscreen)(void *backend_user, void *impl, int enable_fullscreen);
    int (*minimize)(void *backend_user, void *impl);
    int (*restore)(void *backend_user, void *impl);
    int (*maximize)(void *backend_user, void *impl);

    int (*set_opacity_q16)(void *backend_user, void *impl, HOWM89_Fix alpha_q16);
    int (*set_component_opacity_q16)(void *backend_user,
                                     void *impl,
                                     unsigned int component_mask,
                                     HOWM89_Fix alpha_q16);

    void *(*get_native_handle)(void *backend_user, void *impl);

    int (*show_window)(void *backend_user, void *impl, int show);
    int (*is_window_visible)(void *backend_user, void *impl, int *out_visible);

    int (*set_title)(void *backend_user, void *impl, const char *title);
    int (*get_title)(void *backend_user,
                     void *impl,
                     char *buffer,
                     int buffer_size,
                     int *out_required_size);

    int (*set_position)(void *backend_user, void *impl, int x, int y);
    int (*get_position)(void *backend_user, void *impl, int *out_x, int *out_y);
    int (*center_on_monitor)(void *backend_user, void *impl, int monitor_index);

    int (*set_resizable)(void *backend_user, void *impl, int resizable);
    int (*set_decorated)(void *backend_user, void *impl, int decorated);
    int (*set_topmost)(void *backend_user, void *impl, int topmost);

    int (*is_minimized)(void *backend_user, void *impl, int *out_minimized);
    int (*is_maximized)(void *backend_user, void *impl, int *out_maximized);

    int (*focus)(void *backend_user, void *impl);
    int (*request_attention)(void *backend_user, void *impl);
    int (*request_close)(void *backend_user, void *impl);

    int (*set_size_limits)(void *backend_user,
                           void *impl,
                           int min_w,
                           int min_h,
                           int max_w,
                           int max_h);

    int (*get_scale_factor_q16)(void *backend_user,
                                void *impl,
                                HOWM89_Fix *out_scale_q16);
} HOWM89_BackendVTable;

void howm89_window_desc_defaults(HOWM89_WindowDesc *desc);

HOWM89_Result howm89_register_backend(const HOWM89_BackendVTable *vtable,
                                      void *backend_user);
HOWM89_Result howm89_unregister_backend(void);
HOWM89_Bool howm89_backend_is_registered(void);
HOWM89_Result howm89_library_init(void);
HOWM89_Bool howm89_library_is_initialized(void);
void howm89_library_shutdown(void);

int howm89_window_capacity(void);
int howm89_window_active_count(void);

HOWM89_Window *howm89_window_create(const char *title, int width, int height);
HOWM89_Window *howm89_window_create_ex(const HOWM89_WindowDesc *desc);
void howm89_window_destroy(HOWM89_Window *window);
HOWM89_Bool howm89_window_is_valid(const HOWM89_Window *window);

HOWM89_Result howm89_window_set_size(HOWM89_Window *window, int width, int height);
HOWM89_Result howm89_window_get_size(HOWM89_Window *window, int *out_width, int *out_height);
HOWM89_Result howm89_window_set_fullscreen(HOWM89_Window *window, HOWM89_Bool enable_fullscreen);
HOWM89_Result howm89_window_minimize(HOWM89_Window *window);
HOWM89_Result howm89_window_restore(HOWM89_Window *window);
HOWM89_Result howm89_window_maximize(HOWM89_Window *window);

HOWM89_Result howm89_window_set_opacity_q16(HOWM89_Window *window, HOWM89_Fix alpha_q16);
HOWM89_Result howm89_window_set_component_opacity_q16(HOWM89_Window *window,
                                                       unsigned int component_mask,
                                                       HOWM89_Fix alpha_q16);

HOWM89_Result howm89_window_show(HOWM89_Window *window);
HOWM89_Result howm89_window_hide(HOWM89_Window *window);
HOWM89_Result howm89_window_is_visible(HOWM89_Window *window, HOWM89_Bool *out_visible);

HOWM89_Result howm89_window_set_title(HOWM89_Window *window, const char *title);
HOWM89_Result howm89_window_get_title(HOWM89_Window *window,
                                      char *buffer,
                                      int buffer_size,
                                      int *out_required_size);

HOWM89_Result howm89_window_set_position(HOWM89_Window *window, int x, int y);
HOWM89_Result howm89_window_get_position(HOWM89_Window *window, int *out_x, int *out_y);
HOWM89_Result howm89_window_center_on_monitor(HOWM89_Window *window, int monitor_index);
HOWM89_Result howm89_window_set_resizable(HOWM89_Window *window, HOWM89_Bool resizable);
HOWM89_Result howm89_window_set_decorated(HOWM89_Window *window, HOWM89_Bool decorated);
HOWM89_Result howm89_window_set_topmost(HOWM89_Window *window, HOWM89_Bool topmost);

HOWM89_Result howm89_window_is_minimized(HOWM89_Window *window, HOWM89_Bool *out_minimized);
HOWM89_Result howm89_window_is_maximized(HOWM89_Window *window, HOWM89_Bool *out_maximized);
HOWM89_Result howm89_window_focus(HOWM89_Window *window);
HOWM89_Result howm89_window_request_attention(HOWM89_Window *window);
HOWM89_Result howm89_window_request_close(HOWM89_Window *window);

HOWM89_Result howm89_window_set_size_limits(HOWM89_Window *window,
                                             int min_w,
                                             int min_h,
                                             int max_w,
                                             int max_h);
HOWM89_Result howm89_window_get_scale_factor_q16(HOWM89_Window *window,
                                                  HOWM89_Fix *out_scale_q16);

void *howm89_window_get_native_handle(HOWM89_Window *window);

#ifdef __cplusplus
}
#endif

#endif

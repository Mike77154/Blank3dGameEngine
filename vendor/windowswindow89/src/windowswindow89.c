#include "windowswindow89.h"

#include <string.h>

static void ww89_copy(char *dst, int cap, const char *src)
{
    int i;
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    i = 0;
    while (src[i] && i + 1 < cap) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static WindowsWindow89_Slot *ww89_alloc(WindowsWindow89_Context *ctx)
{
    int i;
    if (!ctx) return 0;
    for (i = 0; i < WW89_MAX_WINDOWS; ++i) {
        if (!ctx->slots[i].used) {
            memset(&ctx->slots[i], 0, sizeof(ctx->slots[i]));
            ctx->slots[i].used = 1;
            ctx->slots[i].resizable = 1;
            ctx->slots[i].decorated = 1;
            ctx->slots[i].visible = 1;
            return &ctx->slots[i];
        }
    }
    return 0;
}

static void ww89_release(WindowsWindow89_Slot *slot)
{
    if (slot) memset(slot, 0, sizeof(*slot));
}

static DWORD ww89_style(const WindowsWindow89_Slot *slot)
{
    DWORD style;
    if (!slot) return WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    if (slot->fullscreen || !slot->decorated) style = WS_POPUP;
    else {
        style = WS_OVERLAPPEDWINDOW;
        if (!slot->resizable) {
            style &= ~((DWORD)WS_THICKFRAME);
            style &= ~((DWORD)WS_MAXIMIZEBOX);
        }
    }
    if (slot->visible) style |= WS_VISIBLE;
    return style;
}

static void ww89_outer_size(int client_w, int client_h, DWORD style,
                            int *out_w, int *out_h)
{
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = client_w;
    rect.bottom = client_h;
    if (style & WS_OVERLAPPEDWINDOW) (void)AdjustWindowRect(&rect, style, 0);
    *out_w = (int)(rect.right - rect.left);
    *out_h = (int)(rect.bottom - rect.top);
}

static int ww89_register_class(WindowsWindow89_Context *ctx)
{
    WNDCLASSA wc;
    if (!ctx || !ctx->instance || !ctx->window_proc) return -1;
    if (ctx->class_registered) return 0;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = ctx->window_proc;
    wc.hInstance = ctx->instance;
    wc.lpszClassName = ctx->class_name;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    if (!RegisterClassA(&wc)) return -1;
    ctx->class_registered = 1;
    return 0;
}

static int ww89_init(void *user)
{
    return ww89_register_class((WindowsWindow89_Context *)user);
}

static void ww89_shutdown(void *user)
{
    WindowsWindow89_Context *ctx;
    ctx = (WindowsWindow89_Context *)user;
    if (!ctx) return;
    if (ctx->class_registered) {
        (void)UnregisterClassA(ctx->class_name, ctx->instance);
        ctx->class_registered = 0;
    }
}

static void *ww89_create(void *user, const char *title,
                         int width, int height,
                         int *out_width, int *out_height)
{
    WindowsWindow89_Context *ctx;
    WindowsWindow89_Slot *slot;
    DWORD style;
    int outer_w;
    int outer_h;
    RECT rect;
    ctx = (WindowsWindow89_Context *)user;
    if (!ctx || width <= 0 || height <= 0) return 0;
    if (ww89_register_class(ctx) != 0) return 0;
    slot = ww89_alloc(ctx);
    if (!slot) return 0;
    slot->client_width = width;
    slot->client_height = height;
    slot->windowed_width = width;
    slot->windowed_height = height;
    slot->windowed_x = CW_USEDEFAULT;
    slot->windowed_y = CW_USEDEFAULT;
    style = ww89_style(slot);
    ww89_outer_size(width, height, style, &outer_w, &outer_h);
    slot->native_window = CreateWindowA(ctx->class_name,
                                        title ? title : "Blank3D",
                                        style,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        outer_w, outer_h,
                                        0, 0, ctx->instance, 0);
    if (!slot->native_window) {
        ww89_release(slot);
        return 0;
    }
    if (GetClientRect(slot->native_window, &rect)) {
        slot->client_width = (int)(rect.right - rect.left);
        slot->client_height = (int)(rect.bottom - rect.top);
    }
    if (out_width) *out_width = slot->client_width;
    if (out_height) *out_height = slot->client_height;
    return slot;
}

static void ww89_destroy(void *user, void *impl)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot) return;
    if (slot->native_window) (void)DestroyWindow(slot->native_window);
    ww89_release(slot);
}

static int ww89_set_size(void *user, void *impl, int width, int height)
{
    WindowsWindow89_Slot *slot;
    DWORD style;
    int outer_w;
    int outer_h;
    RECT rect;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window || width <= 0 || height <= 0) return -1;
    if (slot->fullscreen) return 0;
    style = ww89_style(slot);
    ww89_outer_size(width, height, style, &outer_w, &outer_h);
    if (!SetWindowPos(slot->native_window, 0, 0, 0, outer_w, outer_h,
                      SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED)) return -1;
    slot->client_width = width;
    slot->client_height = height;
    slot->windowed_width = width;
    slot->windowed_height = height;
    if (GetClientRect(slot->native_window, &rect)) {
        slot->client_width = (int)(rect.right - rect.left);
        slot->client_height = (int)(rect.bottom - rect.top);
    }
    return 0;
}

static int ww89_get_size(void *user, void *impl, int *width, int *height)
{
    WindowsWindow89_Slot *slot;
    RECT rect;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window || !width || !height) return -1;
    if (GetClientRect(slot->native_window, &rect)) {
        slot->client_width = (int)(rect.right - rect.left);
        slot->client_height = (int)(rect.bottom - rect.top);
    }
    *width = slot->client_width;
    *height = slot->client_height;
    return 0;
}

static int ww89_set_fullscreen(void *user, void *impl, int enabled)
{
    WindowsWindow89_Slot *slot;
    DWORD style;
    int outer_w;
    int outer_h;
    int screen_w;
    int screen_h;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    if (!!enabled == !!slot->fullscreen) return 0;
    if (enabled) {
        slot->windowed_width = slot->client_width;
        slot->windowed_height = slot->client_height;
        slot->fullscreen = 1;
        style = ww89_style(slot);
        (void)SetWindowLongA(slot->native_window, GWL_STYLE, (LONG)style);
        screen_w = GetSystemMetrics(SM_CXSCREEN);
        screen_h = GetSystemMetrics(SM_CYSCREEN);
        if (!SetWindowPos(slot->native_window, HWND_TOP, 0, 0,
                          screen_w, screen_h,
                          SWP_FRAMECHANGED | SWP_SHOWWINDOW)) return -1;
        slot->client_width = screen_w;
        slot->client_height = screen_h;
    } else {
        slot->fullscreen = 0;
        style = ww89_style(slot);
        (void)SetWindowLongA(slot->native_window, GWL_STYLE, (LONG)style);
        ww89_outer_size(slot->windowed_width, slot->windowed_height,
                        style, &outer_w, &outer_h);
        if (!SetWindowPos(slot->native_window, 0,
                          slot->windowed_x == CW_USEDEFAULT ? 0 : slot->windowed_x,
                          slot->windowed_y == CW_USEDEFAULT ? 0 : slot->windowed_y,
                          outer_w, outer_h,
                          SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW)) return -1;
        slot->client_width = slot->windowed_width;
        slot->client_height = slot->windowed_height;
    }
    return 0;
}

static int ww89_show(void *user, void *impl, int show)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    slot->visible = show ? 1 : 0;
    (void)ShowWindow(slot->native_window, show ? SW_SHOW : SW_HIDE);
    return 0;
}

static int ww89_set_title(void *user, void *impl, const char *title)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window || !title) return -1;
    return SetWindowTextA(slot->native_window, title) ? 0 : -1;
}

static int ww89_set_position(void *user, void *impl, int x, int y)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    if (!SetWindowPos(slot->native_window, 0, x, y, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER)) return -1;
    slot->x = x;
    slot->y = y;
    if (!slot->fullscreen) {
        slot->windowed_x = x;
        slot->windowed_y = y;
    }
    return 0;
}

static int ww89_center_on_monitor(void *user, void *impl, int monitor_index)
{
    WindowsWindow89_Slot *slot;
    DWORD style;
    int outer_w;
    int outer_h;
    int screen_w;
    int screen_h;
    int x;
    int y;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window || monitor_index != 0) return -1;
    if (slot->fullscreen) return 0;
    style = ww89_style(slot);
    ww89_outer_size(slot->client_width, slot->client_height,
                    style, &outer_w, &outer_h);
    screen_w = GetSystemMetrics(SM_CXSCREEN);
    screen_h = GetSystemMetrics(SM_CYSCREEN);
    x = (screen_w - outer_w) / 2;
    y = (screen_h - outer_h) / 2;
    if (!SetWindowPos(slot->native_window, 0, x, y, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER)) return -1;
    slot->x = x;
    slot->y = y;
    slot->windowed_x = x;
    slot->windowed_y = y;
    return 0;
}

static int ww89_restyle(WindowsWindow89_Slot *slot)
{
    DWORD style;
    if (!slot || !slot->native_window) return -1;
    style = ww89_style(slot);
    (void)SetWindowLongA(slot->native_window, GWL_STYLE, (LONG)style);
    return SetWindowPos(slot->native_window,
                        slot->topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                        0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED) ? 0 : -1;
}

static int ww89_set_resizable(void *user, void *impl, int enabled)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot) return -1;
    slot->resizable = enabled ? 1 : 0;
    return ww89_restyle(slot);
}

static int ww89_set_decorated(void *user, void *impl, int enabled)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot) return -1;
    slot->decorated = enabled ? 1 : 0;
    return ww89_restyle(slot);
}

static int ww89_set_topmost(void *user, void *impl, int enabled)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    slot->topmost = enabled ? 1 : 0;
    return SetWindowPos(slot->native_window,
                        slot->topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                        0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE) ? 0 : -1;
}

static int ww89_minimize(void *user, void *impl)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    (void)ShowWindow(slot->native_window, SW_MINIMIZE);
    return 0;
}

static int ww89_restore(void *user, void *impl)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    (void)ShowWindow(slot->native_window, SW_RESTORE);
    return 0;
}

static int ww89_maximize(void *user, void *impl)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    if (!slot || !slot->native_window) return -1;
    (void)ShowWindow(slot->native_window, SW_MAXIMIZE);
    return 0;
}

static void *ww89_native(void *user, void *impl)
{
    WindowsWindow89_Slot *slot;
    (void)user;
    slot = (WindowsWindow89_Slot *)impl;
    return slot ? (void *)slot->native_window : 0;
}

static int ww89_scale(void *user, void *impl, HOWM89_Fix *out_scale_q16)
{
    (void)user;
    (void)impl;
    if (!out_scale_q16) return -1;
    *out_scale_q16 = HOWM89_Q16_ONE;
    return 0;
}

void windowswindow89_context_init(WindowsWindow89_Context *ctx,
                                  HINSTANCE instance,
                                  WNDPROC window_proc,
                                  const char *class_name)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->instance = instance;
    ctx->window_proc = window_proc;
    ww89_copy(ctx->class_name, (int)sizeof(ctx->class_name),
              class_name && *class_name ? class_name : "Blank3DWindowsWindow89");
}

void windowswindow89_make_backend(WindowsWindow89_Context *ctx,
                                  HOWM89_BackendVTable *out_backend)
{
    if (!out_backend) return;
    memset(out_backend, 0, sizeof(*out_backend));
    if (!ctx) return;
    out_backend->init = ww89_init;
    out_backend->shutdown = ww89_shutdown;
    out_backend->create_window = ww89_create;
    out_backend->destroy_window = ww89_destroy;
    out_backend->set_window_size = ww89_set_size;
    out_backend->get_window_size = ww89_get_size;
    out_backend->set_fullscreen = ww89_set_fullscreen;
    out_backend->minimize = ww89_minimize;
    out_backend->restore = ww89_restore;
    out_backend->maximize = ww89_maximize;
    out_backend->get_native_handle = ww89_native;
    out_backend->show_window = ww89_show;
    out_backend->set_title = ww89_set_title;
    out_backend->set_position = ww89_set_position;
    out_backend->center_on_monitor = ww89_center_on_monitor;
    out_backend->set_resizable = ww89_set_resizable;
    out_backend->set_decorated = ww89_set_decorated;
    out_backend->set_topmost = ww89_set_topmost;
    out_backend->get_scale_factor_q16 = ww89_scale;
}

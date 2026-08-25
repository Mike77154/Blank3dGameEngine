#include "howm89_x11_backend.h"

#if defined(__unix__) && !defined(__APPLE__)

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifndef HOWM89_X11_MAX_WINDOWS
#define HOWM89_X11_MAX_WINDOWS HOWM89_MAX_WINDOWS
#endif

typedef struct HOWM89_X11Impl {
    int used;
    Display *dpy;
    int screen;
    Window win;
    Atom wm_delete;
    int client_w;
    int client_h;
    int windowed_x;
    int windowed_y;
    unsigned int windowed_w;
    unsigned int windowed_h;
    int fullscreen;
    int resizable;
    int min_w;
    int min_h;
    int max_w;
    int max_h;
} HOWM89_X11Impl;

static Display *g_dpy = 0;
static int g_screen = 0;
static Atom g_wm_delete = None;
static Atom g_net_wm_state = None;
static Atom g_net_wm_state_fullscreen = None;
static Atom g_net_wm_state_above = None;
static Atom g_net_wm_state_max_v = None;
static Atom g_net_wm_state_max_h = None;
static Atom g_net_wm_state_demands_attention = None;
static Atom g_net_active_window = None;
static Atom g_net_wm_window_opacity = None;
static Atom g_net_wm_name = None;
static Atom g_utf8_string = None;
static HOWM89_X11Impl g_pool[HOWM89_X11_MAX_WINDOWS];

static HOWM89_X11Impl *x11_alloc(void)
{
    int i;
    for (i = 0; i < HOWM89_X11_MAX_WINDOWS; ++i) {
        if (!g_pool[i].used) {
            memset(&g_pool[i], 0, sizeof(g_pool[i]));
            g_pool[i].used = 1;
            return &g_pool[i];
        }
    }
    return 0;
}

static void x11_release(HOWM89_X11Impl *impl)
{
    if (impl != 0) memset(impl, 0, sizeof(*impl));
}

static int x11_init(void *user)
{
    (void)user;
    if (g_dpy != 0) return 0;
    g_dpy = XOpenDisplay(0);
    if (g_dpy == 0) return -1;
    g_screen = DefaultScreen(g_dpy);
    g_wm_delete = XInternAtom(g_dpy, "WM_DELETE_WINDOW", False);
    g_net_wm_state = XInternAtom(g_dpy, "_NET_WM_STATE", False);
    g_net_wm_state_fullscreen = XInternAtom(g_dpy, "_NET_WM_STATE_FULLSCREEN", False);
    g_net_wm_state_above = XInternAtom(g_dpy, "_NET_WM_STATE_ABOVE", False);
    g_net_wm_state_max_v = XInternAtom(g_dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    g_net_wm_state_max_h = XInternAtom(g_dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    g_net_wm_state_demands_attention = XInternAtom(g_dpy, "_NET_WM_STATE_DEMANDS_ATTENTION", False);
    g_net_active_window = XInternAtom(g_dpy, "_NET_ACTIVE_WINDOW", False);
    g_net_wm_window_opacity = XInternAtom(g_dpy, "_NET_WM_WINDOW_OPACITY", False);
    g_net_wm_name = XInternAtom(g_dpy, "_NET_WM_NAME", False);
    g_utf8_string = XInternAtom(g_dpy, "UTF8_STRING", False);
    return 0;
}

static void x11_shutdown(void *user)
{
    int i;
    (void)user;
    if (g_dpy == 0) return;
    for (i = 0; i < HOWM89_X11_MAX_WINDOWS; ++i) {
        if (g_pool[i].used && g_pool[i].win != 0) XDestroyWindow(g_dpy, g_pool[i].win);
        memset(&g_pool[i], 0, sizeof(g_pool[i]));
    }
    XCloseDisplay(g_dpy);
    g_dpy = 0;
}

static void x11_measure(HOWM89_X11Impl *impl)
{
    XWindowAttributes wa;
    if (impl == 0 || impl->dpy == 0) return;
    if (XGetWindowAttributes(impl->dpy, impl->win, &wa) == 0) return;
    impl->client_w = wa.width;
    impl->client_h = wa.height;
}

static void x11_send_state(HOWM89_X11Impl *impl, int action, Atom a, Atom b)
{
    XEvent e;
    Window root;
    if (impl == 0 || impl->dpy == 0 || g_net_wm_state == None) return;
    memset(&e, 0, sizeof(e));
    root = RootWindow(impl->dpy, impl->screen);
    e.xclient.type = ClientMessage;
    e.xclient.window = impl->win;
    e.xclient.message_type = g_net_wm_state;
    e.xclient.format = 32;
    e.xclient.data.l[0] = action;
    e.xclient.data.l[1] = (long)a;
    e.xclient.data.l[2] = (long)b;
    e.xclient.data.l[3] = 1;
    XSendEvent(impl->dpy, root, False,
               SubstructureNotifyMask | SubstructureRedirectMask, &e);
    XFlush(impl->dpy);
}

static void x11_apply_hints(HOWM89_X11Impl *impl)
{
    XSizeHints hints;
    if (impl == 0) return;
    memset(&hints, 0, sizeof(hints));
    if (!impl->resizable) {
        x11_measure(impl);
        hints.flags = PMinSize | PMaxSize;
        hints.min_width = impl->client_w;
        hints.min_height = impl->client_h;
        hints.max_width = impl->client_w;
        hints.max_height = impl->client_h;
    } else {
        if (impl->min_w > 0 && impl->min_h > 0) {
            hints.flags |= PMinSize;
            hints.min_width = impl->min_w;
            hints.min_height = impl->min_h;
        }
        if (impl->max_w > 0 && impl->max_h > 0) {
            hints.flags |= PMaxSize;
            hints.max_width = impl->max_w;
            hints.max_height = impl->max_h;
        }
    }
    XSetWMNormalHints(impl->dpy, impl->win, &hints);
    XFlush(impl->dpy);
}

static void *x11_create(void *user, const char *title, int width, int height, int *out_w, int *out_h)
{
    HOWM89_X11Impl *impl;
    Window root;
    (void)user;
    if (g_dpy == 0 || width <= 0 || height <= 0) return 0;
    impl = x11_alloc();
    if (impl == 0) return 0;
    impl->dpy = g_dpy;
    impl->screen = g_screen;
    root = RootWindow(g_dpy, g_screen);
    impl->win = XCreateSimpleWindow(g_dpy, root, 0, 0,
                                    (unsigned int)width, (unsigned int)height,
                                    0, BlackPixel(g_dpy, g_screen), WhitePixel(g_dpy, g_screen));
    if (impl->win == 0) {
        x11_release(impl);
        return 0;
    }
    impl->wm_delete = g_wm_delete;
    impl->resizable = 1;
    impl->client_w = width;
    impl->client_h = height;
    XSetWMProtocols(g_dpy, impl->win, &impl->wm_delete, 1);
    XStoreName(g_dpy, impl->win, title ? title : "Howlund Window");
    XMapWindow(g_dpy, impl->win);
    XFlush(g_dpy);
    if (out_w != 0) *out_w = width;
    if (out_h != 0) *out_h = height;
    return impl;
}

static void x11_destroy(void *user, void *ptr)
{
    HOWM89_X11Impl *impl;
    (void)user;
    impl = (HOWM89_X11Impl *)ptr;
    if (impl == 0 || !impl->used) return;
    if (impl->dpy != 0 && impl->win != 0) {
        XDestroyWindow(impl->dpy, impl->win);
        XFlush(impl->dpy);
    }
    x11_release(impl);
}

static int x11_set_size(void *user, void *ptr, int w, int h)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0 || w <= 0 || h <= 0) return -1;
    XResizeWindow(impl->dpy, impl->win, (unsigned int)w, (unsigned int)h);
    XFlush(impl->dpy);
    impl->client_w = w;
    impl->client_h = h;
    return 0;
}

static int x11_get_size(void *user, void *ptr, int *w, int *h)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0 || w == 0 || h == 0) return -1;
    x11_measure(impl);
    *w = impl->client_w;
    *h = impl->client_h;
    return 0;
}

static int x11_fullscreen(void *user, void *ptr, int enabled)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    XWindowAttributes wa;
    (void)user;
    if (impl == 0) return -1;
    if (enabled && !impl->fullscreen) {
        if (XGetWindowAttributes(impl->dpy, impl->win, &wa) != 0) {
            impl->windowed_x = wa.x;
            impl->windowed_y = wa.y;
            impl->windowed_w = (unsigned int)wa.width;
            impl->windowed_h = (unsigned int)wa.height;
        }
        x11_send_state(impl, 1, g_net_wm_state_fullscreen, None);
        impl->fullscreen = 1;
    } else if (!enabled && impl->fullscreen) {
        x11_send_state(impl, 0, g_net_wm_state_fullscreen, None);
        impl->fullscreen = 0;
    }
    return 0;
}

static int x11_minimize(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    return XIconifyWindow(impl->dpy, impl->win, impl->screen) ? 0 : -1;
}

static int x11_restore(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    XMapRaised(impl->dpy, impl->win);
    XFlush(impl->dpy);
    return 0;
}

static int x11_maximize(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    x11_send_state(impl, 1, g_net_wm_state_max_v, g_net_wm_state_max_h);
    return 0;
}

static int x11_opacity(void *user, void *ptr, HOWM89_Fix alpha)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    unsigned long value;
    (void)user;
    if (impl == 0 || g_net_wm_window_opacity == None) return -1;
    if (alpha < 0) alpha = 0;
    if (alpha > HOWM89_Q16_ONE) alpha = HOWM89_Q16_ONE;
    value = ((unsigned long)(unsigned int)alpha * 65535UL) & 0xFFFFFFFFUL;
    XChangeProperty(impl->dpy, impl->win, g_net_wm_window_opacity,
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&value, 1);
    XFlush(impl->dpy);
    return 0;
}

static int x11_component_opacity(void *user, void *ptr, unsigned int mask, HOWM89_Fix alpha)
{
    (void)mask;
    return x11_opacity(user, ptr, alpha);
}

static void *x11_native(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return 0;
    return (void *)(unsigned long)impl->win;
}

static int x11_show(void *user, void *ptr, int show)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    if (show) XMapWindow(impl->dpy, impl->win); else XUnmapWindow(impl->dpy, impl->win);
    XFlush(impl->dpy);
    return 0;
}

static int x11_visible(void *user, void *ptr, int *visible)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    XWindowAttributes wa;
    (void)user;
    if (impl == 0 || visible == 0) return -1;
    if (XGetWindowAttributes(impl->dpy, impl->win, &wa) == 0) return -1;
    *visible = wa.map_state == IsViewable ? 1 : 0;
    return 0;
}

static int x11_title(void *user, void *ptr, const char *title)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    if (title == 0) title = "";
    XStoreName(impl->dpy, impl->win, title);
    if (g_net_wm_name != None && g_utf8_string != None) {
        XChangeProperty(impl->dpy, impl->win, g_net_wm_name, g_utf8_string, 8,
                        PropModeReplace, (const unsigned char *)title, (int)strlen(title));
    }
    XFlush(impl->dpy);
    return 0;
}

static int x11_position(void *user, void *ptr, int x, int y)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    XMoveWindow(impl->dpy, impl->win, x, y);
    XFlush(impl->dpy);
    return 0;
}

static int x11_get_position(void *user, void *ptr, int *x, int *y)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    Window child;
    int tx;
    int ty;
    (void)user;
    if (impl == 0 || x == 0 || y == 0) return -1;
    if (!XTranslateCoordinates(impl->dpy, impl->win,
                               RootWindow(impl->dpy, impl->screen),
                               0, 0, &tx, &ty, &child)) return -1;
    *x = tx;
    *y = ty;
    return 0;
}

static int x11_resizable(void *user, void *ptr, int value)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    impl->resizable = value ? 1 : 0;
    x11_apply_hints(impl);
    return 0;
}

static int x11_decorated(void *user, void *ptr, int value)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    Atom motif;
    struct MotifHints { unsigned long flags; unsigned long functions; unsigned long decorations; long input_mode; unsigned long status; } hints;
    (void)user;
    if (impl == 0) return -1;
    motif = XInternAtom(impl->dpy, "_MOTIF_WM_HINTS", False);
    hints.flags = 2UL;
    hints.functions = 0UL;
    hints.decorations = value ? 1UL : 0UL;
    hints.input_mode = 0;
    hints.status = 0UL;
    XChangeProperty(impl->dpy, impl->win, motif, motif, 32, PropModeReplace,
                    (unsigned char *)&hints, 5);
    XFlush(impl->dpy);
    return 0;
}

static int x11_topmost(void *user, void *ptr, int value)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    x11_send_state(impl, value ? 1 : 0, g_net_wm_state_above, None);
    return 0;
}

static int x11_query_minimized(void *user, void *ptr, int *out)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    XWindowAttributes wa;
    (void)user;
    if (impl == 0 || out == 0) return -1;
    if (XGetWindowAttributes(impl->dpy, impl->win, &wa) == 0) return -1;
    *out = wa.map_state == IsUnmapped ? 1 : 0;
    return 0;
}

static int x11_query_maximized(void *user, void *ptr, int *out)
{
    (void)user;
    (void)ptr;
    if (out == 0) return -1;
    *out = 0;
    return 0;
}

static int x11_focus(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    XEvent e;
    Window root;
    (void)user;
    if (impl == 0) return -1;
    root = RootWindow(impl->dpy, impl->screen);
    memset(&e, 0, sizeof(e));
    e.xclient.type = ClientMessage;
    e.xclient.window = impl->win;
    e.xclient.message_type = g_net_active_window;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 1;
    XSendEvent(impl->dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
    XFlush(impl->dpy);
    return 0;
}

static int x11_attention(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    x11_send_state(impl, 1, g_net_wm_state_demands_attention, None);
    return 0;
}

static int x11_close(void *user, void *ptr)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    XEvent e;
    (void)user;
    if (impl == 0) return -1;
    memset(&e, 0, sizeof(e));
    e.xclient.type = ClientMessage;
    e.xclient.window = impl->win;
    e.xclient.message_type = XInternAtom(impl->dpy, "WM_PROTOCOLS", True);
    e.xclient.format = 32;
    e.xclient.data.l[0] = (long)impl->wm_delete;
    e.xclient.data.l[1] = CurrentTime;
    XSendEvent(impl->dpy, impl->win, False, NoEventMask, &e);
    XFlush(impl->dpy);
    return 0;
}

static int x11_limits(void *user, void *ptr, int min_w, int min_h, int max_w, int max_h)
{
    HOWM89_X11Impl *impl = (HOWM89_X11Impl *)ptr;
    (void)user;
    if (impl == 0) return -1;
    impl->min_w = min_w;
    impl->min_h = min_h;
    impl->max_w = max_w;
    impl->max_h = max_h;
    x11_apply_hints(impl);
    return 0;
}

static int x11_scale(void *user, void *ptr, HOWM89_Fix *scale)
{
    (void)user;
    (void)ptr;
    if (scale == 0) return -1;
    *scale = HOWM89_Q16_ONE;
    return 0;
}

HOWM89_Result howm89_register_x11_backend(void)
{
    HOWM89_BackendVTable v;
    memset(&v, 0, sizeof(v));
    v.init = x11_init;
    v.shutdown = x11_shutdown;
    v.create_window = x11_create;
    v.destroy_window = x11_destroy;
    v.set_window_size = x11_set_size;
    v.get_window_size = x11_get_size;
    v.set_fullscreen = x11_fullscreen;
    v.minimize = x11_minimize;
    v.restore = x11_restore;
    v.maximize = x11_maximize;
    v.set_opacity_q16 = x11_opacity;
    v.set_component_opacity_q16 = x11_component_opacity;
    v.get_native_handle = x11_native;
    v.show_window = x11_show;
    v.is_window_visible = x11_visible;
    v.set_title = x11_title;
    v.set_position = x11_position;
    v.get_position = x11_get_position;
    v.set_resizable = x11_resizable;
    v.set_decorated = x11_decorated;
    v.set_topmost = x11_topmost;
    v.is_minimized = x11_query_minimized;
    v.is_maximized = x11_query_maximized;
    v.focus = x11_focus;
    v.request_attention = x11_attention;
    v.request_close = x11_close;
    v.set_size_limits = x11_limits;
    v.get_scale_factor_q16 = x11_scale;
    return howm89_register_backend(&v, 0);
}

#else

HOWM89_Result howm89_register_x11_backend(void)
{
    return HOWM89_ERROR_NOT_SUPPORTED;
}

#endif

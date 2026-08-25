#include "howm89_win32_backend.h"
#include <string.h>

#ifndef HOWM89_WIN32_MAX_WINDOWS
#define HOWM89_WIN32_MAX_WINDOWS HOWM89_MAX_WINDOWS
#endif

typedef struct HOWM89_Win32Impl {
    int used;
    HWND native_window;
    int client_w;
    int client_h;
    int fullscreen;
    RECT windowed_rect;
    DWORD windowed_style;
    DWORD windowed_exstyle;
    int min_w;
    int min_h;
    int max_w;
    int max_h;
} HOWM89_Win32Impl;

static HOWM89_Win32Impl g_pool[HOWM89_WIN32_MAX_WINDOWS];
static HINSTANCE g_instance = NULL;
static WNDPROC g_user_wndproc = NULL;
static char g_class_name[64] = "HOWM89_WIN32";
static int g_class_registered = 0;

static HOWM89_Win32Impl *win32_alloc(void)
{
    int i;
    for (i = 0; i < HOWM89_WIN32_MAX_WINDOWS; ++i) {
        if (!g_pool[i].used) {
            memset(&g_pool[i], 0, sizeof(g_pool[i]));
            g_pool[i].used = 1;
            return &g_pool[i];
        }
    }
    return 0;
}

static void win32_release(HOWM89_Win32Impl *impl)
{
    if (impl != 0) memset(impl, 0, sizeof(*impl));
}

static const char *win32_class_name(void)
{
    return g_class_name[0] != '\0' ? g_class_name : "HOWM89_WIN32";
}

static HOWM89_Win32Impl *win32_impl_from_native(HWND native_window)
{
#if defined(_WIN64)
    return (HOWM89_Win32Impl *)(void *)GetWindowLongPtrA(native_window, GWLP_USERDATA);
#else
    return (HOWM89_Win32Impl *)(void *)GetWindowLongA(native_window, GWL_USERDATA);
#endif
}

static void win32_set_impl_for_native(HWND native_window, HOWM89_Win32Impl *impl)
{
#if defined(_WIN64)
    SetWindowLongPtrA(native_window, GWLP_USERDATA, (LONG_PTR)impl);
#else
    SetWindowLongA(native_window, GWL_USERDATA, (LONG)impl);
#endif
}

static void win32_measure(HOWM89_Win32Impl *impl)
{
    RECT r;
    if (impl == 0 || impl->native_window == NULL) return;
    if (GetClientRect(impl->native_window, &r)) {
        impl->client_w = r.right - r.left;
        impl->client_h = r.bottom - r.top;
    }
}

static void win32_apply_minmax(HWND native_window, MINMAXINFO *mmi, HOWM89_Win32Impl *impl)
{
    RECT r;
    LONG style;
    LONG exstyle;
    if (mmi == 0 || impl == 0) return;
    style = GetWindowLongA(native_window, GWL_STYLE);
    exstyle = GetWindowLongA(native_window, GWL_EXSTYLE);
    if (impl->min_w > 0 && impl->min_h > 0) {
        r.left = 0; r.top = 0; r.right = impl->min_w; r.bottom = impl->min_h;
        AdjustWindowRectEx(&r, (DWORD)style, FALSE, (DWORD)exstyle);
        mmi->ptMinTrackSize.x = r.right - r.left;
        mmi->ptMinTrackSize.y = r.bottom - r.top;
    }
    if (impl->max_w > 0 && impl->max_h > 0) {
        r.left = 0; r.top = 0; r.right = impl->max_w; r.bottom = impl->max_h;
        AdjustWindowRectEx(&r, (DWORD)style, FALSE, (DWORD)exstyle);
        mmi->ptMaxTrackSize.x = r.right - r.left;
        mmi->ptMaxTrackSize.y = r.bottom - r.top;
    }
}

static LRESULT CALLBACK win32_wndproc(HWND native_window, UINT msg, WPARAM wp, LPARAM lp)
{
    HOWM89_Win32Impl *impl;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lp;
        if (cs != 0 && cs->lpCreateParams != 0) {
            win32_set_impl_for_native(native_window, (HOWM89_Win32Impl *)cs->lpCreateParams);
        }
    }
    impl = win32_impl_from_native(native_window);
    if (msg == WM_GETMINMAXINFO) {
        if (g_user_wndproc != NULL) CallWindowProcA(g_user_wndproc, native_window, msg, wp, lp);
        else DefWindowProcA(native_window, msg, wp, lp);
        if (impl != 0) win32_apply_minmax(native_window, (MINMAXINFO *)lp, impl);
        return 0;
    }
    if (msg == WM_NCDESTROY && impl != 0) win32_set_impl_for_native(native_window, 0);
    if (g_user_wndproc != NULL) return CallWindowProcA(g_user_wndproc, native_window, msg, wp, lp);
    return DefWindowProcA(native_window, msg, wp, lp);
}

static int win32_register_class(void)
{
    WNDCLASSA wc;
    if (g_class_registered) return 0;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = win32_wndproc;
    wc.hInstance = g_instance;
    wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = win32_class_name();
    if (RegisterClassA(&wc) == 0) return -1;
    g_class_registered = 1;
    return 0;
}

static int win32_init(void *user)
{
    (void)user;
    return win32_register_class();
}

static void win32_shutdown(void *user)
{
    (void)user;
    if (g_class_registered) {
        UnregisterClassA(win32_class_name(), g_instance);
        g_class_registered = 0;
    }
}

static void *win32_create(void *user, const char *title, int width, int height, int *out_w, int *out_h)
{
    HOWM89_Win32Impl *impl;
    HWND native_window;
    RECT r;
    DWORD style;
    DWORD exstyle;
    (void)user;
    if (width <= 0 || height <= 0) return 0;
    if (!g_class_registered && win32_register_class() != 0) return 0;
    impl = win32_alloc();
    if (impl == 0) return 0;
    style = WS_OVERLAPPEDWINDOW;
    exstyle = 0;
    r.left = 0; r.top = 0; r.right = width; r.bottom = height;
    AdjustWindowRectEx(&r, style, FALSE, exstyle);
    native_window = CreateWindowExA(exstyle, win32_class_name(),
                                    title ? title : "Howlund Window",
                                    style, CW_USEDEFAULT, CW_USEDEFAULT,
                                    r.right - r.left, r.bottom - r.top,
                                    NULL, NULL, g_instance, (LPVOID)impl);
    if (native_window == NULL) {
        win32_release(impl);
        return 0;
    }
    impl->native_window = native_window;
    impl->windowed_style = style;
    impl->windowed_exstyle = exstyle;
    ShowWindow(native_window, SW_SHOW);
    UpdateWindow(native_window);
    win32_measure(impl);
    if (impl->client_w <= 0) impl->client_w = width;
    if (impl->client_h <= 0) impl->client_h = height;
    if (out_w != 0) *out_w = impl->client_w;
    if (out_h != 0) *out_h = impl->client_h;
    return impl;
}

static void win32_destroy(void *user, void *ptr)
{
    HOWM89_Win32Impl *impl = (HOWM89_Win32Impl *)ptr;
    (void)user;
    if (impl == 0) return;
    if (impl->native_window != NULL) DestroyWindow(impl->native_window);
    win32_release(impl);
}

static int win32_set_size(void *user, void *ptr, int width, int height)
{
    HOWM89_Win32Impl *impl = (HOWM89_Win32Impl *)ptr;
    RECT r;
    LONG style;
    LONG exstyle;
    (void)user;
    if (impl == 0 || impl->native_window == NULL || width <= 0 || height <= 0) return -1;
    r.left = 0; r.top = 0; r.right = width; r.bottom = height;
    style = GetWindowLongA(impl->native_window, GWL_STYLE);
    exstyle = GetWindowLongA(impl->native_window, GWL_EXSTYLE);
    AdjustWindowRectEx(&r, (DWORD)style, FALSE, (DWORD)exstyle);
    if (!SetWindowPos(impl->native_window, NULL, 0, 0, r.right-r.left, r.bottom-r.top,
                      SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER)) return -1;
    win32_measure(impl);
    return 0;
}

static int win32_get_size(void *user, void *ptr, int *width, int *height)
{
    HOWM89_Win32Impl *impl = (HOWM89_Win32Impl *)ptr;
    (void)user;
    if (impl == 0 || width == 0 || height == 0) return -1;
    win32_measure(impl);
    *width = impl->client_w;
    *height = impl->client_h;
    return 0;
}

static int win32_fullscreen(void *user, void *ptr, int enabled)
{
    HOWM89_Win32Impl *impl = (HOWM89_Win32Impl *)ptr;
    MONITORINFO mi;
    HWND native_window;
    (void)user;
    if (impl == 0 || impl->native_window == NULL) return -1;
    native_window = impl->native_window;
    if (enabled) {
        if (impl->fullscreen) return 0;
        impl->windowed_style = (DWORD)GetWindowLongA(native_window, GWL_STYLE);
        impl->windowed_exstyle = (DWORD)GetWindowLongA(native_window, GWL_EXSTYLE);
        GetWindowRect(native_window, &impl->windowed_rect);
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoA(MonitorFromWindow(native_window, MONITOR_DEFAULTTONEAREST), &mi)) return -1;
        SetWindowLongA(native_window, GWL_STYLE, impl->windowed_style & ~(WS_OVERLAPPEDWINDOW));
        SetWindowPos(native_window, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        impl->fullscreen = 1;
    } else {
        if (!impl->fullscreen) return 0;
        SetWindowLongA(native_window, GWL_STYLE, impl->windowed_style);
        SetWindowLongA(native_window, GWL_EXSTYLE, impl->windowed_exstyle);
        SetWindowPos(native_window, NULL, impl->windowed_rect.left, impl->windowed_rect.top,
                     impl->windowed_rect.right-impl->windowed_rect.left,
                     impl->windowed_rect.bottom-impl->windowed_rect.top,
                     SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        impl->fullscreen = 0;
    }
    return 0;
}

static int win32_minimize(void *user, void *ptr) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;ShowWindow(i->native_window,SW_MINIMIZE);return 0; }
static int win32_restore(void *user, void *ptr) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;ShowWindow(i->native_window,SW_RESTORE);return 0; }
static int win32_maximize(void *user, void *ptr) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;ShowWindow(i->native_window,SW_MAXIMIZE);return 0; }

static int win32_opacity(void *user, void *ptr, HOWM89_Fix alpha)
{
    HOWM89_Win32Impl *impl=(HOWM89_Win32Impl*)ptr;
    BYTE a;
    LONG ex;
    (void)user;
    if(!impl||!impl->native_window)return -1;
    if(alpha<0)alpha=0;if(alpha>HOWM89_Q16_ONE)alpha=HOWM89_Q16_ONE;
    a=(BYTE)(((unsigned int)alpha*255U+32768U)>>16);
    ex=GetWindowLongA(impl->native_window,GWL_EXSTYLE);
    if(!(ex&WS_EX_LAYERED))SetWindowLongA(impl->native_window,GWL_EXSTYLE,ex|WS_EX_LAYERED);
    return SetLayeredWindowAttributes(impl->native_window,0,a,LWA_ALPHA)?0:-1;
}

static int win32_component_opacity(void *user, void *ptr, unsigned int mask, HOWM89_Fix alpha)
{
    (void)mask;
    return win32_opacity(user,ptr,alpha);
}

static void *win32_native(void *user, void *ptr) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;return i?(void*)i->native_window:0; }
static int win32_show(void *user, void *ptr, int show) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;ShowWindow(i->native_window,show?SW_SHOW:SW_HIDE);return 0; }
static int win32_visible(void *user, void *ptr, int *out) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window||!out)return -1;*out=IsWindowVisible(i->native_window)?1:0;return 0; }
static int win32_title(void *user, void *ptr, const char *title) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;return SetWindowTextA(i->native_window,title?title:"")?0:-1; }

static int win32_get_title(void *user, void *ptr, char *buffer, int cap, int *required)
{
    HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;
    int len;
    (void)user;
    if(!i||!i->native_window)return -1;
    len=GetWindowTextLengthA(i->native_window);if(len<0)len=0;
    if(required)*required=len+1;
    if(buffer&&cap>0){GetWindowTextA(i->native_window,buffer,cap);buffer[cap-1]='\0';}
    return 0;
}

static int win32_position(void *user, void *ptr, int x, int y) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;return SetWindowPos(i->native_window,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER)?0:-1; }
static int win32_get_position(void *user, void *ptr, int *x, int *y) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;RECT r;(void)user;if(!i||!i->native_window||!x||!y)return -1;if(!GetWindowRect(i->native_window,&r))return -1;*x=r.left;*y=r.top;return 0; }

static int win32_resizable(void *user, void *ptr, int value)
{
    HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;LONG s;(void)user;if(!i||!i->native_window)return -1;
    s=GetWindowLongA(i->native_window,GWL_STYLE);if(value)s|=(WS_THICKFRAME|WS_MAXIMIZEBOX);else s&=~(WS_THICKFRAME|WS_MAXIMIZEBOX);
    SetWindowLongA(i->native_window,GWL_STYLE,s);SetWindowPos(i->native_window,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);return 0;
}

static int win32_decorated(void *user, void *ptr, int value)
{
    HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;LONG s;(void)user;if(!i||!i->native_window)return -1;
    s=GetWindowLongA(i->native_window,GWL_STYLE);if(value){s&=~WS_POPUP;s|=(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX);}else{s&=~(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME);s|=WS_POPUP;}
    SetWindowLongA(i->native_window,GWL_STYLE,s);SetWindowPos(i->native_window,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);return 0;
}

static int win32_topmost(void *user, void *ptr, int value) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;return SetWindowPos(i->native_window,value?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE)?0:-1; }
static int win32_is_minimized(void *user, void *ptr, int *out) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window||!out)return -1;*out=IsIconic(i->native_window)?1:0;return 0; }
static int win32_is_maximized(void *user, void *ptr, int *out) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window||!out)return -1;*out=IsZoomed(i->native_window)?1:0;return 0; }
static int win32_focus(void *user, void *ptr) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;SetForegroundWindow(i->native_window);SetFocus(i->native_window);return 0; }

static int win32_attention(void *user, void *ptr)
{
    HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;FLASHWINFO f;(void)user;if(!i||!i->native_window)return -1;
    memset(&f,0,sizeof(f));f.cbSize=sizeof(f);f.hwnd=i->native_window;f.dwFlags=FLASHW_ALL|FLASHW_TIMERNOFG;f.uCount=3;FlashWindowEx(&f);return 0;
}

static int win32_close(void *user, void *ptr) { HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i||!i->native_window)return -1;return PostMessageA(i->native_window,WM_CLOSE,0,0)?0:-1; }
static int win32_limits(void *user, void *ptr, int a,int b,int c,int d){HOWM89_Win32Impl*i=(HOWM89_Win32Impl*)ptr;(void)user;if(!i)return -1;i->min_w=a;i->min_h=b;i->max_w=c;i->max_h=d;SetWindowPos(i->native_window,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);return 0;}

static int win32_scale(void *user, void *ptr, HOWM89_Fix *out)
{
    HOWM89_Win32Impl *i=(HOWM89_Win32Impl*)ptr;
    UINT dpi=96;
    HDC dc;
    int xdpi;
    (void)user;
    if(!i||!i->native_window||!out)return -1;
    dc=GetDC(i->native_window);
    if(dc){xdpi=GetDeviceCaps(dc,LOGPIXELSX);if(xdpi>0)dpi=(UINT)xdpi;ReleaseDC(i->native_window,dc);}
    *out=(HOWM89_Fix)(((unsigned int)dpi<<16)/96U);
    return 0;
}

HOWM89_Result howm89_register_win32_backend(HINSTANCE instance, WNDPROC wndproc, const char *class_name)
{
    HOWM89_BackendVTable v;
    int i;
    g_instance=instance;g_user_wndproc=wndproc;
    if(class_name!=0&&class_name[0]!='\0'){
        i=0;while(i<63&&class_name[i]!='\0'){g_class_name[i]=class_name[i];++i;}g_class_name[i]='\0';
    }
    memset(&v,0,sizeof(v));
    v.init=win32_init;v.shutdown=win32_shutdown;v.create_window=win32_create;v.destroy_window=win32_destroy;
    v.set_window_size=win32_set_size;v.get_window_size=win32_get_size;v.set_fullscreen=win32_fullscreen;
    v.minimize=win32_minimize;v.restore=win32_restore;v.maximize=win32_maximize;v.set_opacity_q16=win32_opacity;
    v.set_component_opacity_q16=win32_component_opacity;v.get_native_handle=win32_native;v.show_window=win32_show;
    v.is_window_visible=win32_visible;v.set_title=win32_title;v.get_title=win32_get_title;v.set_position=win32_position;
    v.get_position=win32_get_position;v.set_resizable=win32_resizable;v.set_decorated=win32_decorated;v.set_topmost=win32_topmost;
    v.is_minimized=win32_is_minimized;v.is_maximized=win32_is_maximized;v.focus=win32_focus;v.request_attention=win32_attention;
    v.request_close=win32_close;v.set_size_limits=win32_limits;v.get_scale_factor_q16=win32_scale;
    return howm89_register_backend(&v,0);
}

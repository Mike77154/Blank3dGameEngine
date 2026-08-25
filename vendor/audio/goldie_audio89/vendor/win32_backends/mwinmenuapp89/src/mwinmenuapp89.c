#include "mwinmenuapp89.h"
#include "mwinfont89.h"
#include "mwingdi89.h"

static const mwinmenuapp89_config *g_mwinmenuapp89_config;
static mwinfont89_state g_mwinmenuapp89_fonts;
static unsigned long g_mwinmenuapp89_start_tick;
static HDC g_mwinmenuapp89_back_dc;
static HBITMAP g_mwinmenuapp89_back_bitmap;
static HGDIOBJ g_mwinmenuapp89_back_old_bitmap;
static int g_mwinmenuapp89_back_width;
static int g_mwinmenuapp89_back_height;
static int g_mwinmenuapp89_hand_cursor;

static void mwinmenuapp89_release_backbuffer(void)
{
    if (g_mwinmenuapp89_back_dc != (HDC)0) {
        if (g_mwinmenuapp89_back_old_bitmap != (HGDIOBJ)0) {
            (void)SelectObject(
                g_mwinmenuapp89_back_dc,
                g_mwinmenuapp89_back_old_bitmap
            );
        }
        if (g_mwinmenuapp89_back_bitmap != (HBITMAP)0) {
            (void)DeleteObject((HGDIOBJ)g_mwinmenuapp89_back_bitmap);
        }
        (void)DeleteDC(g_mwinmenuapp89_back_dc);
    }
    g_mwinmenuapp89_back_dc = (HDC)0;
    g_mwinmenuapp89_back_bitmap = (HBITMAP)0;
    g_mwinmenuapp89_back_old_bitmap = (HGDIOBJ)0;
    g_mwinmenuapp89_back_width = 0;
    g_mwinmenuapp89_back_height = 0;
}

static int mwinmenuapp89_ensure_backbuffer(HDC target, int width, int height)
{
    HDC new_dc;
    HBITMAP new_bitmap;
    HGDIOBJ old_bitmap;
    if (target == (HDC)0 || width <= 0 || height <= 0) {
        return 0;
    }
    if (g_mwinmenuapp89_back_dc != (HDC)0 &&
        g_mwinmenuapp89_back_bitmap != (HBITMAP)0 &&
        g_mwinmenuapp89_back_width == width &&
        g_mwinmenuapp89_back_height == height) {
        return 1;
    }
    new_dc = CreateCompatibleDC(target);
    if (new_dc == (HDC)0) {
        return 0;
    }
    new_bitmap = CreateCompatibleBitmap(target, width, height);
    if (new_bitmap == (HBITMAP)0) {
        (void)DeleteDC(new_dc);
        return 0;
    }
    old_bitmap = SelectObject(new_dc, (HGDIOBJ)new_bitmap);
    if (old_bitmap == (HGDIOBJ)0) {
        (void)DeleteObject((HGDIOBJ)new_bitmap);
        (void)DeleteDC(new_dc);
        return 0;
    }
    mwinmenuapp89_release_backbuffer();
    g_mwinmenuapp89_back_dc = new_dc;
    g_mwinmenuapp89_back_bitmap = new_bitmap;
    g_mwinmenuapp89_back_old_bitmap = old_bitmap;
    g_mwinmenuapp89_back_width = width;
    g_mwinmenuapp89_back_height = height;
    return 1;
}

static int mwinmenuapp89_menu_visible(void)
{
    const mwinmenuapp89_config *config;
    config = g_mwinmenuapp89_config;
    if (config == (const mwinmenuapp89_config *)0) {
        return 0;
    }
    if (config->is_menu_visible == (mwinmenuapp89_visible_fn)0) {
        return 1;
    }
    return config->is_menu_visible(config->user);
}

static unsigned long mwinmenuapp89_elapsed(void)
{
    const mwinmenuapp89_config *config;
    config = g_mwinmenuapp89_config;
    if (config == (const mwinmenuapp89_config *)0 ||
        config->engine == (mmenuengine89_state *)0) {
        return 0UL;
    }
    return config->engine->timeline.elapsed_ms;
}

static void mwinmenuapp89_ui_event(int event_id)
{
    const mwinmenuapp89_config *config;
    config = g_mwinmenuapp89_config;
    if (config != (const mwinmenuapp89_config *)0 &&
        config->on_ui_event != (mwinmenuapp89_ui_event_fn)0) {
        config->on_ui_event(config->user, event_id, mwinmenuapp89_elapsed());
    }
}

static void mwinmenuapp89_process_cues(void)
{
    int cue;
    const mwinmenuapp89_config *config;
    config = g_mwinmenuapp89_config;
    if (config == (const mwinmenuapp89_config *)0 ||
        config->engine == (mmenuengine89_state *)0) {
        return;
    }
    cue = mmenuengine89_take_cue(config->engine);
    while (cue != 0) {
        if (config->on_cue != (mwinmenuapp89_cue_fn)0) {
            config->on_cue(
                config->user,
                cue,
                config->engine->timeline.elapsed_ms
            );
        }
        cue = mmenuengine89_take_cue(config->engine);
    }
}

typedef struct mwinmenuapp89_text_proxy {
    mtext89_provider base;
    const mwinmenuapp89_config *config;
} mwinmenuapp89_text_proxy;

static void mwinmenuapp89_proxy_draw_text(
    void *user,
    const mcanvas89_rect *source,
    const char *text,
    const mtext89_style *style)
{
    mwinmenuapp89_text_proxy *proxy;
    mcanvas89_rect rect;
    mtext89_style transformed_style;
    int draw_text;
    proxy = (mwinmenuapp89_text_proxy *)user;
    if (proxy == (mwinmenuapp89_text_proxy *)0 ||
        proxy->base.draw_text == (void (*)(void *, const mcanvas89_rect *,
            const char *, const mtext89_style *))0 ||
        source == (const mcanvas89_rect *)0 ||
        style == (const mtext89_style *)0) {
        return;
    }
    rect = *source;
    transformed_style = *style;
    draw_text = 1;
    if (proxy->config != (const mwinmenuapp89_config *)0 &&
        proxy->config->transform_text !=
            (mwinmenuapp89_text_transform_fn)0) {
        draw_text = proxy->config->transform_text(
            proxy->config->user,
            source,
            text,
            style,
            &rect,
            &transformed_style
        );
    }
    if (draw_text) {
        proxy->base.draw_text(
            proxy->base.user,
            &rect,
            text,
            &transformed_style
        );
    }
}

static int mwinmenuapp89_proxy_measure_text(
    void *user,
    const char *text,
    const mtext89_style *style,
    mtext89_extent *extent)
{
    mwinmenuapp89_text_proxy *proxy;
    proxy = (mwinmenuapp89_text_proxy *)user;
    if (proxy == (mwinmenuapp89_text_proxy *)0 ||
        proxy->base.measure_text == (int (*)(void *, const char *,
            const mtext89_style *, mtext89_extent *))0) {
        return 0;
    }
    return proxy->base.measure_text(
        proxy->base.user,
        text,
        style,
        extent
    );
}

static void mwinmenuapp89_render_frame(
    HDC target_dc,
    int width,
    int height,
    unsigned long elapsed)
{
    RECT clear_rect;
    mwingdi89_state gdi;
    mcanvas89_provider canvas;
    mtext89_provider text;
    mtext89_provider transformed_text;
    mwinmenuapp89_text_proxy text_proxy;
    const mwinmenuapp89_config *config;
    config = g_mwinmenuapp89_config;
    clear_rect.left = 0;
    clear_rect.top = 0;
    clear_rect.right = width;
    clear_rect.bottom = height;
    (void)SetDCBrushColor(target_dc, RGB(0, 0, 0));
    (void)FillRect(target_dc, &clear_rect, (HBRUSH)GetStockObject(DC_BRUSH));
    mwingdi89_init(&gdi, target_dc);
    canvas = mwingdi89_canvas_provider(&gdi);
    text = mwingdi89_text_provider(&gdi);
    text_proxy.base = text;
    text_proxy.config = config;
    transformed_text.user = &text_proxy;
    transformed_text.draw_text = mwinmenuapp89_proxy_draw_text;
    transformed_text.measure_text = mwinmenuapp89_proxy_measure_text;
    if (config != (const mwinmenuapp89_config *)0) {
        if (config->prepare_render !=
            (mwinmenuapp89_prepare_render_fn)0) {
            config->prepare_render(
                config->user,
                width,
                height,
                elapsed
            );
        }
        if (config->render_underlay != (mwinmenuapp89_render_fn)0) {
            config->render_underlay(
                config->user,
                width,
                height,
                elapsed,
                &canvas,
                &transformed_text
            );
        }
        if (config->engine != (mmenuengine89_state *)0 &&
            mwinmenuapp89_menu_visible()) {
            mmenuengine89_render(
                config->engine,
                width,
                height,
                &canvas,
                &transformed_text
            );
        }
        if (config->render_overlay != (mwinmenuapp89_render_fn)0) {
            config->render_overlay(
                config->user,
                width,
                height,
                elapsed,
                &canvas,
                &transformed_text
            );
        }
    }
}

static void mwinmenuapp89_paint(HWND window)
{
    PAINTSTRUCT paint;
    HDC dc;
    HDC render_dc;
    RECT client;
    const mwinmenuapp89_config *config;
    int width;
    int height;
    int buffered;
    unsigned long elapsed;
    config = g_mwinmenuapp89_config;
    dc = BeginPaint(window, &paint);
    GetClientRect(window, &client);
    width = client.right - client.left;
    height = client.bottom - client.top;
    elapsed = mwinmenuapp89_elapsed();
    buffered = config != (const mwinmenuapp89_config *)0 &&
        config->double_buffer &&
        mwinmenuapp89_ensure_backbuffer(dc, width, height);
    render_dc = buffered ? g_mwinmenuapp89_back_dc : dc;
    mwinmenuapp89_render_frame(render_dc, width, height, elapsed);
    if (buffered) {
        (void)BitBlt(dc, 0, 0, width, height,
            g_mwinmenuapp89_back_dc, 0, 0, SRCCOPY);
    }
    EndPaint(window, &paint);
}

static void mwinmenuapp89_activate(HWND window)
{
    const mwinmenuapp89_config *config;
    int command;
    int close_requested;
    config = g_mwinmenuapp89_config;
    if (config == (const mwinmenuapp89_config *)0 ||
        config->engine == (mmenuengine89_state *)0 ||
        !mwinmenuapp89_menu_visible()) {
        return;
    }
    command = mmenuengine89_activate(config->engine);
    close_requested = 0;
    if (command != 0) {
        mwinmenuapp89_ui_event(MWINMENUAPP89_UI_ACTIVATE);
    }
    if (command != 0 && config->on_command != (mwinmenuapp89_command_fn)0) {
        close_requested = config->on_command(
            config->user,
            command,
            config->engine->timeline.elapsed_ms
        );
    }
    if (close_requested) {
        DestroyWindow(window);
    } else {
        InvalidateRect(window, (const RECT *)0, FALSE);
    }
}

static int mwinmenuapp89_forward_key(
    HWND window,
    unsigned int key,
    int is_down)
{
    const mwinmenuapp89_config *config;
    int result;
    config = g_mwinmenuapp89_config;
    if (config == (const mwinmenuapp89_config *)0 ||
        config->on_key == (mwinmenuapp89_key_fn)0) {
        return 0;
    }
    result = config->on_key(
        config->user,
        key,
        is_down,
        mwinmenuapp89_elapsed()
    );
    if (result & MWINMENUAPP89_KEY_REQUEST_CLOSE) {
        DestroyWindow(window);
        return 1;
    }
    if (result & MWINMENUAPP89_KEY_HANDLED) {
        InvalidateRect(window, (const RECT *)0, FALSE);
        return 1;
    }
    return 0;
}

static int mwinmenuapp89_mouse_x(LPARAM value)
{
    return (int)(short)(value & 0xFFFFL);
}

static int mwinmenuapp89_mouse_y(LPARAM value)
{
    return (int)(short)((value >> 16) & 0xFFFFL);
}

static int mwinmenuapp89_forward_pointer(
    HWND window,
    int event_type,
    int x,
    int y,
    int button)
{
    const mwinmenuapp89_config *config;
    RECT client;
    int result;
    int width;
    int height;
    config = g_mwinmenuapp89_config;
    if (config == (const mwinmenuapp89_config *)0 ||
        config->on_pointer == (mwinmenuapp89_pointer_fn)0) {
        g_mwinmenuapp89_hand_cursor = 0;
        return 0;
    }
    GetClientRect(window, &client);
    width = client.right - client.left;
    height = client.bottom - client.top;
    result = config->on_pointer(
        config->user,
        event_type,
        x,
        y,
        width,
        height,
        button,
        mwinmenuapp89_elapsed()
    );
    g_mwinmenuapp89_hand_cursor =
        (result & MWINMENUAPP89_POINTER_HAND_CURSOR) != 0;
    if (result & MWINMENUAPP89_POINTER_REQUEST_CLOSE) {
        DestroyWindow(window);
        return 1;
    }
    if (result & MWINMENUAPP89_POINTER_HANDLED) {
        InvalidateRect(window, (const RECT *)0, FALSE);
        return 1;
    }
    return 0;
}

static LRESULT CALLBACK mwinmenuapp89_window_proc(
    HWND window,
    UINT message,
    WPARAM w_param,
    LPARAM l_param)
{
    const mwinmenuapp89_config *config;
    HDC dc;
    mfont89_provider font_provider;
    DWORD elapsed;
    config = g_mwinmenuapp89_config;
    switch (message) {
    case WM_CREATE:
        if (config == (const mwinmenuapp89_config *)0 ||
            config->engine == (mmenuengine89_state *)0) {
            return -1;
        }
        dc = GetDC(window);
        mwinfont89_init(&g_mwinmenuapp89_fonts, dc);
        font_provider = mwinfont89_make_provider(&g_mwinmenuapp89_fonts);
        mmenuengine89_prepare_fonts(config->engine, &font_provider);
        mwinfont89_set_lookup_dc(&g_mwinmenuapp89_fonts, (HDC)0);
        ReleaseDC(window, dc);
        g_mwinmenuapp89_start_tick = GetTickCount();
        SetTimer(window, config->timer_id, config->timer_ms, (TIMERPROC)0);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
        (void)mwinmenuapp89_forward_pointer(
            window,
            MWINMENUAPP89_POINTER_MOVE,
            mwinmenuapp89_mouse_x(l_param),
            mwinmenuapp89_mouse_y(l_param),
            MWINMENUAPP89_POINTER_BUTTON_NONE
        );
        return 0;
    case WM_LBUTTONDOWN:
        (void)SetFocus(window);
        if (mwinmenuapp89_forward_pointer(
                window,
                MWINMENUAPP89_POINTER_DOWN,
                mwinmenuapp89_mouse_x(l_param),
                mwinmenuapp89_mouse_y(l_param),
                MWINMENUAPP89_POINTER_BUTTON_LEFT)) {
            (void)SetCapture(window);
        }
        return 0;
    case WM_LBUTTONUP:
        (void)mwinmenuapp89_forward_pointer(
            window,
            MWINMENUAPP89_POINTER_UP,
            mwinmenuapp89_mouse_x(l_param),
            mwinmenuapp89_mouse_y(l_param),
            MWINMENUAPP89_POINTER_BUTTON_LEFT
        );
        (void)ReleaseCapture();
        return 0;
    case WM_RBUTTONUP:
        (void)mwinmenuapp89_forward_pointer(
            window,
            MWINMENUAPP89_POINTER_UP,
            mwinmenuapp89_mouse_x(l_param),
            mwinmenuapp89_mouse_y(l_param),
            MWINMENUAPP89_POINTER_BUTTON_RIGHT
        );
        return 0;
    case WM_CAPTURECHANGED:
        (void)mwinmenuapp89_forward_pointer(
            window,
            MWINMENUAPP89_POINTER_CANCEL,
            0,
            0,
            MWINMENUAPP89_POINTER_BUTTON_NONE
        );
        return 0;
    case WM_SETCURSOR:
        if (g_mwinmenuapp89_hand_cursor) {
            (void)SetCursor(LoadCursorA((HINSTANCE)0, IDC_HAND));
            return TRUE;
        }
        return DefWindowProcA(window, message, w_param, l_param);
    case WM_KEYDOWN:
        if (mwinmenuapp89_forward_key(window, (unsigned int)w_param, 1)) {
            return 0;
        }
        if (!mwinmenuapp89_menu_visible()) {
            return 0;
        }
        if (w_param == VK_ESCAPE) {
            if (config->escape_closes_window) {
                DestroyWindow(window);
            }
            return 0;
        }
        if (!mmenuengine89_accepts_input(config->engine)) {
            if (w_param == VK_RETURN || w_param == VK_SPACE) {
                mmenuengine89_skip_intro(config->engine);
                mwinmenuapp89_ui_event(MWINMENUAPP89_UI_SKIP);
                mwinmenuapp89_process_cues();
                InvalidateRect(window, (const RECT *)0, FALSE);
            }
            return 0;
        }
        if (w_param == VK_UP) {
            mmenuengine89_move(config->engine, -1);
            mwinmenuapp89_ui_event(MWINMENUAPP89_UI_MOVE);
            InvalidateRect(window, (const RECT *)0, FALSE);
        } else if (w_param == VK_DOWN) {
            mmenuengine89_move(config->engine, 1);
            mwinmenuapp89_ui_event(MWINMENUAPP89_UI_MOVE);
            InvalidateRect(window, (const RECT *)0, FALSE);
        } else if (w_param == VK_RETURN || w_param == VK_SPACE) {
            mwinmenuapp89_activate(window);
        }
        return 0;
    case WM_KEYUP:
        mwinmenuapp89_forward_key(window, (unsigned int)w_param, 0);
        return 0;
    case WM_TIMER:
        if (config != (const mwinmenuapp89_config *)0 &&
            w_param == config->timer_id) {
            elapsed = GetTickCount() - g_mwinmenuapp89_start_tick;
            mmenuengine89_update(config->engine, (unsigned long)elapsed);
            mwinmenuapp89_process_cues();
            if (config->on_tick != (mwinmenuapp89_tick_fn)0) {
                config->on_tick(
                    config->user,
                    config->engine->timeline.elapsed_ms
                );
            }
            InvalidateRect(window, (const RECT *)0, FALSE);
        }
        return 0;
    case WM_PAINT:
        mwinmenuapp89_paint(window);
        return 0;
    case WM_DESTROY:
        if (config != (const mwinmenuapp89_config *)0) {
            KillTimer(window, config->timer_id);
            if (config->on_shutdown != (mwinmenuapp89_shutdown_fn)0) {
                config->on_shutdown(config->user);
            }
        }
        mwinmenuapp89_release_backbuffer();
        mwinfont89_shutdown(&g_mwinmenuapp89_fonts);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(window, message, w_param, l_param);
    }
}

int mwinmenuapp89_run(
    HINSTANCE instance,
    int show_command,
    const mwinmenuapp89_config *config)
{
    WNDCLASSA window_class;
    HWND window;
    MSG message;
    RECT window_rect;
    DWORD style;
    if (config == (const mwinmenuapp89_config *)0 ||
        config->engine == (mmenuengine89_state *)0) {
        return 1;
    }
    g_mwinmenuapp89_config = config;
    g_mwinmenuapp89_hand_cursor = 0;
    ZeroMemory(&window_class, sizeof(window_class));
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = mwinmenuapp89_window_proc;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIconA((HINSTANCE)0, IDI_APPLICATION);
    window_class.hCursor = LoadCursorA((HINSTANCE)0, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)0;
    window_class.lpszClassName = config->class_name;
    if (!RegisterClassA(&window_class)) {
        return 2;
    }
    style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    window_rect.left = 0;
    window_rect.top = 0;
    window_rect.right = config->client_width;
    window_rect.bottom = config->client_height;
    AdjustWindowRect(&window_rect, style, FALSE);
    window = CreateWindowExA(
        0,
        config->class_name,
        config->window_title,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        (HWND)0,
        (HMENU)0,
        instance,
        (LPVOID)0
    );
    if (window == (HWND)0) {
        return 3;
    }
    ShowWindow(window, show_command);
    UpdateWindow(window);
    while (GetMessageA(&message, (HWND)0, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    g_mwinmenuapp89_config = (const mwinmenuapp89_config *)0;
    return (int)message.wParam;
}

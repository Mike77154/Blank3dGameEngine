#include <stdio.h>
#include <string.h>

#include "howm89.h"
#include "howm89_genwinconfigc89.h"
#include "genwinconfigc89.h"

typedef struct FakeWindow {
    int alive;
    int width;
    int height;
    int x;
    int y;
    int fullscreen;
    int resizable;
    int decorated;
    int visible;
    int topmost;
    char title[128];
} FakeWindow;

typedef struct FakeBackend {
    FakeWindow window;
    int init_count;
    int shutdown_count;
} FakeBackend;

static void copy_text(char *dst, int cap, const char *src)
{
    int i;
    i = 0;
    if (!src) src = "";
    while (src[i] && i + 1 < cap) { dst[i] = src[i]; ++i; }
    dst[i] = '\0';
}

static int fb_init(void *u)
{
    FakeBackend *b = (FakeBackend *)u;
    ++b->init_count;
    return 0;
}

static void fb_shutdown(void *u)
{
    FakeBackend *b = (FakeBackend *)u;
    ++b->shutdown_count;
}

static void *fb_create(void *u, const char *title, int w, int h, int *ow, int *oh)
{
    FakeBackend *b = (FakeBackend *)u;
    memset(&b->window, 0, sizeof(b->window));
    b->window.alive = 1;
    b->window.width = w;
    b->window.height = h;
    b->window.resizable = 1;
    b->window.decorated = 1;
    b->window.visible = 1;
    copy_text(b->window.title, (int)sizeof(b->window.title), title);
    if (ow) *ow = w;
    if (oh) *oh = h;
    return &b->window;
}

static void fb_destroy(void *u, void *impl)
{
    FakeWindow *w = (FakeWindow *)impl;
    (void)u;
    if (w) w->alive = 0;
}

static int fb_set_size(void *u, void *impl, int width, int height)
{
    FakeWindow *w = (FakeWindow *)impl;
    (void)u;
    w->width = width; w->height = height; return 0;
}
static int fb_get_size(void *u, void *impl, int *width, int *height)
{
    FakeWindow *w = (FakeWindow *)impl;
    (void)u;
    *width = w->width; *height = w->height; return 0;
}
static int fb_fullscreen(void *u, void *impl, int v)
{ (void)u; ((FakeWindow *)impl)->fullscreen = v ? 1 : 0; return 0; }
static int fb_show(void *u, void *impl, int v)
{ (void)u; ((FakeWindow *)impl)->visible = v ? 1 : 0; return 0; }
static int fb_title(void *u, void *impl, const char *s)
{ (void)u; copy_text(((FakeWindow *)impl)->title, 128, s); return 0; }
static int fb_position(void *u, void *impl, int x, int y)
{ (void)u; ((FakeWindow *)impl)->x=x; ((FakeWindow *)impl)->y=y; return 0; }
static int fb_resizable(void *u, void *impl, int v)
{ (void)u; ((FakeWindow *)impl)->resizable=v?1:0; return 0; }
static int fb_decorated(void *u, void *impl, int v)
{ (void)u; ((FakeWindow *)impl)->decorated=v?1:0; return 0; }
static int fb_topmost(void *u, void *impl, int v)
{ (void)u; ((FakeWindow *)impl)->topmost=v?1:0; return 0; }
static void *fb_native(void *u, void *impl)
{ (void)u; return impl; }

int main(void)
{
    FakeBackend backend;
    HOWM89_BackendVTable vt;
    howm89_gwc89_adapter adapter;
    gwc89_provider provider;
    gwc89_state state;
    int w;
    int h;

    memset(&backend, 0, sizeof(backend));
    memset(&vt, 0, sizeof(vt));
    vt.init = fb_init;
    vt.shutdown = fb_shutdown;
    vt.create_window = fb_create;
    vt.destroy_window = fb_destroy;
    vt.set_window_size = fb_set_size;
    vt.get_window_size = fb_get_size;
    vt.set_fullscreen = fb_fullscreen;
    vt.show_window = fb_show;
    vt.set_title = fb_title;
    vt.set_position = fb_position;
    vt.set_resizable = fb_resizable;
    vt.set_decorated = fb_decorated;
    vt.set_topmost = fb_topmost;
    vt.get_native_handle = fb_native;

    if (howm89_register_backend(&vt, &backend) != HOWM89_OK) return 1;
    howm89_gwc89_adapter_init(&adapter);
    howm89_gwc89_adapter_make_provider(&adapter, &provider);
    gwc89_defaults(&state);
    gwc89_set_provider(&state, &provider);
    if (!gwc89_set_client_size(&state, 800, 450)) return 2;
    if (!gwc89_set_title(&state, "Adapter Test")) return 3;
    if (!gwc89_create(&state)) return 4;
    if (!backend.window.alive || strcmp(backend.window.title, "Adapter Test") != 0) return 5;
    if (!gwc89_refresh(&state)) return 6;
    if (state.observed_client_width != 800 || state.observed_client_height != 450) return 7;

    if (!gwc89_set_client_size(&state, 1280, 720)) return 8;
    if (!gwc89_set_resizable(&state, 0)) return 9;
    if (!gwc89_set_decorated(&state, 0)) return 10;
    if (!gwc89_set_topmost(&state, 1)) return 11;
    if (!gwc89_set_position(&state, 22, 33)) return 12;
    if (!gwc89_apply(&state)) return 13;
    if (backend.window.width != 1280 || backend.window.height != 720) return 14;
    if (backend.window.resizable || backend.window.decorated || !backend.window.topmost) return 15;
    if (backend.window.x != 22 || backend.window.y != 33) return 16;

    if (!gwc89_set_play_mode(&state, GWC89_MODE_BORDERLESS_FULLSCREEN)) return 17;
    if (!gwc89_apply(&state) || !backend.window.fullscreen) return 18;
    if (!gwc89_set_play_mode(&state, GWC89_MODE_WINDOWED)) return 19;
    if (!gwc89_apply(&state) || backend.window.fullscreen) return 20;

    w = h = 0;
    if (!provider.query_client_size(provider.user, &w, &h)) return 21;
    if (w != 1280 || h != 720) return 22;
    if (howm89_gwc89_adapter_native_handle(&adapter) != &backend.window) return 23;
    if (!gwc89_destroy(&state) || backend.window.alive) return 24;

    howm89_library_shutdown();
    if (howm89_unregister_backend() != HOWM89_OK) return 25;
    printf("GenWinConfigC89 <-> HOWM89 adapter: PASS\n");
    return 0;
}

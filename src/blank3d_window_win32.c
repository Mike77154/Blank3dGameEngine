#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#include "blank3d_window_win32.h"

static int b3d_howlund_after_create(void *user, HOWM89_Window *window)
{
    Blank3DWindowWin32Host *host;
    void *native;
    host = (Blank3DWindowWin32Host *)user;
    if (!host || !window || !host->window || !host->device) return 0;
    native = howm89_window_get_native_handle(window);
    if (!native) return 0;
    *host->window = (HWND)native;
    *host->device = GetDC(*host->window);
    if (!*host->device) {
        *host->window = 0;
        return 0;
    }
    return 1;
}

static int b3d_howlund_before_destroy(void *user, HOWM89_Window *window)
{
    Blank3DWindowWin32Host *host;
    (void)window;
    host = (Blank3DWindowWin32Host *)user;
    if (!host || !host->window || !host->device) return 0;
    if (*host->device && *host->window) {
        (void)ReleaseDC(*host->window, *host->device);
        *host->device = 0;
    }
    *host->window = 0;
    return 1;
}

void blank3d_window_win32_host_init(Blank3DWindowWin32Host *host,
                                    HINSTANCE instance,
                                    WNDPROC window_proc,
                                    HWND *window,
                                    HDC *device)
{
    if (!host) return;
    memset(host, 0, sizeof(*host));
    host->instance = instance;
    host->window_proc = window_proc;
    host->window = window;
    host->device = device;
    windowswindow89_context_init(&host->windows_backend,
                                 instance,
                                 window_proc,
                                 "Blank3DWindowsWindow89");
    windowswindow89_make_backend(&host->windows_backend,
                                 &host->howm_backend);
    howm89_gwc89_adapter_init(&host->genwin_adapter);
    howm89_gwc89_adapter_set_hooks(&host->genwin_adapter,
                                    host,
                                    b3d_howlund_after_create,
                                    b3d_howlund_before_destroy);
}

int blank3d_window_win32_make_provider(Blank3DWindowWin32Host *host,
                                       gwc89_provider *out_provider)
{
    HOWM89_Result r;
    if (!host || !out_provider) return 0;
    if (!host->backend_registered) {
        if (howm89_library_is_initialized()) return 0;
        if (howm89_backend_is_registered()) {
            r = howm89_unregister_backend();
            if (r != HOWM89_OK) return 0;
        }
        r = howm89_register_backend(&host->howm_backend,
                                    &host->windows_backend);
        if (r != HOWM89_OK) return 0;
        host->backend_registered = 1;
    }
    howm89_gwc89_adapter_make_provider(&host->genwin_adapter,
                                        out_provider);
    return out_provider->create_window != 0;
}

void blank3d_window_win32_host_shutdown(Blank3DWindowWin32Host *host)
{
    if (!host) return;
    if (howm89_library_is_initialized()) howm89_library_shutdown();
    if (host->backend_registered && howm89_backend_is_registered())
        (void)howm89_unregister_backend();
    host->backend_registered = 0;
}

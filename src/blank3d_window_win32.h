#ifndef BLANK3D_WINDOW_WIN32_H
#define BLANK3D_WINDOW_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "genwinconfigc89.h"
#include "howm89.h"
#include "howm89_genwinconfigc89.h"
#include "windowswindow89.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Blank3D-specific interop bridge only.
 * Native window creation lives in WindowsWindow89 behind HOWM89.
 * GenWinConfigC89 remains unaware of HOWM89 and Windows.
 */
typedef struct Blank3DWindowWin32HostTag {
    HINSTANCE instance;
    WNDPROC window_proc;
    HWND *window;
    HDC *device;
    WindowsWindow89_Context windows_backend;
    HOWM89_BackendVTable howm_backend;
    howm89_gwc89_adapter genwin_adapter;
    int backend_registered;
} Blank3DWindowWin32Host;

void blank3d_window_win32_host_init(Blank3DWindowWin32Host *host,
                                    HINSTANCE instance,
                                    WNDPROC window_proc,
                                    HWND *window,
                                    HDC *device);
int blank3d_window_win32_make_provider(Blank3DWindowWin32Host *host,
                                       gwc89_provider *out_provider);
void blank3d_window_win32_host_shutdown(Blank3DWindowWin32Host *host);

#ifdef __cplusplus
}
#endif
#endif

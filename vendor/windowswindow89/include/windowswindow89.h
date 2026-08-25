#ifndef WINDOWSWINDOW89_H
#define WINDOWSWINDOW89_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "howm89.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WW89_MAX_WINDOWS
#define WW89_MAX_WINDOWS HOWM89_MAX_WINDOWS
#endif

typedef struct WindowsWindow89_Slot {
    int used;
    HWND native_window;
    int client_width;
    int client_height;
    int x;
    int y;
    int resizable;
    int decorated;
    int visible;
    int topmost;
    int fullscreen;
    int windowed_width;
    int windowed_height;
    int windowed_x;
    int windowed_y;
} WindowsWindow89_Slot;

typedef struct WindowsWindow89_Context {
    HINSTANCE instance;
    WNDPROC window_proc;
    char class_name[64];
    int class_registered;
    WindowsWindow89_Slot slots[WW89_MAX_WINDOWS];
} WindowsWindow89_Context;

void windowswindow89_context_init(WindowsWindow89_Context *ctx,
                                  HINSTANCE instance,
                                  WNDPROC window_proc,
                                  const char *class_name);
void windowswindow89_make_backend(WindowsWindow89_Context *ctx,
                                  HOWM89_BackendVTable *out_backend);

#ifdef __cplusplus
}
#endif

#endif

#ifndef HOWM89_WIN32_BACKEND_H
#define HOWM89_WIN32_BACKEND_H

#include <windows.h>
#include "howm89.h"

#ifdef __cplusplus
extern "C" {
#endif

HOWM89_Result howm89_register_win32_backend(HINSTANCE instance,
                                             WNDPROC wndproc,
                                             const char *class_name);

#ifdef __cplusplus
}
#endif

#endif

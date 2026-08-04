/* input_hook_backend_macos32_eventtap.h
   32-bit macOS alias for the EventTap backend.
   (Note: modern macOS is 64-bit only; this exists for legacy toolchains.)
*/
#ifndef INPUT_HOOK_BACKEND_MACOS32_EVENTTAP_H
#define INPUT_HOOK_BACKEND_MACOS32_EVENTTAP_H
#if defined(__APPLE__) && !defined(__LP64__)
#include "input_hook_backend_macos_eventtap.h"
#endif
#endif

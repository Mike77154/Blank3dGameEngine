/* input_hook_backend_win64_llhook.h

   Win64 alias for the robust low-level hook backend.
   Same implementation works for both x86 and x64.

   Include this if you want a "separate" backend name for 64-bit builds.
*/

#ifndef INPUT_HOOK_BACKEND_WIN64_LLHOOK_H
#define INPUT_HOOK_BACKEND_WIN64_LLHOOK_H

#ifdef _WIN64
#include "input_hook_backend_win32_llhook.h"
#endif

#endif /* INPUT_HOOK_BACKEND_WIN64_LLHOOK_H */

#ifdef _WIN64
/* This file is a thin wrapper that reuses the Win32 LL hook implementation.
   Compile either:
     - input_hook_backend_win32_llhook.c  (for x86/x64), OR
     - this file (for x64 only)
   but NOT both at the same time.
*/
#include "input_hook_backend_win32_llhook.c"
#endif

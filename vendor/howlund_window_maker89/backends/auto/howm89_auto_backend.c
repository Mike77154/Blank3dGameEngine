#include "howm89_auto_backend.h"

#if defined(_WIN32)
#include "howm89_win32_backend.h"
#include <windows.h>
HOWM89_Result howm89_register_default_backend(void)
{
    return howm89_register_win32_backend(GetModuleHandleA(0), 0, "HOWM89_WINDOW");
}
#elif defined(__APPLE__)
#include "howm89_cocoa_backend.h"
HOWM89_Result howm89_register_default_backend(void)
{
    return howm89_register_cocoa_backend();
}
#elif defined(__unix__)
#include "howm89_x11_backend.h"
HOWM89_Result howm89_register_default_backend(void)
{
    return howm89_register_x11_backend();
}
#else
HOWM89_Result howm89_register_default_backend(void)
{
    return HOWM89_ERROR_NOT_SUPPORTED;
}
#endif

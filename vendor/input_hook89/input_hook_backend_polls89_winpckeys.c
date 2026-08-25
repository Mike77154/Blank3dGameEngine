#include "input_hook_backend_polls89_winpckeys.h"
#if defined(_WIN64)
#include "by_system_backend/win64/winpckeys_backend.h"
#else
#include "by_system_backend/win32/winpckeys_backend.h"
#endif

static void ihk_polls89_win_update(void *backend)
{
    winpckeys_backend_update((winpckeys_backend *)backend);
}

static int ihk_polls89_win_down(const void *backend, input_key89 key)
{
    return winpckeys_input_key_down((const winpckeys_backend *)backend, key);
}

const ihk_polls89_key_api *ihk_polls89_api_winpckeys(void)
{
    static const ihk_polls89_key_api api = {
        ihk_polls89_win_update,
        ihk_polls89_win_down,
        (ihk_u32)IHK_CAP_GLOBAL_CAPTURE
    };
    return &api;
}

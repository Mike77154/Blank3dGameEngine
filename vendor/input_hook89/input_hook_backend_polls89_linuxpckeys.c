#include "input_hook_backend_polls89_linuxpckeys.h"
#if defined(__x86_64__) || defined(__aarch64__) || defined(__LP64__)
#include "by_system_backend/linux64/linuxpckeys_backend.h"
#else
#include "by_system_backend/linux32/linuxpckeys_backend.h"
#endif

static void ihk_polls89_linux_update(void *backend)
{
    linuxpckeys_backend_update((linuxpckeys_backend *)backend);
}
static int ihk_polls89_linux_down(const void *backend, input_key89 key)
{
    return linuxpckeys_input_key_down((const linuxpckeys_backend *)backend, key);
}
const ihk_polls89_key_api *ihk_polls89_api_linuxpckeys(void)
{
    static const ihk_polls89_key_api api = {
        ihk_polls89_linux_update,
        ihk_polls89_linux_down,
        (ihk_u32)IHK_CAP_LAYOUT_INDEPENDENT
    };
    return &api;
}

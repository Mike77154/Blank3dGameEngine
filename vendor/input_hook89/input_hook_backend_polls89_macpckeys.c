#include "input_hook_backend_polls89_macpckeys.h"
#if defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__)
#include "by_system_backend/mac64/macpckeys_backend.h"
#else
#include "by_system_backend/mac32/macpckeys_backend.h"
#endif

static void ihk_polls89_mac_update(void *backend)
{
    macpckeys_backend_update((macpckeys_backend *)backend);
}
static int ihk_polls89_mac_down(const void *backend, input_key89 key)
{
    return macpckeys_input_key_down((const macpckeys_backend *)backend, key);
}
const ihk_polls89_key_api *ihk_polls89_api_macpckeys(void)
{
    static const ihk_polls89_key_api api = {
        ihk_polls89_mac_update,
        ihk_polls89_mac_down,
        (ihk_u32)IHK_CAP_GLOBAL_CAPTURE
    };
    return &api;
}

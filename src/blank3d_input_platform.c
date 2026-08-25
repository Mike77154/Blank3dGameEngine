#include "blank3d_input_platform.h"

#include <string.h>

#define B3D_INPUT_PROVIDER_NONE 0
#define B3D_INPUT_PROVIDER_POLLS_WIN32 1
#define B3D_INPUT_PROVIDER_POLLS_LINUX 2
#define B3D_INPUT_PROVIDER_POLLS_MACOS 3

#if defined(_WIN32)
#include "input_hook_backend_polls89_winpckeys.h"
#elif defined(__linux__)
#include "input_hook_backend_polls89_linuxpckeys.h"
#elif defined(__APPLE__)
#include "input_hook_backend_polls89_macpckeys.h"
#endif

int blank3d_input_platform_attach_default(Blank3DInputPlatform *platform,
                                          Blank3DInput *input)
{
    const ihk_backend *backend;
    if (!platform || !input) return 0;
    memset(platform, 0, sizeof(*platform));

#if defined(_WIN32)
    winpckeys_backend_init(&platform->provider);
    if (ihk_polls89_backend_init(&platform->polls_adapter,
                                 &platform->provider,
                                 ihk_polls89_api_winpckeys()) != 0) {
        winpckeys_backend_shutdown(&platform->provider);
        return 0;
    }
    platform->provider_kind = B3D_INPUT_PROVIDER_POLLS_WIN32;
#elif defined(__linux__)
    linuxpckeys_backend_init(&platform->provider);
    if (ihk_polls89_backend_init(&platform->polls_adapter,
                                 &platform->provider,
                                 ihk_polls89_api_linuxpckeys()) != 0) {
        linuxpckeys_backend_shutdown(&platform->provider);
        return 0;
    }
    platform->provider_kind = B3D_INPUT_PROVIDER_POLLS_LINUX;
#elif defined(__APPLE__)
    macpckeys_backend_init(&platform->provider);
    if (ihk_polls89_backend_init(&platform->polls_adapter,
                                 &platform->provider,
                                 ihk_polls89_api_macpckeys()) != 0) {
        macpckeys_backend_shutdown(&platform->provider);
        return 0;
    }
    platform->provider_kind = B3D_INPUT_PROVIDER_POLLS_MACOS;
#else
    platform->provider_kind = B3D_INPUT_PROVIDER_NONE;
    return 0;
#endif

    backend = ihk_polls89_backend_as_ihk(&platform->polls_adapter);
    if (!backend) {
#if defined(_WIN32)
        winpckeys_backend_shutdown(&platform->provider);
#elif defined(__linux__)
        linuxpckeys_backend_shutdown(&platform->provider);
#elif defined(__APPLE__)
        macpckeys_backend_shutdown(&platform->provider);
#endif
        ihk_polls89_backend_shutdown(&platform->polls_adapter);
        platform->provider_kind = B3D_INPUT_PROVIDER_NONE;
        return 0;
    }

    blank3d_input_set_backend(input, backend);
    platform->initialized = 1;
    return 1;
}

void blank3d_input_platform_shutdown(Blank3DInputPlatform *platform,
                                     Blank3DInput *input)
{
    if (!platform) return;

    /* Detach first: Blank3D/input_hook must never retain a pointer into a
     * provider whose OS resources are about to be released. */
    if (input) blank3d_input_set_backend(input, (const ihk_backend *)0);

    if (platform->initialized) {
        ihk_polls89_backend_shutdown(&platform->polls_adapter);
#if defined(_WIN32)
        if (platform->provider_kind == B3D_INPUT_PROVIDER_POLLS_WIN32)
            winpckeys_backend_shutdown(&platform->provider);
#elif defined(__linux__)
        if (platform->provider_kind == B3D_INPUT_PROVIDER_POLLS_LINUX)
            linuxpckeys_backend_shutdown(&platform->provider);
#elif defined(__APPLE__)
        if (platform->provider_kind == B3D_INPUT_PROVIDER_POLLS_MACOS)
            macpckeys_backend_shutdown(&platform->provider);
#endif
    }
    memset(platform, 0, sizeof(*platform));
}

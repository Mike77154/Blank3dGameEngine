#ifndef BLANK3D_INPUT_PLATFORM_H
#define BLANK3D_INPUT_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "blank3d_input.h"
#include "input_hook_backend_polls89.h"

/*
 * Platform ownership lives here, never in blank3d_input.h.
 * The core sees only ihk_backend.  This thin layer owns whichever polls89
 * provider exists for the target OS and adapts it into input_hook89.
 */
#if defined(_WIN32)
# if defined(_WIN64)
#  include "by_system_backend/win64/winpckeys_backend.h"
# else
#  include "by_system_backend/win32/winpckeys_backend.h"
# endif
typedef winpckeys_backend Blank3DInputNativeProvider;
#define B3D_INPUT_PLATFORM_HAS_DEFAULT 1
#elif defined(__linux__)
# if defined(__x86_64__) || defined(__aarch64__) || defined(__LP64__)
#  include "by_system_backend/linux64/linuxpckeys_backend.h"
# else
#  include "by_system_backend/linux32/linuxpckeys_backend.h"
# endif
typedef linuxpckeys_backend Blank3DInputNativeProvider;
#define B3D_INPUT_PLATFORM_HAS_DEFAULT 1
#elif defined(__APPLE__)
# if defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__)
#  include "by_system_backend/mac64/macpckeys_backend.h"
# else
#  include "by_system_backend/mac32/macpckeys_backend.h"
# endif
typedef macpckeys_backend Blank3DInputNativeProvider;
#define B3D_INPUT_PLATFORM_HAS_DEFAULT 1
#else
typedef struct Blank3DInputNativeProviderTag {
    int unused;
} Blank3DInputNativeProvider;
#define B3D_INPUT_PLATFORM_HAS_DEFAULT 0
#endif

typedef struct Blank3DInputPlatformTag {
    ihk_polls89_backend polls_adapter;
    Blank3DInputNativeProvider provider;
    int provider_kind;
    int initialized;
} Blank3DInputPlatform;

/* Select the built-in polls89 provider for the target OS and attach it to
 * Blank3D's platform-neutral input core.  Returns 0 on unsupported targets;
 * callers may still inject any ihk_backend with blank3d_input_set_backend(). */
int blank3d_input_platform_attach_default(Blank3DInputPlatform *platform,
                                          Blank3DInput *input);
void blank3d_input_platform_shutdown(Blank3DInputPlatform *platform,
                                     Blank3DInput *input);

#ifdef __cplusplus
}
#endif

#endif

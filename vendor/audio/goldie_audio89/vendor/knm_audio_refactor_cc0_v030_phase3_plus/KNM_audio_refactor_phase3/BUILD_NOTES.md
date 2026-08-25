# KNM audio build notes

The tree defaults all optional backends to **disabled** so the sources can be dropped into a project without forcing platform SDK headers to exist on every build machine.

## CMake options

- `KNM_AUDIO_BUILD_TESTS=ON|OFF`
- `KNM_AUDIO_BUILD_EXAMPLES=ON|OFF`
- `KNM_AUDIO_STRICT_WARNINGS=ON|OFF`
- `KNM_AUDIO_ENABLE_<BACKEND>=ON|OFF` for each optional backend

Example:

```sh
cmake -S . -B build \
  -DKNM_AUDIO_BUILD_TESTS=ON \
  -DKNM_AUDIO_BUILD_EXAMPLES=ON \
  -DKNM_AUDIO_ENABLE_ALSA=ON
cmake --build build
ctest --test-dir build
```

## Manual source-only integration

If you are not using CMake, always compile:

- `KNM_audio/knm_hwr_audio.c`
- `KNM_audio/mnk_core.c`
- `knm_backends/mka_audio_null.c`

Then add only the backend source files that match your enabled `KNM_AUDIO_ENABLE_<BACKEND>` macros.

## Notes

- Internal sample format is signed Q16.16 fixed point everywhere in the shared core.
- Public `knm_fix16` is forced to a stable 32-bit ABI instead of following the host `long` size.
- The capture ring is synchronized internally and only provisioned when input is enabled.
- `knm_audio_service()` is intended for deterministic null-backend stepping and tests.
- The default backend picker prefers the most native backend available for the target platform and falls back to `null`.

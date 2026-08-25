# KNM_audio

CC0-1.0 licensed, C89-oriented hardware audio library with a fixed-point Q16.16 shared core.

## Current profile

- C89 public/core code
- no heap allocation in the shared core
- fixed-point shared audio path
- static device slots and static capture ring
- backend registry with compile-time selection
- installable CMake target and pkg-config metadata
- deterministic null backend for tests/tools

## Public API guarantees

- `knm_fix16` is a stable signed 32-bit Q16.16 type
- public versioning follows Semantic Versioning expectations for the declared API
- callback execution is backend-owned; callbacks must not block, allocate, or call control APIs
- capture overflow is observable through `knm_audio_capture_overruns()`, `knm_audio_status_flags()`, and `knm_audio_last_error()`
- an empty `cfg.device_id` means "use the backend default device"

## Key runtime queries

- backend capabilities: `knm_audio_backend_info()` / `knm_audio_device_info()`
- explicit search: `knm_audio_find_device()`
- default backend policy: `knm_audio_default_backend_order()`
- requested vs negotiated config: `knm_audio_get_requested_config()` / `knm_audio_get_config()`
- actual opened endpoint metadata: `knm_audio_get_opened_device_info()`
- stream stats: `knm_audio_get_stats()` / `knm_audio_reset_stats()`
- manual null servicing: `knm_audio_service()`

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Optional backends stay disabled until you enable the corresponding CMake option or define `KNM_AUDIO_ENABLE_<BACKEND>=1` in your own build.

## Default backend preference

- Windows: WASAPI -> WinMM -> NULL
- Android: AAudio -> OpenSL ES -> NULL
- macOS: CoreAudio -> NULL
- Web: WebAudio -> NULL
- Haiku: Haiku -> NULL
- BSD: sndio -> JACK -> NULL
- Linux: PipeWire -> PulseAudio -> ALSA -> JACK -> NULL

# Changelog

## 0.3.0

- added `cfg.device_id` so callers can request a backend-specific device without changing the fixed-size public config model
- added `knm_audio_find_device()` and `knm_audio_default_backend_order()` for discovery and default-policy introspection
- added `knm_audio_get_opened_device_info()` so callers can query the actual device metadata attached to an open handle
- added stream observability through `knm_stream_stats`, `knm_audio_get_stats()`, and `knm_audio_reset_stats()`
- added `knm_audio_clear_status()` for explicit status-flag lifecycle control
- added `knm_audio_service()` for deterministic offline/manual servicing of the null backend and tests
- improved the capture-ring lock with periodic yield backoff instead of pure busy-spin
- promoted input-overflow and backend-xrun accounting into reusable internal helpers
- upgraded the null backend to validate `device_id` and expose a stable reference target (`null.default`)
- refreshed examples/tests and moved the build to configurable CMake options for tests/examples/backends

## 0.2.0

- added explicit version macros and runtime version queries
- documented callback/threading contract in the public header
- added backend compiled/available capability queries
- added device config, latency, last-error, and status-flag queries
- removed float/double usage from the shared core and fixed-point helpers
- improved fixed-point ratio and multiply helpers to use integer math only
- promoted capture overflow to a persistent status flag and last error
- added installable CMake build, pkg-config template, docs, examples, and tests

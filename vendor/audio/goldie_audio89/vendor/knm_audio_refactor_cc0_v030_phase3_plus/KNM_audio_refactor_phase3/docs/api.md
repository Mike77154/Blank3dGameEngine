# API notes

## Runtime queries

- `knm_audio_version()` / `knm_audio_version_string()`
- `knm_audio_backend_compiled()`
- `knm_audio_backend_available()`
- `knm_audio_backend_info()`
- `knm_audio_device_info()`
- `knm_audio_find_device()`
- `knm_audio_default_backend_order()`
- `knm_audio_get_requested_config()`
- `knm_audio_get_config()`
- `knm_audio_get_opened_device_info()`
- `knm_audio_output_latency_frames()`
- `knm_audio_input_latency_frames()`
- `knm_audio_last_error()`
- `knm_audio_status_flags()`
- `knm_audio_clear_status()`
- `knm_audio_get_stats()` / `knm_audio_reset_stats()`
- `knm_audio_service()` (null backend / deterministic tests)

## Fixed-point rules

- sample type: signed Q16.16
- full-scale positive clamp: `+1.0 == 65536`
- full-scale negative clamp: `-1.0 == -65536`
- shared core does integer math only

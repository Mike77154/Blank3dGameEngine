# Callback contract

The callback receives fixed-point Q16.16 buffers.

Rules:
- Treat input as read-only.
- Always fill the output buffer when output is enabled.
- Do not block or allocate.
- Do not assume the callback is invoked from the thread that opened the device.
- Do not call start/stop/close/control APIs from the callback.

Notes:
- Input-only devices receive callbacks through the capture path.
- Duplex/output devices receive input via the same render callback.
- The null backend can be stepped deterministically with `knm_audio_service()` for tests.

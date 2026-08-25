# Platform notes

This project is set up so native I/O modules can be added without changing the fixed-point mixer core.

## Windows

Use:

- MMDevice API for endpoint enumeration
- WASAPI for stream creation and buffer servicing

Suggested first target:

- shared-mode render
- shared-mode capture
- event-driven buffering
- callback/thread shim that feeds `rm_engine_process_duplex_s16()`

## Linux

Use:

- ALSA PCM for native low-level streams

Suggested first target:

- blocking or poll-driven duplex pump
- explicit hw/sw parameter setup
- fixed period size
- interleaved `S16_LE`

PipeWire can be added later as an optional higher-level Linux backend.

## macOS

Use:

- Core Audio HAL for device discovery
- AUHAL / Audio Units or equivalent low-level I/O path for duplex streaming

Suggested first target:

- default device open
- fixed buffer callback
- signed 16-bit conversion at the boundary if native format differs

## Boundary rule

All platform code should do format negotiation at the edge and keep the core on its simple internal contract:

- input to core: interleaved S16 PCM
- output from core: interleaved S16 PCM
- timing: frames-per-period supplied by backend

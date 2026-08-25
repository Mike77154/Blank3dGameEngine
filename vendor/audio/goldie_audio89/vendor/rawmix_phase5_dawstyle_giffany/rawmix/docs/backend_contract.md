# Backend contract for future native I/O modules

The current zip ships the **engine** and **host-driven duplex path**. The native device layer should stay separate.

## Why the split exists

A portable mixer core and a native device layer solve different problems:

- core: voices, mixing, resampling, routing, capture ring, deterministic memory use
- backend: enumerate devices, open streams, negotiate sample rate/channels, drive the callback or pump loop

## Recommended backend surface

```c
struct rm_device_info {
    char id[96];
    char name[128];
    unsigned int can_input;
    unsigned int can_output;
    unsigned int preferred_rate;
    unsigned short max_input_channels;
    unsigned short max_output_channels;
};

struct rm_backend_stream_desc {
    unsigned int sample_rate;
    unsigned short input_channels;
    unsigned short output_channels;
    unsigned int period_frames;
    void *user;
};

typedef int (*rm_backend_duplex_cb)(const short *in,
                                    unsigned short in_channels,
                                    short *out,
                                    unsigned short out_channels,
                                    unsigned int frames,
                                    void *user);
```

## Two integration styles

### Style A: backend callback directly into rawmix

- backend obtains input/output buffers from the OS
- backend calls `rm_engine_process_duplex_s16()`
- backend submits output buffer back to OS

### Style B: backend pushes/pulls through host-owned ring buffers

- backend copies input into host ring
- backend thread calls rawmix
- backend outputs from host ring

Style A is the cleanest first target.

## Backend responsibilities

- device enumeration
- default device selection
- stream open/close/start/stop
- rate/channel negotiation
- device-lost handling
- timing / period sizing
- hotplug notifications later

## Core responsibilities

- no backend-specific conditionals in the mixer fast path
- no heap
- no platform calls inside the render path
- no hidden worker thread requirement

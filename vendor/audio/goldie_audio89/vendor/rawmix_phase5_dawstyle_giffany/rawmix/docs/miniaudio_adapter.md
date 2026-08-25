# Optional miniaudio adapter

`rawmix` keeps native device I/O out of the mixer core.
This document covers the optional bridge in `extras/rawmix_miniaudio_adapter.*`.

## What it does

The adapter wraps a `ma_device` and forwards its callback into a `rm_engine`:

- playback device -> rawmix output
- duplex device -> rawmix duplex process path

## What it does not do

- it does not change the rawmix core API
- it does not add heap use to the core
- it does not add decoding/resource-manager features
- it does not build by default

## Build

```sh
make miniaudio_adapter MINIAUDIO_INCLUDE_DIR=/path/to/miniaudio
```

That target only builds the adapter object. It assumes `miniaudio.h` is available in the supplied include directory.

## Integration sketch

```c
#include "miniaudio.h"
#include "rawmix.h"
#include "rawmix_miniaudio_adapter.h"

rm_engine engine;
rm_ma_device adapter;
rm_ma_device_config cfg;

rm_engine_config engine_cfg;
rm_engine_config_init(&engine_cfg);
engine_cfg.sample_rate = 48000U;
engine_cfg.channels = 2U;
rm_engine_init(&engine, &engine_cfg);

rm_ma_device_config_init(&cfg);
cfg.device_type = ma_device_type_playback;
cfg.sample_rate = 48000;
cfg.playback_channels = 2;

if (rm_ma_device_init(&adapter, &engine, &cfg) == MA_SUCCESS) {
    rm_ma_device_start(&adapter);
    /* main loop */
    rm_ma_device_stop(&adapter);
    rm_ma_device_uninit(&adapter);
}
```

## Notes

- the bridge uses `ma_format_s16`
- duplex mode expects the same sample rate on capture and playback, matching rawmix's host-driven expectations
- if you want device enumeration or device selection UX, use miniaudio's normal context/device APIs before filling the adapter config

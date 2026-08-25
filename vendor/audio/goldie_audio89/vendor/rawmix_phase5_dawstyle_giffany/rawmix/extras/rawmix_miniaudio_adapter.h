#ifndef RAWMIX_MINIAUDIO_ADAPTER_H
#define RAWMIX_MINIAUDIO_ADAPTER_H

/*
    Optional miniaudio bridge for rawmix.

    This file lives outside the rawmix core on purpose. The mixer core keeps its
    C89/no-heap/fixed-point constraints, while this adapter lets an app plug the
    core into miniaudio's device callback path when a native backend is needed.

    Requirements:
    - include path must contain miniaudio.h
    - compile this file as part of the application or as an extra object
*/

#include "miniaudio.h"
#include "rawmix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rm_ma_device_config_s {
    ma_device_type device_type;
    const ma_device_id *playback_device_id;
    const ma_device_id *capture_device_id;
    ma_uint32 sample_rate;
    ma_uint32 playback_channels;
    ma_uint32 capture_channels;
    ma_uint32 period_size_in_frames;
} rm_ma_device_config;

typedef struct rm_ma_device_s {
    ma_device device;
    rm_engine *engine;
    ma_uint32 playback_channels;
    ma_uint32 capture_channels;
} rm_ma_device;

void rm_ma_device_config_init(rm_ma_device_config *cfg);
ma_result rm_ma_device_init(rm_ma_device *adapter,
                            rm_engine *engine,
                            const rm_ma_device_config *cfg);
ma_result rm_ma_device_start(rm_ma_device *adapter);
ma_result rm_ma_device_stop(rm_ma_device *adapter);
void rm_ma_device_uninit(rm_ma_device *adapter);

#ifdef __cplusplus
}
#endif

#endif

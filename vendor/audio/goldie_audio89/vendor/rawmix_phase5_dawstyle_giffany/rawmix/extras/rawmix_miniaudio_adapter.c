#include "rawmix_miniaudio_adapter.h"

#include <string.h>

static void rm_ma_data_callback(ma_device *pDevice,
                                void *pOutput,
                                const void *pInput,
                                ma_uint32 frameCount)
{
    rm_ma_device *adapter;

    adapter = (rm_ma_device *)pDevice->pUserData;
    if (adapter == NULL || adapter->engine == NULL || pOutput == NULL) {
        return;
    }

    if (pInput != NULL && adapter->capture_channels > 0U) {
        (void)rm_engine_process_duplex_s16(adapter->engine,
                                           (const rm_s16 *)pInput,
                                           (rm_u16)adapter->capture_channels,
                                           (rm_s16 *)pOutput,
                                           (rm_u16)adapter->playback_channels,
                                           (rm_u32)frameCount);
    } else {
        (void)rm_engine_process_duplex_s16(adapter->engine,
                                           NULL,
                                           0U,
                                           (rm_s16 *)pOutput,
                                           (rm_u16)adapter->playback_channels,
                                           (rm_u32)frameCount);
    }
}

void rm_ma_device_config_init(rm_ma_device_config *cfg)
{
    if (cfg == NULL) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->device_type = ma_device_type_playback;
    cfg->sample_rate = 0U;
    cfg->playback_channels = 0U;
    cfg->capture_channels = 0U;
    cfg->period_size_in_frames = 0U;
}

ma_result rm_ma_device_init(rm_ma_device *adapter,
                            rm_engine *engine,
                            const rm_ma_device_config *cfg)
{
    rm_ma_device_config local_cfg;
    ma_device_config device_cfg;

    if (adapter == NULL || engine == NULL) {
        return MA_INVALID_ARGS;
    }

    if (cfg == NULL) {
        rm_ma_device_config_init(&local_cfg);
        cfg = &local_cfg;
    }

    if (cfg->device_type != ma_device_type_playback &&
        cfg->device_type != ma_device_type_duplex) {
        return MA_NOT_IMPLEMENTED;
    }

    memset(adapter, 0, sizeof(*adapter));
    adapter->engine = engine;
    adapter->playback_channels = cfg->playback_channels != 0U ?
                                 cfg->playback_channels : (ma_uint32)engine->channels;
    adapter->capture_channels = cfg->capture_channels;

    if (cfg->device_type == ma_device_type_duplex && adapter->capture_channels == 0U) {
        adapter->capture_channels = 1U;
    }

    device_cfg = ma_device_config_init(cfg->device_type);
    device_cfg.sampleRate = cfg->sample_rate != 0U ? cfg->sample_rate : (ma_uint32)engine->sample_rate;
    device_cfg.dataCallback = rm_ma_data_callback;
    device_cfg.pUserData = adapter;
    if (cfg->period_size_in_frames != 0U) {
        device_cfg.periodSizeInFrames = cfg->period_size_in_frames;
    }

    device_cfg.playback.format = ma_format_s16;
    device_cfg.playback.channels = adapter->playback_channels;
    device_cfg.playback.pDeviceID = (ma_device_id *)cfg->playback_device_id;

    if (cfg->device_type == ma_device_type_duplex) {
        device_cfg.capture.format = ma_format_s16;
        device_cfg.capture.channels = adapter->capture_channels;
        device_cfg.capture.pDeviceID = (ma_device_id *)cfg->capture_device_id;
    }

    return ma_device_init(NULL, &device_cfg, &adapter->device);
}

ma_result rm_ma_device_start(rm_ma_device *adapter)
{
    if (adapter == NULL) {
        return MA_INVALID_ARGS;
    }
    return ma_device_start(&adapter->device);
}

ma_result rm_ma_device_stop(rm_ma_device *adapter)
{
    if (adapter == NULL) {
        return MA_INVALID_ARGS;
    }
    return ma_device_stop(&adapter->device);
}

void rm_ma_device_uninit(rm_ma_device *adapter)
{
    if (adapter == NULL) {
        return;
    }
    ma_device_uninit(&adapter->device);
    memset(adapter, 0, sizeof(*adapter));
}

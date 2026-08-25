#include "../KNM_audio/mnk_core_internal.h"

static int mka_null_probe(void)
{
    return 1;
}

static unsigned int mka_null_device_count(void)
{
    return 1U;
}

static int mka_null_device_info(unsigned int index, knm_device_info *out_info)
{
    if (out_info == (knm_device_info *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (index != 0U) {
        return KNM_LIMIT_EXCEEDED;
    }

    mnk_zero((void *)out_info, (unsigned long)sizeof(*out_info));
    out_info->backend = KNM_BACKEND_NULL;
    out_info->device_index = 0U;
    mnk_copy_text(out_info->device_id, (unsigned int)sizeof(out_info->device_id), "null.default");
    mnk_copy_text(out_info->name, (unsigned int)sizeof(out_info->name), "Null Device");
    out_info->is_default = 1;
    out_info->supports_input = 1;
    out_info->supports_output = 1;
    out_info->supports_duplex = 1;
    out_info->max_input_channels = KNM_MAX_CHANNELS;
    out_info->max_output_channels = KNM_MAX_CHANNELS;
    out_info->default_sample_rate = 48000UL;
    return KNM_OK;
}

static int mka_null_device_id_matches(const char *device_id)
{
    if (device_id == (const char *)0 || device_id[0] == '\0') {
        return 1;
    }
    if (strcmp(device_id, "default") == 0) {
        return 1;
    }
    if (strcmp(device_id, "null.default") == 0) {
        return 1;
    }
    return 0;
}

static int mka_null_open(knm_device *dev)
{
    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    if (!mka_null_device_id_matches(dev->requested_cfg.device_id)) {
        return KNM_DEVICE_NOT_FOUND;
    }
    dev->cfg.backend = KNM_BACKEND_NULL;
    return KNM_OK;
}

static int mka_null_start(knm_device *dev)
{
    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    dev->running = 1;
    return KNM_OK;
}

static int mka_null_stop(knm_device *dev)
{
    if (dev == (knm_device *)0) {
        return KNM_BAD_ARGUMENT;
    }
    dev->running = 0;
    return KNM_OK;
}

static void mka_null_close(knm_device *dev)
{
    (void)dev;
}

const knm_backend_vtbl knm_backend_null_vtbl = {
    KNM_BACKEND_NULL,
    "null",
    1,
    1,
    1,
    1,
    mka_null_probe,
    mka_null_device_count,
    mka_null_device_info,
    mka_null_open,
    mka_null_start,
    mka_null_stop,
    mka_null_close
};

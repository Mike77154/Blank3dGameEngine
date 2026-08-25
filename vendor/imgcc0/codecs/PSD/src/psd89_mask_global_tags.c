#include "psd89/psd89.h"

void psd89_mask_global_bool_write(psd89_u8 raw[4], psd89_u8 value)
{
    if (raw == 0) {
        return;
    }
    raw[0] = value ? 1U : 0U;
    raw[1] = 0U;
    raw[2] = 0U;
    raw[3] = 0U;
}

int psd89_mask_global_bool_parse(const psd89_u8 raw[4], psd89_u8 *value)
{
    if (raw == 0 || value == 0) {
        return PSD89_E_BAD_ARGUMENT;
    }
    *value = raw[0] ? 1U : 0U;
    return PSD89_OK;
}

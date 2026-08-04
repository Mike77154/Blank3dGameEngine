#include "shotpumpkin89.h"

#include <stdio.h>

static unsigned int render_checksum(sp89_preset preset,
                                    unsigned short eq6_enabled,
                                    unsigned int *nonzero_out)
{
    sp89_config cfg;
    sp89_state state;
    unsigned int i;
    unsigned int nonzero;
    unsigned int checksum;
    signed short sample;

    sp89_config_preset(&cfg, 44100U, preset);
    cfg.eq6_enabled = eq6_enabled;
    sp89_init(&state, &cfg);
    sp89_trigger_cycle(&state);

    nonzero = 0U;
    checksum = 2166136261U;
    for (i = 0U; i < 44100U; ++i) {
        sample = sp89_process(&state);
        if (sample != 0) {
            nonzero++;
        }
        checksum ^= (unsigned int)(unsigned short)sample;
        checksum *= 16777619U;
    }

    if (nonzero_out != 0) {
        *nonzero_out = nonzero;
    }
    if (sp89_is_active(&state)) {
        return 0U;
    }
    return checksum;
}

int main(void)
{
    unsigned int i;
    unsigned int nonzero;
    unsigned int checksum_a;
    unsigned int checksum_b;
    unsigned int legacy_checksum;

    for (i = 0U; i < (unsigned int)SP89_PRESET_COUNT; ++i) {
        checksum_a = render_checksum((sp89_preset)i, 1U, &nonzero);
        checksum_b = render_checksum((sp89_preset)i, 1U, 0);
        if (checksum_a == 0U || nonzero == 0U) {
            fprintf(stderr, "FAIL: preset %u stayed silent or active\n", i);
            return 1;
        }
        if (checksum_a != checksum_b) {
            fprintf(stderr, "FAIL: preset %u was not deterministic\n", i);
            return 1;
        }
    }

    legacy_checksum = render_checksum(SP89_PRESET_REALISTIC, 0U, &nonzero);
    if (legacy_checksum == 0U || nonzero == 0U) {
        fprintf(stderr, "FAIL: legacy 3-band fallback failed\n");
        return 1;
    }

    printf("PASS: %u presets, EQ6 and legacy EQ; state=%u bytes\n",
           (unsigned int)SP89_PRESET_COUNT, sp89_state_bytes());
    return 0;
}

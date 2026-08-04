#include "ecg_profiles.h"
#include "ecg_surface.h"

static const ECG_I8 ecg_signal_fine[ECG_SIGNAL_COLS] = {
    15,15,15,15,15,15,15,15,15,15,
    15,15,14,13,12,13,15,16,15,14,
    15,18,20,12, 4, 7,11,16,21,25,
    20,17,14,12,13,14,15,15,15,14,
    13,12,13,14,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15
};

static const ECG_I8 ecg_signal_caution[ECG_SIGNAL_COLS] = {
    15,15,15,15,15,15,15,15,15,15,
    15,14,13,12,13,15,16,15,14,15,
    18,20,13, 6, 8,12,16,20,22,20,
    17,15,14,14,13,13,14,15,15,15,
    14,14,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15
};

static const ECG_I8 ecg_signal_orange[ECG_SIGNAL_COLS] = {
    15,15,15,15,15,15,15,15,15,15,
    16,16,17,17,17,16,15,14,14,14,
    15,15,14,11,10, 9,10,13,16,19,
    20,19,18,16,14,13,12,12,13,14,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15
};

static const ECG_I8 ecg_signal_danger[ECG_SIGNAL_COLS] = {
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,14,14,13,
    13,14,15,15,16,17,17,17,14,10,
     9,10,12,15,16,16,16,15,14,13,
    13,14,14,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15,
    15,15,15,15,15,15,15,15,15,15
};

static const ECG_I8 ecg_signal_poison[ECG_SIGNAL_COLS] = {
    15,15,15,15,15,15,15,15,15,15,
    15,14,13,12,12,13,15,18,20,16,
     8, 5, 4, 5, 8,15,19,24,26,25,
    15,14,15,16,18,17,10, 9,10,12,
    16,17,18,18,17,16,15,15,15,12,
    10, 9,10,16,19,19,17,15,15,15,
    15,15,15,14,15,16,15,14,15,16,
    15,15,15,15,15,15,15,15,15,15
};

ECG_Status ecg_profile_from_signal(ECG_Profile *profile,
                                   const char *name,
                                   ECG_Color color,
                                   ECG_Color gradient,
                                   const ECG_I8 signal[ECG_SIGNAL_COLS])
{
    unsigned int i;
    int prev;
    int curr;
    int min_y;
    int diff;

    if (profile == (ECG_Profile *)0 || signal == (const ECG_I8 *)0) {
        return ECG_STATUS_NULL;
    }

    profile->name = name;
    profile->color = color;
    profile->gradient = gradient;

    for (i = 0u; i < ECG_SIGNAL_COLS; i++) {
        if (i == 0u) {
            prev = (int)signal[i];
        } else {
            prev = (int)signal[i - 1u];
        }

        curr = (int)signal[i];
        profile->samples[i] = signal[i];
        min_y = prev < curr ? prev : curr;
        diff = prev > curr ? prev - curr : curr - prev;

        profile->lines[i].y = (ECG_I8)min_y;
        profile->lines[i].h = (ECG_I8)diff;
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_profiles_init(ECG_Profile profiles[ECG_PROFILE_COUNT])
{
    if (profiles == (ECG_Profile *)0) {
        return ECG_STATUS_NULL;
    }

    (void)ecg_profile_from_signal(&profiles[ECG_PROFILE_FINE],
                                  "FINE",
                                  ecg_color_make(32u, 255u, 32u),
                                  ecg_color_make(1u, 8u, 1u),
                                  ecg_signal_fine);
    (void)ecg_profile_from_signal(&profiles[ECG_PROFILE_CAUTION],
                                  "CAUTION",
                                  ecg_color_make(255u, 255u, 32u),
                                  ecg_color_make(8u, 8u, 1u),
                                  ecg_signal_caution);
    (void)ecg_profile_from_signal(&profiles[ECG_PROFILE_ORANGE],
                                  "ORANGE",
                                  ecg_color_make(255u, 128u, 32u),
                                  ecg_color_make(8u, 4u, 1u),
                                  ecg_signal_orange);
    (void)ecg_profile_from_signal(&profiles[ECG_PROFILE_DANGER],
                                  "DANGER",
                                  ecg_color_make(255u, 32u, 32u),
                                  ecg_color_make(8u, 1u, 1u),
                                  ecg_signal_danger);
    (void)ecg_profile_from_signal(&profiles[ECG_PROFILE_POISON],
                                  "POISON",
                                  ecg_color_make(255u, 32u, 255u),
                                  ecg_color_make(8u, 1u, 8u),
                                  ecg_signal_poison);

    return ECG_STATUS_OK;
}

const char *ecg_profile_name(unsigned int profile_id)
{
    static const char *names[ECG_PROFILE_COUNT] = {
        "FINE",
        "CAUTION",
        "ORANGE",
        "DANGER",
        "POISON"
    };

    if (profile_id >= ECG_PROFILE_COUNT) {
        return "UNKNOWN";
    }

    return names[profile_id];
}

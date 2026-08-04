#ifndef ECG_PROFILES_H
#define ECG_PROFILES_H

#include "ecg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ECG_Status ecg_profile_from_signal(ECG_Profile *profile,
                                   const char *name,
                                   ECG_Color color,
                                   ECG_Color gradient,
                                   const ECG_I8 signal[ECG_SIGNAL_COLS]);
ECG_Status ecg_profiles_init(ECG_Profile profiles[ECG_PROFILE_COUNT]);
const char *ecg_profile_name(unsigned int profile_id);

#ifdef __cplusplus
}
#endif

#endif

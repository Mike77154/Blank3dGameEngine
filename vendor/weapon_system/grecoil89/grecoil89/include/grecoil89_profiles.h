#ifndef GRECOIL89_PROFILES_H
#define GRECOIL89_PROFILES_H

#include "grecoil89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GREC_PROFILE_PISTOL_9MM     0
#define GREC_PROFILE_MAGNUM_44      1
#define GREC_PROFILE_SMG_9MM        2
#define GREC_PROFILE_RIFLE_556      3
#define GREC_PROFILE_SHOTGUN_12G    4
#define GREC_PROFILE_SNIPER_762     5
#define GREC_PROFILE_LAUNCHER_LIGHT 6
#define GREC_PROFILE_TURRET_MEDIUM  7
#define GREC_PROFILE_COUNT          8

extern const GRecPatternStep grec_pattern_smg_9mm[];
extern const GRecPatternStep grec_pattern_rifle_556[];
extern const GRecPatternStep grec_pattern_turret_medium[];
extern const GRecProfile grec_profiles[GREC_PROFILE_COUNT];

const GRecProfile *grec_get_profile(int profile_id);

#ifdef __cplusplus
}
#endif

#endif

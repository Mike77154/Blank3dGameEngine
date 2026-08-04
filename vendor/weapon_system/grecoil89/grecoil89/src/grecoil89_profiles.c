#include "grecoil89_profiles.h"

#define FP(n) GREC_FP_FROM_INT(n)
#define FPR(n,d) GREC_FP_FROM_RATIO(n,d)

const GRecPatternStep grec_pattern_smg_9mm[] = {
    { FPR(10,100), FPR( 1,100), 0, 0, FPR( 2,100), FPR( 8,100), FPR(2,100) },
    { FPR(11,100), FPR(-1,100), 0, 0, FPR( 2,100), FPR( 8,100), FPR(2,100) },
    { FPR(12,100), FPR( 2,100), 0, 0, FPR( 3,100), FPR( 9,100), FPR(3,100) },
    { FPR(13,100), FPR(-2,100), 0, 0, FPR( 3,100), FPR( 9,100), FPR(3,100) },
    { FPR(14,100), FPR( 3,100), 0, 0, FPR( 3,100), FPR(10,100), FPR(3,100) },
    { FPR(15,100), FPR(-3,100), 0, 0, FPR( 4,100), FPR(10,100), FPR(4,100) },
    { FPR(15,100), FPR( 4,100), 0, 0, FPR( 4,100), FPR(10,100), FPR(4,100) },
    { FPR(14,100), FPR(-4,100), 0, 0, FPR( 4,100), FPR(10,100), FPR(4,100) }
};

const GRecPatternStep grec_pattern_rifle_556[] = {
    { FPR(18,100), FPR( 0,100), 0, 0, FPR(3,100), FPR(10,100), FPR(2,100) },
    { FPR(22,100), FPR( 1,100), 0, 0, FPR(3,100), FPR(11,100), FPR(3,100) },
    { FPR(24,100), FPR(-1,100), 0, 0, FPR(4,100), FPR(11,100), FPR(3,100) },
    { FPR(27,100), FPR( 3,100), 0, 0, FPR(4,100), FPR(12,100), FPR(4,100) },
    { FPR(28,100), FPR(-3,100), 0, 0, FPR(4,100), FPR(12,100), FPR(4,100) },
    { FPR(25,100), FPR( 4,100), 0, 0, FPR(5,100), FPR(13,100), FPR(5,100) },
    { FPR(22,100), FPR(-4,100), 0, 0, FPR(5,100), FPR(13,100), FPR(5,100) },
    { FPR(20,100), FPR( 2,100), 0, 0, FPR(5,100), FPR(13,100), FPR(5,100) },
    { FPR(18,100), FPR(-2,100), 0, 0, FPR(5,100), FPR(13,100), FPR(5,100) }
};

const GRecPatternStep grec_pattern_turret_medium[] = {
    { FPR(10,100), FPR( 2,100), FPR( 1,100), FPR( 1,100), FPR(1,100), FPR(20,100), FPR(3,100) },
    { FPR(11,100), FPR(-2,100), FPR(-1,100), FPR(-1,100), FPR(1,100), FPR(20,100), FPR(3,100) },
    { FPR(12,100), FPR( 1,100), FPR( 1,100), FPR( 0,100), FPR(1,100), FPR(22,100), FPR(4,100) },
    { FPR(12,100), FPR(-1,100), FPR(-1,100), FPR( 0,100), FPR(1,100), FPR(22,100), FPR(4,100) }
};

const GRecProfile grec_profiles[GREC_PROFILE_COUNT] = {
    {
        "pistol_9mm",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_RANDOM_YAW,
        FPR(55,100), FPR(0,100), FPR(4,100), FPR(0,100), FPR(10,100), FPR(28,100),
        FPR(0,100), FPR(7,100),
        FPR(6,100), FPR(22,100), FPR(9,100),
        FPR(10,100), FPR(24,100), FPR(12,100),
        FP(5), FP(2), FP(1), FP(1), FP(1), FP(2),
        FP(1), FPR(85,100), FPR(90,100), FP(1),
        FPR(8,100), FPR(5,100), FPR(55,100), FPR(2,100),
        2, 14, 8,
        0, 0, 0
    },
    {
        "magnum_44",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_RANDOM_YAW | GREC_FLAG_RANDOM_PITCH,
        FPR(150,100), FPR(0,100), FPR(16,100), FPR(0,100), FPR(26,100), FPR(70,100),
        FPR(8,100), FPR(12,100),
        FPR(5,100), FPR(18,100), FPR(6,100),
        FPR(8,100), FPR(20,100), FPR(8,100),
        FP(9), FP(3), FP(2), FP(1), FP(2), FP(4),
        FP(1), FP(1), FPR(120,100), FPR(115,100),
        FPR(15,100), FPR(14,100), FPR(100,100), FPR(2,100),
        4, 24, 6,
        0, 0, 0
    },
    {
        "smg_9mm",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_PATTERN | GREC_FLAG_RANDOM_YAW,
        FPR(18,100), FPR(0,100), FPR(3,100), FPR(0,100), FPR(3,100), FPR(12,100),
        FPR(0,100), FPR(4,100),
        FPR(8,100), FPR(28,100), FPR(18,100),
        FPR(13,100), FPR(30,100), FPR(18,100),
        FP(7), FP(4), FP(1), FP(1), FP(1), FP(2),
        FPR(95,100), FPR(70,100), FPR(85,100), FPR(90,100),
        FPR(12,100), FPR(4,100), FPR(120,100), FPR(3,100),
        1, 10, 24,
        grec_pattern_smg_9mm, 8, 0
    },
    {
        "rifle_556",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_PATTERN | GREC_FLAG_RANDOM_YAW,
        FPR(24,100), FPR(0,100), FPR(5,100), FPR(0,100), FPR(5,100), FPR(20,100),
        FPR(0,100), FPR(5,100),
        FPR(7,100), FPR(25,100), FPR(13,100),
        FPR(12,100), FPR(28,100), FPR(16,100),
        FP(9), FP(5), FP(1), FP(1), FP(1), FP(2),
        FP(1), FPR(80,100), FPR(90,100), FP(1),
        FPR(10,100), FPR(5,100), FPR(140,100), FPR(3,100),
        2, 12, 30,
        grec_pattern_rifle_556, 9, 0
    },
    {
        "shotgun_12g",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_RANDOM_YAW | GREC_FLAG_RANDOM_PITCH,
        FPR(120,100), FPR(0,100), FPR(10,100), FPR(0,100), FPR(20,100), FPR(75,100),
        FPR(8,100), FPR(16,100),
        FPR(5,100), FPR(17,100), FPR(7,100),
        FPR(8,100), FPR(20,100), FPR(9,100),
        FP(8), FP(4), FP(2), FP(1), FP(2), FP(4),
        FPR(90,100), FPR(95,100), FPR(130,100), FPR(120,100),
        FPR(70,100), FPR(35,100), FP(3), FPR(7,100),
        5, 28, 5,
        0, 0, 0
    },
    {
        "sniper_762",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_RANDOM_YAW,
        FPR(190,100), FPR(0,100), FPR(14,100), FPR(0,100), FPR(18,100), FPR(65,100),
        FPR(0,100), FPR(5,100),
        FPR(4,100), FPR(16,100), FPR(4,100),
        FPR(7,100), FPR(18,100), FPR(7,100),
        FP(12), FP(4), FP(2), FP(1), FP(2), FP(4),
        FPR(120,100), FPR(130,100), FPR(95,100), FPR(80,100),
        FPR(4,100), FPR(22,100), FPR(90,100), FPR(4,100),
        7, 36, 4,
        0, 0, 0
    },
    {
        "launcher_light",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM,
        FPR(95,100), FPR(0,100), FPR(20,100), FPR(0,100), FPR(30,100), FPR(95,100),
        FPR(0,100), FPR(0,100),
        FPR(3,100), FPR(14,100), FPR(3,100),
        FPR(5,100), FPR(18,100), FPR(5,100),
        FP(7), FP(3), FP(3), FP(1), FP(3), FP(6),
        FPR(65,100), FPR(95,100), FPR(160,100), FPR(150,100),
        FPR(20,100), FPR(10,100), FPR(85,100), FPR(2,100),
        9, 50, 3,
        0, 0, 0
    },
    {
        "turret_medium",
        GREC_FLAG_AIM_RECOIL | GREC_FLAG_CAMERA_RECOIL | GREC_FLAG_WEAPON_RECOIL |
        GREC_FLAG_SPREAD_BLOOM | GREC_FLAG_PATTERN | GREC_FLAG_RANDOM_YAW,
        FPR(10,100), FPR(0,100), FPR(2,100), FPR(0,100), FPR(1,100), FPR(30,100),
        FPR(0,100), FPR(3,100),
        FPR(9,100), FPR(35,100), FPR(20,100),
        FPR(15,100), FPR(38,100), FPR(22,100),
        FP(5), FP(3), FP(1), FP(1), FP(1), FP(4),
        FPR(80,100), FPR(35,100), FPR(100,100), FPR(120,100),
        FPR(18,100), FPR(6,100), FPR(160,100), FPR(4,100),
        1, 8, 80,
        grec_pattern_turret_medium, 4, 1
    }
};

const GRecProfile *grec_get_profile(int profile_id)
{
    if (profile_id < 0 || profile_id >= GREC_PROFILE_COUNT) return 0;
    return &grec_profiles[profile_id];
}

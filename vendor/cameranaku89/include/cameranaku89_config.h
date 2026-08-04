/*
    cameranaku89_config.h
    CC0 1.0 Universal.
    C89 fixed-point camera solver configuration.
*/
#ifndef CAMERANAKU89_CONFIG_H
#define CAMERANAKU89_CONFIG_H

#ifndef CNK_API
#define CNK_API
#endif

#ifndef CNK_FX_SHIFT
#define CNK_FX_SHIFT 8
#endif

#ifndef CNK_MAX_VCAMS
#define CNK_MAX_VCAMS 8
#endif

#ifndef CNK_MAX_KEYFRAMES
#define CNK_MAX_KEYFRAMES 32
#endif

#ifndef CNK_MAX_SHAKES
#define CNK_MAX_SHAKES 4
#endif

#ifndef CNK_MAX_TARGET_GROUP_ITEMS
#define CNK_MAX_TARGET_GROUP_ITEMS 8
#endif

#ifndef CNK_MAX_MANAGER_VCAMS
#define CNK_MAX_MANAGER_VCAMS CNK_MAX_VCAMS
#endif

#ifndef CNK_MAX_ZONES
#define CNK_MAX_ZONES 16
#endif

#ifndef CNK_MAX_DIRECTOR_CUES
#define CNK_MAX_DIRECTOR_CUES 16
#endif

#ifndef CNK_MIN_FOV_DEG
#define CNK_MIN_FOV_DEG 15
#endif

#ifndef CNK_MAX_FOV_DEG
#define CNK_MAX_FOV_DEG 150
#endif

#ifndef CNK_DEFAULT_NEAR_CLIP
#define CNK_DEFAULT_NEAR_CLIP 4
#endif

#ifndef CNK_DEFAULT_FAR_CLIP
#define CNK_DEFAULT_FAR_CLIP 4096
#endif

#ifndef CNK_DEFAULT_VIEW_W
#define CNK_DEFAULT_VIEW_W 640
#endif

#ifndef CNK_DEFAULT_VIEW_H
#define CNK_DEFAULT_VIEW_H 480
#endif

#ifndef CNK_ORBIT_PITCH_LIMIT_DEG
#define CNK_ORBIT_PITCH_LIMIT_DEG 89
#endif

#ifndef CNK_USE_ASSERTS
#define CNK_USE_ASSERTS 0
#endif

#endif

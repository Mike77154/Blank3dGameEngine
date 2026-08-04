#ifndef EAI_CONFIG_H
#define EAI_CONFIG_H

#include <limits.h>

#if INT_MAX == 2147483647
typedef signed int EAI_Fixed;
#define EAI_FIXED_MAX_LITERAL 2147483647
#define EAI_FIXED_MIN_LITERAL (-2147483647 - 1)
#elif LONG_MAX == 2147483647L
typedef signed long EAI_Fixed;
#define EAI_FIXED_MAX_LITERAL 2147483647L
#define EAI_FIXED_MIN_LITERAL (-2147483647L - 1L)
#else
#error "EnlightenerAI requires a 32-bit signed integer type for EAI_Fixed."
#endif


#define EAI_FX_SHIFT 16
#define EAI_FX_ONE   ((EAI_Fixed)65536L)
#define EAI_FX_ZERO  ((EAI_Fixed)0L)
#define EAI_FX_HALF  ((EAI_Fixed)32768L)
#define EAI_FX_MAX   ((EAI_Fixed)EAI_FIXED_MAX_LITERAL)
#define EAI_FX_MIN   ((EAI_Fixed)EAI_FIXED_MIN_LITERAL)
#define EAI_FX_EPSILON ((EAI_Fixed)1L)
#define EAI_FX_FROM_RAW(v) ((EAI_Fixed)(v))
#define EAI_FX_FROM_INT(v) ((EAI_Fixed)(((EAI_Fixed)(v) > (EAI_Fixed)32767) ? EAI_FX_MAX : (((EAI_Fixed)(v) < (EAI_Fixed)-32768) ? EAI_FX_MIN : ((EAI_Fixed)((EAI_Fixed)(v) * EAI_FX_ONE)))))

#define EAI_VERSION_MAJOR 0
#define EAI_VERSION_MINOR 1
#define EAI_VERSION_PATCH 0

#define EAI_MAX_ENTITIES        128
#define EAI_MAX_NAV_NODES       1024
#define EAI_MAX_NAV_EDGES       4096
#define EAI_MAX_ZONES           64
#define EAI_MAX_TEAMS           8
#define EAI_MAX_EVENTS          512
#define EAI_MAX_STIMULI         256
#define EAI_MAX_PATH_LEN        64
#define EAI_MAX_SEARCH_OPEN     1024

#define EAI_SOUND_MASK_WORDS    ((EAI_MAX_ENTITIES + 31) / 32)

#define EAI_DEFAULT_VIEW_RANGE         EAI_FX_FROM_RAW(1310720)
#define EAI_DEFAULT_VIEW_COS_HALF_FOV  EAI_FX_FROM_RAW(46341)
#define EAI_DEFAULT_HEARING_RANGE      EAI_FX_FROM_RAW(983040)

#define EAI_SOUND_LIFETIME             EAI_FX_FROM_RAW(81920)
#define EAI_ALERT_DECAY_PER_SECOND     EAI_FX_FROM_RAW(11796)
#define EAI_SUSPICION_DECAY_PER_SECOND EAI_FX_FROM_RAW(6554)
#define EAI_TARGET_FORGET_TIME         EAI_FX_FROM_RAW(196608)
#define EAI_INVESTIGATE_MEMORY_TIME    EAI_FX_FROM_RAW(327680)
#define EAI_COVER_SEARCH_RADIUS        EAI_FX_FROM_RAW(786432)
#define EAI_FLANK_SEARCH_RADIUS        EAI_FX_FROM_RAW(1048576)

#define EAI_ENABLE_HEARING  1
#define EAI_ENABLE_TACTICAL 1
#define EAI_ENABLE_DEBUG    1

#endif

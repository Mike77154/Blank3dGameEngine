#ifndef EAI_TYPES_H
#define EAI_TYPES_H

#include "eai_config.h"

typedef unsigned char  EAI_U8;
typedef unsigned short EAI_U16;
#if UINT_MAX == 4294967295U
typedef unsigned int   EAI_U32;
#elif ULONG_MAX == 4294967295UL
typedef unsigned long  EAI_U32;
#else
#error "EnlightenerAI requires a 32-bit unsigned integer type for EAI_U32."
#endif
typedef signed short   EAI_S16;

typedef EAI_U16 EAI_EntityId;
typedef EAI_U16 EAI_NodeId;
typedef EAI_U16 EAI_EdgeId;
typedef EAI_U16 EAI_ZoneId;

#define EAI_TRUE  1
#define EAI_FALSE 0

#define EAI_INVALID_ID ((EAI_U16)0xFFFFu)
#define EAI_ARRAY_COUNT(a) ((EAI_U16)(sizeof(a) / sizeof((a)[0])))

typedef struct EAI_Vec3
{
    EAI_Fixed x;
    EAI_Fixed y;
    EAI_Fixed z;
} EAI_Vec3;

typedef struct EAI_Path
{
    EAI_NodeId nodes[EAI_MAX_PATH_LEN];
    EAI_U16 count;
} EAI_Path;

#endif

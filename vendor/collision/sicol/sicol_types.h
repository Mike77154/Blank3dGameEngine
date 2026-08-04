#ifndef SICOL_TYPES_H
#define SICOL_TYPES_H

/* -------------------------------------------------
 * SICOL - Portable integer types (C89 style)
 * ------------------------------------------------- */

#if defined(_MSC_VER) && (_MSC_VER < 1600)

typedef signed __int8    int8_t;
typedef unsigned __int8  uint8_t;

typedef signed __int16   int16_t;
typedef unsigned __int16 uint16_t;

typedef signed __int32   int32_t;
typedef unsigned __int32 uint32_t;

typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;

#else
#include <stdint.h>
#endif

#ifndef SICOL_BOOL_DEFINED
#define SICOL_BOOL_DEFINED

typedef int sicol_bool;

#define SICOL_TRUE  1
#define SICOL_FALSE 0

#endif

#endif /* SICOL_TYPES_H */

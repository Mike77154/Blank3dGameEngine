#ifndef FPI_COMMON_H
#define FPI_COMMON_H

#include <stddef.h>
#include "fpi_config.h"

#define FPI_TRUE 1
#define FPI_FALSE 0
#define FPI_UNUSED(x) ((void)(x))
#define FPI_ARRAY_COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* FPI_COMMON_H */

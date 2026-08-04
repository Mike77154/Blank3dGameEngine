#ifndef GKINV_TYPES_H
#define GKINV_TYPES_H

#include <stddef.h>
#include "gkinv_config.h"

typedef signed char gkinv_i8;
typedef unsigned char gkinv_u8;
typedef signed short gkinv_i16;
typedef unsigned short gkinv_u16;
typedef signed long gkinv_i32;
typedef unsigned long gkinv_u32;
typedef int gkinv_bool;

#define GKINV_FALSE 0
#define GKINV_TRUE 1

#define GKINV_UNUSED(x) ((void)(x))

#endif

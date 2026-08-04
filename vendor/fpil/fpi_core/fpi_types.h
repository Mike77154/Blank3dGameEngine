#ifndef FPI_TYPES_H
#define FPI_TYPES_H

#include "fpi_common.h"

typedef long FPI_Fixed;
typedef unsigned long FPI_U32;
typedef unsigned short FPI_U16;
typedef unsigned char FPI_U8;

typedef enum FPI_Namespace {
    FPI_NS_CONDITION = 1,
    FPI_NS_ACTION = 2
} FPI_Namespace;

typedef enum FPI_ExecMode {
    FPI_EXEC_SEQUENTIAL_IMMEDIATE = 0,
    FPI_EXEC_TWO_PHASE = 1
} FPI_ExecMode;

typedef struct FPI_RunOptions {
    int stop_on_first_match;
    int exec_mode;
} FPI_RunOptions;

#endif /* FPI_TYPES_H */

#ifndef CM89_TYPES_H
#define CM89_TYPES_H

#include "cm89_config.h"

typedef unsigned short cm89_class_h;
typedef unsigned short cm89_instance_h;
typedef unsigned short cm89_count;
typedef unsigned char cm89_bool;

#define CM89_FALSE ((cm89_bool)0)
#define CM89_TRUE  ((cm89_bool)1)

#define CM89_CLASS_NONE    ((cm89_class_h)0)
#define CM89_INSTANCE_NONE ((cm89_instance_h)0)

typedef enum cm89_result {
    CM89_OK = 0,
    CM89_ERR_ARGUMENT = -1,
    CM89_ERR_CAPACITY = -2,
    CM89_ERR_NOT_FOUND = -3,
    CM89_ERR_DUPLICATE = -4,
    CM89_ERR_INVALID_HANDLE = -5,
    CM89_ERR_MRO_CONFLICT = -6,
    CM89_ERR_DUPLICATE_BASE = -7,
    CM89_ERR_CYCLE = -8,
    CM89_ERR_NOT_CALLABLE = -9,
    CM89_ERR_READ_ONLY = -10,
    CM89_ERR_PROVIDER = -11,
    CM89_ERR_CLASS_NOT_FINAL = -12,
    CM89_ERR_DISABLED = -13,
    CM89_ERR_SEALED = -14
} cm89_result;

typedef enum cm89_value_kind {
    CM89_VALUE_NONE = 0,
    CM89_VALUE_SINT,
    CM89_VALUE_UINT,
    CM89_VALUE_PTR,
    CM89_VALUE_HOST_HANDLE,
    CM89_VALUE_CLASS,
    CM89_VALUE_INSTANCE
} cm89_value_kind;

typedef union cm89_value_data {
    long sint_value;
    unsigned long uint_value;
    void *ptr_value;
} cm89_value_data;

typedef struct cm89_value {
    cm89_value_kind kind;
    cm89_value_data data;
} cm89_value;

typedef enum cm89_member_kind {
    CM89_MEMBER_VALUE = 0,
    CM89_MEMBER_METHOD,
    CM89_MEMBER_STATIC_METHOD,
    CM89_MEMBER_CLASS_METHOD,
    CM89_MEMBER_PROPERTY
} cm89_member_kind;

typedef struct cm89_call {
    const cm89_value *args;
    cm89_count arg_count;
    cm89_value kwargs_handle;
} cm89_call;

typedef struct cm89_member {
    char name[CM89_NAME_MAX];
    cm89_member_kind kind;
    cm89_value value;
    cm89_value aux;
    cm89_bool used;
} cm89_member;

#endif

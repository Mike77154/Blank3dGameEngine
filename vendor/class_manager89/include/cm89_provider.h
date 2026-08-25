#ifndef CM89_PROVIDER_H
#define CM89_PROVIDER_H

#include "cm89_types.h"

/*
 * Language/VM bridge.
 *
 * callable:
 *   Opaque host value previously registered as a class member.
 *
 * receiver:
 *   INSTANCE for normal methods/property access,
 *   CLASS for class methods,
 *   NONE for static methods.
 *
 * owner_class:
 *   Class where the resolved member was defined. Useful to implement
 *   host-side metadata, tracing, or equivalent behavior.
 *
 * call:
 *   Positional args plus an opaque kwargs handle. The manager never
 *   interprets the kwargs handle.
 */
typedef cm89_result (*cm89_invoke_fn)(
    void *user,
    cm89_value callable,
    cm89_value receiver,
    cm89_class_h owner_class,
    const cm89_call *call,
    cm89_value *out_value
);

typedef struct cm89_provider {
    void *user;
    cm89_invoke_fn invoke;
} cm89_provider;

#endif

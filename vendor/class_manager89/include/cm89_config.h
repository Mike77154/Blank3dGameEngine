#ifndef CM89_CONFIG_H
#define CM89_CONFIG_H

/*
 * class_manager89 configuration.
 * Override these macros from the build system before including cm89.h.
 *
 * The library performs no dynamic allocation. Every capacity is fixed.
 */

#ifndef CM89_MAX_CLASSES
#define CM89_MAX_CLASSES 128
#endif

#ifndef CM89_MAX_INSTANCES
#define CM89_MAX_INSTANCES 256
#endif

#ifndef CM89_MAX_BASES
#define CM89_MAX_BASES 8
#endif

#ifndef CM89_MAX_MRO
#define CM89_MAX_MRO 64
#endif

#ifndef CM89_MAX_CLASS_MEMBERS
#define CM89_MAX_CLASS_MEMBERS 64
#endif

#ifndef CM89_MAX_INSTANCE_ATTRS
#define CM89_MAX_INSTANCE_ATTRS 32
#endif

#ifndef CM89_NAME_MAX
#define CM89_NAME_MAX 48
#endif

/*
 * Standalone users may keep the compact internal instance pool.  Hosts that
 * already have a runtime identity system (Thing/ECS/etc.) should set this to
 * 0 and use cm89_call_bound()/cm89_call_bound_super() instead.
 */
#ifndef CM89_ENABLE_INTERNAL_INSTANCES
#define CM89_ENABLE_INTERNAL_INSTANCES 1
#endif

/* cm89_class_h is 16-bit: low byte = slot+1, high byte = generation. */
#if CM89_MAX_CLASSES > 255
#error "CM89_MAX_CLASSES must be <= 255 for generational class handles"
#endif

#endif

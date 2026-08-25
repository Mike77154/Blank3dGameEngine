#ifndef IMGCC0_COMPAT_STDINT_H
#define IMGCC0_COMPAT_STDINT_H

/*
 * Tiny fallback for old C89 toolchains that do not ship <stdint.h>.
 * On modern GCC/Clang builds, prefer the system header to avoid typedef
 * conflicts with libc internals.
 */
#if defined(__GNUC__) || defined(__clang__)
#  include_next <stdint.h>
#else
#  include <limits.h>

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

#if UINT_MAX == 0xffffffffUL
typedef signed int int32_t;
typedef unsigned int uint32_t;
#elif ULONG_MAX == 0xffffffffUL
typedef signed long int32_t;
typedef unsigned long uint32_t;
#else
#error "imgcc0 compat stdint requires a 32-bit int or long"
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1900)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
# if defined(ULLONG_MAX) && (ULLONG_MAX > 0xffffffffUL)
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
# elif defined(ULONG_MAX) && (ULONG_MAX > 0xffffffffUL)
typedef signed long int64_t;
typedef unsigned long uint64_t;
# else
#error "imgcc0 compat stdint requires a 64-bit integer type"
# endif
#endif

#endif /* compiler choice */

#endif /* IMGCC0_COMPAT_STDINT_H */

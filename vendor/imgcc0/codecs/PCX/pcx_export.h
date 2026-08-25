/* PCX89 public ABI/annotation layer.
 * Keeps DLL/shared-library visibility and static-analysis contracts while
 * intentionally disabling dynamic-allocation ownership annotations.
 */
#ifndef PCX_EXPORT_H
#define PCX_EXPORT_H

#if defined(PCX_STATIC_DEFINE)
#  define PCX_EXPORT
#  define PCX_NO_EXPORT
#else
#  if defined(_WIN32) || defined(__CYGWIN__)
#    if defined(PCX_BUILDING_LIBRARY)
#      define PCX_EXPORT __declspec(dllexport)
#    else
#      define PCX_EXPORT __declspec(dllimport)
#    endif
#    define PCX_NO_EXPORT
#  else
#    if defined(__GNUC__) || defined(__clang__)
#      define PCX_EXPORT __attribute__((visibility("default")))
#      define PCX_NO_EXPORT __attribute__((visibility("hidden")))
#    else
#      define PCX_EXPORT
#      define PCX_NO_EXPORT
#    endif
#  endif
#endif

#if defined(_MSC_VER)
#  include <sal.h>
#endif

#ifndef PCX_HAS_ATTRIBUTE
#  if defined(__has_attribute)
#    define PCX_HAS_ATTRIBUTE(x) __has_attribute(x)
#  else
#    define PCX_HAS_ATTRIBUTE(x) 0
#  endif
#endif

#ifndef PCX_HAS_BUILTIN
#  if defined(__has_builtin)
#    define PCX_HAS_BUILTIN(x) __has_builtin(x)
#  else
#    define PCX_HAS_BUILTIN(x) 0
#  endif
#endif

#ifndef PCX_HAS_DECLSPEC_ATTRIBUTE
#  if defined(__has_declspec_attribute)
#    define PCX_HAS_DECLSPEC_ATTRIBUTE(x) __has_declspec_attribute(x)
#  else
#    define PCX_HAS_DECLSPEC_ATTRIBUTE(x) 0
#  endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  define PCX_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#  define PCX_GCC_VERSION 0
#endif

#if defined(__clang__)
#  define PCX_CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#else
#  define PCX_CLANG_VERSION 0
#endif

#if defined(__clang_analyzer__)
#  define PCX_CLANG_ANALYZER 1
#else
#  define PCX_CLANG_ANALYZER 0
#endif

#if defined(__GNUC_ANALYZER__)
#  define PCX_GCC_ANALYZER 1
#else
#  define PCX_GCC_ANALYZER 0
#endif

#if defined(_PREFAST_)
#  define PCX_MSVC_ANALYZER 1
#else
#  define PCX_MSVC_ANALYZER 0
#endif

#if (PCX_CLANG_ANALYZER || PCX_GCC_ANALYZER || PCX_MSVC_ANALYZER)
#  define PCX_ANALYZER_ACTIVE 1
#else
#  define PCX_ANALYZER_ACTIVE 0
#endif

#if defined(_MSC_VER)
#  define PCX_WARN_UNUSED_RESULT _Check_return_
#elif defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(warn_unused_result)
#  define PCX_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#  define PCX_WARN_UNUSED_RESULT
#endif

#if defined(_MSC_VER)
#  define PCX_RETURNS_NONNULL _Ret_notnull_
#elif defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(returns_nonnull)
#  define PCX_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#  define PCX_RETURNS_NONNULL
#endif

#if defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(nonnull)
#  define PCX_ATTR_NONNULL_1(_1) __attribute__((nonnull(_1)))
#  define PCX_ATTR_NONNULL_2(_1,_2) __attribute__((nonnull(_1,_2)))
#  define PCX_ATTR_NONNULL_3(_1,_2,_3) __attribute__((nonnull(_1,_2,_3)))
#  define PCX_ATTR_NONNULL_4(_1,_2,_3,_4) __attribute__((nonnull(_1,_2,_3,_4)))
#  define PCX_ATTR_NONNULL_5(_1,_2,_3,_4,_5) __attribute__((nonnull(_1,_2,_3,_4,_5)))
#else
#  define PCX_ATTR_NONNULL_1(_1)
#  define PCX_ATTR_NONNULL_2(_1,_2)
#  define PCX_ATTR_NONNULL_3(_1,_2,_3)
#  define PCX_ATTR_NONNULL_4(_1,_2,_3,_4)
#  define PCX_ATTR_NONNULL_5(_1,_2,_3,_4,_5)
#endif

#if defined(_MSC_VER)
#  define PCX_SAL_IN _In_
#  define PCX_SAL_OUT _Out_
#  define PCX_SAL_INOUT _Inout_
#  define PCX_SAL_IN_Z _In_z_
#  define PCX_SAL_IN_READS_BYTES(_n) _In_reads_bytes_(_n)
#  define PCX_SAL_OUT_WRITES_BYTES(_n) _Out_writes_bytes_(_n)
#  define PCX_SAL_OUT_WRITES_BYTES_ALL(_n) _Out_writes_bytes_all_(_n)
#  define PCX_SAL_INOUT_UPDATES_BYTES(_n) _Inout_updates_bytes_(_n)
#  define PCX_SAL_INOUT_UPDATES_BYTES_ALL(_n) _Inout_updates_bytes_all_(_n)
#  define PCX_SAL_OUTPTR_RESULT_MAYBENULL _Outptr_result_maybenull_
#else
#  define PCX_SAL_IN
#  define PCX_SAL_OUT
#  define PCX_SAL_INOUT
#  define PCX_SAL_IN_Z
#  define PCX_SAL_IN_READS_BYTES(_n)
#  define PCX_SAL_OUT_WRITES_BYTES(_n)
#  define PCX_SAL_OUT_WRITES_BYTES_ALL(_n)
#  define PCX_SAL_INOUT_UPDATES_BYTES(_n)
#  define PCX_SAL_INOUT_UPDATES_BYTES_ALL(_n)
#  define PCX_SAL_OUTPTR_RESULT_MAYBENULL
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  define PCX_ATTR_ACCESS_RO_1(_ptr) __attribute__((access(read_only, _ptr)))
#  define PCX_ATTR_ACCESS_RO_2(_ptr,_size) __attribute__((access(read_only, _ptr, _size)))
#  define PCX_ATTR_ACCESS_WO_1(_ptr) __attribute__((access(write_only, _ptr)))
#  define PCX_ATTR_ACCESS_WO_2(_ptr,_size) __attribute__((access(write_only, _ptr, _size)))
#  define PCX_ATTR_ACCESS_RW_1(_ptr) __attribute__((access(read_write, _ptr)))
#  define PCX_ATTR_ACCESS_RW_2(_ptr,_size) __attribute__((access(read_write, _ptr, _size)))
#else
#  define PCX_ATTR_ACCESS_RO_1(_ptr)
#  define PCX_ATTR_ACCESS_RO_2(_ptr,_size)
#  define PCX_ATTR_ACCESS_WO_1(_ptr)
#  define PCX_ATTR_ACCESS_WO_2(_ptr,_size)
#  define PCX_ATTR_ACCESS_RW_1(_ptr)
#  define PCX_ATTR_ACCESS_RW_2(_ptr,_size)
#endif

#if defined(__clang__) && defined(__cplusplus) && PCX_HAS_ATTRIBUTE(lifetimebound)
#  define PCX_LIFETIMEBOUND __attribute__((lifetimebound))
#else
#  define PCX_LIFETIMEBOUND
#endif

/* Legacy annotation spellings stay source-compatible, but they are deliberately
 * inert because PCX89 has no dynamic ownership API. */
#define PCX_ALLOCATOR
#define PCX_ATTR_ALLOC_SIZE_1(_1)
#define PCX_ATTR_ALLOC_SIZE_2(_1,_2)
#define PCX_ATTR_MALLOC_DEALLOCATOR(_fn,_idx)
#define PCX_OWNERSHIP_RETURNS_MALLOC
#define PCX_OWNERSHIP_TAKES_MALLOC(_idx)
#define PCX_OWNERSHIP_HOLDS_MALLOC(_idx)

#if defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(pure)
#  define PCX_PURE_FN __attribute__((pure))
#else
#  define PCX_PURE_FN
#endif

#if defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(const)
#  define PCX_CONST_FN __attribute__((const))
#else
#  define PCX_CONST_FN
#endif

#if defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(cold)
#  define PCX_COLD __attribute__((cold))
#else
#  define PCX_COLD
#endif

#if defined(_MSC_VER)
#  define PCX_NORETURN __declspec(noreturn)
#elif defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(noreturn)
#  define PCX_NORETURN __attribute__((noreturn))
#else
#  define PCX_NORETURN
#endif

#if defined(_MSC_VER)
#  define PCX_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__) || PCX_HAS_ATTRIBUTE(noinline)
#  define PCX_NOINLINE __attribute__((noinline))
#else
#  define PCX_NOINLINE
#endif

#if defined(_MSC_VER)
#  define PCX_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
#  define PCX_DEPRECATED __attribute__((deprecated))
#else
#  define PCX_DEPRECATED
#endif

#define PCX_DEPRECATED_EXPORT PCX_EXPORT PCX_DEPRECATED
#define PCX_DEPRECATED_NO_EXPORT PCX_NO_EXPORT PCX_DEPRECATED
#define PCX_API PCX_EXPORT

#ifndef PCX_NO_DEPRECATED
#  define PCX_NO_DEPRECATED 0
#endif

#endif /* PCX_EXPORT_H */

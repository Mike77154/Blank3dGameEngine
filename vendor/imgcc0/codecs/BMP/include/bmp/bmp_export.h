/* Symbol visibility and non-owning buffer contracts for the C89 BMP API. */
#ifndef BMP_EXPORT_H
#define BMP_EXPORT_H

#if defined(BMP_STATIC_DEFINE)
# define BMP_EXPORT
# define BMP_NO_EXPORT
#else
# if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(BMP_BUILDING_LIBRARY)
#   define BMP_EXPORT __declspec(dllexport)
#  else
#   define BMP_EXPORT __declspec(dllimport)
#  endif
#  define BMP_NO_EXPORT
# else
#  if defined(__GNUC__) || defined(__clang__)
#   define BMP_EXPORT __attribute__((visibility("default")))
#   define BMP_NO_EXPORT __attribute__((visibility("hidden")))
#  else
#   define BMP_EXPORT
#   define BMP_NO_EXPORT
#  endif
# endif
#endif

#if defined(_MSC_VER)
# include <sal.h>
#endif

#ifndef BMP_HAS_ATTRIBUTE
# if defined(__has_attribute)
#  define BMP_HAS_ATTRIBUTE(x) __has_attribute(x)
# else
#  define BMP_HAS_ATTRIBUTE(x) 0
# endif
#endif

#ifndef BMP_HAS_BUILTIN
# if defined(__has_builtin)
#  define BMP_HAS_BUILTIN(x) __has_builtin(x)
# else
#  define BMP_HAS_BUILTIN(x) 0
# endif
#endif

#ifndef BMP_HAS_DECLSPEC_ATTRIBUTE
# if defined(__has_declspec_attribute)
#  define BMP_HAS_DECLSPEC_ATTRIBUTE(x) __has_declspec_attribute(x)
# else
#  define BMP_HAS_DECLSPEC_ATTRIBUTE(x) 0
# endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
# define BMP_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
# define BMP_GCC_VERSION 0
#endif

#if defined(__clang__)
# define BMP_CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#else
# define BMP_CLANG_VERSION 0
#endif

#if defined(__clang_analyzer__)
# define BMP_CLANG_ANALYZER 1
#else
# define BMP_CLANG_ANALYZER 0
#endif

#if defined(__GNUC_ANALYZER__)
# define BMP_GCC_ANALYZER 1
#else
# define BMP_GCC_ANALYZER 0
#endif

#if defined(_PREFAST_)
# define BMP_MSVC_ANALYZER 1
#else
# define BMP_MSVC_ANALYZER 0
#endif

#if (BMP_CLANG_ANALYZER || BMP_GCC_ANALYZER || BMP_MSVC_ANALYZER)
# define BMP_ANALYZER_ACTIVE 1
#else
# define BMP_ANALYZER_ACTIVE 0
#endif

#if defined(_MSC_VER)
# define BMP_WARN_UNUSED_RESULT _Check_return_
#elif defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(warn_unused_result)
# define BMP_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
# define BMP_WARN_UNUSED_RESULT
#endif

#if defined(_MSC_VER)
# define BMP_RETURNS_NONNULL _Ret_notnull_
#elif defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(returns_nonnull)
# define BMP_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
# define BMP_RETURNS_NONNULL
#endif

#if defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(nonnull)
# define BMP_ATTR_NONNULL_1(_1) __attribute__((nonnull(_1)))
# define BMP_ATTR_NONNULL_2(_1,_2) __attribute__((nonnull(_1,_2)))
# define BMP_ATTR_NONNULL_3(_1,_2,_3) __attribute__((nonnull(_1,_2,_3)))
# define BMP_ATTR_NONNULL_4(_1,_2,_3,_4) __attribute__((nonnull(_1,_2,_3,_4)))
# define BMP_ATTR_NONNULL_5(_1,_2,_3,_4,_5) __attribute__((nonnull(_1,_2,_3,_4,_5)))
# define BMP_ATTR_NONNULL_6(_1,_2,_3,_4,_5,_6) __attribute__((nonnull(_1,_2,_3,_4,_5,_6)))
#else
# define BMP_ATTR_NONNULL_1(_1)
# define BMP_ATTR_NONNULL_2(_1,_2)
# define BMP_ATTR_NONNULL_3(_1,_2,_3)
# define BMP_ATTR_NONNULL_4(_1,_2,_3,_4)
# define BMP_ATTR_NONNULL_5(_1,_2,_3,_4,_5)
# define BMP_ATTR_NONNULL_6(_1,_2,_3,_4,_5,_6)
#endif

#if defined(_MSC_VER)
# define BMP_SAL_IN _In_
# define BMP_SAL_OUT _Out_
# define BMP_SAL_INOUT _Inout_
# define BMP_SAL_IN_Z _In_z_
# define BMP_SAL_IN_READS_BYTES(_n) _In_reads_bytes_(_n)
# define BMP_SAL_OUT_WRITES_BYTES(_n) _Out_writes_bytes_(_n)
# define BMP_SAL_OUT_WRITES_BYTES_ALL(_n) _Out_writes_bytes_all_(_n)
# define BMP_SAL_INOUT_UPDATES_BYTES(_n) _Inout_updates_bytes_(_n)
# define BMP_SAL_INOUT_UPDATES_BYTES_ALL(_n) _Inout_updates_bytes_all_(_n)
# define BMP_SAL_OUTPTR_RESULT_MAYBENULL _Outptr_result_maybenull_
#else
# define BMP_SAL_IN
# define BMP_SAL_OUT
# define BMP_SAL_INOUT
# define BMP_SAL_IN_Z
# define BMP_SAL_IN_READS_BYTES(_n)
# define BMP_SAL_OUT_WRITES_BYTES(_n)
# define BMP_SAL_OUT_WRITES_BYTES_ALL(_n)
# define BMP_SAL_INOUT_UPDATES_BYTES(_n)
# define BMP_SAL_INOUT_UPDATES_BYTES_ALL(_n)
# define BMP_SAL_OUTPTR_RESULT_MAYBENULL
#endif

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 10)
# define BMP_ATTR_ACCESS_RO_1(_ptr) __attribute__((access(read_only, _ptr)))
# define BMP_ATTR_ACCESS_RO_2(_ptr,_size) __attribute__((access(read_only, _ptr, _size)))
# define BMP_ATTR_ACCESS_WO_1(_ptr) __attribute__((access(write_only, _ptr)))
# define BMP_ATTR_ACCESS_WO_2(_ptr,_size) __attribute__((access(write_only, _ptr, _size)))
# define BMP_ATTR_ACCESS_RW_1(_ptr) __attribute__((access(read_write, _ptr)))
# define BMP_ATTR_ACCESS_RW_2(_ptr,_size) __attribute__((access(read_write, _ptr, _size)))
#else
# define BMP_ATTR_ACCESS_RO_1(_ptr)
# define BMP_ATTR_ACCESS_RO_2(_ptr,_size)
# define BMP_ATTR_ACCESS_WO_1(_ptr)
# define BMP_ATTR_ACCESS_WO_2(_ptr,_size)
# define BMP_ATTR_ACCESS_RW_1(_ptr)
# define BMP_ATTR_ACCESS_RW_2(_ptr,_size)
#endif

#if defined(__clang__) && defined(__cplusplus) && BMP_HAS_ATTRIBUTE(lifetimebound)
# define BMP_LIFETIMEBOUND __attribute__((lifetimebound))
#else
# define BMP_LIFETIMEBOUND
#endif

#if defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(pure)
# define BMP_PURE_FN __attribute__((pure))
#else
# define BMP_PURE_FN
#endif

#if defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(const)
# define BMP_CONST_FN __attribute__((const))
#else
# define BMP_CONST_FN
#endif

#if defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(cold)
# define BMP_COLD __attribute__((cold))
#else
# define BMP_COLD
#endif

#if defined(_MSC_VER)
# define BMP_NORETURN __declspec(noreturn)
#elif defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(noreturn)
# define BMP_NORETURN __attribute__((noreturn))
#else
# define BMP_NORETURN
#endif

#if defined(_MSC_VER)
# define BMP_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__) || BMP_HAS_ATTRIBUTE(noinline)
# define BMP_NOINLINE __attribute__((noinline))
#else
# define BMP_NOINLINE
#endif

#if defined(_MSC_VER)
# define BMP_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__) || defined(__clang__)
# define BMP_DEPRECATED __attribute__((deprecated))
#else
# define BMP_DEPRECATED
#endif

#define BMP_DEPRECATED_EXPORT BMP_EXPORT BMP_DEPRECATED
#define BMP_DEPRECATED_NO_EXPORT BMP_NO_EXPORT BMP_DEPRECATED
#define BMP_API BMP_EXPORT

#ifndef BMP_NO_DEPRECATED
# define BMP_NO_DEPRECATED 0
#endif

#endif /* BMP_EXPORT_H */

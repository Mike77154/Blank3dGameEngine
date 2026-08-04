#ifndef ZLIB89_H
#define ZLIB89_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef ZLIB89_U8_DEFINED
#define ZLIB89_U8_DEFINED
typedef unsigned char zlib89_u8;
#endif
#ifndef ZLIB89_U16_DEFINED
#define ZLIB89_U16_DEFINED
typedef unsigned short zlib89_u16;
#endif
#ifndef ZLIB89_U32_DEFINED
#define ZLIB89_U32_DEFINED
typedef unsigned long zlib89_u32;
#endif

#define ZLIB89_MAX_BITS           15u
#define ZLIB89_MAX_LITLEN_SYMS    288u
#define ZLIB89_MAX_DIST_SYMS      32u
#define ZLIB89_MAX_CL_SYMS        19u
#define ZLIB89_MAX_TREE_NODES     640u

enum {
    ZLIB89_OK = 0,
    ZLIB89_EINVAL = -1,
    ZLIB89_EFORMAT = -2,
    ZLIB89_EUNSUPPORTED = -3,
    ZLIB89_ERANGE = -4,
    ZLIB89_ENOSPC = -5,
    ZLIB89_EADLER = -6,
    ZLIB89_EDICT = -7,
    ZLIB89_ETRUNCATED = -8
};

enum {
    ZLIB89_FLAG_IGNORE_ADLER = 1u << 0,
    ZLIB89_FLAG_ALLOW_TRAILING = 1u << 1
};

typedef struct Zlib89Huff {
    short left[ZLIB89_MAX_TREE_NODES];
    short right[ZLIB89_MAX_TREE_NODES];
    short sym[ZLIB89_MAX_TREE_NODES];
    short nodes_used;
    short used_symbols;
} Zlib89Huff;

typedef struct Zlib89Scratch {
    Zlib89Huff litlen;
    Zlib89Huff dist;
    Zlib89Huff code;
    zlib89_u8 ll_lengths[ZLIB89_MAX_LITLEN_SYMS];
    zlib89_u8 dist_lengths[ZLIB89_MAX_DIST_SYMS];
    zlib89_u8 code_lengths[ZLIB89_MAX_CL_SYMS];
} Zlib89Scratch;

typedef struct Zlib89Info {
    zlib89_u8 cmf;
    zlib89_u8 flg;
    zlib89_u32 total_in;
    zlib89_u32 total_out;
    zlib89_u32 adler_expected;
    zlib89_u32 adler_actual;
    int used_dict;
} Zlib89Info;

int zlib89_inflate_zlib(const zlib89_u8 *src, zlib89_u32 src_size,
                        zlib89_u8 *dst, zlib89_u32 dst_size,
                        zlib89_u32 flags,
                        Zlib89Scratch *scratch,
                        zlib89_u32 *out_written,
                        Zlib89Info *out_info);

int zlib89_inflate_raw(const zlib89_u8 *src, zlib89_u32 src_size,
                       zlib89_u8 *dst, zlib89_u32 dst_size,
                       Zlib89Scratch *scratch,
                       zlib89_u32 *out_written);

#ifdef __cplusplus
}
#endif

#endif

#ifndef ZRAGFLIB_INTERNAL_H_INCLUDED
#define ZRAGFLIB_INTERNAL_H_INCLUDED

#include "zragflib.h"
#include <string.h>
#include <limits.h>
#include "zragf_protocol89_mem.h"

/* Fallback INLINE para C89/C99 */
#ifndef ZRAGF_C89_MODE
#  if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#    define ZRAGF_C89_MODE 1
#  else
#    define ZRAGF_C89_MODE 0
#  endif
#endif

#ifndef ZRAGF_INLINE
#  if ZRAGF_C89_MODE
#    define ZRAGF_INLINE
#  elif defined(_MSC_VER)
#    define ZRAGF_INLINE __inline
#  elif defined(__GNUC__) || defined(__clang__)
#    define ZRAGF_INLINE inline
#  else
#    define ZRAGF_INLINE
#  endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define ZRAGF_MAYBE_UNUSED __attribute__((unused))
#else
#  define ZRAGF_MAYBE_UNUSED
#endif

/* ============================================================
   MAGIC & VERSION (modo ZRAGF nativo)
   ============================================================ */
#define ZRAGF_MAGIC0 'Z'
#define ZRAGF_MAGIC1 'R'
#define ZRAGF_MAGIC2 'A'
#define ZRAGF_MAGIC3 'G'
#define ZRAGF_VERSION 0x01

/* ============================================================
   LÍMITES DE TU MODO NATIVO ZRAGF
   ============================================================ */
#define ZRAGF_MAX_LITERAL   128
#define ZRAGF_MIN_MATCH     3
#define ZRAGF_MAX_MATCH     130

#define ZRAGF_MAX_WINDOW    65535u

#define ZRAGF_FLAG_NONE     0x00u
#define ZRAGF_FLAG_HUFFMAN  0x01u
#define ZRAGF_FLAG_CHECKSUM 0x02u

#define ZRAGF_HUFF_SYMS     256u
#define ZRAGF_HUFF_MAX_BITS 16u

/* ============================================================
   FORMATOS INTERNOS (modo C)
   ============================================================ */
typedef enum {
    ZRAGF_INTERNAL_FMT_ZRAGF = 0,
    ZRAGF_INTERNAL_FMT_DEFLATE_RAW,
    ZRAGF_INTERNAL_FMT_ZLIB,
    ZRAGF_INTERNAL_FMT_GZIP,
    ZRAGF_INTERNAL_FMT_AUTO
} zragf_internal_format;

/* ============================================================
   Tipo explícito de estado (evita heurísticas frágiles)
   ============================================================ */
typedef enum {
    ZRAGF_STATE_KIND_NONE = 0,
    ZRAGF_STATE_KIND_NATIVE_DEFLATE,
    ZRAGF_STATE_KIND_NATIVE_INFLATE,
    ZRAGF_STATE_KIND_Z_DEFLATE,
    ZRAGF_STATE_KIND_Z_INFLATE
} zragf_state_kind;

/* ============================================================
   Estructuras internas de DEFLATE estilo zlib
   ============================================================ */
typedef struct {
    zragf_state_kind kind;
    zragf_internal_format format;

    int level;
    int method;
    int windowBits;
    int memLevel;
    int strategy;

    zragf_u8    *in_buf;
    zragf_size_t in_size;
    zragf_size_t in_cap;

    zragf_u8 *window;
    int       window_size;
    int       window_pos;
    zragf_size_t window_filled;

    zragf_u32 bitbuf;
    int       bitcount;

    zragf_u8    *blk_out;
    zragf_size_t blk_size;
    zragf_size_t blk_cap;

    zragf_u8    *pending_out;
    zragf_size_t pending_pos;
    zragf_size_t pending_size;
    zragf_size_t pending_cap;

    zragf_u8    *dict_buf;
    zragf_size_t dict_len;
    zragf_u32    dict_adler;
    int          dict_set;

    int tune_good_length;
    int tune_max_lazy;
    int tune_nice_length;
    int tune_max_chain;
    int tune_set;

    zragf_u8    *gzip_extra;
    zragf_size_t gzip_extra_len;
    zragf_u8    *gzip_name;
    zragf_size_t gzip_name_len;
    zragf_u8    *gzip_comment;
    zragf_size_t gzip_comment_len;
    zragf_u32    gzip_mtime;
    int          gzip_text;
    int          gzip_os;
    int          gzip_hcrc;
    int          gzip_header_set;

    int final_emitted;
    int finished;
    int wrote_header;
    zragf_u32 running_adler;
    zragf_u32 running_crc;
    zragf_u32 input_isize;
} zragf_deflate_state_z;

typedef struct {
    zragf_state_kind kind;
    zragf_internal_format format;
    zragf_internal_format init_format;

    int windowBits;
    int requested_windowBits;
    int max_window_bits;
    int max_window_size;

    zragf_u8    *in_buf;
    zragf_size_t in_size;
    zragf_size_t in_cap;

    zragf_u8 *window;
    int       window_size;
    int       window_pos;
    zragf_size_t window_filled;

    zragf_u32 bitbuf;
    int       bitcount;

    zragf_u8    *dict_buf;
    zragf_size_t dict_len;
    zragf_u32    expected_dict_adler;
    int          need_dict;
    int          saw_header;

    zragf_gz_header *gzip_header_out;

    int hit_final_block;
    int sync_skip_checks;
    int validate_checks;
} zragf_inflate_state_z;

/* ============================================================
   READ/WRITE helpers (modo nativo)
   ============================================================ */
void zragf_write_u32_le(zragf_u8 *p, zragf_u32 v);
zragf_u32 zragf_read_u32_le(const zragf_u8 *p);

/* ============================================================
   SELECTOR DE FORMATO SEGÚN windowBits (zlib-like)
   ============================================================ */
zragf_internal_format zragf_select_format(int windowBits);

/* ============================================================
   ALOCS compatibles con zlib / internos
   ============================================================ */
void *zragf_alloc_default(void *opaque, zragf_size_t items, zragf_size_t size);
void *zragf_realloc_default(void *opaque, void *ptr, zragf_size_t old_size, zragf_size_t new_size);
void  zragf_free_default(void *opaque, void *addr);

/* ============================================================
   BITSTREAM helpers genéricos (DEFLATE / inflate core)
   ============================================================ */
void zragf_bw_init_state(zragf_deflate_state_z *st);
void zragf_bw_putbits(zragf_deflate_state_z *st, zragf_u32 code, int bits);
void zragf_bw_flushbits(zragf_deflate_state_z *st);
int  zragf_br_getbit(zragf_inflate_state_z *st, int *out);

/* ============================================================
   Backend RFC1951 (bloques STORED/FIXED/DYNAMIC)
   ============================================================ */
zragf_size_t zragf_deflate_rfc1951_stored_bound(zragf_size_t src_size);
int zragf_deflate_rfc1951_store_chunk(const zragf_u8 *src,
                                      zragf_size_t    src_size,
                                      zragf_u8       *dst,
                                      zragf_size_t    dst_cap,
                                      zragf_size_t   *dst_size,
                                      int             final_block);
int zragf_deflate_rfc1951_store_chunk_stream(const zragf_u8 *src,
                                             zragf_size_t    src_size,
                                             zragf_u8       *dst,
                                             zragf_size_t    dst_cap,
                                             zragf_size_t   *dst_size,
                                             int             final_block,
                                             unsigned       *bitbuf_io,
                                             int            *bitcount_io);
int zragf_deflate_rfc1951_compress_chunk(const zragf_u8 *src,
                                         zragf_size_t    src_size,
                                         zragf_u8       *dst,
                                         zragf_size_t    dst_cap,
                                         zragf_size_t   *dst_size,
                                         int             final_block,
                                         int             level,
                                         int             strategy,
                                         int             tune_set,
                                         int             good_length,
                                         int             max_lazy,
                                         int             nice_length,
                                         int             max_chain);
int zragf_deflate_rfc1951_compress_chunk_single_with_dict(const zragf_u8 *dict,
                                                          zragf_size_t    dict_size,
                                                          const zragf_u8 *src,
                                                          zragf_size_t    src_size,
                                                          zragf_u8       *dst,
                                                          zragf_size_t    dst_cap,
                                                          zragf_size_t   *dst_size,
                                                          int             final_block,
                                                          int             level,
                                                          int             strategy,
                                                          int             tune_set,
                                                          int             good_length,
                                                          int             max_lazy,
                                                          int             nice_length,
                                                          int             max_chain);
int zragf_deflate_rfc1951_compress_chunk_single_stream(const zragf_u8 *dict,
                                                       zragf_size_t    dict_size,
                                                       const zragf_u8 *src,
                                                       zragf_size_t    src_size,
                                                       zragf_u8       *dst,
                                                       zragf_size_t    dst_cap,
                                                       zragf_size_t   *dst_size,
                                                       int             final_block,
                                                       int             level,
                                                       int             strategy,
                                                       int             tune_set,
                                                       int             good_length,
                                                       int             max_lazy,
                                                       int             nice_length,
                                                       int             max_chain,
                                                       unsigned       *bitbuf_io,
                                                       int            *bitcount_io,
                                                       int             flush_final_bits);
int zragf_deflate_rfc1951_compress_chunk_with_dict(const zragf_u8 *dict,
                                                   zragf_size_t    dict_size,
                                                   const zragf_u8 *src,
                                                   zragf_size_t    src_size,
                                                   zragf_u8       *dst,
                                                   zragf_size_t    dst_cap,
                                                   zragf_size_t   *dst_size,
                                                   int             final_block,
                                                   int             level,
                                                   int             strategy,
                                                   int             tune_set,
                                                   int             good_length,
                                                   int             max_lazy,
                                                   int             nice_length,
                                                   int             max_chain);
int zragf_deflate_rfc1951_compress_chunk_stream(const zragf_u8 *dict,
                                                    zragf_size_t    dict_size,
                                                    const zragf_u8 *src,
                                                    zragf_size_t    src_size,
                                                    zragf_u8       *dst,
                                                    zragf_size_t    dst_cap,
                                                    zragf_size_t   *dst_size,
                                                    int             final_block,
                                                    int             level,
                                                    int             strategy,
                                                    int             tune_set,
                                                    int             good_length,
                                                    int             max_lazy,
                                                    int             nice_length,
                                                    int             max_chain,
                                                    unsigned       *bitbuf_io,
                                                    int            *bitcount_io,
                                                    int             flush_final_bits);
int zragf_deflate_rfc1951_compress(const zragf_u8 *src,
                                   zragf_size_t    src_size,
                                   zragf_u8       *dst,
                                   zragf_size_t    dst_cap,
                                   zragf_size_t   *dst_size,
                                   int             level,
                                   int             strategy,
                                   int             tune_set,
                                   int             good_length,
                                   int             max_lazy,
                                   int             nice_length,
                                   int             max_chain);


#endif /* ZRAGFLIB_INTERNAL_H_INCLUDED */

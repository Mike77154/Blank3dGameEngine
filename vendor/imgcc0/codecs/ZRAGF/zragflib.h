/* zragflib.h - zragf: LZ/HUF compressor + zlib-like compatibility API
 *
 * Licencia: CC0 (Dominio público)
 */

#ifndef ZRAGFLIB_H_INCLUDED
#define ZRAGFLIB_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------
 * Tipos base
 * ------------------------------------------------------------ */
typedef unsigned char  zragf_u8;
typedef unsigned short zragf_u16;
typedef unsigned int   zragf_u32;   /* exactamente 32 bits en targets comunes */
typedef int            zragf_s32;
typedef size_t         zragf_size_t;

/* ------------------------------------------------------------
 * Versionado
 * ------------------------------------------------------------ */
#define ZRAGFLIB_VERSION_MAJOR 0
#define ZRAGFLIB_VERSION_MINOR 67
#define ZRAGFLIB_VERSION_PATCH 0

/* ------------------------------------------------------------
 * Códigos de retorno estilo ZLIB (compatibles 1:1)
 * ------------------------------------------------------------ */
typedef int zragf_status_zlib;
enum {
    ZRAGF_OK            = 0,
    ZRAGF_STREAM_END    = 1,
    ZRAGF_NEED_DICT     = 2,

    ZRAGF_ERRNO         = -1,
    ZRAGF_STREAM_ERROR  = -2,
    ZRAGF_DATA_ERROR    = -3,
    ZRAGF_MEM_ERROR     = -4,
    ZRAGF_BUF_ERROR     = -5,
    ZRAGF_VERSION_ERROR = -6
};

/* ------------------------------------------------------------
 * Códigos de retorno de la API nativa
 * ------------------------------------------------------------ */
typedef int zragf_status;
enum {
    ZRAGF_ST_OK = 0,
    ZRAGF_ST_OUTPUT_TOO_SMALL = 1,
    ZRAGF_ST_CORRUPTED_DATA   = 2,
    ZRAGF_ST_NULL_POINTER     = 3,
    ZRAGF_ST_INTERNAL         = 4,
    ZRAGF_ST_WORKSPACE_TOO_SMALL = 5,
    ZRAGF_ST_INVALID_ARGUMENT = 6,
    ZRAGF_ST_NO_MEMORY        = 7
};

/* Aliases de compatibilidad con nombres antiguos usados en los .c */
#define ZRAGF_ERR_OUTPUT_TOO_SMALL  ZRAGF_ST_OUTPUT_TOO_SMALL
#define ZRAGF_ERR_CORRUPTED_DATA    ZRAGF_ST_CORRUPTED_DATA
#define ZRAGF_ERR_NULL_POINTER      ZRAGF_ST_NULL_POINTER
#define ZRAGF_ERR_INTERNAL          ZRAGF_ST_INTERNAL

/* ------------------------------------------------------------
 * Modos/formatos de compresión
 * ------------------------------------------------------------ */
typedef enum {
    ZRAGF_FMT_ZRAGF = 0,     /* formato nativo */
    ZRAGF_FMT_DEFLATE_RAW,   /* DEFLATE "puro" */
    ZRAGF_FMT_ZLIB_WRAPPED,  /* zlib header + deflate */
    ZRAGF_FMT_GZIP_WRAPPED   /* gzip header + deflate */
} zragf_format;

/* ------------------------------------------------------------
 * Niveles de compresión (tu semántica)
 * ------------------------------------------------------------ */
typedef enum zragf_level_e {
    ZRAGF_LEVEL_FAST    = 1,
    ZRAGF_LEVEL_DEFAULT = 5,
    ZRAGF_LEVEL_MAX     = 9
} zragf_level;

/* ------------------------------------------------------------
 * Flush estilo zlib
 * ------------------------------------------------------------ */
typedef enum {
    ZRAGF_NO_FLUSH      = 0,
    ZRAGF_PARTIAL_FLUSH = 1,
    ZRAGF_SYNC_FLUSH    = 2,
    ZRAGF_FULL_FLUSH    = 3,
    ZRAGF_FINISH        = 4
} zragf_flush;

#define ZRAGF_FLUSH_NONE ZRAGF_NO_FLUSH

/* ------------------------------------------------------------
 * Estrategias compatibles con zlib (subset útil)
 * ------------------------------------------------------------ */
#define ZRAGF_Z_DEFAULT_STRATEGY 0
#define ZRAGF_Z_FILTERED         1
#define ZRAGF_Z_HUFFMAN_ONLY     2
#define ZRAGF_Z_RLE              3
#define ZRAGF_Z_FIXED            4

/* ------------------------------------------------------------
 * Allocator configurable (global / helpers one-shot)
 * ------------------------------------------------------------ */
typedef void *(*zragf_alloc_fn)(void *user, zragf_size_t size);
typedef void *(*zragf_realloc_fn)(void *user, void *ptr, zragf_size_t old_size, zragf_size_t new_size);
typedef void  (*zragf_free_fn)(void *user, void *ptr);

typedef struct zragf_allocator_s {
    zragf_alloc_fn   alloc_fn;
    zragf_realloc_fn realloc_fn;
    zragf_free_fn    free_fn;
    void            *user;
} zragf_allocator;

void zragf_set_default_allocator(const zragf_allocator *allocator);
void zragf_get_default_allocator(zragf_allocator *allocator);

typedef struct zragf_workspace_s {
    zragf_u8    *buffer;
    zragf_size_t size;
    zragf_size_t used;
    zragf_u32    generation;
    int          failed;
} zragf_workspace;

typedef struct zragf_stream_s zragf_stream;

void zragf_workspace_init(zragf_workspace *workspace, void *buffer, zragf_size_t size);
void zragf_workspace_reset(zragf_workspace *workspace);
zragf_size_t zragf_workspace_used(const zragf_workspace *workspace);
int zragf_workspace_failed(const zragf_workspace *workspace);
void zragf_stream_set_workspace(zragf_stream *strm, zragf_workspace *workspace);
zragf_size_t zragf_deflate_workspace_bound_z(unsigned long sourceLen, int windowBits, int memLevel, int strategy);
zragf_size_t zragf_inflate_workspace_bound_z(unsigned long sourceLen, int windowBits);

/* ------------------------------------------------------------
 * Estructura tipo z_stream (drop-in compatible a nivel de campos)
 * ------------------------------------------------------------ */
struct zragf_stream_s {
    zragf_u8    *next_in;
    zragf_size_t avail_in;
    zragf_u32    total_in;

    zragf_u8    *next_out;
    zragf_size_t avail_out;
    zragf_u32    total_out;

    char        *msg;

    void        *state;      /* interno de zragf */

    /* callbacks estilo zlib */
    void *(*zalloc)(void *opaque, unsigned int items, unsigned int size);
    void  (*zfree)(void *opaque, void *address);
    void        *opaque;     /* para usuario */

    int          data_type;  /* best-effort guess / decoder state */
    zragf_u32    adler;      /* Adler-32 o CRC-32 según wrapper */
    zragf_u32    reserved;
};

/* ------------------------------------------------------------
 * Información adicional (para tu modo nativo)
 * ------------------------------------------------------------ */
typedef struct zragf_info_s {
    zragf_u32 uncompressed_size;
    zragf_u32 used_input;
    zragf_u32 used_output;
} zragf_info;

/* ------------------------------------------------------------
 *  gzip header (subset compatible con zlib::gz_header)
 * ------------------------------------------------------------ */
typedef struct zragf_gz_header_s {
    int       text;
    zragf_u32 time;
    int       xflags;
    int       os;
    zragf_u8 *extra;
    unsigned int extra_len;
    unsigned int extra_max;
    zragf_u8 *name;
    unsigned int name_max;
    zragf_u8 *comment;
    unsigned int comm_max;
    int       hcrc;
    int       done;
} zragf_gz_header;

typedef zragf_gz_header *zragf_gz_headerp;

/* ------------------------------------------------------------
 *  API NATIVA
 * ------------------------------------------------------------ */
zragf_status
zragf_compress(const void *in_data,
               zragf_size_t in_size,
               void *out_data,
               zragf_size_t *out_size,
               zragf_level level);

zragf_status
zragf_decompress(const void *in_data,
                 zragf_size_t in_size,
                 void *out_data,
                 zragf_size_t *out_size,
                 zragf_info *info);

/* STREAM NATIVO (buffered) */
zragf_status zragf_deflate_init(zragf_stream *strm, zragf_level level);
zragf_status zragf_deflate(zragf_stream *strm, zragf_flush flush);
zragf_status zragf_deflate_end(zragf_stream *strm);

zragf_status zragf_inflate_init(zragf_stream *strm);
zragf_status zragf_inflate(zragf_stream *strm, zragf_flush flush);
zragf_status zragf_inflate_end(zragf_stream *strm);

/* ------------------------------------------------------------
 *  API COMPATIBLE ZLIB (buffered / baseline)
 * ------------------------------------------------------------ */
const char *zragf_version(void);


#define ZRAGF_Z_BINARY 0
#define ZRAGF_Z_TEXT   1
#define ZRAGF_Z_UNKNOWN 2


int zragf_deflateInit(zragf_stream *strm, int level);
int zragf_deflateInit2(zragf_stream *strm,
                       int level,
                       int method,
                       int windowBits,
                       int memLevel,
                       int strategy);
int zragf_deflateZ(zragf_stream *strm, int flush);
int zragf_deflateEndZ(zragf_stream *strm);
int zragf_deflateReset(zragf_stream *strm);
int zragf_deflatePending(zragf_stream *strm, unsigned *pending, int *bits);
unsigned long zragf_deflateBound(zragf_stream *strm, unsigned long sourceLen);
int zragf_deflateParams(zragf_stream *strm, int level, int strategy);
int zragf_deflateCopy(zragf_stream *dest, zragf_stream *source);
int zragf_deflateTune(zragf_stream *strm, int good_length, int max_lazy, int nice_length, int max_chain);
int zragf_deflateSetDictionary(zragf_stream *strm, const zragf_u8 *dictionary, unsigned int dictLength);
int zragf_deflateSetHeader(zragf_stream *strm, zragf_gz_headerp head);

int zragf_inflateInit(zragf_stream *strm);
int zragf_inflateInit2(zragf_stream *strm, int windowBits);
int zragf_inflateZ(zragf_stream *strm, int flush);
int zragf_inflateEndZ(zragf_stream *strm);
int zragf_inflateReset(zragf_stream *strm);
int zragf_inflateReset2(zragf_stream *strm, int windowBits);
int zragf_inflateCopy(zragf_stream *dest, zragf_stream *source);
int zragf_inflatePrime(zragf_stream *strm, int bits, int value);
int zragf_inflateValidate(zragf_stream *strm, int check);
int zragf_inflateSync(zragf_stream *strm);
int zragf_inflateSetDictionary(zragf_stream *strm, const zragf_u8 *dictionary, unsigned int dictLength);
int zragf_inflateGetHeader(zragf_stream *strm, zragf_gz_headerp head);


/* ------------------------------------------------------------
 * Helpers públicos / diagnóstico
 * ------------------------------------------------------------ */
const char *zragf_strerror(int code);
const char *zragf_build_config(void);
unsigned long zragf_compressBound(unsigned long sourceLen);
int zragf_compress2(zragf_u8 *dest, unsigned long *destLen,
                    const zragf_u8 *source, unsigned long sourceLen, int level);
int zragf_uncompress(zragf_u8 *dest, unsigned long *destLen,
                     const zragf_u8 *source, unsigned long sourceLen);
int zragf_inspect_wrapper(const zragf_u8 *src, zragf_size_t src_size, zragf_format *out_format);

/* ------------------------------------------------------------
 * Workspace fijo (modo nativo one-shot, sin heap si el caller lo da)
 * ------------------------------------------------------------ */
zragf_size_t zragf_compress_workspace_bound(zragf_size_t sourceLen);
zragf_status zragf_decompress_workspace_bound(const void *in_data,
                                             zragf_size_t in_size,
                                             zragf_size_t *needed);
zragf_status zragf_compress_with_workspace(const void *in_data,
                                           zragf_size_t in_size,
                                           void *out_data,
                                           zragf_size_t *out_size,
                                           void *workspace,
                                           zragf_size_t workspace_size,
                                           zragf_level level);
zragf_status zragf_decompress_with_workspace(const void *in_data,
                                             zragf_size_t in_size,
                                             void *out_data,
                                             zragf_size_t *out_size,
                                             void *workspace,
                                             zragf_size_t workspace_size,
                                             zragf_info *info);

#ifdef __cplusplus
}
#endif
#endif /* ZRAGFLIB_H_INCLUDED */

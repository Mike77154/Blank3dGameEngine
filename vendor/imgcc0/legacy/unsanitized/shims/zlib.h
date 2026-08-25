#ifndef IMGCC0_ZLIB_SHIM_H
#define IMGCC0_ZLIB_SHIM_H

#include "zragflib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZLIB_VERSION "1.3.1-zragf-shim"
#define ZLIB_VERNUM 0x1310

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO         (-1)
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define Z_VERSION_ERROR (-6)

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION   (-1)

#define Z_FILTERED         1
#define Z_HUFFMAN_ONLY     2
#define Z_RLE              3
#define Z_FIXED            4
#define Z_DEFAULT_STRATEGY 0

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_UNKNOWN  2

#define Z_DEFLATED 8
#define MAX_WBITS  15
#define Z_NULL     0

typedef void *voidpf;
typedef const void *voidpc;
typedef unsigned char Byte;
typedef Byte Bytef;
typedef char charf;
typedef int intf;
typedef unsigned int uInt;
typedef unsigned long uLong;
typedef uLong uLongf;

typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
typedef void (*free_func)(voidpf opaque, voidpf address);

typedef zragf_stream z_stream;
typedef z_stream *z_streamp;

typedef zragf_gz_header gz_header;
typedef gz_header *gz_headerp;

const char * zlibVersion(void);
const char * zError(int err);

int deflateInit(z_streamp strm, int level);
int deflateInit2(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy);
int deflate(z_streamp strm, int flush);
int deflateEnd(z_streamp strm);
int deflateReset(z_streamp strm);
int deflateParams(z_streamp strm, int level, int strategy);
int deflateTune(z_streamp strm, int good_length, int max_lazy, int nice_length, int max_chain);
int deflateSetDictionary(z_streamp strm, const Bytef *dictionary, uInt dictLength);
int deflateSetHeader(z_streamp strm, gz_headerp head);
uLong deflateBound(z_streamp strm, uLong sourceLen);
int deflatePending(z_streamp strm, unsigned *pending, int *bits);
int deflateCopy(z_streamp dest, z_streamp source);

int inflateInit(z_streamp strm);
int inflateInit2(z_streamp strm, int windowBits);
int inflate(z_streamp strm, int flush);
int inflateEnd(z_streamp strm);
int inflateReset(z_streamp strm);
int inflateReset2(z_streamp strm, int windowBits);
int inflatePrime(z_streamp strm, int bits, int value);
int inflateValidate(z_streamp strm, int check);
int inflateSync(z_streamp strm);
int inflateSetDictionary(z_streamp strm, const Bytef *dictionary, uInt dictLength);
int inflateGetHeader(z_streamp strm, gz_headerp head);
int inflateCopy(z_streamp dest, z_streamp source);

uLong compressBound(uLong sourceLen);
int compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
int uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

uLong crc32(uLong crc, const Bytef *buf, uInt len);
uLong adler32(uLong adler, const Bytef *buf, uInt len);

#ifdef __cplusplus
}
#endif

#endif /* IMGCC0_ZLIB_SHIM_H */

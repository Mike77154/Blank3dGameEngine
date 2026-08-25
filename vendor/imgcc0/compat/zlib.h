#ifndef IMGCC0_ZRAGF_ZLIB_COMPAT89_H
#define IMGCC0_ZRAGF_ZLIB_COMPAT89_H
#include "zragflib.h"

typedef zragf_u8 Byte;
typedef zragf_u8 Bytef;
typedef unsigned int uInt;
typedef unsigned long uLong;
typedef void *voidpf;
typedef const void *voidpc;
typedef zragf_stream z_stream;
typedef zragf_stream *z_streamp;

#define Z_NULL 0
#define Z_OK ZRAGF_OK
#define Z_STREAM_END ZRAGF_STREAM_END
#define Z_NEED_DICT ZRAGF_NEED_DICT
#define Z_ERRNO ZRAGF_ERRNO
#define Z_STREAM_ERROR ZRAGF_STREAM_ERROR
#define Z_DATA_ERROR ZRAGF_DATA_ERROR
#define Z_MEM_ERROR ZRAGF_MEM_ERROR
#define Z_BUF_ERROR ZRAGF_BUF_ERROR
#define Z_VERSION_ERROR ZRAGF_VERSION_ERROR

#define Z_NO_FLUSH ZRAGF_NO_FLUSH
#define Z_PARTIAL_FLUSH ZRAGF_PARTIAL_FLUSH
#define Z_SYNC_FLUSH ZRAGF_SYNC_FLUSH
#define Z_FULL_FLUSH ZRAGF_FULL_FLUSH
#define Z_FINISH ZRAGF_FINISH

#define Z_DEFAULT_COMPRESSION -1
#define Z_DEFAULT_STRATEGY ZRAGF_Z_DEFAULT_STRATEGY
#define Z_FILTERED ZRAGF_Z_FILTERED
#define Z_HUFFMAN_ONLY ZRAGF_Z_HUFFMAN_ONLY
#define Z_RLE ZRAGF_Z_RLE
#define Z_FIXED ZRAGF_Z_FIXED
#define Z_DEFLATED 8
#define MAX_WBITS 15
#define DEF_MEM_LEVEL 8
#define ZLIB_VERNUM 0x12D0

#define inflateInit(strm) zragf_inflateInit((strm))
#define inflateInit2(strm, wb) zragf_inflateInit2((strm), (wb))
#define inflate(strm, flush) zragf_inflateZ((strm), (flush))
#define inflateEnd(strm) zragf_inflateEndZ((strm))
#define inflateReset(strm) zragf_inflateReset((strm))
#define inflateReset2(strm, wb) zragf_inflateReset2((strm), (wb))

#define deflateInit(strm, level) zragf_deflateInit((strm), (level))
#define deflateInit2(strm, level, method, wb, ml, strategy) zragf_deflateInit2((strm), (level), (method), (wb), (ml), (strategy))
#define deflate(strm, flush) zragf_deflateZ((strm), (flush))
#define deflateEnd(strm) zragf_deflateEndZ((strm))
#define deflateReset(strm) zragf_deflateReset((strm))
#define deflateParams(strm, level, strategy) zragf_deflateParams((strm), (level), (strategy))
#define deflateTune(strm, good, lazy, nice, chain) zragf_deflateTune((strm), (good), (lazy), (nice), (chain))
#define deflateBound(strm, len) zragf_deflateBound((strm), (len))
#define compressBound(len) zragf_compressBound((len))

#endif

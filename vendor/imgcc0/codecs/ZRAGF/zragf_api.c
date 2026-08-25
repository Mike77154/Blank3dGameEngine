#include <string.h>

#include "zragflib.h"
#include "zragflib_internal.h"

const char *zragf_strerror(int code)
{
    if (code == ZRAGF_OK || code == ZRAGF_ST_OK)
        return "ok";
    if (code == ZRAGF_STREAM_END)
        return "stream end";
    if (code == ZRAGF_NEED_DICT)
        return "dictionary required";
    if (code == ZRAGF_ERRNO)
        return "errno";
    if (code == ZRAGF_STREAM_ERROR)
        return "stream error";
    if (code == ZRAGF_DATA_ERROR || code == ZRAGF_ST_CORRUPTED_DATA)
        return "corrupted or malformed stream";
    if (code == ZRAGF_MEM_ERROR || code == ZRAGF_ST_NO_MEMORY)
        return "out of memory";
    if (code == ZRAGF_BUF_ERROR || code == ZRAGF_ST_OUTPUT_TOO_SMALL || code == ZRAGF_ST_WORKSPACE_TOO_SMALL)
        return "insufficient output or workspace";
    if (code == ZRAGF_VERSION_ERROR)
        return "version mismatch";
    if (code == ZRAGF_ST_NULL_POINTER)
        return "null pointer";
    if (code == ZRAGF_ST_INTERNAL)
        return "internal error";
    if (code == ZRAGF_ST_INVALID_ARGUMENT)
        return "invalid argument";
    return "unknown zragf status";
}

const char *zragf_build_config(void)
{
    return "zragf/compat-0.70.0;c_standard=c89;memory=static-arena+workspace;workspace=native-oneshot+wrapper-stream;wrappers=raw,zlib,gzip;preset_dict=yes;streaming=yes;c89=library+examples;bench=public-suite+locked-corpora+png-tiff-validation+real-corpus-materialization+optional-miniz";
}

unsigned long zragf_compressBound(unsigned long sourceLen)
{
    zragf_size_t raw_bound;
    raw_bound = zragf_deflate_rfc1951_stored_bound((zragf_size_t)sourceLen);
    if (raw_bound == 0u)
        return sourceLen + 70u;
    raw_bound += 6u + 64u;
    if (raw_bound < (zragf_size_t)sourceLen)
        return sourceLen + 70u;
    return (unsigned long)raw_bound;
}

int zragf_compress2(zragf_u8 *dest, unsigned long *destLen,
                    const zragf_u8 *source, unsigned long sourceLen, int level)
{
    zragf_stream strm;
    int rc;

    if (!dest || !destLen || (!source && sourceLen != 0u))
        return ZRAGF_STREAM_ERROR;

    memset(&strm, 0, sizeof(strm));
    rc = zragf_deflateInit(&strm, level);
    if (rc != ZRAGF_OK)
        return rc;

    strm.next_in = (zragf_u8 *)source;
    strm.avail_in = (zragf_size_t)sourceLen;
    strm.next_out = dest;
    strm.avail_out = (zragf_size_t)(*destLen);

    rc = zragf_deflateZ(&strm, ZRAGF_FINISH);
    if (rc == ZRAGF_STREAM_END) {
        *destLen = (unsigned long)strm.total_out;
        zragf_deflateEndZ(&strm);
        return ZRAGF_OK;
    }
    *destLen = (unsigned long)strm.total_out;
    zragf_deflateEndZ(&strm);
    return rc;
}

int zragf_uncompress(zragf_u8 *dest, unsigned long *destLen,
                     const zragf_u8 *source, unsigned long sourceLen)
{
    zragf_stream strm;
    int rc;

    if (!dest || !destLen || (!source && sourceLen != 0u))
        return ZRAGF_STREAM_ERROR;

    memset(&strm, 0, sizeof(strm));
    rc = zragf_inflateInit(&strm);
    if (rc != ZRAGF_OK)
        return rc;

    strm.next_in = (zragf_u8 *)source;
    strm.avail_in = (zragf_size_t)sourceLen;
    strm.next_out = dest;
    strm.avail_out = (zragf_size_t)(*destLen);

    while (1) {
        rc = zragf_inflateZ(&strm, ZRAGF_FINISH);
        if (rc == ZRAGF_STREAM_END) {
            *destLen = (unsigned long)strm.total_out;
            zragf_inflateEndZ(&strm);
            return ZRAGF_OK;
        }
        if (rc != ZRAGF_OK) {
            *destLen = (unsigned long)strm.total_out;
            zragf_inflateEndZ(&strm);
            return rc;
        }
        if (strm.avail_out == 0u) {
            *destLen = (unsigned long)strm.total_out;
            zragf_inflateEndZ(&strm);
            return ZRAGF_BUF_ERROR;
        }
    }
}

int zragf_inspect_wrapper(const zragf_u8 *src, zragf_size_t src_size, zragf_format *out_format)
{
    zragf_u32 hdr;
    if (!src || !out_format || src_size == 0u)
        return ZRAGF_ST_INVALID_ARGUMENT;

    if (src_size >= 4u) {
        hdr = zragf_read_u32_le(src);
        if (hdr == ((zragf_u32)ZRAGF_MAGIC0 |
                    ((zragf_u32)ZRAGF_MAGIC1 << 8) |
                    ((zragf_u32)ZRAGF_MAGIC2 << 16) |
                    ((zragf_u32)ZRAGF_MAGIC3 << 24))) {
            *out_format = ZRAGF_FMT_ZRAGF;
            return ZRAGF_ST_OK;
        }
    }

    if (src_size >= 2u && src[0] == 0x1fu && src[1] == 0x8bu) {
        *out_format = ZRAGF_FMT_GZIP_WRAPPED;
        return ZRAGF_ST_OK;
    }

    if (src_size >= 2u) {
        unsigned cmf = src[0];
        unsigned flg = src[1];
        if ((cmf & 0x0Fu) == 8u && ((cmf >> 4) <= 7u) && (((cmf << 8) + flg) % 31u) == 0u) {
            *out_format = ZRAGF_FMT_ZLIB_WRAPPED;
            return ZRAGF_ST_OK;
        }
    }

    *out_format = ZRAGF_FMT_DEFLATE_RAW;
    return ZRAGF_ST_OK;
}

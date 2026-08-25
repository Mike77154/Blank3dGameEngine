#include <string.h>
#include "zlib.h"
#include "zragf_crc32.h"

const char * zlibVersion(void)
{
    return ZLIB_VERSION;
}

const char * zError(int err)
{
    return zragf_strerror(err);
}

int deflateInit(z_streamp strm, int level)
{
    return zragf_deflateInit((zragf_stream*)strm, level);
}

int deflateInit2(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy)
{
    return zragf_deflateInit2((zragf_stream*)strm, level, method, windowBits, memLevel, strategy);
}

int deflate(z_streamp strm, int flush)
{
    return zragf_deflateZ((zragf_stream*)strm, flush);
}

int deflateEnd(z_streamp strm)
{
    return zragf_deflateEndZ((zragf_stream*)strm);
}

int deflateReset(z_streamp strm)
{
    return zragf_deflateReset((zragf_stream*)strm);
}

int deflateParams(z_streamp strm, int level, int strategy)
{
    return zragf_deflateParams((zragf_stream*)strm, level, strategy);
}

int deflateTune(z_streamp strm, int good_length, int max_lazy, int nice_length, int max_chain)
{
    return zragf_deflateTune((zragf_stream*)strm, good_length, max_lazy, nice_length, max_chain);
}

int deflateSetDictionary(z_streamp strm, const Bytef *dictionary, uInt dictLength)
{
    return zragf_deflateSetDictionary((zragf_stream*)strm, (const zragf_u8*)dictionary, dictLength);
}

int deflateSetHeader(z_streamp strm, gz_headerp head)
{
    return zragf_deflateSetHeader((zragf_stream*)strm, (zragf_gz_headerp)head);
}

uLong deflateBound(z_streamp strm, uLong sourceLen)
{
    return zragf_deflateBound((zragf_stream*)strm, sourceLen);
}

int deflatePending(z_streamp strm, unsigned *pending, int *bits)
{
    return zragf_deflatePending((zragf_stream*)strm, pending, bits);
}

int deflateCopy(z_streamp dest, z_streamp source)
{
    return zragf_deflateCopy((zragf_stream*)dest, (zragf_stream*)source);
}

int inflateInit(z_streamp strm)
{
    return zragf_inflateInit((zragf_stream*)strm);
}

int inflateInit2(z_streamp strm, int windowBits)
{
    return zragf_inflateInit2((zragf_stream*)strm, windowBits);
}

int inflate(z_streamp strm, int flush)
{
    return zragf_inflateZ((zragf_stream*)strm, flush);
}

int inflateEnd(z_streamp strm)
{
    return zragf_inflateEndZ((zragf_stream*)strm);
}

int inflateReset(z_streamp strm)
{
    return zragf_inflateReset((zragf_stream*)strm);
}

int inflateReset2(z_streamp strm, int windowBits)
{
    return zragf_inflateReset2((zragf_stream*)strm, windowBits);
}

int inflatePrime(z_streamp strm, int bits, int value)
{
    return zragf_inflatePrime((zragf_stream*)strm, bits, value);
}

int inflateValidate(z_streamp strm, int check)
{
    return zragf_inflateValidate((zragf_stream*)strm, check);
}

int inflateSync(z_streamp strm)
{
    return zragf_inflateSync((zragf_stream*)strm);
}

int inflateSetDictionary(z_streamp strm, const Bytef *dictionary, uInt dictLength)
{
    return zragf_inflateSetDictionary((zragf_stream*)strm, (const zragf_u8*)dictionary, dictLength);
}

int inflateGetHeader(z_streamp strm, gz_headerp head)
{
    return zragf_inflateGetHeader((zragf_stream*)strm, (zragf_gz_headerp)head);
}

int inflateCopy(z_streamp dest, z_streamp source)
{
    return zragf_inflateCopy((zragf_stream*)dest, (zragf_stream*)source);
}

uLong compressBound(uLong sourceLen)
{
    return zragf_compressBound(sourceLen);
}

int compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
    return zragf_compress2((zragf_u8*)dest, (unsigned long*)destLen, (const zragf_u8*)source, sourceLen, level);
}

int uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    z_stream strm;
    int rc;
    Bytef dummy;
    uLongf out_len;

    if (!dest || !destLen || (!source && sourceLen != 0UL)) {
        return Z_STREAM_ERROR;
    }

    memset(&strm, 0, sizeof(strm));
    rc = inflateInit(&strm);
    if (rc != Z_OK) {
        return rc;
    }

    strm.next_in = (Bytef*)source;
    strm.avail_in = (uInt)sourceLen;
    strm.next_out = dest;
    strm.avail_out = (uInt)(*destLen);

    for (;;) {
        rc = inflate(&strm, Z_FINISH);
        if (rc == Z_STREAM_END) {
            out_len = (uLongf)strm.total_out;
            inflateEnd(&strm);
            *destLen = out_len;
            return Z_OK;
        }
        if (rc != Z_OK && rc != Z_BUF_ERROR) {
            out_len = (uLongf)strm.total_out;
            inflateEnd(&strm);
            *destLen = out_len;
            return rc;
        }

        if (strm.avail_out == 0U) {
            /* Exact-fit streams should still succeed: probe once more with a
               one-byte scratch output to see whether the wrapped stream had
               already ended without producing more bytes. */
            strm.next_out = &dummy;
            strm.avail_out = 1U;
            rc = inflate(&strm, Z_FINISH);
            if (rc == Z_STREAM_END && strm.avail_out == 1U) {
                out_len = (uLongf)strm.total_out;
                inflateEnd(&strm);
                *destLen = out_len;
                return Z_OK;
            }
            out_len = (uLongf)strm.total_out;
            inflateEnd(&strm);
            *destLen = out_len;
            return Z_BUF_ERROR;
        }

        if (strm.avail_in == 0U && rc == Z_BUF_ERROR) {
            out_len = (uLongf)strm.total_out;
            inflateEnd(&strm);
            *destLen = out_len;
            return Z_DATA_ERROR;
        }
    }
}

uLong crc32(uLong crc, const Bytef *buf, uInt len)
{
    zragf_u32 c;
    c = (zragf_u32)crc;
    if (buf == 0) {
        return 0UL;
    }
    /* zragf_crc32() is one-shot; emulate incremental usage by seeding with prior CRC when it is zero only. */
    if (c == 0UL) {
        return (uLong)zragf_crc32((const zragf_u8*)buf, (zragf_size_t)len);
    }
    {
        zragf_u32 poly = 0xEDB88320UL;
        zragf_size_t i;
        c = c ^ 0xFFFFFFFFUL;
        for (i = 0; i < (zragf_size_t)len; ++i) {
            zragf_u32 j;
            c ^= (zragf_u32)buf[i];
            for (j = 0; j < 8U; ++j) {
                c = (c >> 1) ^ ((c & 1U) ? poly : 0U);
            }
        }
        return (uLong)(c ^ 0xFFFFFFFFUL);
    }
}

uLong adler32(uLong adler, const Bytef *buf, uInt len)
{
    unsigned long s1;
    unsigned long s2;
    unsigned long n;
    if (buf == 0) {
        return 1UL;
    }
    s1 = adler & 0xFFFFUL;
    s2 = (adler >> 16) & 0xFFFFUL;
    for (n = 0; n < (unsigned long)len; ++n) {
        s1 += (unsigned long)buf[n];
        if (s1 >= 65521UL) s1 -= 65521UL;
        s2 += s1;
        if (s2 >= 65521UL) s2 %= 65521UL;
    }
    return (s2 << 16) | s1;
}

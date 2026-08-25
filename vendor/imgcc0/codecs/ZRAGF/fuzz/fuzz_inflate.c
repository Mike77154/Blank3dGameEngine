#include <stddef.h>
#include <string.h>

#include "zragflib.h"

static void zragf_fuzz_exercise_wrapper(const unsigned char *data,
                                        size_t size,
                                        int window_bits,
                                        int want_header,
                                        int validate_checks)
{
    zragf_stream is;
    zragf_gz_header hdr;
    unsigned char out[16384];
    size_t in_pos = 0u;
    unsigned steps = 0u;
    int rc;

    memset(&is, 0, sizeof(is));
    memset(&hdr, 0, sizeof(hdr));

    if (zragf_inflateInit2(&is, window_bits) != ZRAGF_OK)
        return;

    if (want_header)
        (void)zragf_inflateGetHeader(&is, &hdr);
    (void)zragf_inflateValidate(&is, validate_checks ? 1 : 0);

    while (steps++ < 512u) {
        size_t in_chunk;
        size_t out_chunk;

        if (in_pos < size) {
            in_chunk = 1u + ((data[(in_pos + steps) % size] ^ (unsigned char)steps) & 0x1Fu);
            if (in_chunk > size - in_pos)
                in_chunk = size - in_pos;
            is.next_in = (zragf_u8 *)(data + in_pos);
            is.avail_in = in_chunk;
        } else {
            is.next_in = NULL;
            is.avail_in = 0u;
        }

        out_chunk = 1u + ((steps * 13u) & 0x7Fu);
        if (out_chunk > sizeof(out))
            out_chunk = sizeof(out);
        memset(out, 0, out_chunk);
        is.next_out = out;
        is.avail_out = out_chunk;

        rc = zragf_inflateZ(&is, (in_pos >= size) ? ZRAGF_FINISH : ZRAGF_NO_FLUSH);
        in_pos += in_chunk;

        if (rc == ZRAGF_STREAM_END || rc == ZRAGF_DATA_ERROR ||
            rc == ZRAGF_MEM_ERROR || rc == ZRAGF_STREAM_ERROR)
            break;

        if (rc == ZRAGF_NEED_DICT) {
            zragf_u8 dict_buf[32];
            unsigned dict_len = (unsigned)((size > sizeof(dict_buf)) ? sizeof(dict_buf) : size);
            if (dict_len > 0u)
                memcpy(dict_buf, data, dict_len);
            (void)zragf_inflateSetDictionary(&is, dict_buf, dict_len);
        } else if (rc == ZRAGF_BUF_ERROR && in_pos >= size) {
            (void)zragf_inflateSync(&is);
        }
    }

    (void)zragf_inflateEndZ(&is);
}

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    unsigned char out[8192];
    zragf_size_t out_size;
    zragf_info info;

    out_size = sizeof(out);
    (void)zragf_decompress(data, size, out, &out_size, &info);

    zragf_fuzz_exercise_wrapper(data, size, -15, 0, 1);
    zragf_fuzz_exercise_wrapper(data, size, 15, 0, 1);
    zragf_fuzz_exercise_wrapper(data, size, 31, 1, 1);
    zragf_fuzz_exercise_wrapper(data, size, 47, 1, 1);
    zragf_fuzz_exercise_wrapper(data, size, 47, 1, 0);

    return 0;
}

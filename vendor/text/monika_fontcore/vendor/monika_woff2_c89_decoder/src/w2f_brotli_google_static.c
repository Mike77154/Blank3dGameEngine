#include "w2f.h"

#ifdef W2F_USE_GOOGLE_BROTLI

#include <stddef.h>
#include <brotli/decode.h>

static W2F_U8 w2f_brotli_arena[W2F_BROTLI_ARENA_SIZE];
static W2F_U32 w2f_brotli_arena_pos = 0u;

typedef struct W2F_BrotliArenaMark_ {
    W2F_U32 mark;
} W2F_BrotliArenaMark;

static void *w2f_brotli_alloc(void *opaque, size_t size)
{
    W2F_U32 aligned;
    W2F_U32 new_pos;
    (void)opaque;
    if (size == 0u) return 0;
    aligned = (W2F_U32)((size + 7u) & ~((size_t)7u));
    new_pos = w2f_brotli_arena_pos + aligned;
    if (new_pos < w2f_brotli_arena_pos) return 0;
    if (new_pos > (W2F_U32)W2F_BROTLI_ARENA_SIZE) return 0;
    {
        void *p = (void *)(w2f_brotli_arena + w2f_brotli_arena_pos);
        w2f_brotli_arena_pos = new_pos;
        return p;
    }
}

static void w2f_brotli_release(void *opaque, void *address)
{
    (void)opaque;
    (void)address;
}

int w2f_brotli_google_static_decode(const W2F_U8 *src,
                                    W2F_U32 src_len,
                                    W2F_U8 *dst,
                                    W2F_U32 dst_cap,
                                    W2F_U32 *dst_len)
{
    BrotliDecoderState *st;
    BrotliDecoderResult br;
    const uint8_t *next_in;
    uint8_t *next_out;
    size_t avail_in;
    size_t avail_out;
    size_t total_out;

    if (src == 0 || dst == 0 || dst_len == 0) return W2F_ERR_NULL_ARG;
    *dst_len = 0u;
    w2f_brotli_arena_pos = 0u;

    st = BrotliDecoderCreateInstance(w2f_brotli_alloc, w2f_brotli_release, 0);
    if (st == 0) return W2F_ERR_BROTLI_FAILED;

    next_in = (const uint8_t *)src;
    avail_in = (size_t)src_len;
    next_out = (uint8_t *)dst;
    avail_out = (size_t)dst_cap;
    total_out = 0u;

    for (;;) {
        br = BrotliDecoderDecompressStream(st, &avail_in, &next_in, &avail_out, &next_out, &total_out);
        if (br == BROTLI_DECODER_RESULT_SUCCESS) break;
        if (br == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            BrotliDecoderDestroyInstance(st);
            return W2F_ERR_BROTLI_FAILED;
        }
        if (br == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            BrotliDecoderDestroyInstance(st);
            return W2F_ERR_SCRATCH_TOO_SMALL;
        }
        if (br == BROTLI_DECODER_RESULT_ERROR) {
            BrotliDecoderDestroyInstance(st);
            return W2F_ERR_BROTLI_FAILED;
        }
    }

    BrotliDecoderDestroyInstance(st);
    if (total_out > (size_t)0xfffffffful) return W2F_ERR_SIZE_OVERFLOW;
    *dst_len = (W2F_U32)total_out;
    return W2F_OK;
}

#endif

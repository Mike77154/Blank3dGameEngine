#ifndef ZRAGF_DEFLATE_HUFFMAN_SHARED_H_INCLUDED
#define ZRAGF_DEFLATE_HUFFMAN_SHARED_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_blocks.h"

#define ZRAGF_DEFLATE_CL_TOKEN_CAP 1024

typedef struct {
    unsigned code;
    int      len;
} zragf_deflate_huff_code;

typedef struct {
    int      sym;
    int      extra_bits;
    unsigned extra_val;
} zragf_deflate_cl_token;

extern const int zragf_deflate_cl_order[19];


unsigned zragf_deflate_huff_reverse_bits(unsigned code, int bits);

void zragf_deflate_huff_reverse_codes(zragf_deflate_huff_code *codes,
                                      int count);

typedef struct {
    int ll_len[286];
    int d_len[30];
    int cl_len[19];
    zragf_deflate_huff_code ll_codes[286];
    zragf_deflate_huff_code d_codes[30];
    zragf_deflate_huff_code cl_codes[19];
    zragf_deflate_cl_token rle[ZRAGF_DEFLATE_CL_TOKEN_CAP];
    zragf_size_t rle_count;
    int hlit;
    int hdist;
    int hclen;
    unsigned header_bits;
    int valid;
} zragf_deflate_dynamic_prepared;

int zragf_deflate_prepare_dynamic(const zragf_block_stats *stats,
                                  zragf_deflate_dynamic_prepared *out);
void zragf_deflate_huff_build_lengths(const unsigned *freq,
                                      int count,
                                      int maxbits,
                                      int *out_len);

int zragf_deflate_huff_lengths_oversubscribed(const int *lens,
                                              int count,
                                              int maxbits);

void zragf_deflate_huff_make_codes(const int *lens,
                                   int count,
                                   zragf_deflate_huff_code *out);

int zragf_deflate_huff_rle_code_lengths(const int *ll_len,
                                        int ll_count,
                                        const int *d_len,
                                        int d_count,
                                        zragf_deflate_cl_token *out,
                                        zragf_size_t out_cap,
                                        zragf_size_t *out_count);

void zragf_deflate_huff_trim_counts(const int *ll_len,
                                    const int *d_len,
                                    int *out_ll_count,
                                    int *out_d_count,
                                    int *out_hclen_count);

#endif

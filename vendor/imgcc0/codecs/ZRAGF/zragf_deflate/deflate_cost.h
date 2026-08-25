#ifndef ZRAGF_DEFLATE_COST_H_INCLUDED
#define ZRAGF_DEFLATE_COST_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_blocks.h"
#include "deflate_tokens.h"
#include "deflate_huffman_shared.h"

typedef struct {
    zragf_size_t size_bytes;
    int          end_bitcount;
} zragf_deflate_cost_result;

int zragf_deflate_cost_stored(const zragf_u8 *src,
                              zragf_size_t src_size,
                              int final_block,
                              int start_bitcount,
                              int flush_final_bits,
                              zragf_deflate_cost_result *out);

int zragf_deflate_cost_fixed(const zragf_token *toks,
                             zragf_size_t ntoks,
                             int final_block,
                             int start_bitcount,
                             int flush_final_bits,
                             zragf_deflate_cost_result *out);


int zragf_deflate_cost_dynamic_prepared(const zragf_token *toks,
                                        zragf_size_t ntoks,
                                        const zragf_deflate_dynamic_prepared *prep,
                                        int final_block,
                                        int start_bitcount,
                                        int flush_final_bits,
                                        zragf_deflate_cost_result *out);

int zragf_deflate_cost_dynamic(const zragf_token *toks,
                               zragf_size_t ntoks,
                               const zragf_block_stats *stats,
                               int final_block,
                               int start_bitcount,
                               int flush_final_bits,
                               zragf_deflate_cost_result *out);

#endif

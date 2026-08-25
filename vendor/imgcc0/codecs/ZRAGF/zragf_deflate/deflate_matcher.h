#ifndef ZRAGF_DEFLATE_MATCHER_H_INCLUDED
#define ZRAGF_DEFLATE_MATCHER_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_tokens.h"
#include "deflate_blocks.h"

typedef struct
{
    int level;
    int strategy;
    int tune_set;
    int good_length;
    int max_lazy;
    int nice_length;
    int max_chain;
} zragf_matcher_config;

int zragf_deflate_build_tokens(const zragf_u8 *dict,
                               zragf_size_t    dict_size,
                               const zragf_u8 *src,
                               zragf_size_t    src_size,
                               const zragf_matcher_config *cfg,
                               zragf_token_buffer *tb,
                               zragf_block_stats  *stats);

#endif /* ZRAGF_DEFLATE_MATCHER_H_INCLUDED */

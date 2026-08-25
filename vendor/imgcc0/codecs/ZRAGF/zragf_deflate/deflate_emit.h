#ifndef ZRAGF_DEFLATE_EMIT_H_INCLUDED
#define ZRAGF_DEFLATE_EMIT_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_blocks.h"
#include "deflate_tokens.h"
#include "deflate_huffman_shared.h"

int zragf_deflate_emit_stored_chunk(const zragf_u8 *src,
                                    zragf_size_t    src_size,
                                    zragf_u8       *dst,
                                    zragf_size_t    dst_cap,
                                    zragf_size_t   *dst_size,
                                    int             final_block,
                                    unsigned       *bitbuf_io,
                                    int            *bitcount_io);

int zragf_deflate_emit_fixed_block(const zragf_token *toks,
                                   zragf_size_t      ntoks,
                                   zragf_u8         *dst,
                                   zragf_size_t      dst_cap,
                                   zragf_size_t     *dst_size,
                                   int               final_block,
                                   unsigned         *bitbuf_io,
                                   int              *bitcount_io,
                                   int               flush_final_bits);


int zragf_deflate_emit_dynamic_block_prepared(const zragf_token *toks,
                                              zragf_size_t ntoks,
                                              const zragf_deflate_dynamic_prepared *prep,
                                              zragf_u8 *dst,
                                              zragf_size_t dst_cap,
                                              zragf_size_t *dst_size,
                                              int final_block,
                                              unsigned *bitbuf_io,
                                              int *bitcount_io,
                                              int flush_final_bits);

int zragf_deflate_emit_dynamic_block(const zragf_token      *toks,
                                     zragf_size_t           ntoks,
                                     const zragf_block_stats *stats,
                                     zragf_u8              *dst,
                                     zragf_size_t           dst_cap,
                                     zragf_size_t          *dst_size,
                                     int                    final_block,
                                     unsigned              *bitbuf_io,
                                     int                   *bitcount_io,
                                     int                    flush_final_bits);

#endif

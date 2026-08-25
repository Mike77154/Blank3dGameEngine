#ifndef ZRAGF_DEFLATE_STREAM_H_INCLUDED
#define ZRAGF_DEFLATE_STREAM_H_INCLUDED

#include "../zragflib_internal.h"
#include "deflate_core.h"

#define ZRAGF_DEFLATE_STREAM_HISTORY 32768u

typedef struct
{
    zragf_deflate_params params;

    zragf_u8     *out;
    zragf_size_t  out_pos;
    zragf_size_t  out_cap;

    unsigned bitbuf;
    int      bitcount;

    zragf_u8     history[ZRAGF_DEFLATE_STREAM_HISTORY];
    zragf_size_t history_len;

    int tune_set;
    int good_length;
    int max_lazy;
    int nice_length;
    int max_chain;

    int finished;
} zragf_deflate_stream;

int zragf_deflate_stream_init(zragf_deflate_stream       *ds,
                              const zragf_deflate_params *params,
                              zragf_u8                   *out,
                              zragf_size_t                out_cap);

int zragf_deflate_stream_write_block(zragf_deflate_stream *ds,
                                     const zragf_u8       *raw,
                                     zragf_size_t          raw_size,
                                     int                   final_block);

int zragf_deflate_stream_flush(zragf_deflate_stream *ds);

zragf_size_t zragf_deflate_stream_size(const zragf_deflate_stream *ds);

#endif

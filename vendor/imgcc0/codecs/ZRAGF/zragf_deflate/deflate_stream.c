#include "deflate_stream.h"
#include <string.h>

static void zragf_deflate_stream_update_history(zragf_deflate_stream *ds,
                                                const zragf_u8 *raw,
                                                zragf_size_t raw_size)
{
    zragf_size_t keep;

    if (!ds || !raw || raw_size == 0u)
        return;

    if (raw_size >= ZRAGF_DEFLATE_STREAM_HISTORY) {
        memcpy(ds->history,
               raw + (raw_size - ZRAGF_DEFLATE_STREAM_HISTORY),
               ZRAGF_DEFLATE_STREAM_HISTORY);
        ds->history_len = ZRAGF_DEFLATE_STREAM_HISTORY;
        return;
    }

    keep = ds->history_len;
    if (keep + raw_size > ZRAGF_DEFLATE_STREAM_HISTORY) {
        zragf_size_t drop = keep + raw_size - ZRAGF_DEFLATE_STREAM_HISTORY;
        memmove(ds->history, ds->history + drop, keep - drop);
        keep -= drop;
        ds->history_len = keep;
    }

    memcpy(ds->history + ds->history_len, raw, raw_size);
    ds->history_len += raw_size;
}

int zragf_deflate_stream_init(zragf_deflate_stream       *ds,
                              const zragf_deflate_params *params,
                              zragf_u8                   *out,
                              zragf_size_t                out_cap)
{
    if (!ds || !params || !out)
        return 0;

    memset(ds, 0, sizeof(*ds));
    ds->params = *params;
    ds->out = out;
    ds->out_cap = out_cap;
    ds->out_pos = 0u;
    ds->bitbuf = 0u;
    ds->bitcount = 0;
    ds->history_len = 0u;
    ds->tune_set = 0;
    ds->good_length = 0;
    ds->max_lazy = 0;
    ds->nice_length = 0;
    ds->max_chain = 0;
    ds->finished = 0;
    return 1;
}

int zragf_deflate_stream_write_block(zragf_deflate_stream *ds,
                                     const zragf_u8       *raw,
                                     zragf_size_t          raw_size,
                                     int                   final_block)
{
    zragf_size_t wrote = 0u;
    if (!ds || (!raw && raw_size > 0u) || !ds->out)
        return 0;
    if (ds->finished)
        return 0;
    if (ds->out_pos > ds->out_cap)
        return 0;

    if (!zragf_deflate_rfc1951_compress_chunk_stream(
            (ds->history_len > 0u) ? ds->history : NULL,
            ds->history_len,
            raw,
            raw_size,
            ds->out + ds->out_pos,
            ds->out_cap - ds->out_pos,
            &wrote,
            final_block ? 1 : 0,
            ds->params.level,
            ds->params.strategy,
            ds->tune_set,
            ds->good_length,
            ds->max_lazy,
            ds->nice_length,
            ds->max_chain,
            &ds->bitbuf,
            &ds->bitcount,
            final_block ? 1 : 0))
        return 0;

    ds->out_pos += wrote;
    zragf_deflate_stream_update_history(ds, raw, raw_size);
    if (final_block)
        ds->finished = 1;
    return 1;
}

int zragf_deflate_stream_flush(zragf_deflate_stream *ds)
{
    if (!ds)
        return 0;
    return (ds->bitcount == 0) ? 1 : 0;
}

zragf_size_t zragf_deflate_stream_size(const zragf_deflate_stream *ds)
{
    if (!ds)
        return 0u;
    return ds->out_pos;
}

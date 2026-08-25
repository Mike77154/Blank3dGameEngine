#include "inflate_stream.h"
#include <string.h>

/* ------------------------------------------------------------
   Inicialización del stream de inflate
   ------------------------------------------------------------ */
int zragf_inflate_stream_init(zragf_inflate_stream *s,
                              zragf_inflate_state_z *zs,
                              int window_bits)
{
    if (!s || !zs)
        return 0;

    memset(s, 0, sizeof(*s));

    s->zs = zs;

    if (!zragf_inflate_core_init(&s->core, zs, window_bits))
        return 0;

    s->initialized = 1;
    return 1;
}

/* ------------------------------------------------------------
   Descompresión completa del stream
   ------------------------------------------------------------ */
zragf_inflate_status
zragf_inflate_stream_decompress(zragf_inflate_stream *s,
                                zragf_u8             *out,
                                zragf_size_t         *out_size)
{
    zragf_inflate_status st;

    if (!s || !s->initialized || !out || !out_size)
        return ZRAGF_INF_STATUS_ERROR;

    st = zragf_inflate_core_decompress(&s->core, out, out_size);
    return st;
}

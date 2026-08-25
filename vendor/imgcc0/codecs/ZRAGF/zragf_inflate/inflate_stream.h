#ifndef ZRAGF_INFLATE_STREAM_H_INCLUDED
#define ZRAGF_INFLATE_STREAM_H_INCLUDED

#include "../zragflib_internal.h"
#include "inflate_core.h"

/* ------------------------------------------------------------
   Stream wrapper para el motor DEFLATE (inflate)
   ------------------------------------------------------------ */
typedef struct
{
    zragf_inflate_state_z *zs;   /* estado interno de zragf */
    zragf_inflate_core     core; /* motor RFC1951 */
    int                    initialized;

} zragf_inflate_stream;

/* ------------------------------------------------------------
   Inicializa el stream de inflate.

   - 'zs' debe estar creado por tu capa superior
     (p.ej. zragf_inflateInit2) y tener:
       * window / window_size
       * in_buf / in_size (datos DEFLATE presentes)
   - window_bits normalmente 15.
   ------------------------------------------------------------ */
int zragf_inflate_stream_init(zragf_inflate_stream *s,
                              zragf_inflate_state_z *zs,
                              int window_bits);

/* ------------------------------------------------------------
   Descomprime el stream completo disponible en zs->in_buf
   hacia 'out'.

   Entradas:
     s        = stream inicializado
     out      = buffer de salida
     out_size = entrada: tamaño del buffer
                salida : bytes escritos

   Devuelve:
     ZRAGF_INF_STATUS_STREAM_END en éxito,
     ZRAGF_INF_STATUS_ERROR en fallo.
   ------------------------------------------------------------ */
zragf_inflate_status
zragf_inflate_stream_decompress(zragf_inflate_stream *s,
                                zragf_u8             *out,
                                zragf_size_t         *out_size);

#endif /* ZRAGF_INFLATE_STREAM_H_INCLUDED */

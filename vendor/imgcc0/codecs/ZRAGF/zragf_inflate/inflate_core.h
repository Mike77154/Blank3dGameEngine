#ifndef ZRAGF_INFLATE_CORE_H_INCLUDED
#define ZRAGF_INFLATE_CORE_H_INCLUDED

#include "../zragflib_internal.h"
#include "inflate_huffman.h"
#include "inflate_shared.h"

/* ------------------------------------------------------------
   Estados de retorno del core de inflate
   ------------------------------------------------------------ */
typedef enum
{
    ZRAGF_INF_STATUS_OK         = 0,
    ZRAGF_INF_STATUS_STREAM_END = 1,
    ZRAGF_INF_STATUS_ERROR      = -1
} zragf_inflate_status;

/* ------------------------------------------------------------
   Estado principal del motor DEFLATE (inflate)
   ------------------------------------------------------------ */
typedef struct
{
    zragf_inflate_state_z *zs;   /* estado de entrada / bitreader */

    zragf_inflate_huff     huff; /* tablas lit/len + dist */

    zragf_u8  *window;           /* ventana circular (compartida con zs) */
    int        window_size;
    int        wpos;
    int        dist_table_dummy;

} zragf_inflate_core;

/* ------------------------------------------------------------
   Inicializa el motor DEFLATE para inflate.

   - 'zs' debe tener:
       * in_buf / in_size con el stream DEFLATE completo
       * window ya alocada (p.ej. desde inflateInit2)
   - 'window_bits' normalmente 15 (ventana 32 KB).
   ------------------------------------------------------------ */
int zragf_inflate_core_init(zragf_inflate_core   *core,
                            zragf_inflate_state_z *zs,
                            int window_bits);

/* ------------------------------------------------------------
   Descomprime un stream DEFLATE completo conforme a RFC1951.

   Entradas:
     core     = estado inicializado
     out      = buffer de salida
     out_size = entrada: tamaño del buffer
                salida : bytes realmente escritos

   Devuelve:
     ZRAGF_INF_STATUS_STREAM_END si la descompresión terminó bien,
     ZRAGF_INF_STATUS_ERROR en caso de error de datos o buffer.
   ------------------------------------------------------------ */
zragf_inflate_status
zragf_inflate_core_decompress(zragf_inflate_core *core,
                              zragf_u8           *out,
                              zragf_size_t       *out_size);

#endif /* ZRAGF_INFLATE_CORE_H_INCLUDED */

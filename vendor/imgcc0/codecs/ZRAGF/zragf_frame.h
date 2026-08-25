/* zragf_frame.h - manejo de header/frame de zragflib */

#ifndef ZRAGF_FRAME_H_INCLUDED
#define ZRAGF_FRAME_H_INCLUDED

#include "zragflib_internal.h"

/* Escribe el header del frame.
 * out: buffer de salida
 * out_cap: capacidad total
 * uncompressed_size: tamaño original
 * out_pos: (out) offset después del header
 */
zragf_status
zragf_frame_write_header(zragf_u8 *out,
                         zragf_size_t out_cap,
                         zragf_u32 uncompressed_size,
                         zragf_u8 flags,
                         zragf_size_t *out_pos);


/* Lee el header del frame.
 * in: buffer de entrada
 * in_size: tamaño total de entrada
 * out_uncompressed_size: (out) tamaño esperado
 * in_pos: (out) offset tras el header
 */
zragf_status
zragf_frame_read_header(const zragf_u8 *in,
                        zragf_size_t in_size,
                        zragf_u32 *out_uncompressed_size,
                        zragf_u8 *out_flags,
                        zragf_size_t *in_pos);

#endif /* ZRAGF_FRAME_H_INCLUDED */

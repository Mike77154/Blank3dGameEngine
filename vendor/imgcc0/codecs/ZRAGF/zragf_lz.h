/* zragf_lz.h - núcleo LZ (búsqueda de matches) */

#ifndef ZRAGF_LZ_H_INCLUDED
#define ZRAGF_LZ_H_INCLUDED

#include "zragflib_internal.h"

/*
 * Implementación de búsqueda de matches LZ basada en hash-chain.
 *
 * - Se mantiene la API pública original zragf_find_match().
 * - Internamente se utiliza una tabla hash y listas enlazadas para
 *   acelerar la búsqueda de coincidencias en la ventana.
 *
 * Nota: el estado de hash se maneja de forma interna y se resetea
 *       cuando cambia (base, size) o cuando pos == 0.
 */

#define ZRAGF_LZ_HASH_BITS 15u
#define ZRAGF_LZ_HASH_SIZE (1u << ZRAGF_LZ_HASH_BITS)
#define ZRAGF_LZ_HASH_MASK (ZRAGF_LZ_HASH_SIZE - 1u)

/* Encuentra el mejor match en la ventana [pos - max_window, pos)
 * base: buffer de entrada completo
 * pos: posición actual
 * size: tamaño total
 * out_offset: (out) offset hacia atrás
 * max_window: tamaño máximo de ventana
 * max_len: longitud máxima permitida
 *
 * Retorna longitud del mejor match o 0 si no hay match >= ZRAGF_MIN_MATCH.
 */
zragf_u32
zragf_find_match(const zragf_u8 *base,
                 zragf_u32 pos,
                 zragf_u32 size,
                 zragf_u32 *out_offset,
                 zragf_u32 max_window,
                 zragf_u32 max_len);

#endif /* ZRAGF_LZ_H_INCLUDED */

/* pcx_render.h - paletas y conversión de buffers a RGB */

#ifndef PCX_RENDER_H
#define PCX_RENDER_H

#include "pcx_chunk.h"
#include "pcx_parser.h"

#include "include/pcx/pcx_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Carga o construye la paleta adecuada para una imagen indexada.
 *
 * Reglas:
 *  - Si el archivo es 8bpp indexado y existe trailer VGA (0x0C + 768 bytes),
 *    se usa dicho trailer.
 *  - Si no existe trailer y paletteInfo == 2, se construye una rampa gris.
 *  - En cualquier otro caso se toma la paleta de 16 colores de cabecera y se
 *    replica hasta 256 entradas.
 *
 * Requiere un FILE* seekable porque consulta el final del archivo.
 *
 * El cuarto argumento indica la posición mínima válida a partir de la cual
 * puede empezar un trailer VGA. Normalmente se pasa la posición actual tras
 * terminar de decodificar la imagen para evitar falsos positivos cuando un
 * 0x0C aparece dentro de los datos codificados.
 */
PCX_EXPORT int pcx_load_palette(FILE *f,
                     const PCXHeader *hdr,
                     PCXPalette *pal,
                     pcx_off minimumPaletteOffset);
PCX_EXPORT int pcx_load_palette_ex(FILE *f,
                        const PCXHeader *hdr,
                        PCXPalette *pal,
                        pcx_off minimumPaletteOffset,
                        PCXDiagnostics *outDiag);

/* Convierte un buffer indexado completo a RGB. */
PCX_EXPORT int pcx_indexed_to_rgb(const pcx_u8 *indices,
                       const PCXPalette *pal,
                       int width,
                       int height,
                       pcx_u8 *outRGB);

/* Convierte una sola fila indexada a RGB. */
PCX_EXPORT int pcx_indexed_row_to_rgb(const pcx_u8 *indices,
                           const PCXPalette *pal,
                           int width,
                           pcx_u8 *outRGB);

/* Convierte un buffer planar 24bpp (R,G,B por scanline) a RGB empaquetado. */
PCX_EXPORT int pcx_planar24_to_rgb(const pcx_u8 *planar,
                        int width,
                        int height,
                        int bytesPerLine,
                        pcx_u8 *outRGB);

/* Helpers públicos útiles para callers avanzados. */
PCX_EXPORT int pcx_palette_build_from_header16(const PCXHeader *hdr, PCXPalette *pal);
PCX_EXPORT int pcx_palette_build_grayscale256(PCXPalette *pal);

#ifdef __cplusplus
}
#endif

#endif /* PCX_RENDER_H */

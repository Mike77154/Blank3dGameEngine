/* pcx_parser.h - lectura y validación de cabecera PCX */

#ifndef PCX_PARSER_H
#define PCX_PARSER_H

#include "pcx_chunk.h"

#include "include/pcx/pcx_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PCXParseFlagsTag
{
    PCX_PARSE_FLAG_NONE   = 0,
    PCX_PARSE_FLAG_STRICT = 1
} PCXParseFlags;

/* Lee y valida la cabecera de 128 bytes.
 *
 PCX_EXPORT * Por compatibilidad, pcx_parse_header() usa modo tolerante: acepta
 * encoding=0 (raw) y no fuerza reglas históricas como versiones conocidas,
 * reserved=0 o bytesPerLine par.
 */
PCX_EXPORT int pcx_parse_header(FILE *f, PCXHeader *hdr, int *outWidth, int *outHeight);

/* Variante con flags. En modo estricto aplica validaciones históricas:
 *  - version en {0,2,3,4,5}
 *  - encoding = 1
 *  - reserved = 0
 *  - bytesPerLine par
 *  - combinaciones de layout compatibles con modos PCX clásicos soportados
 */
PCX_EXPORT int pcx_parse_header_ex(FILE *f,
                        PCXHeader *hdr,
                        int *outWidth,
                        int *outHeight,
                        unsigned flags);

/* Inspección/validación estricta sobre una cabecera ya parseada. */
PCX_EXPORT int pcx_validate_header_strict(const PCXHeader *hdr,
                               int width,
                               int height);

PCX_EXPORT int pcx_collect_header_diagnostics(const PCXHeader *hdr,
                                   int width,
                                   int height,
                                   PCXDiagnostics *outDiag);

#ifdef __cplusplus
}
#endif

#endif /* PCX_PARSER_H */

#ifndef PCX_REPORT_H
#define PCX_REPORT_H

#include <stdio.h>
#include <stddef.h>

#include "pcx_decoder.h"

#include "include/pcx/pcx_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PCX_VERSION_MAJOR 1
#define PCX_VERSION_MINOR 16
#define PCX_VERSION_PATCH 0
#define PCX_VERSION_STRING "1.16.0"
#define PCX_VERSION_NUMBER ((pcx_u32)(PCX_VERSION_MAJOR * 10000U + PCX_VERSION_MINOR * 100U + PCX_VERSION_PATCH))

PCX_EXPORT const char *pcx_version_string(void) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT pcx_u32 pcx_version_number(void) PCX_CONST_FN;
PCX_EXPORT const char *pcx_decoded_format_to_string(PCXDecodedFormat fmt) PCX_RETURNS_NONNULL PCX_CONST_FN;
PCX_EXPORT int         pcx_warning_count(pcx_u32 mask);

/* Devuelven el tamaño lógico producido (sin contar el byte NU final).
 * Si el buffer es pequeño, el contenido queda truncado pero el valor
 * devuelto permite saber cuánto espacio habría hecho falta.
 * Errores de argumentos devuelven un PCXResult negativo.
 */
PCX_EXPORT int pcx_format_diagnostics_text(const PCXFileInfo *info,
                                char *buffer,
                                pcx_size bufferSize);
PCX_EXPORT int pcx_format_diagnostics_json(const PCXFileInfo *info,
                                char *buffer,
                                pcx_size bufferSize);

PCX_EXPORT int pcx_write_diagnostics_text(FILE *f, const PCXFileInfo *info);
PCX_EXPORT int pcx_write_diagnostics_json(FILE *f, const PCXFileInfo *info);

#ifdef __cplusplus
}
#endif

#endif /* PCX_REPORT_H */

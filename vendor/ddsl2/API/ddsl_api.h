#ifndef DDSL_API_H
#define DDSL_API_H

#include "config/config.h"
#include "common/ddsl.h"
#include "compiler/compiler.h"
#include "program/program.h"
#include "warper/warper.h"
#include "registry/registry.h"
#include "built-ins/builtins.h"
#include "stdlib/stdlib.h"
#include "semantics/semantics.h"
#include "opcodes/opcodes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Versión del perfil/estándar expuesto por esta API.
 * Los valores corresponden a DDSL_STANDARD_MAJOR/MINOR.
 */
int ddsl_api_version_major(void);
int ddsl_api_version_minor(void);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_API_H */

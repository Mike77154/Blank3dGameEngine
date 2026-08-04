#ifndef FPI_SEMANTICS_H
#define FPI_SEMANTICS_H

#include "fpi_ast.h"

int fpi_semantics_validate(const FPI_AST* ast, const FPI_Registry* registry, FPI_Error* error);

#endif /* FPI_SEMANTICS_H */

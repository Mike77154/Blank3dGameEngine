#ifndef DDSL_H
#define DDSL_H

/* Umbrella header para la mini-lib del DSL.
 *
 * Componentes:
 * - token.h     : tipos de token
 * - lexer.h     : lexer
 * - ast.h       : nodos AST
 * - parser.h    : parser
 * - value.h     : valores runtime
 * - store.h     : key/value store
 * - runtime.h   : ejecución + callback
 * - arena.h     : allocator sin asignador dinámico
 * - ir.h        : IR
 * - bytecode.h  : bytecode
 * - vm.h        : VM
 * - transpile.h : transpiler (a C)
 */

#include "error/error.h"
#include "common/strview.h"
#include "types/fixed.h"
#include "token/token.h"
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "parser/parser.h"
#include "value/value.h"
#include "store/store.h"
#include "runtime/runtime.h"
#include "arena/arena.h"
#include "IR/ir.h"
#include "bytecode/bytecode.h"
#include "VM/vm.h"
#include "Transpiler/transpile.h"

#define DDSL_VERSION_MAJOR 0
#define DDSL_VERSION_MINOR 3
#define DDSL_VERSION_PATCH 2
#define DDSL_VERSION "0.3.2"

#endif /* DDSL_H */

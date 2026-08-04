#ifndef FPI_PARSER_H
#define FPI_PARSER_H

#include "fpi_ast.h"
#include "fpi_lexer.h"
#include "fpi_symtab.h"

typedef struct FPI_Parser {
    FPI_Lexer lexer;
    FPI_Token current;
    FPI_Registry* registry;
    FPI_AST* ast;
    FPI_Error* error;
    int at_bol;
    int current_bol;
} FPI_Parser;

void fpi_parser_init(FPI_Parser* parser, const char* source, FPI_Registry* registry, FPI_AST* ast, FPI_Error* error);
int fpi_parse_script(FPI_Parser* parser);

/* Legacy parser surface. */
typedef FPI_Parser Parser;
void parser_init(Parser* parser, const char* source, FPI_SymTab* symtab);
int parse_script(Parser* parser, AST_Script* out_ast);

#endif /* FPI_PARSER_H */

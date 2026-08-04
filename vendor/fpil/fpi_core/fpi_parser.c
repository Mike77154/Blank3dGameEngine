#include "fpi_parser.h"
#include "fpi_util.h"

static void parser_next(FPI_Parser* parser) {
    if (!parser) return;
    parser->current_bol = parser->at_bol;
    parser->current = fpi_lexer_next(&parser->lexer);
    if (parser->current.type == FPI_TOK_EOL) parser->at_bol = 1;
    else if (parser->current.type != FPI_TOK_EOF) parser->at_bol = 0;
}

static int parser_fail(FPI_Parser* parser, int code, FPI_Span span, const char* message) {
    fpi_error_set(parser ? parser->error : 0, code, span, message);
    return code;
}

void fpi_parser_init(FPI_Parser* parser, const char* source, FPI_Registry* registry, FPI_AST* ast, FPI_Error* error) {
    if (!parser) return;
    parser->registry = registry;
    parser->ast = ast;
    parser->error = error;
    parser->at_bol = 1;
    parser->current_bol = 1;
    fpi_lexer_init(&parser->lexer, source, error);
    parser_next(parser);
}

static int is_rhs_delimiter(char c) {
    return c == ',' || c == ':' || c == ';' || c == '\r' || c == '\n' || c == '\0';
}

static int parse_escaped_char(FPI_Parser* parser, char* out_char) {
    char c;
    c = fpi_stream_advance(&parser->lexer.stream);
    switch (c) {
        case 'n': *out_char = '\n'; return FPI_OK;
        case 'r': *out_char = '\r'; return FPI_OK;
        case 't': *out_char = '\t'; return FPI_OK;
        case '\\': *out_char = '\\'; return FPI_OK;
        case '"': *out_char = '"'; return FPI_OK;
        case '\'': *out_char = '\''; return FPI_OK;
        case ',': *out_char = ','; return FPI_OK;
        case ':': *out_char = ':'; return FPI_OK;
        default: return FPI_ERR_INVALID_ESCAPE;
    }
}

static int parse_value(FPI_Parser* parser, FPI_ValueRef* out_ref) {
    FPI_Stream* stream;
    FPI_U32 mark;
    FPI_U32 start;
    int line;
    int column;
    char c;
    char quote;
    char decoded;
    int rc;
    FPI_Span span;

    out_ref->text_offset = 0UL;
    out_ref->text_length = 0;
    out_ref->has = 0;
    out_ref->kind = FPI_VALUE_NONE;
    out_ref->fixed = 0L;
    if (parser->current.type != FPI_TOK_EQUAL) return FPI_OK;

    stream = &parser->lexer.stream;
    while (fpi_stream_peek(stream) == ' ' || fpi_stream_peek(stream) == '\t')
        (void)fpi_stream_advance(stream);
    start = stream->position;
    line = stream->line;
    column = stream->column;
    (void)fpi_ast_pool_begin(parser->ast, &mark);
    c = fpi_stream_peek(stream);
    if (c == '"' || c == '\'') {
        quote = fpi_stream_advance(stream);
        for (;;) {
            c = fpi_stream_peek(stream);
            if (c == '\0' || c == '\r' || c == '\n') {
                fpi_ast_pool_rollback(parser->ast, mark);
                span = fpi_stream_span_from(stream, start, line, column);
                return parser_fail(parser, FPI_ERR_UNTERMINATED_STRING, span, "quoted RHS is not terminated");
            }
            if (c == quote) {
                (void)fpi_stream_advance(stream);
                break;
            }
            if (c == '\\') {
                (void)fpi_stream_advance(stream);
                rc = parse_escaped_char(parser, &decoded);
                if (rc != FPI_OK) {
                    fpi_ast_pool_rollback(parser->ast, mark);
                    span = fpi_stream_span_from(stream, start, line, column);
                    return parser_fail(parser, rc, span, "invalid escape in quoted RHS");
                }
                rc = fpi_ast_pool_push(parser->ast, decoded, parser->error, fpi_stream_span_from(stream, start, line, column));
            } else {
                rc = fpi_ast_pool_push(parser->ast, fpi_stream_advance(stream), parser->error, fpi_stream_span_from(stream, start, line, column));
            }
            if (rc != FPI_OK) { fpi_ast_pool_rollback(parser->ast, mark); return rc; }
        }
        while (fpi_stream_peek(stream) == ' ' || fpi_stream_peek(stream) == '\t')
            (void)fpi_stream_advance(stream);
        if (!is_rhs_delimiter(fpi_stream_peek(stream))) {
            fpi_ast_pool_rollback(parser->ast, mark);
            span = fpi_stream_span_from(stream, start, line, column);
            return parser_fail(parser, FPI_ERR_SYNTAX, span, "unexpected text after quoted RHS");
        }
    } else {
        while (!is_rhs_delimiter(fpi_stream_peek(stream))) {
            rc = fpi_ast_pool_push(parser->ast, fpi_stream_advance(stream), parser->error, fpi_stream_span_from(stream, start, line, column));
            if (rc != FPI_OK) { fpi_ast_pool_rollback(parser->ast, mark); return rc; }
        }
        while (parser->ast->value_pool_used > mark) {
            c = parser->ast->value_pool[parser->ast->value_pool_used - 1UL];
            if (c != ' ' && c != '\t') break;
            parser->ast->value_pool_used--;
        }
    }
    span = fpi_stream_span_from(stream, start, line, column);
    rc = fpi_ast_pool_finish(parser->ast, mark, out_ref, parser->error, span);
    if (rc != FPI_OK) return rc;
    parser_next(parser);
    return parser->current.type == FPI_TOK_ERROR ? parser->error->code : FPI_OK;
}

static int parse_term(FPI_Parser* parser, int rule_index, int is_action) {
    FPI_Token name;
    FPI_ValueRef value;
    int symbol_id;
    int rc;
    if (parser->current.type != FPI_TOK_IDENTIFIER)
        return parser_fail(parser, FPI_ERR_SYNTAX, parser->current.span, "expected a condition or action name");
    name = parser->current;
    if (is_action) symbol_id = fpi_registry_register_act(parser->registry, name.text, parser->error);
    else symbol_id = fpi_registry_register_cond(parser->registry, name.text, parser->error);
    if (symbol_id < 0) return symbol_id;
    parser_next(parser);
    if (parser->current.type == FPI_TOK_ERROR) return parser->error->code;
    rc = parse_value(parser, &value);
    if (rc != FPI_OK) return rc;
    if (is_action)
        return fpi_ast_add_action(parser->ast, rule_index, symbol_id, &value, name.span, parser->error);
    return fpi_ast_add_condition(parser->ast, rule_index, symbol_id, &value, name.span, parser->error);
}

static int parse_term_list(FPI_Parser* parser, int rule_index, int is_action) {
    int count;
    int rc;
    count = 0;
    for (;;) {
        rc = parse_term(parser, rule_index, is_action);
        if (rc != FPI_OK) return rc;
        count++;
        if (parser->current.type != FPI_TOK_COMMA) break;
        parser_next(parser);
        if (parser->current.type != FPI_TOK_IDENTIFIER)
            return parser_fail(parser, FPI_ERR_SYNTAX, parser->current.span, "trailing comma or missing term");
    }
    return count > 0 ? FPI_OK : FPI_ERR_SYNTAX;
}

static int parse_rule(FPI_Parser* parser) {
    int rule_index;
    int rc;
    FPI_Span rule_span;
    if (parser->current.type != FPI_TOK_COLON || !parser->current_bol)
        return parser_fail(parser, FPI_ERR_SYNTAX, parser->current.span, "rule must start with ':' at beginning of line");
    rule_span = parser->current.span;
    rule_index = fpi_ast_begin_rule(parser->ast, rule_span, parser->error);
    if (rule_index < 0) return rule_index;
    parser_next(parser);
    rc = parse_term_list(parser, rule_index, 0);
    if (rc != FPI_OK) return rc;
    if (parser->current.type != FPI_TOK_COLON)
        return parser_fail(parser, FPI_ERR_SYNTAX, parser->current.span, "expected ':' between conditions and actions");
    rc = fpi_ast_begin_actions(parser->ast, rule_index, parser->error);
    if (rc != FPI_OK) return rc;
    parser_next(parser);
    rc = parse_term_list(parser, rule_index, 1);
    if (rc != FPI_OK) return rc;
    if (parser->current.type != FPI_TOK_EOL && parser->current.type != FPI_TOK_EOF)
        return parser_fail(parser, FPI_ERR_SYNTAX, parser->current.span, "expected end of line after actions");
    return FPI_OK;
}

int fpi_parse_script(FPI_Parser* parser) {
    int rc;
    if (!parser || !parser->registry || !parser->ast) return FPI_ERR_ARGUMENT;
    while (parser->current.type != FPI_TOK_EOF) {
        if (parser->current.type == FPI_TOK_ERROR) return parser->error ? parser->error->code : FPI_ERR_SYNTAX;
        if (parser->current.type == FPI_TOK_EOL) {
            parser_next(parser);
        } else if (parser->current.type == FPI_TOK_COLON && parser->current_bol) {
            rc = parse_rule(parser);
            if (rc != FPI_OK) return rc;
        } else {
            return parser_fail(parser, FPI_ERR_SYNTAX, parser->current.span,
                               "expected ':' at beginning of rule");
        }
    }
    return FPI_OK;
}

void parser_init(Parser* parser, const char* source, FPI_SymTab* symtab) {
    static FPI_AST legacy_ast;
    static FPI_Error legacy_error;
    fpi_ast_init(&legacy_ast);
    fpi_error_clear(&legacy_error);
    fpi_parser_init(parser, source, symtab, &legacy_ast, &legacy_error);
}

int parse_script(Parser* parser, AST_Script* out_ast) {
    int rc;
    if (!parser || !out_ast) return -1;
    parser->ast = out_ast;
    fpi_ast_init(out_ast);
    rc = fpi_parse_script(parser);
    return rc == FPI_OK ? out_ast->rule_count : -1;
}

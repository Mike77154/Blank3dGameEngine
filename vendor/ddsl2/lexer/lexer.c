#include "lexer/lexer.h"

#include <ctype.h>
#include <string.h>

static int ddsl_is_ident_start(int c) {
    return (isalpha(c) || c == '_') ? 1 : 0;
}

static int ddsl_is_ident_continue(int c) {
    return (isalnum(c) || c == '_' || c == '-') ? 1 : 0;
}

static int ddsl_sv_eq_cstr_ci(ddsl_strview sv, const char *cstr) {
    int i;
    int n;

    if (!cstr) cstr = "";
    n = (int)strlen(cstr);
    if (sv.len != n) return 0;

    for (i = 0; i < n; ++i) {
        char a = (char)tolower((unsigned char)sv.data[i]);
        char b = (char)tolower((unsigned char)cstr[i]);
        if (a != b) return 0;
    }
    return 1;
}

/* Salta hasta fin de línea (sin consumir \n). */
static void ddsl_skip_to_eol(const char *s, int len, int *io_pos, int *io_col) {
    int p;
    int c;

    p = *io_pos;
    c = *io_col;
    while (p < len && s[p] != '\n') {
        if (s[p] == '\r') {
            p++;
            continue;
        }
        p++;
        c++;
    }
    *io_pos = p;
    *io_col = c;
}

static int ddsl_push(ddsl_token_vec *out, ddsl_tok_kind kind,
                     const char *start, int tok_len,
                     int line, int col, int offset,
                     ddsl_error *err) {
    ddsl_token t;

    (void)err;
    t.kind = kind;
    t.lexeme.data = start;
    t.lexeme.len = tok_len;
    t.line = line;
    t.col = col;
    t.offset = offset;
    return ddsl_tokens_push(out, t);
}

int ddsl_lex_all(const char *source, ddsl_token_vec *out_tokens, ddsl_error *err) {
    const char *s;
    int len;
    int pos;
    int line;
    int col;

    if (err) ddsl_error_clear(err);
    if (!source || !out_tokens) {
        if (err) ddsl_error_set(err, 0, 0, 0, "lexer: argumentos inválidos");
        return 0;
    }

    ddsl_tokens_init(out_tokens);

    s = source;
    len = (int)strlen(source);
    pos = 0;
    line = 1;
    col = 1;

    while (pos < len) {
        char ch;

        ch = s[pos];

        /* NEWLINE */
        if (ch == '\n') {
            if (!ddsl_push(out_tokens, DDSL_TOK_NEWLINE, s + pos, 1, line, col, pos, err)) goto oom;
            pos++;
            line++;
            col = 1;
            continue;
        }

        /* Ignora CR */
        if (ch == '\r') {
            pos++;
            continue;
        }

        /* whitespace */
        if (ch == ' ' || ch == '\t' || ch == '\f' || ch == '\v') {
            pos++;
            col++;
            continue;
        }

        /* comentarios: #, ; o // */
        if (ch == '#' || ch == ';') {
            ddsl_skip_to_eol(s, len, &pos, &col);
            continue;
        }
        if (ch == '/' && pos + 1 < len && s[pos + 1] == '/') {
            ddsl_skip_to_eol(s, len, &pos, &col);
            continue;
        }

        /* símbolos */
        if (ch == ',') {
            if (!ddsl_push(out_tokens, DDSL_TOK_COMMA, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == '(') {
            if (!ddsl_push(out_tokens, DDSL_TOK_LPAREN, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == ')') {
            if (!ddsl_push(out_tokens, DDSL_TOK_RPAREN, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == '?') {
            if (!ddsl_push(out_tokens, DDSL_TOK_QMARK, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }

        /* arrows => / -> como THEN */
        if (ch == '=' && pos + 1 < len && s[pos + 1] == '>') {
            if (!ddsl_push(out_tokens, DDSL_TOK_THEN, s + pos, 2, line, col, pos, err)) goto oom;
            pos += 2; col += 2; continue;
        }
        if (ch == '-' && pos + 1 < len && s[pos + 1] == '>') {
            if (!ddsl_push(out_tokens, DDSL_TOK_THEN, s + pos, 2, line, col, pos, err)) goto oom;
            pos += 2; col += 2; continue;
        }

        /* operadores de comparación */
        if (ch == '=' && pos + 1 < len && s[pos + 1] == '=') {
            if (!ddsl_push(out_tokens, DDSL_TOK_EQEQ, s + pos, 2, line, col, pos, err)) goto oom;
            pos += 2; col += 2; continue;
        }
        if (ch == '!' && pos + 1 < len && s[pos + 1] == '=') {
            if (!ddsl_push(out_tokens, DDSL_TOK_NEQ, s + pos, 2, line, col, pos, err)) goto oom;
            pos += 2; col += 2; continue;
        }
        if (ch == '<' && pos + 1 < len && s[pos + 1] == '=') {
            if (!ddsl_push(out_tokens, DDSL_TOK_LTE, s + pos, 2, line, col, pos, err)) goto oom;
            pos += 2; col += 2; continue;
        }
        if (ch == '>' && pos + 1 < len && s[pos + 1] == '=') {
            if (!ddsl_push(out_tokens, DDSL_TOK_GTE, s + pos, 2, line, col, pos, err)) goto oom;
            pos += 2; col += 2; continue;
        }
        if (ch == '=') {
            if (!ddsl_push(out_tokens, DDSL_TOK_EQ, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == '<') {
            if (!ddsl_push(out_tokens, DDSL_TOK_LT, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == '>') {
            if (!ddsl_push(out_tokens, DDSL_TOK_GT, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }

        /* operadores aritméticos (NOTA: '-' puede estar dentro de IDENT, ver abajo) */
        if (ch == '+') {
            if (!ddsl_push(out_tokens, DDSL_TOK_PLUS, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == '*') {
            if (!ddsl_push(out_tokens, DDSL_TOK_STAR, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }
        if (ch == '/') {
            if (!ddsl_push(out_tokens, DDSL_TOK_SLASH, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }

        /* strings "..." o '...' */
        if (ch == '"' || ch == '\'') {
            char quote;
            int start_pos;
            int start_col;
            int start_off;
            int p;

            quote = ch;
            start_pos = pos + 1; /* contenido sin comillas */
            start_col = col + 1;
            start_off = pos + 1;

            p = start_pos;
            while (p < len) {
                char c2 = s[p];
                if (c2 == '\\' && p + 1 < len) {
                    /* escapamos el siguiente char; lo mantenemos en la slice cruda */
                    p += 2;
                    continue;
                }
                if (c2 == quote) break;
                if (c2 == '\n') {
                    if (err) ddsl_error_set(err, line, col, pos, "string sin cerrar antes de fin de línea");
                    ddsl_tokens_reset(out_tokens);
                    return 0;
                }
                p++;
            }
            if (p >= len) {
                if (err) ddsl_error_set(err, line, col, pos, "string sin cerrar antes de EOF");
                ddsl_tokens_reset(out_tokens);
                return 0;
            }

            if (!ddsl_push(out_tokens, DDSL_TOK_STRING, s + start_pos, p - start_pos, line, start_col, start_off, err)) goto oom;

            /* consume contenido + comilla final */
            {
                int consumed = (p - pos) + 1;
                pos += consumed;
                col += consumed;
            }
            continue;
        }

        /* números: como máximo un punto decimal */
        if (isdigit((unsigned char)ch) || (ch == '.' && pos + 1 < len && isdigit((unsigned char)s[pos + 1]))) {
            int start;
            int start_col;
            int p;
            int saw_dot;

            start = pos;
            start_col = col;
            p = pos;
            saw_dot = 0;
            while (p < len) {
                char c2;
                c2 = s[p];
                if (isdigit((unsigned char)c2)) {
                    p++;
                    continue;
                }
                if (c2 == '.') {
                    if (saw_dot) {
                        if (err) ddsl_error_set(err, line, start_col, start, "número inválido: más de un punto decimal");
                        ddsl_tokens_reset(out_tokens);
                        return 0;
                    }
                    saw_dot = 1;
                    p++;
                    continue;
                }
                break;
            }

            if (!ddsl_push(out_tokens, DDSL_TOK_NUMBER, s + start, p - start, line, start_col, start, err)) goto oom;
            col += (p - start);
            pos = p;
            continue;
        }

        /* identifiers / keywords */
        if (ddsl_is_ident_start((unsigned char)ch)) {
            int start;
            int start_col;
            int p;
            ddsl_strview sv;
            ddsl_tok_kind kind;

            start = pos;
            start_col = col;
            p = pos;
            while (p < len) {
                int c2 = (unsigned char)s[p];
                if (ddsl_is_ident_continue(c2)) {
                    p++;
                    continue;
                }
                break;
            }

            sv.data = s + start;
            sv.len = p - start;

            kind = DDSL_TOK_IDENT;
            if (ddsl_sv_eq_cstr_ci(sv, "if")) kind = DDSL_TOK_IF;
            else if (ddsl_sv_eq_cstr_ci(sv, "elif")) kind = DDSL_TOK_ELIF;
            else if (ddsl_sv_eq_cstr_ci(sv, "else")) kind = DDSL_TOK_ELSE;
            else if (ddsl_sv_eq_cstr_ci(sv, "then")) kind = DDSL_TOK_THEN;
            else if (ddsl_sv_eq_cstr_ci(sv, "do")) kind = DDSL_TOK_THEN;
            else if (ddsl_sv_eq_cstr_ci(sv, "execute")) kind = DDSL_TOK_THEN;
            else if (ddsl_sv_eq_cstr_ci(sv, "and")) kind = DDSL_TOK_AND;
            else if (ddsl_sv_eq_cstr_ci(sv, "or")) kind = DDSL_TOK_OR;

            if (!ddsl_push(out_tokens, kind, sv.data, sv.len, line, start_col, start, err)) goto oom;
            col += (p - start);
            pos = p;
            continue;
        }

        /* '-' como operador MINUS (si no está dentro de IDENT, lo capturamos aquí) */
        if (ch == '-') {
            if (!ddsl_push(out_tokens, DDSL_TOK_MINUS, s + pos, 1, line, col, pos, err)) goto oom;
            pos++; col++; continue;
        }

        /* desconocido */
        if (err) ddsl_error_set(err, line, col, pos, "carácter desconocido en lexer");
        ddsl_tokens_reset(out_tokens);
        return 0;
    }

    /* EOF */
    if (!ddsl_push(out_tokens, DDSL_TOK_EOF, s + len, 0, line, col, len, err)) goto oom;
    return 1;

oom:
    if (err) ddsl_error_set(err, line, col, pos, "sin memoria en lexer");
    ddsl_tokens_reset(out_tokens);
    return 0;
}

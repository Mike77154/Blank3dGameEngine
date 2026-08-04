#ifndef CONFPARSE_H
#define CONFPARSE_H

/* C89 compatible, heap-free config parser. */

typedef enum {
    CONF_FMT_UNKNOWN = 0,
    CONF_FMT_INI,
    CONF_FMT_YAML
} conf_format_t;

typedef struct {
    const char *section;
    int section_len;

    const char *key;
    int key_len;

    const char *value;
    int value_len;
} conf_entry_t;

typedef struct {
    const char *data;
    int length;
    int pos;

    conf_format_t format;

    const char *current_section;
    int current_section_len;
} conf_parser_t;

/* Inicializa el parser con texto en memoria. */
void conf_init(conf_parser_t *p, const char *data, int length);

/* Lee la siguiente entrada. Retorna 1 si hay dato, 0 si termino. */
int conf_next(conf_parser_t *p, conf_entry_t *out);

/* Comparacion de slices contra strings C, sin requerir null-termination. */
int conf_slice_eq(const char *s, int len, const char *z);
int conf_slice_ieq(const char *s, int len, const char *z);

/* Helpers sobre conf_entry_t. */
int conf_entry_section_eq(const conf_entry_t *e, const char *section);
int conf_entry_key_eq(const conf_entry_t *e, const char *key);
int conf_entry_value_eq(const conf_entry_t *e, const char *value);
int conf_entry_value_ieq(const conf_entry_t *e, const char *value);

/* Parseo de valores. Retornan 1 en exito, 0 en error. */
int conf_entry_parse_int(const conf_entry_t *e, int *out_value);
int conf_entry_parse_bool(const conf_entry_t *e, int *out_value);
int conf_entry_parse_fixed(const conf_entry_t *e, int frac_bits, int *out_value);

#endif

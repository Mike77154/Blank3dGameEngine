#ifndef DDSL_STRVIEW_H
#define DDSL_STRVIEW_H

/* Vista de string: (puntero, longitud). No es necesariamente NUL-terminated. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_strview {
    const char *data;
    int len;
} ddsl_strview;

/* Construye una vista desde un C-string (NUL-terminated). */
ddsl_strview ddsl_sv_from_cstr(const char *s);

/* Comparación exacta con C-string. */
int ddsl_sv_eq_cstr(ddsl_strview a, const char *b);

/* Copia a buffer NUL-terminated. Devuelve longitud copiada (sin contar NUL). */
int ddsl_sv_to_cstr(ddsl_strview sv, char *dst, int dst_cap);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_STRVIEW_H */

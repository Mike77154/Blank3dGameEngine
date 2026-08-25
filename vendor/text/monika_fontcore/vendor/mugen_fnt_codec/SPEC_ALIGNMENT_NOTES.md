# SPEC_ALIGNMENT_NOTES

Este ajuste rehace la parte FNT de la lib para que calce con el layout clásico de Elecbyte FNT:

- Header total: 64 bytes
- Firma: `ElecbyteFnt\0`
- Version: 2 bytes `ver_hi` + 2 bytes `ver_lo`
- PCX offset/size: offsets 16..23
- Text offset/size: offsets 24..31
- Campo reservado/comentario: 32 bytes en 32..63

## Cambios aplicados

1. `mft_fnt_header.comment` cambió de 40 a 32 bytes.
2. `mft_fnt_parse_header()` ahora compara los 12 bytes completos de la firma.
3. `mft_fnt_parse_header()` valida:
   - offsets >= 64
   - rangos dentro del archivo
   - PCX antes que texto
   - sin solapamiento entre PCX y texto
4. `mft_fnt_pack()` escribe exactamente 32 bytes de comentario.
5. `mfonttool inspect-fnt` ya no lee bytes del comienzo del PCX como si fueran comentario.
6. Se agregó `mft_fnt_comment_to_cstr()` para exponer el comentario como C-string segura.

7. `mft_u32` ya no depende de `unsigned long`; ahora el header público selecciona un tipo nativo de 32 bits con `limits.h`, evitando descalce en plataformas LP64.

## Estado

Compila con `-std=c89 -pedantic -Wall -Wextra -O2` y conserva la restricción de cero heap dinámico.

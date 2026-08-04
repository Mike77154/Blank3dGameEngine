# config / confparse

Parser de configuracion minimo, C89-compatible y sin heap.

## Formatos soportados

- INI basico: `[section]` y `key=value`.
- Mini-YAML: `section:` y `key: value`.

No es YAML completo. Es un formato tipo `key: value` pensado para bootstrap/configs controladas.

## Cambios de esta version

- Bounds-check en mini-YAML.
- Comentarios de linea con `#` y `;`.
- Comentarios inline cuando el marcador aparece al inicio del valor o despues de espacio.
- Trim de `section`, `key` y `value`.
- Parser iterativo, sin recursion.
- Helpers de comparacion por slice, sin null-termination.
- Parseo de `int`, `bool` y fixed-point Qn con `frac_bits` de 0 a 16.

## Ejemplo

```c
conf_parser_t p;
conf_entry_t e;
int value;

conf_init(&p, data, data_len);

while (conf_next(&p, &e)) {
    if (conf_entry_section_eq(&e, "video") &&
        conf_entry_key_eq(&e, "width") &&
        conf_entry_parse_int(&e, &value)) {
        cfg->video_width = value;
    }
}
```

## Bool

Acepta:

- `true`, `yes`, `on`, `1`
- `false`, `no`, `off`, `0`

La comparacion booleana es case-insensitive.

## Fixed-point

`conf_entry_parse_fixed(&e, 16, &out)` convierte decimal a Q16.16 truncado.

Ejemplo: `-1.5` con `frac_bits=16` devuelve `-98304`.

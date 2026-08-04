# Changelog

## ABI 3 - vector outline

- Sube `GC89_TYPES_ABI_VERSION` y `GC89_CORE_ABI_VERSION` a 3.
- Agrega doble pase renderer-agnostic para outline de vectores y punto central.
- El pase exterior aumenta grosor y diametro en dos veces el ancho solicitado.
- No altera el callback ABI ni intenta contornear imagenes.
- Conserva C89 estricto, Q16.16 y cero asignacion dinamica.

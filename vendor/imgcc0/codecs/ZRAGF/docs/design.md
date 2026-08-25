# Diseño y decisiones

## Fixes principales hechos en esta iteración

1. `zragf_u32` dejó de depender de `unsigned long`, evitando corrupción en LP64.
2. Se eliminó la detección heurística del tipo de `state`; ahora hay tags explícitos.
3. Se movieron helpers internos fuera del header para que la build estricta no explote por `unused static`.
4. El backend RFC1951 baseline quedó centralizado en una sola implementación.
5. Los wrappers `zlib/gzip` validan headers y trailers en vez de aceptarlos ciegamente.

## Estado del backend modular

El árbol `zragf_deflate/*` ya trae piezas para un encoder DEFLATE más ambicioso
(hash/lazy/blocks/huffman/tokens), pero todavía no forma un pipeline completo de
producción. Por eso el wrapper público usa hoy STORED blocks como backend estable.

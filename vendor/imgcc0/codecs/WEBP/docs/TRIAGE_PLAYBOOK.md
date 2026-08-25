# Triage playbook

## 1. Si falla `yuv420p`

- Corre `examples/gwpvp8probe case.webp`
- Mira `segment.*`, `loop_filter.*`, `entropy.*`, `modes.*`, `residual.*`
- Si el error cae en odd-size, primero sospecha del borde derecho/inferior
- Si cae en segmentation/loopfilter, sospecha del scheduler o del nivel efectivo

## 2. Si `yuv420p` pasa y `pam` falla

- Sospecha de `vp8_yuv.c`
- Si hay alpha, sospecha de `alpha_dec.c` o del blend/packed-writeback

## 3. Si sólo falla `ALPH + VP8 `

- compara primero `pam` con foco en canal A
- revisa filtros 0/1/2/3 y casos especiales de primer fila/columna

## 4. Si el decoder propio falla y `dwebp` no

- separa si falla en parser (`gwpinfo`) o en la ruta VP8 (`gwpvp8probe`)
- usa el primer desvío reportado por `compare_planes.py` como ancla

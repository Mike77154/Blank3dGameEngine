# katana89 v1.1 — reconstrucción realista low-poly

Biblioteca C89 para generar 33 katanas modulares sin heap y sin coma flotante.

## Qué cambió frente a v1.0

- Kissaki ko, chu y o con longitudes diferenciadas y cierre natural.
- Filo (ha) y lomo (mune) calculados por separado.
- Taper de base tipo funbari y grosor decreciente hacia la punta.
- Sección shinogi de ocho puntos.
- Yokote oblicuo, hamon ondulado y bohi que termina antes del kissaki.
- Tsuba con nakago-ana realmente abierto.
- Tsuka redondeada-rectangular con tsukamaki cruzado en X.
- Habaki ahusado, seppa, fuchi y kashira ajustados al mango.
- Saya reconstruida con perfil envolvente.

## Protocolo

- ISO C89 estricto.
- Q24.8: `256 == 1 cm`.
- Sin `malloc`, `calloc`, `realloc`, `free`, `float` ni `double`.
- Buffers entregados por el caller.
- Salida determinista y agnóstica al renderer.

## Validación

- 33 presets compilados y exportados.
- 664–948 vértices por preset desde la API C.
- 832–1,280 triángulos por preset.
- Sin triángulos degenerados ni coordenadas NaN en los OBJ incluidos.

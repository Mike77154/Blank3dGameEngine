# Encode conformance

## Still

`tests/conformance/run_encode_oracle.py`

Compara:
- `examples/gwpencode`
- `cwebp`
- decode de salida con `gwpdecode` / `dwebp`
- diff PAM con `compare_planes.py`

## Animated

`tests/conformance/run_anim_encode_oracle.py`

Compara:
- `examples/gwpanimframes`
- `img2webp`
- decode de ambas salidas con `gwpanimdump`
- diff frame a frame

También soporta una ruta separada para validar `gif2webp` + decode/demux.

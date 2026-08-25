# Conformance harness

Este directorio arma la fase 7:

- **corpus oficial fijado** (`corpus_pin.txt`)
- **oracle con libwebp/dwebp** (`run_oracle.py`)
- **dif por planos** (`compare_planes.py`)
- **triage quirúrgico** (`report_failures.py`, `gwpvp8probe`)

## Flujo

```text
1. fetch_libwebp_testdata.sh
2. make
3. run_oracle.py --oracle dwebp
4. report_failures.py
5. usar gwpvp8probe sobre los casos que fallen
```

## Formatos soportados

- `pam`: compara salida **RGBA** exacta por canal.
- `yuv420p`: compara salida **YUV420 plana** (`Y + U + V`).

## Oracles

- `dwebp`: oracle canónico recomendado.
- `pillow`: fallback útil para smoke tests locales cuando no tengas `dwebp`; **no** reemplaza la validación canónica.

## Comandos rápidos

```sh
./tests/conformance/fetch_libwebp_testdata.sh ./.cache/libwebp-test-data
make
python tests/conformance/run_oracle.py \
  --oracle dwebp \
  --dwebp /usr/local/bin/dwebp \
  --corpus-dir ./.cache/libwebp-test-data \
  --manifest tests/conformance/manifests/still_lossy_yuv.txt \
  --manifest tests/conformance/manifests/still_alpha_pam.txt \
  --out-dir ./.cache/conformance
python tests/conformance/report_failures.py ./.cache/conformance/results.jsonl
```

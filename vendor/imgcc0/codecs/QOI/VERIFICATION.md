# Verificación local

Se corrieron estas comprobaciones sobre esta v4:

## Build

```bash
make clean
make
```

Compilado con:

```text
-std=c89 -Wall -Wextra -pedantic
```

## Tests core

```bash
./test_qoi89
```

Cobertura práctica:

- vectores fijos para `RGB`, `RGBA`, `DIFF`, `LUMA`, `INDEX`, `RUN`
- validación de header
- truncado
- padding inválido
- trailing data
- repeated consecutive index
- roundtrip determinístico sobre 250 casos

## Tests streaming

```bash
./test_qoi89_stream
```

Cobertura práctica:

- encode streaming contra vectores fijos del harness
- decode streaming contra vectores fijos del harness
- roundtrip streaming determinístico sobre 300 casos
- fragmentación agresiva de input y output
- finish temprano inválido en encoder
- finish temprano inválido/truncado en decoder

## Tests pipeline

```bash
./test_qoi89_pipeline
```

Cobertura práctica:

- encode push/pull contra vectores fijos
- decode push/pull contra vectores fijos
- roundtrip pipeline determinístico sobre 260 casos
- captura temprana del descriptor al completar el header
- errores de `end()` temprano en encoder y decoder
- backpressure explícito con queue chica

## Tests callback-I/O

```bash
./test_qoi89_io
```

Cobertura práctica:

- encode/decode por callbacks contra vectores fijos
- roundtrip callback-I/O determinístico sobre 220 casos
- sinks parciales
- detección de sink trabado (`QOI89_ERR_IO_STALL`)
- detección de trailing data al decodificar

## Compatibilidad con la referencia

```bash
./test_qoi89_ref
```

Cobertura práctica:

- vectores oficiales reproducidos byte a byte
- 1000 casos diferenciales determinísticos one-shot contra `qoi.h`
- 500 casos diferenciales determinísticos de **encoder streaming** contra `qoi.h`
- comparación de:
  - bytes del encoder
  - descriptor decodificado
  - pixels decodificados

## Ejemplos

```bash
./example_memory
./example_streaming
./example_callbacks
./example_pipeline
```

## Nota
La librería `qoi89` no usa heap.
El test de compatibilidad fuerza a `qoi.h` a usar un arena allocator fijo del harness para evitar `malloc`.

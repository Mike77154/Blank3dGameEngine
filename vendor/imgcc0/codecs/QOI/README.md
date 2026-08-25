# qoi89 v4

Implementación **QOI encoder/decoder** en **C89**, sin `malloc` dentro de la librería, con buffers provistos por el caller y usando solo aritmética entera.

La v4 mantiene todo lo de v3 y agrega dos capas nuevas encima del core:

- **drivers con callbacks de I/O**
- **adaptadores push/pull desacoplados para pipelines**

## Estructura

```text
include/
  qoi89.h                API pública

src/
  qoi89_internal.h       helpers y contratos internos compartidos
  qoi89_util.c           hash, BE read/write, overflow helpers, index reset
  qoi89_desc.c           status strings + validación de descriptor
  qoi89_header.c         parseo y escritura de header
  qoi89_size.c           cálculo de tamaños
  qoi89_encode.c         encoder one-shot
  qoi89_decode.c         decoder one-shot
  qoi89_stream_encode.c  encoder incremental feed/finish
  qoi89_stream_decode.c  decoder incremental feed/finish
  qoi89_io.c             drivers callback-based read/write
  qoi89_pipeline.c       adaptadores push/pull con queue caller-owned

examples/
  example_memory.c       uso básico memoria->QOI->memoria
  example_streaming.c    uso incremental con chunks chicos
  example_callbacks.c    uso con callbacks read/write
  example_pipeline.c     uso push/pull desacoplado

tests/
  test_common.h          helpers compartidos de tests
  test_common.c          vectores, checks y generador determinístico
  test_qoi89.c           tests core one-shot
  test_qoi89_stream.c    tests streaming con chunking agresivo
  test_qoi89_pipeline.c  tests push/pull de pipeline
  test_qoi89_io.c        tests callback-I/O
  test_qoi89_ref.c       comparación diferencial contra qoi.h oficial
  third_party/qoi_ref.h  referencia oficial vendorizada para tests
```

## Qué trae v4

- **C89 portable**
- **sin heap dentro de la librería**
- **sin floating point**
- **encoder + decoder QOI**
- **API one-shot y streaming**
- **drivers callback-based de I/O**
- **API push/pull con backpressure explícito**
- **buffers totalmente caller-owned**
- **decoder estricto**
  - valida magic/header
  - valida padding final
  - detecta truncado
  - detecta trailing data
  - rechaza `QOI_OP_INDEX` consecutivo al mismo índice

## APIs nuevas

### Callback I/O

```c
qoi89_status qoi89_encode_io(
    const qoi89_desc *desc,
    qoi89_read_fn read_fn,
    void *read_user,
    qoi89_write_fn write_fn,
    void *write_user,
    const qoi89_io_buffers *buffers
);

qoi89_status qoi89_decode_io(
    unsigned int out_channels,
    qoi89_read_fn read_fn,
    void *read_user,
    qoi89_write_fn write_fn,
    void *write_user,
    const qoi89_io_buffers *buffers,
    qoi89_desc *desc_out
);
```

Pensado para sockets, pipes, streams de archivos, runtimes embebidos o wrappers de plataforma sin meter `stdio` en la librería.

### Push / Pull pipeline

```c
qoi89_status qoi89_encode_pipe_init(...);
qoi89_status qoi89_encode_pipe_push(...);
qoi89_status qoi89_encode_pipe_pull(...);
qoi89_status qoi89_encode_pipe_end(...);

qoi89_status qoi89_decode_pipe_init(...);
qoi89_status qoi89_decode_pipe_push(...);
qoi89_status qoi89_decode_pipe_pull(...);
qoi89_status qoi89_decode_pipe_end(...);
```

Pensado para cadenas de filtros con **backpressure**, donde la fuente y el sink no corren al mismo ritmo.

## Qué cambia respecto a v3

- agrega una capa **callback-I/O** sobre la API streaming
- agrega una capa **pipeline push/pull** con queue interna caller-owned
- mejora la integración en loops de scheduling, workers, runtimes y bombas de datos por etapas
- agrega ejemplos y tests nuevos para ambas capas

## Build

```bash
make
```

## Tests

```bash
make test
```

Eso corre:

- `test_qoi89`
- `test_qoi89_stream`
- `test_qoi89_pipeline`
- `test_qoi89_io`
- `test_qoi89_ref`

## Ejemplos

```bash
./example_memory
./example_streaming
./example_callbacks
./example_pipeline
```

## Notas

- La librería pública no usa `malloc`.
- El test diferencial usa la implementación oficial `qoi.h`, pero con un **arena allocator fijo** del harness, así que el paquete completo sigue siendo usable sin heap.
- `channels` y `colorspace` se validan según la spec, pero siguen siendo campos informativos para el formato.
- Los callbacks usan contratos simples:
  - `read_fn`: `QOI89_OK` + `*dst_read == 0` significa EOF
  - `write_fn`: `QOI89_OK` + `*src_used == 0` se trata como sink trabado (`QOI89_ERR_IO_STALL`)

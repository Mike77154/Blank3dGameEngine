# Arquitectura v4

## Capas

```text
qoi89.h
  ├── API pública estable
  ├── tipos/errores/tamaños
  ├── contexts streaming sin heap
  ├── callbacks de I/O
  └── adaptadores push/pull

qoi89_internal.h
  ├── opcodes y estados internos
  ├── helpers reutilizables
  └── contratos entre módulos .c

qoi89_desc.c
  ├── qoi89_status_string()
  ├── qoi89_validate_desc()
  └── qoi89_pixel_count_from_desc()

qoi89_header.c
  ├── qoi89_decode_header()
  └── qoi89_write_header()

qoi89_size.c
  ├── qoi89_decoded_size()
  └── qoi89_max_encoded_size()

qoi89_util.c
  ├── hash/index ops
  ├── BE read/write
  ├── pixel pack/unpack helpers
  └── overflow helpers

qoi89_encode.c
  └── qoi89_encode()        (one-shot)

qoi89_decode.c
  └── qoi89_decode()        (one-shot)

qoi89_stream_encode.c
  ├── qoi89_encode_stream_init()
  ├── qoi89_encode_stream_process()
  ├── qoi89_encode_stream_finish()
  └── qoi89_encode_stream_finished()

qoi89_stream_decode.c
  ├── qoi89_decode_stream_init()
  ├── qoi89_decode_stream_process()
  ├── qoi89_decode_stream_finish()
  ├── qoi89_decode_stream_header_ready()
  ├── qoi89_decode_stream_get_desc()
  └── qoi89_decode_stream_finished()

qoi89_io.c
  ├── qoi89_encode_io()
  └── qoi89_decode_io()

qoi89_pipeline.c
  ├── qoi89_encode_pipe_*()
  └── qoi89_decode_pipe_*()
```

## Decisiones de diseño

### 1. El core sigue siendo el mismo
La v4 no inventa un dialecto nuevo de QOI. El formato escrito y leído sigue siendo **QOI estándar**, y todo lo nuevo vive en capas de integración.

### 2. Streaming primero, wrappers después
Las capas nuevas no reimplementan el codec. Tanto `qoi89_io.c` como `qoi89_pipeline.c` se apoyan en los contexts streaming de v3.

Eso deja:

- un solo encoder/decoder real
- menos superficie de bugs
- misma compatibilidad byte-a-byte con la referencia

### 3. Callback-I/O sin heap ni stdio obligatorio
`qoi89_encode_io()` y `qoi89_decode_io()` reciben:

- callbacks de lectura/escritura
- buffers scratch caller-owned

Con eso el core queda libre de `FILE *`, `malloc` y dependencias de plataforma.

### 4. Push/pull desacoplado para backpressure real
La capa pipeline agrega una **queue caller-owned** delante del sink inmediato.

```text
push(src) -> core streaming -> queue interna -> pull(dst)
```

Eso permite:

- productores y consumidores a velocidades distintas
- loops de scheduling por etapas
- integración con job systems, redes o filtros encadenados

### 5. End-of-input explícito
Las APIs push/pull usan `end()` para separar claramente:

- “ya no tengo más bytes”
- “todavía faltan bytes, pero ahora no los tengo”

Eso evita ambigüedades al cerrar el encoder o validar truncado en el decoder.

### 6. Colas chicas, caller-owned
La queue de pipeline no se asigna adentro de la librería. El caller la presta al init. El módulo solo compacta y drena esa memoria.

Ventajas:

- cero heap
- tamaño controlado por integración
- fácil usar stack, static storage o arena externa

### 7. Contratos de callbacks simples
Los callbacks no usan un mini-protocolo raro.

- `read_fn`: EOF = `QOI89_OK` + `*dst_read == 0`
- `write_fn`: backpressure parcial = consumir menos de lo pedido
- `write_fn`: consumir 0 sin error = stall

Eso hace más fácil adaptarlos a APIs ya existentes.

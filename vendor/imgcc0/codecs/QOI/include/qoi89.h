#ifndef QOI89_H
#define QOI89_H

/*
 * qoi89.h -- QOI encoder/decoder in portable C89, no malloc required.
 *
 * The caller provides all input and output buffers.
 * The implementation uses only integer arithmetic.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QOI89_SRGB            0u
#define QOI89_LINEAR          1u

#define QOI89_HEADER_SIZE     14u
#define QOI89_PADDING_SIZE    8u
#define QOI89_INDEX_SIZE      64u
#define QOI89_PIXELS_LIMIT    400000000ul
#define QOI89_U32_MAX         4294967295ul
#define QOI89_STREAM_BUF_CAP  16u

typedef struct qoi89_desc_s {
    unsigned long width;      /* 1 .. 0xffffffff */
    unsigned long height;     /* 1 .. 0xffffffff */
    unsigned char channels;   /* 3 or 4 */
    unsigned char colorspace; /* 0 or 1 */
} qoi89_desc;

typedef struct qoi89_rgba_s {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} qoi89_rgba;

typedef enum qoi89_status_e {
    QOI89_OK = 0,
    QOI89_ERR_NULL,
    QOI89_ERR_BAD_ARGUMENT,
    QOI89_ERR_BAD_MAGIC,
    QOI89_ERR_BAD_DIMENSIONS,
    QOI89_ERR_BAD_CHANNELS,
    QOI89_ERR_BAD_COLORSPACE,
    QOI89_ERR_OVERFLOW,
    QOI89_ERR_INPUT_TOO_SMALL,
    QOI89_ERR_OUTPUT_TOO_SMALL,
    QOI89_ERR_TRUNCATED,
    QOI89_ERR_BAD_PADDING,
    QOI89_ERR_TRAILING_DATA,
    QOI89_ERR_REPEATED_INDEX,
    QOI89_ERR_STATE,
    QOI89_ERR_CALLBACK,
    QOI89_ERR_IO_STALL,
    QOI89_STREAM_NEED_INPUT,
    QOI89_STREAM_NEED_OUTPUT,
    QOI89_STREAM_FINISHED
} qoi89_status;

typedef struct qoi89_encode_stream_s {
    qoi89_desc desc;
    unsigned long pixel_count;
    unsigned long pixels_done;
    unsigned int channels;
    unsigned int run;
    unsigned int header_done;
    unsigned int padding_done;
    unsigned int partial_len;
    unsigned int state;
    qoi89_rgba prev;
    qoi89_rgba index_table[QOI89_INDEX_SIZE];
    unsigned char partial[4];
    unsigned char pending[QOI89_STREAM_BUF_CAP];
    unsigned int pending_len;
    unsigned int pending_pos;
} qoi89_encode_stream;

typedef struct qoi89_decode_stream_s {
    qoi89_desc desc;
    unsigned long pixel_count;
    unsigned long pixels_done;
    unsigned int requested_channels;
    unsigned int actual_channels;
    unsigned int header_pos;
    unsigned int desc_ready;
    unsigned int padding_pos;
    unsigned int state;
    unsigned int emit_remaining;
    unsigned int pending_op;
    unsigned int pending_need;
    unsigned int pending_have;
    unsigned int last_chunk_was_index;
    unsigned int last_index;
    qoi89_rgba px;
    qoi89_rgba index_table[QOI89_INDEX_SIZE];
    unsigned char header[QOI89_HEADER_SIZE];
    unsigned char pending[4];
    unsigned char pixel_pending[4];
    unsigned int pixel_pending_len;
    unsigned int pixel_pending_pos;
} qoi89_decode_stream;

typedef qoi89_status (*qoi89_read_fn)(
    void *user,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_read
);

typedef qoi89_status (*qoi89_write_fn)(
    void *user,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used
);

typedef struct qoi89_io_buffers_s {
    unsigned char *input;
    size_t input_cap;
    unsigned char *output;
    size_t output_cap;
} qoi89_io_buffers;

typedef struct qoi89_encode_pipe_s {
    qoi89_encode_stream stream;
    unsigned char *queue;
    size_t queue_cap;
    size_t queue_head;
    size_t queue_len;
    unsigned int end_requested;
} qoi89_encode_pipe;

typedef struct qoi89_decode_pipe_s {
    qoi89_decode_stream stream;
    unsigned char *queue;
    size_t queue_cap;
    size_t queue_head;
    size_t queue_len;
    unsigned int end_requested;
    unsigned int desc_ready_snapshot;
    qoi89_desc desc_snapshot;
} qoi89_decode_pipe;

/* Human-readable status string. */
const char *qoi89_status_string(qoi89_status status);

/* Validate descriptor fields. */
qoi89_status qoi89_validate_desc(const qoi89_desc *desc);

/* Parse and validate the 14-byte QOI header. */
qoi89_status qoi89_decode_header(
    const unsigned char *src,
    size_t src_len,
    qoi89_desc *desc_out
);

/* Compute output size for decode(). out_channels: 0 => desc->channels, else 3 or 4. */
qoi89_status qoi89_decoded_size(
    const qoi89_desc *desc,
    unsigned int out_channels,
    size_t *out_size
);

/* Compute a safe worst-case encoded size. */
qoi89_status qoi89_max_encoded_size(
    const qoi89_desc *desc,
    size_t *out_size
);

/*
 * Encode raw RGB/RGBA pixels into QOI.
 *
 * pixels_len must be exactly width * height * channels bytes.
 * dst_cap is the available capacity in dst.
 * On success, *dst_len receives the bytes written.
 */
qoi89_status qoi89_encode(
    const unsigned char *pixels,
    size_t pixels_len,
    const qoi89_desc *desc,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_len
);

/*
 * Decode a QOI buffer into caller-provided pixels.
 *
 * out_channels: 0 => use file header channels, else force 3 or 4.
 * desc_out receives the descriptor from the file header.
 * pixels_written receives the number of bytes written to pixels.
 */
qoi89_status qoi89_decode(
    const unsigned char *src,
    size_t src_len,
    unsigned int out_channels,
    unsigned char *pixels,
    size_t pixels_cap,
    qoi89_desc *desc_out,
    size_t *pixels_written
);

/*
 * Streaming encoder.
 *
 * qoi89_encode_stream_process() consumes pixel bytes and emits QOI bytes until it
 * blocks on either more input or more output space. After all pixel bytes were
 * provided, call qoi89_encode_stream_finish() to flush the final run and padding.
 */
qoi89_status qoi89_encode_stream_init(
    qoi89_encode_stream *stream,
    const qoi89_desc *desc
);

qoi89_status qoi89_encode_stream_process(
    qoi89_encode_stream *stream,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
);

qoi89_status qoi89_encode_stream_finish(
    qoi89_encode_stream *stream,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
);

int qoi89_encode_stream_finished(const qoi89_encode_stream *stream);

/*
 * Streaming decoder.
 *
 * qoi89_decode_stream_process() consumes QOI bytes and emits raw RGB/RGBA bytes
 * until it blocks on either more input or more output space. When the header has
 * been parsed successfully, qoi89_decode_stream_header_ready() becomes true and
 * qoi89_decode_stream_get_desc() can be used to query the descriptor.
 */
qoi89_status qoi89_decode_stream_init(
    qoi89_decode_stream *stream,
    unsigned int out_channels
);

qoi89_status qoi89_decode_stream_process(
    qoi89_decode_stream *stream,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used,
    unsigned char *pixels,
    size_t pixels_cap,
    size_t *pixels_written
);

qoi89_status qoi89_decode_stream_finish(const qoi89_decode_stream *stream);

int qoi89_decode_stream_header_ready(const qoi89_decode_stream *stream);
qoi89_status qoi89_decode_stream_get_desc(
    const qoi89_decode_stream *stream,
    qoi89_desc *desc_out
);
int qoi89_decode_stream_finished(const qoi89_decode_stream *stream);

/*
 * Callback I/O drivers.
 *
 * read_fn must return QOI89_OK and set *dst_read to the amount produced.
 * Returning QOI89_OK with *dst_read == 0 signals EOF.
 *
 * write_fn must return QOI89_OK and set *src_used to the amount consumed.
 * Returning QOI89_OK with *src_used == 0 is treated as a stalled sink.
 *
 * buffers->input and buffers->output are caller-owned scratch areas.
 */
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

/*
 * Push/pull pipeline adapters.
 *
 * push() feeds bytes into the pipeline's core state machine and stores produced
 * bytes into the caller-provided queue.
 * pull() drains that queue to the caller's output buffer.
 * end() signals end-of-input.
 */
qoi89_status qoi89_encode_pipe_init(
    qoi89_encode_pipe *pipe,
    const qoi89_desc *desc,
    unsigned char *queue,
    size_t queue_cap
);

qoi89_status qoi89_encode_pipe_push(
    qoi89_encode_pipe *pipe,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used
);

qoi89_status qoi89_encode_pipe_end(qoi89_encode_pipe *pipe);

qoi89_status qoi89_encode_pipe_pull(
    qoi89_encode_pipe *pipe,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
);

size_t qoi89_encode_pipe_output_pending(const qoi89_encode_pipe *pipe);
int qoi89_encode_pipe_finished(const qoi89_encode_pipe *pipe);

qoi89_status qoi89_decode_pipe_init(
    qoi89_decode_pipe *pipe,
    unsigned int out_channels,
    unsigned char *queue,
    size_t queue_cap
);

qoi89_status qoi89_decode_pipe_push(
    qoi89_decode_pipe *pipe,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used
);

qoi89_status qoi89_decode_pipe_end(qoi89_decode_pipe *pipe);

qoi89_status qoi89_decode_pipe_pull(
    qoi89_decode_pipe *pipe,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
);

size_t qoi89_decode_pipe_output_pending(const qoi89_decode_pipe *pipe);
int qoi89_decode_pipe_header_ready(const qoi89_decode_pipe *pipe);
qoi89_status qoi89_decode_pipe_get_desc(
    const qoi89_decode_pipe *pipe,
    qoi89_desc *desc_out
);
int qoi89_decode_pipe_finished(const qoi89_decode_pipe *pipe);

#ifdef __cplusplus
}
#endif

#endif /* QOI89_H */

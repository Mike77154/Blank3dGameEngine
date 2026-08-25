#include <string.h>

#include "qoi89_internal.h"

static qoi89_status qoi89_pipe_validate_queue(
    unsigned char *queue,
    size_t queue_cap
) {
    if (queue == NULL) {
        return QOI89_ERR_NULL;
    }
    if (queue_cap == 0u) {
        return QOI89_ERR_BAD_ARGUMENT;
    }
    return QOI89_OK;
}

static void qoi89_pipe_compact(
    unsigned char *queue,
    size_t *queue_head,
    size_t *queue_len
) {
    if (*queue_len == 0u) {
        *queue_head = 0u;
        return;
    }
    if (*queue_head != 0u) {
        memmove(queue, queue + *queue_head, *queue_len);
        *queue_head = 0u;
    }
}

static void qoi89_pipe_consume(
    size_t *queue_head,
    size_t *queue_len,
    size_t amount
) {
    *queue_head += amount;
    *queue_len -= amount;
    if (*queue_len == 0u) {
        *queue_head = 0u;
    }
}

static size_t qoi89_pipe_tail_space(
    size_t queue_cap,
    size_t queue_head,
    size_t queue_len
) {
    return queue_cap - (queue_head + queue_len);
}

static size_t qoi89_pipe_copy_out(
    unsigned char *queue,
    size_t *queue_head,
    size_t *queue_len,
    unsigned char *dst,
    size_t dst_cap
) {
    size_t n;

    n = *queue_len < dst_cap ? *queue_len : dst_cap;
    if (n > 0u) {
        memcpy(dst, queue + *queue_head, n);
        qoi89_pipe_consume(queue_head, queue_len, n);
    }
    return n;
}

static qoi89_status qoi89_encode_pipe_status(const qoi89_encode_pipe *pipe) {
    if (pipe->queue_len > 0u) {
        return QOI89_STREAM_NEED_OUTPUT;
    }
    if (pipe->end_requested && qoi89_encode_stream_finished(&pipe->stream)) {
        return QOI89_STREAM_FINISHED;
    }
    return QOI89_STREAM_NEED_INPUT;
}

static qoi89_status qoi89_encode_pipe_pump_finish(qoi89_encode_pipe *pipe) {
    qoi89_status st;

    while (!qoi89_encode_stream_finished(&pipe->stream)) {
        size_t written;
        size_t tail_space;

        if (pipe->queue_len == pipe->queue_cap) {
            break;
        }

        qoi89_pipe_compact(pipe->queue, &pipe->queue_head, &pipe->queue_len);
        tail_space = qoi89_pipe_tail_space(pipe->queue_cap, pipe->queue_head, pipe->queue_len);
        if (tail_space == 0u) {
            break;
        }

        written = 0u;
        st = qoi89_encode_stream_finish(
            &pipe->stream,
            pipe->queue + pipe->queue_head + pipe->queue_len,
            tail_space,
            &written
        );
        pipe->queue_len += written;

        if (st == QOI89_STREAM_NEED_OUTPUT) {
            if (written == 0u) {
                break;
            }
            continue;
        }
        if (st == QOI89_STREAM_FINISHED) {
            break;
        }
        return st;
    }

    return qoi89_encode_pipe_status(pipe);
}

qoi89_status qoi89_encode_pipe_init(
    qoi89_encode_pipe *pipe,
    const qoi89_desc *desc,
    unsigned char *queue,
    size_t queue_cap
) {
    qoi89_status st;

    if (pipe == NULL || desc == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_pipe_validate_queue(queue, queue_cap);
    if (st != QOI89_OK) {
        return st;
    }

    st = qoi89_encode_stream_init(&pipe->stream, desc);
    if (st != QOI89_OK) {
        return st;
    }

    pipe->queue = queue;
    pipe->queue_cap = queue_cap;
    pipe->queue_head = 0u;
    pipe->queue_len = 0u;
    pipe->end_requested = 0u;
    return QOI89_OK;
}

qoi89_status qoi89_encode_pipe_push(
    qoi89_encode_pipe *pipe,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used
) {
    size_t src_pos;

    if (pipe == NULL || src_used == NULL) {
        return QOI89_ERR_NULL;
    }
    if (src == NULL && src_len != 0u) {
        return QOI89_ERR_NULL;
    }
    if (pipe->end_requested) {
        return QOI89_ERR_STATE;
    }

    *src_used = 0u;
    src_pos = 0u;

    while (src_pos < src_len) {
        qoi89_status st;
        size_t used;
        size_t written;
        size_t tail_space;

        if (pipe->queue_len == pipe->queue_cap) {
            break;
        }

        qoi89_pipe_compact(pipe->queue, &pipe->queue_head, &pipe->queue_len);
        tail_space = qoi89_pipe_tail_space(pipe->queue_cap, pipe->queue_head, pipe->queue_len);
        if (tail_space == 0u) {
            break;
        }

        used = 0u;
        written = 0u;
        st = qoi89_encode_stream_process(
            &pipe->stream,
            src + src_pos,
            src_len - src_pos,
            &used,
            pipe->queue + pipe->queue_head + pipe->queue_len,
            tail_space,
            &written
        );
        src_pos += used;
        pipe->queue_len += written;

        if (st == QOI89_STREAM_NEED_OUTPUT) {
            if (used == 0u && written == 0u) {
                return QOI89_ERR_STATE;
            }
            continue;
        }
        if (st == QOI89_STREAM_NEED_INPUT) {
            if (src_pos == src_len) {
                break;
            }
            if (used == 0u && written == 0u) {
                return QOI89_ERR_STATE;
            }
            continue;
        }
        if (st == QOI89_STREAM_FINISHED) {
            break;
        }
        return st;
    }

    *src_used = src_pos;
    return qoi89_encode_pipe_status(pipe);
}

qoi89_status qoi89_encode_pipe_end(qoi89_encode_pipe *pipe) {
    if (pipe == NULL) {
        return QOI89_ERR_NULL;
    }

    pipe->end_requested = 1u;
    return qoi89_encode_pipe_pump_finish(pipe);
}

qoi89_status qoi89_encode_pipe_pull(
    qoi89_encode_pipe *pipe,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
) {
    size_t out_pos;

    if (pipe == NULL || dst_written == NULL) {
        return QOI89_ERR_NULL;
    }
    if (dst == NULL && dst_cap != 0u) {
        return QOI89_ERR_NULL;
    }

    *dst_written = 0u;
    out_pos = 0u;

    while (1) {
        if (pipe->queue_len > 0u) {
            size_t copied;

            if (out_pos == dst_cap) {
                break;
            }

            copied = qoi89_pipe_copy_out(
                pipe->queue,
                &pipe->queue_head,
                &pipe->queue_len,
                dst + out_pos,
                dst_cap - out_pos
            );
            out_pos += copied;
            if (out_pos == dst_cap) {
                break;
            }
            continue;
        }

        if (pipe->end_requested && !qoi89_encode_stream_finished(&pipe->stream)) {
            qoi89_status st_finish;

            st_finish = qoi89_encode_pipe_pump_finish(pipe);
            if (st_finish != QOI89_STREAM_NEED_OUTPUT &&
                st_finish != QOI89_STREAM_NEED_INPUT &&
                st_finish != QOI89_STREAM_FINISHED) {
                *dst_written = out_pos;
                return st_finish;
            }
            if (pipe->queue_len == 0u) {
                break;
            }
            continue;
        }

        break;
    }

    *dst_written = out_pos;
    return qoi89_encode_pipe_status(pipe);
}

size_t qoi89_encode_pipe_output_pending(const qoi89_encode_pipe *pipe) {
    if (pipe == NULL) {
        return 0u;
    }
    return pipe->queue_len;
}

int qoi89_encode_pipe_finished(const qoi89_encode_pipe *pipe) {
    if (pipe == NULL) {
        return 0;
    }
    return pipe->end_requested != 0u &&
        qoi89_encode_stream_finished(&pipe->stream) &&
        pipe->queue_len == 0u;
}

static qoi89_status qoi89_decode_pipe_capture_desc(qoi89_decode_pipe *pipe) {
    qoi89_status st;

    if (!pipe->desc_ready_snapshot && qoi89_decode_stream_header_ready(&pipe->stream)) {
        st = qoi89_decode_stream_get_desc(&pipe->stream, &pipe->desc_snapshot);
        if (st != QOI89_OK) {
            return st;
        }
        pipe->desc_ready_snapshot = 1u;
    }
    return QOI89_OK;
}

static qoi89_status qoi89_decode_pipe_status(const qoi89_decode_pipe *pipe) {
    if (pipe->queue_len > 0u) {
        return QOI89_STREAM_NEED_OUTPUT;
    }
    if (pipe->end_requested && qoi89_decode_stream_finished(&pipe->stream)) {
        return QOI89_STREAM_FINISHED;
    }
    return QOI89_STREAM_NEED_INPUT;
}

qoi89_status qoi89_decode_pipe_init(
    qoi89_decode_pipe *pipe,
    unsigned int out_channels,
    unsigned char *queue,
    size_t queue_cap
) {
    qoi89_status st;

    if (pipe == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_pipe_validate_queue(queue, queue_cap);
    if (st != QOI89_OK) {
        return st;
    }

    st = qoi89_decode_stream_init(&pipe->stream, out_channels);
    if (st != QOI89_OK) {
        return st;
    }

    pipe->queue = queue;
    pipe->queue_cap = queue_cap;
    pipe->queue_head = 0u;
    pipe->queue_len = 0u;
    pipe->end_requested = 0u;
    pipe->desc_ready_snapshot = 0u;
    pipe->desc_snapshot.width = 0ul;
    pipe->desc_snapshot.height = 0ul;
    pipe->desc_snapshot.channels = 0u;
    pipe->desc_snapshot.colorspace = 0u;
    return QOI89_OK;
}

qoi89_status qoi89_decode_pipe_push(
    qoi89_decode_pipe *pipe,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used
) {
    size_t src_pos;

    if (pipe == NULL || src_used == NULL) {
        return QOI89_ERR_NULL;
    }
    if (src == NULL && src_len != 0u) {
        return QOI89_ERR_NULL;
    }
    if (pipe->end_requested) {
        return QOI89_ERR_STATE;
    }

    *src_used = 0u;
    src_pos = 0u;

    while (src_pos < src_len) {
        qoi89_status stream_st;
        qoi89_status cap_st;
        size_t used;
        size_t written;
        size_t tail_space;

        if (pipe->queue_len == pipe->queue_cap) {
            break;
        }

        qoi89_pipe_compact(pipe->queue, &pipe->queue_head, &pipe->queue_len);
        tail_space = qoi89_pipe_tail_space(pipe->queue_cap, pipe->queue_head, pipe->queue_len);
        if (tail_space == 0u) {
            break;
        }

        used = 0u;
        written = 0u;
        stream_st = qoi89_decode_stream_process(
            &pipe->stream,
            src + src_pos,
            src_len - src_pos,
            &used,
            pipe->queue + pipe->queue_head + pipe->queue_len,
            tail_space,
            &written
        );
        src_pos += used;
        pipe->queue_len += written;

        cap_st = qoi89_decode_pipe_capture_desc(pipe);
        if (cap_st != QOI89_OK) {
            return cap_st;
        }

        if (stream_st == QOI89_STREAM_NEED_OUTPUT) {
            if (used == 0u && written == 0u) {
                return QOI89_ERR_STATE;
            }
            continue;
        }

        if (stream_st == QOI89_STREAM_NEED_INPUT) {
            if (src_pos == src_len) {
                break;
            }
            if (used == 0u && written == 0u) {
                return QOI89_ERR_STATE;
            }
            continue;
        }

        if (stream_st == QOI89_STREAM_FINISHED) {
            break;
        }

        return stream_st;
    }

    *src_used = src_pos;
    return qoi89_decode_pipe_status(pipe);
}

qoi89_status qoi89_decode_pipe_end(qoi89_decode_pipe *pipe) {
    qoi89_status st;

    if (pipe == NULL) {
        return QOI89_ERR_NULL;
    }

    pipe->end_requested = 1u;
    st = qoi89_decode_stream_finish(&pipe->stream);
    if (st != QOI89_STREAM_FINISHED) {
        return st;
    }

    st = qoi89_decode_pipe_capture_desc(pipe);
    if (st != QOI89_OK) {
        return st;
    }

    return qoi89_decode_pipe_status(pipe);
}

qoi89_status qoi89_decode_pipe_pull(
    qoi89_decode_pipe *pipe,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
) {
    size_t out_pos;

    if (pipe == NULL || dst_written == NULL) {
        return QOI89_ERR_NULL;
    }
    if (dst == NULL && dst_cap != 0u) {
        return QOI89_ERR_NULL;
    }

    *dst_written = 0u;
    out_pos = 0u;

    if (pipe->queue_len > 0u && dst_cap > 0u) {
        out_pos = qoi89_pipe_copy_out(
            pipe->queue,
            &pipe->queue_head,
            &pipe->queue_len,
            dst,
            dst_cap
        );
    }

    *dst_written = out_pos;
    return qoi89_decode_pipe_status(pipe);
}

size_t qoi89_decode_pipe_output_pending(const qoi89_decode_pipe *pipe) {
    if (pipe == NULL) {
        return 0u;
    }
    return pipe->queue_len;
}

int qoi89_decode_pipe_header_ready(const qoi89_decode_pipe *pipe) {
    if (pipe == NULL) {
        return 0;
    }
    return pipe->desc_ready_snapshot != 0u;
}

qoi89_status qoi89_decode_pipe_get_desc(
    const qoi89_decode_pipe *pipe,
    qoi89_desc *desc_out
) {
    if (pipe == NULL || desc_out == NULL) {
        return QOI89_ERR_NULL;
    }
    if (!pipe->desc_ready_snapshot) {
        return QOI89_ERR_STATE;
    }

    *desc_out = pipe->desc_snapshot;
    return QOI89_OK;
}

int qoi89_decode_pipe_finished(const qoi89_decode_pipe *pipe) {
    if (pipe == NULL) {
        return 0;
    }
    return pipe->end_requested != 0u &&
        qoi89_decode_stream_finished(&pipe->stream) &&
        pipe->queue_len == 0u;
}

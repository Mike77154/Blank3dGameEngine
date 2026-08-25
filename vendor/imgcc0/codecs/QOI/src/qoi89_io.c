#include "qoi89_internal.h"

static qoi89_status qoi89_io_validate_buffers(const qoi89_io_buffers *buffers) {
    if (buffers == NULL) {
        return QOI89_ERR_NULL;
    }
    if (buffers->input == NULL || buffers->output == NULL) {
        return QOI89_ERR_NULL;
    }
    if (buffers->input_cap == 0u || buffers->output_cap == 0u) {
        return QOI89_ERR_BAD_ARGUMENT;
    }
    return QOI89_OK;
}

static qoi89_status qoi89_io_write_all(
    qoi89_write_fn write_fn,
    void *write_user,
    const unsigned char *src,
    size_t src_len
) {
    size_t src_pos;

    src_pos = 0u;
    while (src_pos < src_len) {
        qoi89_status cb_st;
        size_t used;

        used = 0u;
        cb_st = write_fn(write_user, src + src_pos, src_len - src_pos, &used);
        if (cb_st != QOI89_OK) {
            return cb_st;
        }
        if (used > src_len - src_pos) {
            return QOI89_ERR_CALLBACK;
        }
        if (used == 0u) {
            return QOI89_ERR_IO_STALL;
        }
        src_pos += used;
    }

    return QOI89_OK;
}

qoi89_status qoi89_encode_io(
    const qoi89_desc *desc,
    qoi89_read_fn read_fn,
    void *read_user,
    qoi89_write_fn write_fn,
    void *write_user,
    const qoi89_io_buffers *buffers
) {
    qoi89_status st;
    qoi89_encode_stream stream;
    int eof;

    if (desc == NULL || read_fn == NULL || write_fn == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_io_validate_buffers(buffers);
    if (st != QOI89_OK) {
        return st;
    }

    st = qoi89_encode_stream_init(&stream, desc);
    if (st != QOI89_OK) {
        return st;
    }

    eof = 0;
    while (!eof) {
        size_t got;

        got = 0u;
        st = read_fn(read_user, buffers->input, buffers->input_cap, &got);
        if (st != QOI89_OK) {
            return st;
        }
        if (got > buffers->input_cap) {
            return QOI89_ERR_CALLBACK;
        }
        if (got == 0u) {
            eof = 1;
            break;
        }

        {
            size_t src_pos;

            src_pos = 0u;
            while (src_pos < got) {
                qoi89_status stream_st;
                size_t used;
                size_t written;

                used = 0u;
                written = 0u;
                stream_st = qoi89_encode_stream_process(
                    &stream,
                    buffers->input + src_pos,
                    got - src_pos,
                    &used,
                    buffers->output,
                    buffers->output_cap,
                    &written
                );
                src_pos += used;

                if (written > 0u) {
                    st = qoi89_io_write_all(write_fn, write_user, buffers->output, written);
                    if (st != QOI89_OK) {
                        return st;
                    }
                }

                if (stream_st == QOI89_STREAM_NEED_OUTPUT) {
                    continue;
                }
                if (stream_st == QOI89_STREAM_NEED_INPUT) {
                    if (src_pos == got) {
                        break;
                    }
                    continue;
                }
                if (stream_st != QOI89_OK) {
                    return stream_st;
                }
            }
        }
    }

    while (1) {
        qoi89_status stream_st;
        size_t written;

        written = 0u;
        stream_st = qoi89_encode_stream_finish(
            &stream,
            buffers->output,
            buffers->output_cap,
            &written
        );

        if (written > 0u) {
            st = qoi89_io_write_all(write_fn, write_user, buffers->output, written);
            if (st != QOI89_OK) {
                return st;
            }
        }

        if (stream_st == QOI89_STREAM_NEED_OUTPUT) {
            continue;
        }
        if (stream_st == QOI89_STREAM_FINISHED) {
            return QOI89_OK;
        }
        return stream_st;
    }
}

qoi89_status qoi89_decode_io(
    unsigned int out_channels,
    qoi89_read_fn read_fn,
    void *read_user,
    qoi89_write_fn write_fn,
    void *write_user,
    const qoi89_io_buffers *buffers,
    qoi89_desc *desc_out
) {
    qoi89_status st;
    qoi89_decode_stream stream;
    int desc_ready;
    int finished;
    int finished_from_process;

    if (read_fn == NULL || write_fn == NULL || desc_out == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_io_validate_buffers(buffers);
    if (st != QOI89_OK) {
        return st;
    }

    st = qoi89_decode_stream_init(&stream, out_channels);
    if (st != QOI89_OK) {
        return st;
    }

    desc_out->width = 0ul;
    desc_out->height = 0ul;
    desc_out->channels = 0u;
    desc_out->colorspace = 0u;
    desc_ready = 0;
    finished = 0;
    finished_from_process = 0;

    while (!finished) {
        size_t got;

        got = 0u;
        st = read_fn(read_user, buffers->input, buffers->input_cap, &got);
        if (st != QOI89_OK) {
            return st;
        }
        if (got > buffers->input_cap) {
            return QOI89_ERR_CALLBACK;
        }

        if (got == 0u) {
            st = qoi89_decode_stream_finish(&stream);
            if (st != QOI89_STREAM_FINISHED) {
                return st;
            }
            finished = 1;
            break;
        }

        {
            size_t src_pos;

            src_pos = 0u;
            while (src_pos < got) {
                qoi89_status stream_st;
                size_t used;
                size_t written;

                used = 0u;
                written = 0u;
                stream_st = qoi89_decode_stream_process(
                    &stream,
                    buffers->input + src_pos,
                    got - src_pos,
                    &used,
                    buffers->output,
                    buffers->output_cap,
                    &written
                );
                src_pos += used;

                if (!desc_ready && qoi89_decode_stream_header_ready(&stream)) {
                    st = qoi89_decode_stream_get_desc(&stream, desc_out);
                    if (st != QOI89_OK) {
                        return st;
                    }
                    desc_ready = 1;
                }

                if (written > 0u) {
                    st = qoi89_io_write_all(write_fn, write_user, buffers->output, written);
                    if (st != QOI89_OK) {
                        return st;
                    }
                }

                if (stream_st == QOI89_STREAM_NEED_OUTPUT) {
                    continue;
                }
                if (stream_st == QOI89_STREAM_NEED_INPUT) {
                    if (src_pos == got) {
                        break;
                    }
                    continue;
                }
                if (stream_st == QOI89_STREAM_FINISHED) {
                    if (src_pos != got) {
                        return QOI89_ERR_TRAILING_DATA;
                    }
                    finished = 1;
                    finished_from_process = 1;
                    break;
                }
                return stream_st;
            }
        }
    }

    if (!desc_ready && qoi89_decode_stream_header_ready(&stream)) {
        st = qoi89_decode_stream_get_desc(&stream, desc_out);
        if (st != QOI89_OK) {
            return st;
        }
        desc_ready = 1;
    }

    if (!desc_ready) {
        return QOI89_ERR_TRUNCATED;
    }

    if (finished_from_process) {
        size_t got;

        got = 0u;
        st = read_fn(read_user, buffers->input, buffers->input_cap, &got);
        if (st != QOI89_OK) {
            return st;
        }
        if (got > buffers->input_cap) {
            return QOI89_ERR_CALLBACK;
        }
        if (got != 0u) {
            return QOI89_ERR_TRAILING_DATA;
        }
    }

    return QOI89_OK;
}

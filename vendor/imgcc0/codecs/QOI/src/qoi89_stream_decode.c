#include "qoi89_internal.h"

static qoi89_status qoi89_dec_emit_pixel(
    qoi89_decode_stream *stream,
    unsigned char *pixels,
    size_t pixels_cap,
    size_t *pixels_written
) {
    size_t out_pos;

    if (stream->pixel_pending_len == 0u) {
        stream->pixel_pending[0] = stream->px.r;
        stream->pixel_pending[1] = stream->px.g;
        stream->pixel_pending[2] = stream->px.b;
        if (stream->actual_channels == 4u) {
            stream->pixel_pending[3] = stream->px.a;
        }
        stream->pixel_pending_len = stream->actual_channels;
        stream->pixel_pending_pos = 0u;
    }

    out_pos = *pixels_written;
    while (stream->pixel_pending_pos < stream->pixel_pending_len && out_pos < pixels_cap) {
        pixels[out_pos++] = stream->pixel_pending[stream->pixel_pending_pos++];
    }

    *pixels_written = out_pos;
    if (stream->pixel_pending_pos < stream->pixel_pending_len) {
        return QOI89_STREAM_NEED_OUTPUT;
    }

    stream->pixel_pending_len = 0u;
    stream->pixel_pending_pos = 0u;
    ++stream->pixels_done;
    --stream->emit_remaining;
    return QOI89_OK;
}

static qoi89_status qoi89_dec_complete_pending(qoi89_decode_stream *stream) {
    unsigned int idx;
    unsigned int b2;
    int vg;

    if (stream->pending_op == QOI89_DEC_PENDING_LUMA) {
        b2 = (unsigned int)stream->pending[1];
        vg = (int)(stream->pending[0] & 0x3fu) - 32;
        stream->px.r = (unsigned char)((int)stream->px.r + vg - 8 + (int)((b2 >> 4) & 0x0fu));
        stream->px.g = (unsigned char)((int)stream->px.g + vg);
        stream->px.b = (unsigned char)((int)stream->px.b + vg - 8 + (int)(b2 & 0x0fu));
        stream->last_chunk_was_index = 0u;
    }
    else if (stream->pending_op == QOI89_DEC_PENDING_RGB) {
        stream->px.r = stream->pending[0];
        stream->px.g = stream->pending[1];
        stream->px.b = stream->pending[2];
        stream->last_chunk_was_index = 0u;
    }
    else if (stream->pending_op == QOI89_DEC_PENDING_RGBA) {
        stream->px.r = stream->pending[0];
        stream->px.g = stream->pending[1];
        stream->px.b = stream->pending[2];
        stream->px.a = stream->pending[3];
        stream->last_chunk_was_index = 0u;
    }
    else {
        return QOI89_ERR_STATE;
    }

    stream->pending_op = QOI89_DEC_PENDING_NONE;
    stream->pending_need = 0u;
    stream->pending_have = 0u;
    stream->emit_remaining = 1u;
    idx = qoi89_hash_rgba(&stream->px);
    stream->index_table[idx] = stream->px;
    return QOI89_OK;
}

qoi89_status qoi89_decode_stream_init(
    qoi89_decode_stream *stream,
    unsigned int out_channels
) {
    if (stream == NULL) {
        return QOI89_ERR_NULL;
    }

    if (out_channels != 0u && !qoi89_valid_channels(out_channels)) {
        return QOI89_ERR_BAD_CHANNELS;
    }

    stream->desc.width = 0ul;
    stream->desc.height = 0ul;
    stream->desc.channels = 0u;
    stream->desc.colorspace = 0u;
    stream->pixel_count = 0ul;
    stream->pixels_done = 0ul;
    stream->requested_channels = out_channels;
    stream->actual_channels = 0u;
    stream->header_pos = 0u;
    stream->desc_ready = 0u;
    stream->padding_pos = 0u;
    stream->state = QOI89_DEC_STATE_HEADER;
    stream->emit_remaining = 0u;
    stream->pending_op = QOI89_DEC_PENDING_NONE;
    stream->pending_need = 0u;
    stream->pending_have = 0u;
    stream->last_chunk_was_index = 0u;
    stream->last_index = 0u;
    stream->px.r = 0u;
    stream->px.g = 0u;
    stream->px.b = 0u;
    stream->px.a = 255u;
    qoi89_zero_index(stream->index_table);
    stream->pixel_pending_len = 0u;
    stream->pixel_pending_pos = 0u;

    return QOI89_OK;
}

qoi89_status qoi89_decode_stream_process(
    qoi89_decode_stream *stream,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used,
    unsigned char *pixels,
    size_t pixels_cap,
    size_t *pixels_written
) {
    size_t src_pos;
    qoi89_status st;

    if (stream == NULL || src_used == NULL || pixels_written == NULL) {
        return QOI89_ERR_NULL;
    }
    if (src == NULL && src_len != 0u) {
        return QOI89_ERR_NULL;
    }
    if (pixels == NULL && pixels_cap != 0u) {
        return QOI89_ERR_NULL;
    }

    *src_used = 0u;
    *pixels_written = 0u;

    if (stream->state == QOI89_DEC_STATE_DONE) {
        if (src_len != 0u) {
            return QOI89_ERR_TRAILING_DATA;
        }
        return QOI89_STREAM_FINISHED;
    }

    src_pos = 0u;
    while (1) {
        if (stream->state == QOI89_DEC_STATE_HEADER) {
            while (stream->header_pos < QOI89_HEADER_SIZE && src_pos < src_len) {
                stream->header[stream->header_pos++] = src[src_pos++];
            }

            if (stream->header_pos < QOI89_HEADER_SIZE) {
                *src_used = src_pos;
                return QOI89_STREAM_NEED_INPUT;
            }

            st = qoi89_decode_header(stream->header, (size_t)QOI89_HEADER_SIZE, &stream->desc);
            if (st != QOI89_OK) {
                *src_used = src_pos;
                return st;
            }

            st = qoi89_pixel_count_from_desc(&stream->desc, &stream->pixel_count);
            if (st != QOI89_OK) {
                *src_used = src_pos;
                return st;
            }

            stream->actual_channels =
                stream->requested_channels == 0u ? (unsigned int)stream->desc.channels : stream->requested_channels;
            if (!qoi89_valid_channels(stream->actual_channels)) {
                *src_used = src_pos;
                return QOI89_ERR_BAD_CHANNELS;
            }

            stream->desc_ready = 1u;
            stream->state = QOI89_DEC_STATE_BODY;
            continue;
        }

        if (stream->state == QOI89_DEC_STATE_BODY) {
            if (stream->emit_remaining > 0u) {
                st = qoi89_dec_emit_pixel(stream, pixels, pixels_cap, pixels_written);
                *src_used = src_pos;
                if (st != QOI89_OK) {
                    return st;
                }
                if (stream->pixels_done == stream->pixel_count && stream->emit_remaining == 0u) {
                    stream->state = QOI89_DEC_STATE_PADDING;
                }
                continue;
            }

            if (stream->pixels_done == stream->pixel_count) {
                stream->state = QOI89_DEC_STATE_PADDING;
                continue;
            }

            if (stream->pending_op != QOI89_DEC_PENDING_NONE) {
                while (stream->pending_have < stream->pending_need && src_pos < src_len) {
                    stream->pending[stream->pending_have++] = src[src_pos++];
                }
                if (stream->pending_have < stream->pending_need) {
                    *src_used = src_pos;
                    return QOI89_STREAM_NEED_INPUT;
                }
                st = qoi89_dec_complete_pending(stream);
                if (st != QOI89_OK) {
                    *src_used = src_pos;
                    return st;
                }
                continue;
            }

            if (src_pos >= src_len) {
                *src_used = src_pos;
                return QOI89_STREAM_NEED_INPUT;
            }

            {
                unsigned int b1;

                b1 = (unsigned int)src[src_pos++];

                if (b1 == QOI89_OP_RGB) {
                    stream->pending_op = QOI89_DEC_PENDING_RGB;
                    stream->pending_need = 3u;
                    stream->pending_have = 0u;
                    continue;
                }
                else if (b1 == QOI89_OP_RGBA) {
                    stream->pending_op = QOI89_DEC_PENDING_RGBA;
                    stream->pending_need = 4u;
                    stream->pending_have = 0u;
                    continue;
                }
                else if ((b1 & QOI89_MASK_2) == QOI89_OP_INDEX) {
                    unsigned int idx;

                    idx = b1 & 63u;
                    if (stream->last_chunk_was_index && idx == stream->last_index) {
                        *src_used = src_pos;
                        return QOI89_ERR_REPEATED_INDEX;
                    }
                    stream->px = stream->index_table[idx];
                    stream->last_chunk_was_index = 1u;
                    stream->last_index = idx;
                    stream->emit_remaining = 1u;
                    stream->index_table[qoi89_hash_rgba(&stream->px)] = stream->px;
                    continue;
                }
                else if ((b1 & QOI89_MASK_2) == QOI89_OP_DIFF) {
                    stream->px.r = (unsigned char)((unsigned int)stream->px.r + (((b1 >> 4) & 0x03u) - 2u));
                    stream->px.g = (unsigned char)((unsigned int)stream->px.g + (((b1 >> 2) & 0x03u) - 2u));
                    stream->px.b = (unsigned char)((unsigned int)stream->px.b + ((b1 & 0x03u) - 2u));
                    stream->last_chunk_was_index = 0u;
                    stream->emit_remaining = 1u;
                    stream->index_table[qoi89_hash_rgba(&stream->px)] = stream->px;
                    continue;
                }
                else if ((b1 & QOI89_MASK_2) == QOI89_OP_LUMA) {
                    stream->pending_op = QOI89_DEC_PENDING_LUMA;
                    stream->pending_need = 2u;
                    stream->pending_have = 1u;
                    stream->pending[0] = (unsigned char)b1;
                    continue;
                }
                else {
                    unsigned int run_len;
                    unsigned long remaining_pixels;

                    run_len = (b1 & 0x3fu) + 1u;
                    remaining_pixels = stream->pixel_count - stream->pixels_done;
                    if ((unsigned long)run_len > remaining_pixels) {
                        *src_used = src_pos;
                        return QOI89_ERR_TRUNCATED;
                    }
                    stream->last_chunk_was_index = 0u;
                    stream->emit_remaining = run_len;
                    stream->index_table[qoi89_hash_rgba(&stream->px)] = stream->px;
                    continue;
                }
            }
        }

        if (stream->state == QOI89_DEC_STATE_PADDING) {
            while (stream->padding_pos < QOI89_PADDING_SIZE && src_pos < src_len) {
                if (src[src_pos++] != qoi89_padding[stream->padding_pos++]) {
                    *src_used = src_pos;
                    return QOI89_ERR_BAD_PADDING;
                }
            }

            if (stream->padding_pos < QOI89_PADDING_SIZE) {
                *src_used = src_pos;
                return QOI89_STREAM_NEED_INPUT;
            }

            stream->state = QOI89_DEC_STATE_DONE;
            *src_used = src_pos;
            if (src_pos < src_len) {
                return QOI89_ERR_TRAILING_DATA;
            }
            return QOI89_STREAM_FINISHED;
        }

        if (stream->state == QOI89_DEC_STATE_DONE) {
            *src_used = src_pos;
            if (src_pos < src_len) {
                return QOI89_ERR_TRAILING_DATA;
            }
            return QOI89_STREAM_FINISHED;
        }

        *src_used = src_pos;
        return QOI89_ERR_STATE;
    }
}

qoi89_status qoi89_decode_stream_finish(const qoi89_decode_stream *stream) {
    if (stream == NULL) {
        return QOI89_ERR_NULL;
    }

    if (stream->state == QOI89_DEC_STATE_DONE) {
        return QOI89_STREAM_FINISHED;
    }

    return QOI89_ERR_TRUNCATED;
}

int qoi89_decode_stream_header_ready(const qoi89_decode_stream *stream) {
    return stream != NULL && stream->desc_ready != 0u;
}

qoi89_status qoi89_decode_stream_get_desc(
    const qoi89_decode_stream *stream,
    qoi89_desc *desc_out
) {
    if (stream == NULL || desc_out == NULL) {
        return QOI89_ERR_NULL;
    }
    if (!stream->desc_ready) {
        return QOI89_ERR_STATE;
    }

    *desc_out = stream->desc;
    return QOI89_OK;
}

int qoi89_decode_stream_finished(const qoi89_decode_stream *stream) {
    return stream != NULL && stream->state == QOI89_DEC_STATE_DONE;
}

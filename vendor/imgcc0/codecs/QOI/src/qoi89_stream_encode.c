#include "qoi89_internal.h"

static qoi89_status qoi89_enc_flush_pending(
    qoi89_encode_stream *stream,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
) {
    size_t out_pos;

    out_pos = *dst_written;
    while (stream->pending_pos < stream->pending_len && out_pos < dst_cap) {
        dst[out_pos++] = stream->pending[stream->pending_pos++];
    }

    *dst_written = out_pos;
    if (stream->pending_pos < stream->pending_len) {
        return QOI89_STREAM_NEED_OUTPUT;
    }

    stream->pending_pos = 0u;
    stream->pending_len = 0u;
    return QOI89_OK;
}

static void qoi89_enc_prepare_header(qoi89_encode_stream *stream) {
    size_t pos;

    pos = 0u;
    stream->pending[pos++] = 'q';
    stream->pending[pos++] = 'o';
    stream->pending[pos++] = 'i';
    stream->pending[pos++] = 'f';
    qoi89_write_u32be(stream->pending, &pos, stream->desc.width);
    qoi89_write_u32be(stream->pending, &pos, stream->desc.height);
    stream->pending[pos++] = stream->desc.channels;
    stream->pending[pos++] = stream->desc.colorspace;
    stream->pending_len = (unsigned int)pos;
    stream->pending_pos = 0u;
}

static void qoi89_enc_prepare_pixel(
    qoi89_encode_stream *stream,
    const qoi89_rgba *px
) {
    unsigned int pos;
    unsigned int hash_index;
    int vr;
    int vg;
    int vb;
    int vg_r;
    int vg_b;
    int is_last;

    pos = 0u;
    is_last = (stream->pixels_done + 1ul == stream->pixel_count);

    if (qoi89_rgba_equal(px, &stream->prev)) {
        ++stream->run;
        if (stream->run == 62u || is_last) {
            stream->pending[pos++] = (unsigned char)(QOI89_OP_RUN | (stream->run - 1u));
            stream->run = 0u;
        }
        stream->prev = *px;
        ++stream->pixels_done;
        stream->pending_len = pos;
        stream->pending_pos = 0u;
        return;
    }

    if (stream->run > 0u) {
        stream->pending[pos++] = (unsigned char)(QOI89_OP_RUN | (stream->run - 1u));
        stream->run = 0u;
    }

    hash_index = qoi89_hash_rgba(px);
    if (qoi89_rgba_equal(&stream->index_table[hash_index], px)) {
        stream->pending[pos++] = (unsigned char)(QOI89_OP_INDEX | hash_index);
    }
    else {
        stream->index_table[hash_index] = *px;

        if (px->a == stream->prev.a) {
            vr = qoi89_wrap_diff_u8((unsigned int)px->r, (unsigned int)stream->prev.r);
            vg = qoi89_wrap_diff_u8((unsigned int)px->g, (unsigned int)stream->prev.g);
            vb = qoi89_wrap_diff_u8((unsigned int)px->b, (unsigned int)stream->prev.b);
            vg_r = vr - vg;
            vg_b = vb - vg;

            if (vr > -3 && vr < 2 && vg > -3 && vg < 2 && vb > -3 && vb < 2) {
                stream->pending[pos++] = (unsigned char)(
                    QOI89_OP_DIFF |
                    ((unsigned int)(vr + 2) << 4) |
                    ((unsigned int)(vg + 2) << 2) |
                    (unsigned int)(vb + 2)
                );
            }
            else if (vg_r > -9 && vg_r < 8 && vg > -33 && vg < 32 && vg_b > -9 && vg_b < 8) {
                stream->pending[pos++] = (unsigned char)(QOI89_OP_LUMA | (unsigned int)(vg + 32));
                stream->pending[pos++] = (unsigned char)(
                    ((unsigned int)(vg_r + 8) << 4) |
                    (unsigned int)(vg_b + 8)
                );
            }
            else {
                stream->pending[pos++] = (unsigned char)QOI89_OP_RGB;
                stream->pending[pos++] = px->r;
                stream->pending[pos++] = px->g;
                stream->pending[pos++] = px->b;
            }
        }
        else {
            stream->pending[pos++] = (unsigned char)QOI89_OP_RGBA;
            stream->pending[pos++] = px->r;
            stream->pending[pos++] = px->g;
            stream->pending[pos++] = px->b;
            stream->pending[pos++] = px->a;
        }
    }

    stream->prev = *px;
    ++stream->pixels_done;
    stream->pending_len = pos;
    stream->pending_pos = 0u;
}

qoi89_status qoi89_encode_stream_init(
    qoi89_encode_stream *stream,
    const qoi89_desc *desc
) {
    qoi89_status st;
    unsigned long pixel_count;

    if (stream == NULL || desc == NULL) {
        return QOI89_ERR_NULL;
    }

    st = qoi89_validate_desc(desc);
    if (st != QOI89_OK) {
        return st;
    }

    st = qoi89_pixel_count_from_desc(desc, &pixel_count);
    if (st != QOI89_OK) {
        return st;
    }

    stream->desc = *desc;
    stream->pixel_count = pixel_count;
    stream->pixels_done = 0ul;
    stream->channels = (unsigned int)desc->channels;
    stream->run = 0u;
    stream->header_done = 0u;
    stream->padding_done = 0u;
    stream->partial_len = 0u;
    stream->state = QOI89_ENC_STATE_ACTIVE;
    stream->prev.r = 0u;
    stream->prev.g = 0u;
    stream->prev.b = 0u;
    stream->prev.a = 255u;
    qoi89_zero_index(stream->index_table);
    stream->pending_len = 0u;
    stream->pending_pos = 0u;

    return QOI89_OK;
}

qoi89_status qoi89_encode_stream_process(
    qoi89_encode_stream *stream,
    const unsigned char *src,
    size_t src_len,
    size_t *src_used,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
) {
    qoi89_status st;
    size_t src_pos;
    qoi89_rgba px;

    if (stream == NULL || src_used == NULL || dst_written == NULL) {
        return QOI89_ERR_NULL;
    }
    if (src == NULL && src_len != 0u) {
        return QOI89_ERR_NULL;
    }
    if (dst == NULL && dst_cap != 0u) {
        return QOI89_ERR_NULL;
    }

    *src_used = 0u;
    *dst_written = 0u;

    if (stream->state == QOI89_ENC_STATE_DONE) {
        return QOI89_STREAM_FINISHED;
    }
    if (stream->state != QOI89_ENC_STATE_ACTIVE) {
        return QOI89_ERR_STATE;
    }

    src_pos = 0u;
    while (1) {
        if (stream->pending_len > 0u) {
            st = qoi89_enc_flush_pending(stream, dst, dst_cap, dst_written);
            if (st != QOI89_OK) {
                *src_used = src_pos;
                return st;
            }
            if (!stream->header_done) {
                stream->header_done = 1u;
            }
        }

        if (!stream->header_done) {
            qoi89_enc_prepare_header(stream);
            continue;
        }

        if (stream->pixels_done >= stream->pixel_count) {
            *src_used = src_pos;
            if (src_pos < src_len) {
                return QOI89_ERR_BAD_ARGUMENT;
            }
            return QOI89_STREAM_NEED_INPUT;
        }

        while (stream->partial_len < stream->channels && src_pos < src_len) {
            stream->partial[stream->partial_len++] = src[src_pos++];
        }

        if (stream->partial_len < stream->channels) {
            *src_used = src_pos;
            return QOI89_STREAM_NEED_INPUT;
        }

        qoi89_rgba_from_bytes(&px, stream->partial, stream->channels);
        stream->partial_len = 0u;
        qoi89_enc_prepare_pixel(stream, &px);
    }
}

qoi89_status qoi89_encode_stream_finish(
    qoi89_encode_stream *stream,
    unsigned char *dst,
    size_t dst_cap,
    size_t *dst_written
) {
    qoi89_status st;
    size_t out_pos;

    if (stream == NULL || dst_written == NULL) {
        return QOI89_ERR_NULL;
    }
    if (dst == NULL && dst_cap != 0u) {
        return QOI89_ERR_NULL;
    }

    *dst_written = 0u;

    if (stream->state == QOI89_ENC_STATE_DONE) {
        return QOI89_STREAM_FINISHED;
    }
    if (stream->state != QOI89_ENC_STATE_ACTIVE && stream->state != QOI89_ENC_STATE_FINISHING) {
        return QOI89_ERR_STATE;
    }

    if (stream->pixels_done != stream->pixel_count || stream->partial_len != 0u) {
        return QOI89_ERR_BAD_ARGUMENT;
    }

    stream->state = QOI89_ENC_STATE_FINISHING;

    if (stream->pending_len > 0u) {
        st = qoi89_enc_flush_pending(stream, dst, dst_cap, dst_written);
        if (st != QOI89_OK) {
            return st;
        }
        if (!stream->header_done) {
            stream->header_done = 1u;
        }
    }

    if (!stream->header_done) {
        qoi89_enc_prepare_header(stream);
        st = qoi89_enc_flush_pending(stream, dst, dst_cap, dst_written);
        if (st != QOI89_OK) {
            return st;
        }
        stream->header_done = 1u;
    }

    if (stream->run > 0u) {
        stream->pending[0] = (unsigned char)(QOI89_OP_RUN | (stream->run - 1u));
        stream->pending_len = 1u;
        stream->pending_pos = 0u;
        stream->run = 0u;
        st = qoi89_enc_flush_pending(stream, dst, dst_cap, dst_written);
        if (st != QOI89_OK) {
            return st;
        }
    }

    out_pos = *dst_written;
    while (stream->padding_done < QOI89_PADDING_SIZE && out_pos < dst_cap) {
        dst[out_pos++] = qoi89_padding[stream->padding_done++];
    }
    *dst_written = out_pos;

    if (stream->padding_done < QOI89_PADDING_SIZE) {
        return QOI89_STREAM_NEED_OUTPUT;
    }

    stream->state = QOI89_ENC_STATE_DONE;
    return QOI89_STREAM_FINISHED;
}

int qoi89_encode_stream_finished(const qoi89_encode_stream *stream) {
    return stream != NULL && stream->state == QOI89_ENC_STATE_DONE;
}

#include "mfont_internal.h"

static mft_status mft_fnt_range_check(mft_u32 file_size,
                                      mft_u32 offset,
                                      mft_u32 length)
{
    if (offset > file_size) {
        return MFT_ERR_BOUNDS;
    }
    if (length > (file_size - offset)) {
        return MFT_ERR_BOUNDS;
    }
    return MFT_OK;
}

const char *mft_status_string(mft_status status)
{
    switch (status) {
        case MFT_OK: return "ok";
        case MFT_ERR_ARGS: return "bad arguments";
        case MFT_ERR_SIGNATURE: return "bad signature";
        case MFT_ERR_BOUNDS: return "out of bounds";
        case MFT_ERR_CAPACITY: return "buffer too small";
        case MFT_ERR_FORMAT: return "bad format";
        case MFT_ERR_UNSUPPORTED: return "unsupported feature";
        case MFT_ERR_CHECKSUM: return "checksum mismatch";
        case MFT_ERR_TEXT: return "text parse error";
        case MFT_ERR_IMAGE: return "image parse error";
        case MFT_ERR_IO: return "i/o error";
        case MFT_ERR_INTERNAL: return "internal error";
        default: return "unknown error";
    }
}

void mft_image_reset(mft_image *image)
{
    if (image != 0) {
        mft_memzero(image, (mft_u32)sizeof(*image));
    }
}

mft_status mft_image_validate(const mft_image *image)
{
    mft_u32 min_capacity;
    if (image == 0 || image->pixels == 0) {
        return MFT_ERR_ARGS;
    }
    if (image->width == 0 || image->height == 0) {
        return MFT_ERR_FORMAT;
    }
    min_capacity = image->stride * image->height;
    if (image->pixels_capacity < min_capacity) {
        return MFT_ERR_CAPACITY;
    }
    switch (image->format) {
        case MFT_PIXFMT_INDEX8:
            if (image->stride < image->width) {
                return MFT_ERR_FORMAT;
            }
            if (image->palette_count == 0 || image->palette_count > 256) {
                return MFT_ERR_FORMAT;
            }
            break;
        case MFT_PIXFMT_RGB24:
            if (image->stride < image->width * 3UL) {
                return MFT_ERR_FORMAT;
            }
            break;
        case MFT_PIXFMT_RGBA32:
            if (image->stride < image->width * 4UL) {
                return MFT_ERR_FORMAT;
            }
            break;
        default:
            return MFT_ERR_UNSUPPORTED;
    }
    return MFT_OK;
}

mft_status mft_fnt_parse_header(mft_fnt_header *out,
                                const mft_u8 *fnt,
                                mft_u32 fnt_size)
{
    mft_status st;
    if (out == 0 || fnt == 0) {
        return MFT_ERR_ARGS;
    }
    if (fnt_size < MFT_FNT_HEADER_SIZE) {
        return MFT_ERR_BOUNDS;
    }
    memcpy(out->signature, fnt, MFT_FNT_SIGNATURE_SIZE);
    out->ver_hi = mft_read_le16(fnt + 12);
    out->ver_lo = mft_read_le16(fnt + 14);
    out->pcx_offset = mft_read_le32(fnt + 16);
    out->pcx_size = mft_read_le32(fnt + 20);
    out->text_offset = mft_read_le32(fnt + 24);
    out->text_size = mft_read_le32(fnt + 28);
    memcpy(out->comment, fnt + 32, MFT_FNT_COMMENT_SIZE);

    if (memcmp(out->signature, MFT_FNT_SIGNATURE, MFT_FNT_SIGNATURE_SIZE) != 0) {
        return MFT_ERR_SIGNATURE;
    }
    if (out->pcx_offset < MFT_FNT_HEADER_SIZE || out->text_offset < MFT_FNT_HEADER_SIZE) {
        return MFT_ERR_FORMAT;
    }
    st = mft_fnt_range_check(fnt_size, out->pcx_offset, out->pcx_size);
    if (st != MFT_OK) {
        return st;
    }
    st = mft_fnt_range_check(fnt_size, out->text_offset, out->text_size);
    if (st != MFT_OK) {
        return st;
    }
    if (out->pcx_offset >= out->text_offset) {
        return MFT_ERR_FORMAT;
    }
    if (out->pcx_size > (out->text_offset - out->pcx_offset)) {
        return MFT_ERR_FORMAT;
    }
    return MFT_OK;
}

void mft_fnt_comment_to_cstr(char *dst,
                             mft_u32 dst_size,
                             const mft_fnt_header *header)
{
    mft_u32 i;
    if (dst == 0 || dst_size == 0UL) {
        return;
    }
    dst[0] = 0;
    if (header == 0) {
        return;
    }
    i = 0UL;
    while (i + 1UL < dst_size && i < MFT_FNT_COMMENT_SIZE) {
        dst[i] = header->comment[i];
        if (dst[i] == 0) {
            return;
        }
        ++i;
    }
    if (i >= dst_size) {
        i = dst_size - 1UL;
    }
    dst[i] = 0;
}

mft_status mft_fnt_extract(const mft_u8 *fnt,
                           mft_u32 fnt_size,
                           mft_u8 *pcx_out,
                           mft_u32 *pcx_size,
                           char *text_out,
                           mft_u32 *text_size,
                           mft_fnt_header *header_out)
{
    mft_fnt_header header;
    mft_status st;
    st = mft_fnt_parse_header(&header, fnt, fnt_size);
    if (st != MFT_OK) {
        return st;
    }
    if (pcx_size == 0 || text_size == 0) {
        return MFT_ERR_ARGS;
    }
    if (pcx_out == 0 || *pcx_size < header.pcx_size) {
        *pcx_size = header.pcx_size;
        return MFT_ERR_CAPACITY;
    }
    if (text_out == 0 || *text_size < header.text_size + 1UL) {
        *text_size = header.text_size + 1UL;
        return MFT_ERR_CAPACITY;
    }
    memcpy(pcx_out, fnt + header.pcx_offset, (size_t)header.pcx_size);
    memcpy(text_out, fnt + header.text_offset, (size_t)header.text_size);
    text_out[header.text_size] = 0;
    *pcx_size = header.pcx_size;
    *text_size = header.text_size;
    if (header_out != 0) {
        *header_out = header;
    }
    return MFT_OK;
}

mft_status mft_fnt_pack(mft_u8 *dst,
                        mft_u32 *dst_size,
                        const mft_u8 *pcx,
                        mft_u32 pcx_size,
                        const char *text,
                        mft_u32 text_size,
                        const char *comment,
                        mft_u16 ver_hi,
                        mft_u16 ver_lo)
{
    mft_u32 need;
    mft_u32 pcx_offset;
    mft_u32 text_offset;
    if (dst == 0 || dst_size == 0 || pcx == 0 || text == 0) {
        return MFT_ERR_ARGS;
    }
    pcx_offset = MFT_FNT_HEADER_SIZE;
    text_offset = pcx_offset + pcx_size;
    need = text_offset + text_size;
    if (*dst_size < need) {
        *dst_size = need;
        return MFT_ERR_CAPACITY;
    }
    mft_memzero(dst, need);
    memcpy(dst, MFT_FNT_SIGNATURE, MFT_FNT_SIGNATURE_SIZE);
    mft_write_le16(dst + 12, ver_hi);
    mft_write_le16(dst + 14, ver_lo);
    mft_write_le32(dst + 16, pcx_offset);
    mft_write_le32(dst + 20, pcx_size);
    mft_write_le32(dst + 24, text_offset);
    mft_write_le32(dst + 28, text_size);
    if (comment != 0) {
        strncpy((char *)(dst + 32), comment, MFT_FNT_COMMENT_SIZE);
    }
    memcpy(dst + pcx_offset, pcx, (size_t)pcx_size);
    memcpy(dst + text_offset, text, (size_t)text_size);
    *dst_size = need;
    return MFT_OK;
}

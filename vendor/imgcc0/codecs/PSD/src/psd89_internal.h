#ifndef PSD89_INTERNAL_H
#define PSD89_INTERNAL_H

#include "psd89/psd89.h"

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int psd89_io_read(psd89_io *io, void *dst, psd89_u32 size);
int psd89_io_write(psd89_io *io, const void *src, psd89_u32 size);
int psd89_io_seek(psd89_io *io, psd89_u32 offset);
psd89_u32 psd89_io_tell(psd89_io *io);

int psd89_rd_u8(psd89_io *io, psd89_u8 *v);
int psd89_rd_be16(psd89_io *io, psd89_u16 *v);
int psd89_rd_be16s(psd89_io *io, psd89_s16 *v);
int psd89_rd_be32(psd89_io *io, psd89_u32 *v);
int psd89_rd_be32s(psd89_io *io, psd89_s32 *v);

int psd89_wr_u8(psd89_io *io, psd89_u8 v);
int psd89_wr_be16(psd89_io *io, psd89_u16 v);
int psd89_wr_be32(psd89_io *io, psd89_u32 v);

int psd89_skip(psd89_io *io, psd89_u32 size);
int psd89_read_pascal_string(psd89_io *io, char *dst, psd89_u32 dst_size, psd89_u32 pad_multiple);
int psd89_write_pascal_string(psd89_io *io, const char *src, psd89_u32 pad_multiple);
int psd89_copy_bytes(psd89_io *dst, psd89_io *src, psd89_u32 src_offset, psd89_u32 size);
int psd89_patch_be32(psd89_io *io, psd89_u32 offset, psd89_u32 value);

int psd89_packbits_decode_row(psd89_io *io, psd89_u8 *dst, psd89_u32 row_bytes, psd89_u32 encoded_size);
psd89_u32 psd89_packbits_encoded_len(const psd89_u8 *src, psd89_u32 row_bytes);
int psd89_packbits_write_row(psd89_io *io, const psd89_u8 *src, psd89_u32 row_bytes);
int psd89_packbits_write_zero_row(psd89_io *io, psd89_u32 row_bytes);

int psd89_rd_ieee8_q16(psd89_io *io, psd89_fx16 *out);
int psd89_wr_ieee8_from_q16(psd89_io *io, psd89_fx16 value);

int psd89_zip_decode_plane(psd89_io *io,
                           psd89_u32 data_offset,
                           psd89_u32 encoded_size,
                           psd89_u8 *dst,
                           psd89_u32 stride,
                           psd89_u32 rows,
                           psd89_u32 cols,
                           int prediction);
int psd89_zip_decode_composite(psd89_io *io,
                               psd89_u32 data_offset,
                               psd89_u32 encoded_size,
                               psd89_u8 **planes,
                               psd89_u32 stride,
                               psd89_u16 channels,
                               psd89_u32 rows,
                               psd89_u32 cols,
                               int prediction);
int psd89_zip_write_plane(psd89_io *io,
                          const psd89_u8 *plane,
                          psd89_u32 stride,
                          psd89_u32 rows,
                          psd89_u32 cols,
                          int prediction);
int psd89_zip_write_composite(psd89_io *io,
                              const psd89_u8 **planes,
                              psd89_u32 stride,
                              psd89_u16 channels,
                              psd89_u32 rows,
                              psd89_u32 cols,
                              int prediction);

void psd89_descriptor_summary_init(psd89_descriptor_summary *summary);
int psd89_descriptor_parse_io(psd89_io *io, psd89_descriptor_summary *summary);

int psd89_parse_lrfx_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len);
int psd89_write_lrfx_tag(psd89_io *io, const psd89_layer *layer);
int psd89_parse_tysh_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len);
int psd89_write_tysh_tag(psd89_io *io, const psd89_layer *layer);
int psd89_parse_txt2_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len);
int psd89_write_txt2_tag(psd89_io *io, const psd89_layer *layer);
int psd89_parse_lfx2_tag(psd89_layer *layer, psd89_io *io, psd89_u32 len);
int psd89_write_lfx2_tag(psd89_io *io, const psd89_layer *layer);
int psd89_parse_smart_object_tag(psd89_layer *layer, const char key[4], psd89_io *io, psd89_u32 len);
int psd89_write_smart_object_tag(psd89_io *io, const psd89_layer *layer);

int psd89_layer_channel_dims(const psd89_layer *layer,
                             psd89_s16 channel_id,
                             psd89_u32 *rows,
                             psd89_u32 *cols);
int psd89_layer_mask_info_for_channel(const psd89_layer *layer,
                                      psd89_s16 channel_id,
                                      psd89_s32 *top,
                                      psd89_s32 *left,
                                      psd89_s32 *bottom,
                                      psd89_s32 *right,
                                      psd89_u8 *default_color,
                                      psd89_u8 *flags);

#ifdef __cplusplus
}
#endif

#endif

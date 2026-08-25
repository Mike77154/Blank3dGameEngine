#ifndef W2F_TYPES_H
#define W2F_TYPES_H

/* C89-sized aliases. This code assumes unsigned long has at least 32 bits. */
typedef unsigned char  W2F_U8;
typedef signed char    W2F_S8;
typedef unsigned short W2F_U16;
typedef signed short   W2F_S16;
typedef unsigned long  W2F_U32;
typedef signed long    W2F_S32;

typedef struct W2F_TableRec_ {
    W2F_U32 tag;
    W2F_U32 orig_len;
    W2F_U32 transform_len;
    W2F_U32 final_len;
    W2F_U32 src_off;
    W2F_U32 dst_off;
    W2F_U32 checksum;
    W2F_U8  flags;
    W2F_U8  transform_version;
    W2F_U8  is_custom_tag;
    W2F_U8  reserved;
} W2F_TableRec;

typedef struct W2F_DecoderInfo_ {
    W2F_U32 flavor;
    W2F_U32 declared_length;
    W2F_U32 total_sfnt_size;
    W2F_U32 total_compressed_size;
    W2F_U16 num_tables;
    W2F_U16 major_version;
    W2F_U16 minor_version;
    W2F_U32 meta_offset;
    W2F_U32 meta_length;
    W2F_U32 meta_orig_length;
    W2F_U32 priv_offset;
    W2F_U32 priv_length;
    W2F_U32 compressed_offset;
    W2F_U32 decompressed_table_bytes;
    W2F_U8  is_collection;
    W2F_U8  has_glyf_loca_transform;
    W2F_U8  reserved0;
    W2F_U8  reserved1;
} W2F_DecoderInfo;

typedef int (*W2F_BrotliDecodeFn)(const W2F_U8 *src,
                                  W2F_U32 src_len,
                                  W2F_U8 *dst,
                                  W2F_U32 dst_cap,
                                  W2F_U32 *dst_len);

#endif

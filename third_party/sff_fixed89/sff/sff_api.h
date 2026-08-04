/* sff_api.h - fixed-capacity, no-heap SFF parser/decoder */
#ifndef SFF_API_H
#define SFF_API_H

#include "sff_types.h"
#include "sff_config.h"
#include "sff_codec.h"
#include "sff_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SFF_OK                          (0)
#define SFF_ERR_ARG                     (-1)
#define SFF_ERR_IO                      (-2)
#define SFF_ERR_BAD_SIGNATURE           (-3)
#define SFF_ERR_UNSUPPORTED_VERSION     (-4)
#define SFF_ERR_CORRUPT                 (-5)
#define SFF_ERR_NO_SPRITE               (-6)
#define SFF_ERR_UNSUPPORTED_COMPRESSION (-7)
#define SFF_ERR_CODEC_REQUIRED          (-8)
#define SFF_ERR_BUFFER_TOO_SMALL        (-9)
#define SFF_ERR_CAPACITY                (-10)
#define SFF_ERR_NOT_INDEXED             (-11)

typedef enum SffKind {
    SFF_KIND_UNKNOWN = 0,
    SFF_KIND_V1 = 1,
    SFF_KIND_V2 = 2
} SffKind;

typedef enum SffSourceKind {
    SFF_SOURCE_NONE = 0,
    SFF_SOURCE_MEMORY = 1,
    SFF_SOURCE_IO = 2
} SffSourceKind;

#define SFF_COMP_NONE       (0x00u)
#define SFF_COMP_RLE8_SFF   (0x02u)
#define SFF_COMP_RLE5       (0x03u)
#define SFF_COMP_LZ5        (0x04u)
#define SFF_COMP_PNG8       (0x0Au)
#define SFF_COMP_PNG24      (0x0Bu)
#define SFF_COMP_PNG32      (0x0Cu)
#define SFF_COMP_V1_PCX     (0xFEu)

typedef struct SffOpenOptions {
    int tolerant; /* 0 = spec-first, 1 = allow compatibility fallbacks */
    const SffImageCodec *codec; /* optional PNG backend */
} SffOpenOptions;

typedef struct SffV1Header {
    sff_u8 verhi;
    sff_u8 verlo;
    sff_u8 verlo2;
    sff_u8 verlo3;
    sff_u32 num_groups;
    sff_u32 num_images;
    sff_u32 first_subfile_offset;
    sff_u32 subheader_size;
    sff_u8 palette_type;
} SffV1Header;

typedef struct SffV2Header {
    sff_u8 ver_lo3;
    sff_u8 ver_lo2;
    sff_u8 ver_lo1;
    sff_u8 ver_hi;
    sff_u32 sprite_table_offset;
    sff_u32 sprite_count;
    sff_u32 palette_table_offset;
    sff_u32 palette_count;
    sff_u32 ldata_offset;
    sff_u32 ldata_length;
    sff_u32 tdata_offset;
    sff_u32 tdata_length;
    sff_u32 palette_base_offset;
    sff_u32 header_size;
} SffV2Header;

typedef struct SffSpriteEntry {
    sff_u16 group;
    sff_u16 item;
    sff_u16 w;
    sff_u16 h;
    sff_s16 axis_x;
    sff_s16 axis_y;
    sff_u16 link_index;     /* v1/v2 link target if any */
    sff_u8 compression;     /* v1 uses SFF_COMP_V1_PCX */
    sff_u8 depth;           /* 8 for v1/most indexed v2; 24/32 for PNG truecolor */
    sff_u16 palette_index;  /* v2 only; 0xFFFF for v1 */
    sff_u16 load_mode;      /* v2 only */
    sff_u8 same_palette;    /* v1 only */
    sff_u8 flags;
    sff_u32 offset;         /* v1 subheader offset, 0 for v2 */
    sff_u32 next_offset;    /* v1 next subheader offset */
    sff_u32 raw_length;     /* v1 length field / v2 blob length */
    sff_u32 data_ofs;       /* absolute blob offset in source */
    sff_u32 data_len;       /* effective blob length after link resolution */
    sff_u32 owner_index;    /* resolved owner sprite index or 0xFFFFFFFF */
} SffSpriteEntry;

typedef struct SffPaletteEntry {
    sff_u16 group;
    sff_u16 item;
    sff_u16 raw_a;       /* spec/community variants differ; preserved */
    sff_u16 raw_b;       /* spec/community variants differ; preserved */
    sff_u16 link_index;  /* 0xFFFF if none */
    sff_u16 colors;      /* if known; else 0 */
    sff_u32 data_ofs;    /* absolute file/source offset */
    sff_u32 data_len;    /* bytes */
} SffPaletteEntry;

typedef struct SffSpriteInfo {
    sff_u16 group;
    sff_u16 item;
    sff_s16 axis_x;
    sff_s16 axis_y;
    sff_u16 w;
    sff_u16 h;
    sff_u8 depth;
    sff_u8 compression;
    sff_u16 palette_index;
    sff_u16 load_mode;
    sff_u32 data_ofs;
    sff_u32 data_len;
    sff_u32 link_index;
} SffSpriteInfo;

typedef struct SffFile {
    SffKind kind;
    SffSourceKind source_kind;
    int tolerant;
    const SffImageCodec *codec;

    const sff_u8 *mem;
    SffIo io;
    sff_u32 size;

    SffV1Header v1;
    SffV2Header v2;

    sff_u32 sprite_count;
    sff_u32 palette_count;

    SffSpriteEntry sprites[SFF_MAX_SPRITES];
    SffPaletteEntry palettes[SFF_MAX_PALETTES];
    sff_u32 palette_owner[SFF_MAX_SPRITES];

    SffMap sprite_map;
    SffMapSlot sprite_map_slots[SFF_MAP_CAPACITY];
} SffFile;

void    sff_init(SffFile *s);
void    sff_close(SffFile *s);
int     sff_open_memory(SffFile *s, const void *data, sff_u32 size, const SffOpenOptions *opt);
int     sff_open_io(SffFile *s, const SffIo *io, const SffOpenOptions *opt);

SffKind sff_kind(const SffFile *s);
sff_u32 sff_sprite_count(const SffFile *s);
int     sff_find_sprite(const SffFile *s, sff_u16 group, sff_u16 item, sff_u32 *out_index);
int     sff_get_sprite_info(const SffFile *s, sff_u32 index, SffSpriteInfo *out_info);
int     sff_read_palette_rgb(const SffFile *s, sff_u16 palette_index, sff_u8 out_rgb[768]);

/* Caller-owned decode APIs.
   - out_pixels_size must be >= w*h for indexed decode.
   - out_rgba_size must be >= w*h*4 for RGBA decode.
   - work_buf is optional for memory-backed raw copies, but required for:
     * non-memory-backed PCX/PNG/compressed sprites
     * RGBA conversion of indexed sprites
*/
int sff_decode_sprite_indexed_into(const SffFile *s, sff_u32 index,
                                   sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                   sff_u16 *out_w, sff_u16 *out_h,
                                   sff_u8 out_pal_rgb[768], int *out_has_palette,
                                   sff_u8 *work_buf, sff_u32 work_buf_size);

int sff_decode_sprite_rgba_into(const SffFile *s, sff_u32 index,
                                sff_u8 *out_rgba, sff_u32 out_rgba_size,
                                sff_u16 *out_w, sff_u16 *out_h,
                                sff_u8 *work_buf, sff_u32 work_buf_size);

#ifdef __cplusplus
}
#endif

#endif /* SFF_API_H */

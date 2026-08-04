#include "sff_api.h"
#include "sff_endian.h"
#include "sff_pcx.h"
#include "sff_decomp.h"
#include "sff_png_builtin.h"
#include <string.h>

#define SFF_NO_INDEX 0xFFFFFFFFu

static int sff__guard_range(sff_u32 off, sff_u32 need, sff_u32 size)
{
    if (off > size) return 0;
    if (need > size - off) return 0;
    return 1;
}

static int sff__mul_u32(sff_u32 a, sff_u32 b, sff_u32 *out)
{
    if (!out) return 0;
    if (a == 0u || b == 0u) {
        *out = 0u;
        return 1;
    }
    if (a > (0xFFFFFFFFu / b)) return 0;
    *out = a * b;
    return 1;
}

static int sff__sig_ok(const sff_u8 *hdr)
{
    if (!hdr) return 0;
    if (hdr[0] != 'E' || hdr[1] != 'l' || hdr[2] != 'e' || hdr[3] != 'c' ||
        hdr[4] != 'b' || hdr[5] != 'y' || hdr[6] != 't' || hdr[7] != 'e' ||
        hdr[8] != 'S' || hdr[9] != 'p' || hdr[10] != 'r') {
        return 0;
    }
    return 1;
}

static int sff__looks_like_png(const sff_u8 *p, sff_u32 n)
{
    if (!p || n < 16u) return 0;
    if (p[0] != 0x89u || p[1] != (sff_u8)'P' || p[2] != (sff_u8)'N' || p[3] != (sff_u8)'G') return 0;
    if (p[4] != 0x0Du || p[5] != 0x0Au || p[6] != 0x1Au || p[7] != 0x0Au) return 0;
    return 1;
}

static int sff__png_locate_payload(const sff_u8 *p, sff_u32 n,
                                   const sff_u8 **out_png, sff_u32 *out_png_len)
{
    if (!p || !out_png || !out_png_len) return 0;
    if (sff__looks_like_png(p, n)) {
        *out_png = p;
        *out_png_len = n;
        return 1;
    }
    if (n > 4u && sff__looks_like_png(p + 4u, n - 4u)) {
        *out_png = p + 4u;
        *out_png_len = n - 4u;
        return 1;
    }
    return 0;
}

/* Reads width/height/color type from PNG IHDR.
   color_type: 3 = indexed, 2/4/6 = truecolor variants, 0 = grayscale. */
static int sff__png_peek(const sff_u8 *p, sff_u32 n, sff_u16 *out_w, sff_u16 *out_h, sff_u8 *out_color_type)
{
    sff_u32 ihdr_len;
    sff_u32 w32;
    sff_u32 h32;
    if (!sff__looks_like_png(p, n) || n < 33u) return 0;
    ihdr_len = ((sff_u32)p[8] << 24) | ((sff_u32)p[9] << 16) | ((sff_u32)p[10] << 8) | (sff_u32)p[11];
    if (ihdr_len != 13u) return 0;
    if (p[12] != 'I' || p[13] != 'H' || p[14] != 'D' || p[15] != 'R') return 0;
    w32 = ((sff_u32)p[16] << 24) | ((sff_u32)p[17] << 16) | ((sff_u32)p[18] << 8) | (sff_u32)p[19];
    h32 = ((sff_u32)p[20] << 24) | ((sff_u32)p[21] << 16) | ((sff_u32)p[22] << 8) | (sff_u32)p[23];
    if (w32 == 0u || h32 == 0u || w32 > 0xFFFFu || h32 > 0xFFFFu) return 0;
    if (out_w) *out_w = (sff_u16)w32;
    if (out_h) *out_h = (sff_u16)h32;
    if (out_color_type) *out_color_type = p[25];
    return 1;
}

static int sff__read_at(const SffFile *s, sff_u32 off, void *dst, sff_u32 len)
{
    if (!s || !dst) return 0;
    if (!sff__guard_range(off, len, s->size)) return 0;

    if (s->source_kind == SFF_SOURCE_MEMORY) {
        memcpy(dst, s->mem + off, (size_t)len);
        return 1;
    }

    if (s->source_kind == SFF_SOURCE_IO) {
        if (!s->io.read_at) return 0;
        return s->io.read_at(s->io.user, off, dst, len) ? 1 : 0;
    }

    return 0;
}

static const sff_u8 *sff__memory_ptr(const SffFile *s, sff_u32 off, sff_u32 len)
{
    if (!s || s->source_kind != SFF_SOURCE_MEMORY || !s->mem) return 0;
    if (!sff__guard_range(off, len, s->size)) return 0;
    return s->mem + off;
}

static int sff__blob_to_ptr_or_work(const SffFile *s,
                                    sff_u32 off, sff_u32 len,
                                    sff_u8 *work_buf, sff_u32 work_buf_size,
                                    const sff_u8 **out_blob)
{
    const sff_u8 *p;

    if (!out_blob) return 0;
    *out_blob = 0;

    p = sff__memory_ptr(s, off, len);
    if (p) {
        *out_blob = p;
        return 1;
    }

    if (!work_buf || work_buf_size < len) return 0;
    if (!sff__read_at(s, off, work_buf, len)) return 0;
    *out_blob = work_buf;
    return 1;
}

static int sff__map_build(SffFile *s)
{
    sff_u32 i;

    if (!s || !sff_map_init_fixed(&s->sprite_map, s->sprite_map_slots, SFF_MAP_CAPACITY)) {
        return 0;
    }

    for (i = 0u; i < s->sprite_count; ++i) {
        sff_u32 key;
        key = ((sff_u32)s->sprites[i].group << 16) | (sff_u32)s->sprites[i].item;
        if (!sff_map_has(&s->sprite_map, key)) {
            if (!sff_map_set(&s->sprite_map, key, i)) return 0;
        }
    }

    return 1;
}

static sff_u32 sff__v1_effective_data_len(sff_u32 length_field, sff_u32 subhdr_size, sff_u32 avail)
{
    if (length_field == 0u || avail == 0u) return 0u;
    if (length_field < 128u && avail >= 128u) return avail;
    if (length_field <= avail) return length_field;
    if (length_field > subhdr_size && (length_field - subhdr_size) <= avail) {
        return length_field - subhdr_size;
    }
    return avail;
}

static int sff__sprite_capacity_ok(const SffFile *s)
{
    return s && s->sprite_count < SFF_MAX_SPRITES;
}

static int sff__v1_parse_header(SffFile *s, const sff_u8 hdr[512])
{
    sff_u32 subhdr0;

    if (!s || !hdr) return 0;
    if (!sff__sig_ok(hdr)) return 0;

    s->v1.verhi = hdr[12];
    s->v1.verlo = hdr[13];
    s->v1.verlo2 = hdr[14];
    s->v1.verlo3 = hdr[15];

    if (s->v1.verhi >= 2u) return 0;
    if (s->v1.verhi == 0u && s->v1.verlo == 0u) return 0;

    s->v1.num_groups = sff_rd_le_u32(hdr + 16);
    s->v1.num_images = sff_rd_le_u32(hdr + 20);
    s->v1.first_subfile_offset = sff_rd_le_u32(hdr + 24);

    subhdr0 = sff_rd_le_u32(hdr + 28);
    if (subhdr0 == 28u || subhdr0 == 32u) {
        s->v1.subheader_size = subhdr0;
    } else {
        s->v1.subheader_size = 32u;
    }

    s->v1.palette_type = hdr[32];
    return 1;
}

static int sff__offset_already_seen(const SffFile *s, sff_u32 upto, sff_u32 off)
{
    sff_u32 i;
    for (i = 0u; i < upto; ++i) {
        if (s->sprites[i].offset == off) return 1;
    }
    return 0;
}

static int sff__v1_push_sprite(SffFile *s, const SffSpriteEntry *sp)
{
    if (!s || !sp || !sff__sprite_capacity_ok(s)) return 0;
    s->sprites[s->sprite_count] = *sp;
    s->palette_owner[s->sprite_count] = SFF_NO_INDEX;
    ++s->sprite_count;
    return 1;
}

static int sff__v1_parse_linked(SffFile *s)
{
    sff_u32 off;
    sff_u32 subhdr_size;

    off = s->v1.first_subfile_offset;
    subhdr_size = s->v1.subheader_size;
    s->sprite_count = 0u;

    while (off != 0u) {
        sff_u8 sh[32];
        SffSpriteEntry sp;
        sff_u32 next_off;
        sff_u32 length;

        if (!sff__guard_range(off, subhdr_size, s->size)) return 0;
        if (sff__offset_already_seen(s, s->sprite_count, off)) return 0;
        if (!sff__read_at(s, off, sh, subhdr_size)) return 0;

        memset(&sp, 0, sizeof(sp));
        next_off = sff_rd_le_u32(sh + 0);
        length = sff_rd_le_u32(sh + 4);

        sp.offset = off;
        sp.next_offset = next_off;
        sp.raw_length = length;
        sp.axis_x = sff_rd_le_s16(sh + 8);
        sp.axis_y = sff_rd_le_s16(sh + 10);
        sp.group = sff_rd_le_u16(sh + 12);
        sp.item = sff_rd_le_u16(sh + 14);
        sp.link_index = (subhdr_size >= 18u) ? sff_rd_le_u16(sh + 16) : 0u;
        sp.same_palette = (subhdr_size >= 19u) ? sh[18] : 0u;
        sp.compression = (sff_u8)SFF_COMP_V1_PCX;
        sp.depth = 8u;
        sp.palette_index = 0xFFFFu;
        sp.load_mode = 0u;
        sp.owner_index = SFF_NO_INDEX;
        sp.data_ofs = off + subhdr_size;
        sp.data_len = 0u;
        sp.w = 0u;
        sp.h = 0u;

        if (length == 0u) {
            if ((sff_u32)sp.link_index != s->sprite_count) {
                sp.owner_index = (sff_u32)sp.link_index;
            }
        } else {
            sff_u32 avail;
            avail = 0u;
            if (next_off != 0u && next_off > sp.data_ofs && next_off <= s->size) {
                avail = next_off - sp.data_ofs;
            } else if (sp.data_ofs <= s->size) {
                avail = s->size - sp.data_ofs;
            }
            sp.data_len = sff__v1_effective_data_len(length, subhdr_size, avail);
            if (sp.data_len == 0u || !sff__guard_range(sp.data_ofs, sp.data_len, s->size)) return 0;
            sp.owner_index = s->sprite_count;
        }

        if (!sff__v1_push_sprite(s, &sp)) return 0;
        off = next_off;
    }

    return s->sprite_count > 0u ? 1 : 0;
}

static int sff__v1_parse_linear(SffFile *s)
{
    sff_u32 starts[2];
    sff_u32 si;

    starts[0] = s->v1.first_subfile_offset;
    starts[1] = 512u;
    s->sprite_count = 0u;

    for (si = 0u; si < 2u; ++si) {
        sff_u32 off;
        sff_u32 subhdr_size;
        sff_u32 scanned;

        off = starts[si];
        subhdr_size = s->v1.subheader_size;
        scanned = 0u;

        if (off == 0u || off >= s->size) continue;

        while (1) {
            sff_u8 sh[32];
            SffSpriteEntry sp;
            sff_u32 next_off;
            sff_u32 length;
            sff_u32 step;

            if (!sff__guard_range(off, subhdr_size, s->size)) break;
            if (!sff__read_at(s, off, sh, subhdr_size)) break;

            memset(&sp, 0, sizeof(sp));
            next_off = sff_rd_le_u32(sh + 0);
            length = sff_rd_le_u32(sh + 4);

            sp.offset = off;
            sp.next_offset = next_off;
            sp.raw_length = length;
            sp.axis_x = sff_rd_le_s16(sh + 8);
            sp.axis_y = sff_rd_le_s16(sh + 10);
            sp.group = sff_rd_le_u16(sh + 12);
            sp.item = sff_rd_le_u16(sh + 14);
            sp.link_index = (subhdr_size >= 18u) ? sff_rd_le_u16(sh + 16) : 0u;
            sp.same_palette = (subhdr_size >= 19u) ? sh[18] : 0u;
            sp.compression = (sff_u8)SFF_COMP_V1_PCX;
            sp.depth = 8u;
            sp.palette_index = 0xFFFFu;
            sp.owner_index = SFF_NO_INDEX;
            sp.data_ofs = off + subhdr_size;

            if (length == 0u) {
                if ((sff_u32)sp.link_index != s->sprite_count) {
                    sp.owner_index = (sff_u32)sp.link_index;
                }
                sp.data_len = 0u;
            } else {
                sff_u32 avail;
                avail = 0u;
                if (next_off != 0u && next_off > sp.data_ofs && next_off <= s->size) {
                    avail = next_off - sp.data_ofs;
                } else if (sp.data_ofs <= s->size) {
                    avail = s->size - sp.data_ofs;
                }
                sp.data_len = sff__v1_effective_data_len(length, subhdr_size, avail);
                if (sp.data_len > 0u && !sff__guard_range(sp.data_ofs, sp.data_len, s->size)) {
                    sp.data_len = 0u;
                }
            }

            if (!sff__v1_push_sprite(s, &sp)) return 0;

            step = 0u;
            if (next_off != 0u && next_off > off && next_off <= s->size) {
                step = next_off - off;
                off = next_off;
            } else {
                step = subhdr_size + sp.data_len;
                if (step == 0u) break;
                if (!sff__guard_range(off, step, s->size)) break;
                off += step;
            }

            scanned += step;
            if (scanned > s->size) break;
        }

        if (s->sprite_count > 0u) break;
    }

    return s->sprite_count > 0u ? 1 : 0;
}

static sff_u32 sff__v1_find_owner_same_group_item(const SffFile *s, sff_u32 i, sff_u16 group, sff_u16 item)
{
    sff_s32 j;
    for (j = (sff_s32)i - 1; j >= 0; --j) {
        const SffSpriteEntry *sp;
        sp = &s->sprites[(sff_u32)j];
        if (sp->group == group && sp->item == item && sp->data_len > 0u) {
            return (sff_u32)j;
        }
    }
    return SFF_NO_INDEX;
}

static void sff__v1_resolve_links(SffFile *s)
{
    sff_u32 i;
    sff_u32 last_data;

    last_data = SFF_NO_INDEX;

    for (i = 0u; i < s->sprite_count; ++i) {
        SffSpriteEntry *sp;
        sp = &s->sprites[i];

        if (sp->raw_length > 0u && sp->data_len > 0u) {
            sp->owner_index = i;
            last_data = i;
            continue;
        }

        {
            sff_u32 owner;
            sff_u32 cur;
            sff_u32 hop;

            owner = SFF_NO_INDEX;
            cur = sp->owner_index;
            hop = 0u;

            while (cur != SFF_NO_INDEX && cur < s->sprite_count && hop < s->sprite_count) {
                if (cur == i) break;
                if (s->sprites[cur].raw_length > 0u && s->sprites[cur].data_len > 0u) {
                    owner = cur;
                    break;
                }
                if (s->sprites[cur].owner_index == cur) break;
                cur = s->sprites[cur].owner_index;
                ++hop;
            }

            if (owner == SFF_NO_INDEX && s->tolerant) {
                owner = sff__v1_find_owner_same_group_item(s, i, sp->group, sp->item);
            }
            if (owner == SFF_NO_INDEX && s->tolerant) {
                owner = last_data;
            }

            if (owner != SFF_NO_INDEX && owner < s->sprite_count &&
                s->sprites[owner].data_len > 0u) {
                sp->owner_index = owner;
                sp->data_ofs = s->sprites[owner].data_ofs;
                sp->data_len = s->sprites[owner].data_len;
            } else {
                sp->owner_index = SFF_NO_INDEX;
                sp->data_len = 0u;
            }
        }
    }
}

static void sff__v1_build_palette_owner_cache(SffFile *s)
{
    sff_u32 i;
    sff_u32 current_root;
    sff_u32 first_root;
    sff_u32 first_data;

    if (!s) return;

    current_root = SFF_NO_INDEX;
    first_root = SFF_NO_INDEX;
    first_data = SFF_NO_INDEX;

    /* In SFF v1, the subheader's same_palette flag is the authoritative
       relationship.  Do not guess from a lone 0x0C byte inside compressed
       PCX data: palette-less blobs can contain that value at the candidate
       tail position by coincidence.  This flag-only pass also works for
       streamed SFF sources because no whole sprite blob is needed here. */
    for (i = 0u; i < s->sprite_count; ++i) {
        const SffSpriteEntry *sp;
        sff_u32 owner;

        sp = &s->sprites[i];
        owner = SFF_NO_INDEX;

        if (sp->data_len > 0u && first_data == SFF_NO_INDEX) {
            first_data = i;
        }
        if (sp->data_len > 0u && sp->same_palette == 0u) {
            current_root = i;
            if (first_root == SFF_NO_INDEX) first_root = i;
            owner = i;
        } else {
            owner = current_root;
        }

        if (owner == SFF_NO_INDEX) owner = first_root;
        if (owner == SFF_NO_INDEX) owner = first_data;
        s->palette_owner[i] = owner;
    }
}

static int sff__parse_v1(SffFile *s, const sff_u8 hdr[512])
{
    if (!sff__v1_parse_header(s, hdr)) return 0;
    if (s->v1.first_subfile_offset < 512u || s->v1.first_subfile_offset >= s->size) return 0;

    if (!sff__v1_parse_linked(s)) {
        if (!s->tolerant) return 0;
        if (!sff__v1_parse_linear(s)) return 0;
    }

    sff__v1_resolve_links(s);
    sff__v1_build_palette_owner_cache(s);

    s->kind = SFF_KIND_V1;
    return 1;
}

static int sff__v2_parse_header(SffFile *s, const sff_u8 hdr[512])
{
    if (!s || !hdr) return 0;
    if (!sff__sig_ok(hdr)) return 0;

    s->v2.ver_lo3 = hdr[0x0C];
    s->v2.ver_lo2 = hdr[0x0D];
    s->v2.ver_lo1 = hdr[0x0E];
    s->v2.ver_hi  = hdr[0x0F];
    if (s->v2.ver_hi < 2u) return 0;

    /* This layout matches common Elecbyte-compatible readers and the runtime oracle:
       reserved[20], sprite table, sprite count, palette table, palette count,
       ldata offset/length, tdata offset/length. */
    s->v2.sprite_table_offset  = sff_rd_le_u32(hdr + 0x24);
    s->v2.sprite_count         = sff_rd_le_u32(hdr + 0x28);
    s->v2.palette_table_offset = sff_rd_le_u32(hdr + 0x2C);
    s->v2.palette_count        = sff_rd_le_u32(hdr + 0x30);
    s->v2.ldata_offset         = sff_rd_le_u32(hdr + 0x34);
    s->v2.ldata_length         = sff_rd_le_u32(hdr + 0x38);
    s->v2.tdata_offset         = sff_rd_le_u32(hdr + 0x3C);
    s->v2.tdata_length         = sff_rd_le_u32(hdr + 0x40);
    s->v2.palette_base_offset  = s->v2.ldata_offset;
    s->v2.header_size          = 512u;

    if (s->v2.sprite_count > SFF_MAX_SPRITES) return 0;
    if (s->v2.palette_count > SFF_MAX_PALETTES) return 0;
    if (!sff__guard_range(s->v2.sprite_table_offset, s->v2.sprite_count * 28u, s->size)) return 0;
    if (!sff__guard_range(s->v2.palette_table_offset, s->v2.palette_count * 16u, s->size)) return 0;
    if (!sff__guard_range(s->v2.ldata_offset, s->v2.ldata_length, s->size)) return 0;
    if (!sff__guard_range(s->v2.tdata_offset, s->v2.tdata_length, s->size)) return 0;

    return 1;
}

static void sff__v2_resolve_links(SffFile *s)
{
    sff_u32 i;
    sff_u32 last_data;

    last_data = SFF_NO_INDEX;

    for (i = 0u; i < s->sprite_count; ++i) {
        SffSpriteEntry *sp;
        sp = &s->sprites[i];

        if (sp->data_len > 0u) {
            sp->owner_index = i;
            last_data = i;
            continue;
        }

        {
            sff_u32 owner;
            sff_u32 cur;
            sff_u32 hop;

            owner = SFF_NO_INDEX;
            cur = (sff_u32)sp->link_index;
            hop = 0u;

            while (cur < s->sprite_count && hop < s->sprite_count) {
                if (cur == i) break;
                if (s->sprites[cur].data_len > 0u) {
                    owner = cur;
                    break;
                }
                cur = (sff_u32)s->sprites[cur].link_index;
                ++hop;
            }

            if (owner == SFF_NO_INDEX && s->tolerant) owner = last_data;

            if (owner != SFF_NO_INDEX && owner < s->sprite_count && s->sprites[owner].data_len > 0u) {
                sp->owner_index = owner;
                sp->data_ofs = s->sprites[owner].data_ofs;
                sp->data_len = s->sprites[owner].data_len;
            } else {
                sp->owner_index = SFF_NO_INDEX;
            }
        }
    }
}

static int sff__parse_v2(SffFile *s, const sff_u8 hdr[512])
{
    sff_u32 i;

    if (!sff__v2_parse_header(s, hdr)) return 0;

    s->sprite_count = s->v2.sprite_count;
    s->palette_count = s->v2.palette_count;

    for (i = 0u; i < s->palette_count; ++i) {
        sff_u8 ent[16];
        SffPaletteEntry *p;
        sff_u32 rel_ofs;
        sff_u32 length;

        if (!sff__read_at(s, s->v2.palette_table_offset + (16u * i), ent, 16u)) return 0;
        p = &s->palettes[i];
        memset(p, 0, sizeof(*p));

        p->group = sff_rd_le_u16(ent + 0x00);
        p->item  = sff_rd_le_u16(ent + 0x02);
        p->raw_a = sff_rd_le_u16(ent + 0x04);
        p->raw_b = sff_rd_le_u16(ent + 0x06);
        rel_ofs  = sff_rd_le_u32(ent + 0x08);
        length   = sff_rd_le_u32(ent + 0x0C);

        p->link_index = 0xFFFFu;
        p->colors = 0u;
        if (length == 0u && p->raw_b < s->palette_count) {
            p->link_index = p->raw_b;
        }
        if (length != 0u && p->raw_a > 0u && p->raw_a <= 256u) {
            p->colors = p->raw_a;
        }

        p->data_ofs = s->v2.palette_base_offset + rel_ofs;
        p->data_len = length;
        if (length > 0u && !sff__guard_range(p->data_ofs, length, s->size)) return 0;
    }

    for (i = 0u; i < s->sprite_count; ++i) {
        sff_u8 ent[28];
        SffSpriteEntry *sp;
        sff_u32 rel_ofs;
        sff_u32 length;
        sff_u32 data_base;

        if (!sff__read_at(s, s->v2.sprite_table_offset + (28u * i), ent, 28u)) return 0;

        sp = &s->sprites[i];
        memset(sp, 0, sizeof(*sp));

        sp->group = sff_rd_le_u16(ent + 0x00);
        sp->item  = sff_rd_le_u16(ent + 0x02);
        sp->w     = sff_rd_le_u16(ent + 0x04);
        sp->h     = sff_rd_le_u16(ent + 0x06);
        sp->axis_x = sff_rd_le_s16(ent + 0x08);
        sp->axis_y = sff_rd_le_s16(ent + 0x0A);
        sp->link_index = sff_rd_le_u16(ent + 0x0C);
        sp->compression = ent[0x0E];
        sp->depth = ent[0x0F];
        rel_ofs = sff_rd_le_u32(ent + 0x10);
        length = sff_rd_le_u32(ent + 0x14);
        sp->palette_index = sff_rd_le_u16(ent + 0x18);
        sp->load_mode = sff_rd_le_u16(ent + 0x1A);
        sp->raw_length = length;
        sp->owner_index = SFF_NO_INDEX;

        data_base = ((sp->load_mode & 1u) == 0u) ? s->v2.ldata_offset : s->v2.tdata_offset;

        if (length > 0u) {
            sp->data_ofs = data_base + rel_ofs;
            sp->data_len = length;
            if (!sff__guard_range(sp->data_ofs, sp->data_len, s->size)) return 0;
        } else {
            sp->data_ofs = 0u;
            sp->data_len = 0u;
        }
    }

    sff__v2_resolve_links(s);
    s->kind = SFF_KIND_V2;
    return 1;
}

void sff_init(SffFile *s)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    (void)sff_map_init_fixed(&s->sprite_map, s->sprite_map_slots, SFF_MAP_CAPACITY);
}

void sff_close(SffFile *s)
{
    sff_init(s);
}

static int sff__open_common(SffFile *s, const SffOpenOptions *opt)
{
    sff_u8 hdr[512];

    if (!s) return SFF_ERR_ARG;
    s->kind = SFF_KIND_UNKNOWN;
    s->sprite_count = 0u;
    s->palette_count = 0u;
    s->tolerant = 1;
    s->codec = 0;
    if (opt) {
        s->tolerant = opt->tolerant ? 1 : 0;
        s->codec = opt->codec;
    }

    if (s->size < 512u) return SFF_ERR_CORRUPT;
    if (!sff__read_at(s, 0u, hdr, 512u)) return SFF_ERR_IO;
    if (!sff__sig_ok(hdr)) return SFF_ERR_BAD_SIGNATURE;

    if (sff__parse_v2(s, hdr)) {
        if (!sff__map_build(s)) return SFF_ERR_CAPACITY;
        return SFF_OK;
    }

    if (sff__parse_v1(s, hdr)) {
        if (!sff__map_build(s)) return SFF_ERR_CAPACITY;
        return SFF_OK;
    }

    return SFF_ERR_UNSUPPORTED_VERSION;
}

int sff_open_memory(SffFile *s, const void *data, sff_u32 size, const SffOpenOptions *opt)
{
    if (!s || !data) return SFF_ERR_ARG;
    sff_init(s);
    s->source_kind = SFF_SOURCE_MEMORY;
    s->mem = (const sff_u8*)data;
    s->size = size;
    return sff__open_common(s, opt);
}

int sff_open_io(SffFile *s, const SffIo *io, const SffOpenOptions *opt)
{
    if (!s || !io || !io->read_at) return SFF_ERR_ARG;
    sff_init(s);
    s->source_kind = SFF_SOURCE_IO;
    s->io = *io;
    s->size = io->size;
    return sff__open_common(s, opt);
}

SffKind sff_kind(const SffFile *s)
{
    return s ? s->kind : SFF_KIND_UNKNOWN;
}

sff_u32 sff_sprite_count(const SffFile *s)
{
    return s ? s->sprite_count : 0u;
}

int sff_find_sprite(const SffFile *s, sff_u16 group, sff_u16 item, sff_u32 *out_index)
{
    sff_u32 key;
    sff_u32 val;

    if (!s) return SFF_ERR_ARG;
    key = ((sff_u32)group << 16) | (sff_u32)item;
    if (!sff_map_get(&s->sprite_map, key, &val)) return SFF_ERR_NO_SPRITE;
    if (out_index) *out_index = val;
    return SFF_OK;
}

static int sff__peek_v1_dims(const SffFile *s, const SffSpriteEntry *sp, sff_u16 *out_w, sff_u16 *out_h, sff_u8 *out_png_color_type)
{
    const sff_u8 *blob;
    sff_u8 head[128];
    sff_u32 need;
    sff_u16 w;
    sff_u16 h;
    sff_u8 ct;

    if (!s || !sp || sp->data_len == 0u) return 0;

    blob = sff__memory_ptr(s, sp->data_ofs, sp->data_len);
    if (blob) {
        if (sff_pcx_peek_dims(blob, sp->data_len, &w, &h)) {
            if (out_w) *out_w = w;
            if (out_h) *out_h = h;
            if (out_png_color_type) *out_png_color_type = 0xFFu;
            return 1;
        }
        if (sff__png_peek(blob, sp->data_len, &w, &h, &ct)) {
            if (out_w) *out_w = w;
            if (out_h) *out_h = h;
            if (out_png_color_type) *out_png_color_type = ct;
            return 1;
        }
        return 0;
    }

    need = sp->data_len;
    if (need > (sff_u32)sizeof(head)) need = (sff_u32)sizeof(head);
    if (!sff__read_at(s, sp->data_ofs, head, need)) return 0;

    if (sff_pcx_peek_dims(head, need, &w, &h)) {
        if (out_w) *out_w = w;
        if (out_h) *out_h = h;
        if (out_png_color_type) *out_png_color_type = 0xFFu;
        return 1;
    }
    if (sff__png_peek(head, need, &w, &h, &ct)) {
        if (out_w) *out_w = w;
        if (out_h) *out_h = h;
        if (out_png_color_type) *out_png_color_type = ct;
        return 1;
    }

    return 0;
}

int sff_get_sprite_info(const SffFile *s, sff_u32 index, SffSpriteInfo *out_info)
{
    SffSpriteInfo info;
    const SffSpriteEntry *sp;

    if (!s || !out_info) return SFF_ERR_ARG;
    if (index >= s->sprite_count) return SFF_ERR_NO_SPRITE;

    memset(&info, 0, sizeof(info));
    info.palette_index = 0xFFFFu;
    info.link_index = SFF_NO_INDEX;

    sp = &s->sprites[index];
    info.group = sp->group;
    info.item = sp->item;
    info.axis_x = sp->axis_x;
    info.axis_y = sp->axis_y;
    info.w = sp->w;
    info.h = sp->h;
    info.depth = sp->depth;
    info.compression = sp->compression;
    info.palette_index = sp->palette_index;
    info.load_mode = sp->load_mode;
    info.data_ofs = sp->data_ofs;
    info.data_len = sp->data_len;
    if (sp->owner_index != SFF_NO_INDEX && sp->owner_index != index) {
        info.link_index = sp->owner_index;
        if (sp->raw_length == 0u && sp->owner_index < s->sprite_count) {
            const SffSpriteEntry *owner;
            owner = &s->sprites[sp->owner_index];
            if (owner->data_len > 0u) {
                info.compression = owner->compression;
                info.depth = owner->depth;
                info.load_mode = owner->load_mode;
            }
        }
    }

    if (s->kind == SFF_KIND_V1) {
        sff_u16 w;
        sff_u16 h;
        sff_u8 ct;
        if (sff__peek_v1_dims(s, sp, &w, &h, &ct)) {
            info.w = w;
            info.h = h;
            if (ct == 2u) info.compression = SFF_COMP_PNG24;
            else if (ct == 6u) info.compression = SFF_COMP_PNG32;
            else if (ct == 3u) info.compression = SFF_COMP_PNG8;
            else if (ct != 0xFFu) info.compression = SFF_COMP_PNG24;
        }
    }

    *out_info = info;
    return SFF_OK;
}

static int sff__resolve_palette_index(const SffFile *s, sff_u16 pal_index, sff_u16 *out_final)
{
    sff_u16 cur;
    sff_u32 seen;

    if (!s || !out_final || pal_index >= s->palette_count) return 0;
    cur = pal_index;
    seen = 0u;

    while (seen < s->palette_count) {
        const SffPaletteEntry *p;
        p = &s->palettes[cur];
        if (p->data_len != 0u) {
            *out_final = cur;
            return 1;
        }
        if (p->link_index == 0xFFFFu || p->link_index >= s->palette_count) return 0;
        cur = p->link_index;
        ++seen;
    }

    return 0;
}

int sff_read_palette_rgb(const SffFile *s, sff_u16 palette_index, sff_u8 out_rgb[768])
{
    sff_u16 final_index;
    const SffPaletteEntry *p;
    sff_u32 cols;
    sff_u32 i;

    if (!s || !out_rgb) return SFF_ERR_ARG;
    memset(out_rgb, 0, 768u);

    if (s->kind != SFF_KIND_V2) return SFF_ERR_UNSUPPORTED_VERSION;
    if (!sff__resolve_palette_index(s, palette_index, &final_index)) return SFF_ERR_CORRUPT;

    p = &s->palettes[final_index];
    cols = p->colors ? (sff_u32)p->colors : (p->data_len / 4u);
    if (cols > 256u) cols = 256u;

    for (i = 0u; i < cols; ++i) {
        sff_u8 quad[4];
        if (!sff__read_at(s, p->data_ofs + 4u * i, quad, 4u)) return SFF_ERR_IO;
        out_rgb[3u * i + 0u] = quad[0];
        out_rgb[3u * i + 1u] = quad[1];
        out_rgb[3u * i + 2u] = quad[2];
    }

    return SFF_OK;
}

static int sff__v1_read_palette_tail(const SffFile *s,
                                     const SffSpriteEntry *sp,
                                     sff_u8 out_pal_rgb[768])
{
    sff_u8 tail[769];
    const sff_u8 *blob;

    if (!s || !sp || !out_pal_rgb || sp->data_len < 769u) return 0;

    blob = sff__memory_ptr(s, sp->data_ofs, sp->data_len);
    if (blob) {
        if (blob[sp->data_len - 769u] != 12u) return 0;
        memcpy(out_pal_rgb, blob + sp->data_len - 768u, 768u);
        return 1;
    }

    if (!sff__read_at(s, sp->data_ofs + sp->data_len - 769u, tail, 769u)) return 0;
    if (tail[0] != 12u) return 0;
    memcpy(out_pal_rgb, tail + 1u, 768u);
    return 1;
}

static int sff__v1_get_cached_palette_rgb(const SffFile *s, sff_u32 sprite_index, sff_u8 out_pal_rgb[768])
{
    sff_u32 owner;
    sff_s32 i;

    if (!s || !out_pal_rgb || sprite_index >= s->sprite_count) return 0;

    owner = s->palette_owner[sprite_index];
    if (owner != SFF_NO_INDEX && owner < s->sprite_count) {
        if (sff__v1_read_palette_tail(s, &s->sprites[owner], out_pal_rgb)) return 1;
    }

    /* Tolerant fallback for authoring tools that wrote inconsistent
       same_palette flags: search backward first, then from file start. */
    for (i = (sff_s32)sprite_index; i >= 0; --i) {
        const SffSpriteEntry *sp;
        sp = &s->sprites[(sff_u32)i];
        if (sp->same_palette == 0u &&
            sff__v1_read_palette_tail(s, sp, out_pal_rgb)) return 1;
    }
    for (owner = 0u; owner < s->sprite_count; ++owner) {
        const SffSpriteEntry *sp;
        sp = &s->sprites[owner];
        if (sp->same_palette == 0u &&
            sff__v1_read_palette_tail(s, sp, out_pal_rgb)) return 1;
    }

    return 0;
}

static int sff__rgba_from_indexed(const sff_u8 *idx, sff_u32 n, const sff_u8 pal_rgb[768], sff_u8 *rgba_out)
{
    if (!idx || !pal_rgb || !rgba_out) return 0;

    while (n > 0u) {
        sff_u32 i;
        sff_u8 p;
        --n;
        i = n;
        p = idx[i];
        rgba_out[4u * i + 0u] = pal_rgb[3u * (sff_u32)p + 0u];
        rgba_out[4u * i + 1u] = pal_rgb[3u * (sff_u32)p + 1u];
        rgba_out[4u * i + 2u] = pal_rgb[3u * (sff_u32)p + 2u];
#if SFF_INDEX0_TRANSPARENT
        rgba_out[4u * i + 3u] = (p == 0u) ? 0u : 255u;
#else
        rgba_out[4u * i + 3u] = 255u;
#endif
    }

    return 1;
}

static int sff__decode_png_indexed(const SffFile *s,
                                   const sff_u8 *blob, sff_u32 blob_len,
                                   sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                   sff_u16 *out_w, sff_u16 *out_h,
                                   sff_u8 out_pal_rgb[768], int *out_has_pal,
                                   sff_u8 *work_buf, sff_u32 work_buf_size)
{
    if (!s || !blob || !out_pixels) return SFF_ERR_ARG;
    if (s->codec && s->codec->decode_png_indexed) {
        if (!s->codec->decode_png_indexed(s->codec->user, blob, blob_len,
                                          out_pixels, out_pixels_size,
                                          out_w, out_h,
                                          out_pal_rgb, out_has_pal)) {
            return SFF_ERR_CORRUPT;
        }
        return SFF_OK;
    }
    if (!work_buf || work_buf_size == 0u) return SFF_ERR_BUFFER_TOO_SMALL;
    if (!sff_png_builtin_decode_indexed(blob, blob_len,
                                        s->tolerant,
                                        work_buf, work_buf_size,
                                        out_pixels, out_pixels_size,
                                        out_w, out_h,
                                        out_pal_rgb, out_has_pal)) {
        return SFF_ERR_CORRUPT;
    }
    return SFF_OK;
}

static int sff__decode_png_rgba(const SffFile *s,
                                const sff_u8 *blob, sff_u32 blob_len,
                                sff_u8 *out_rgba, sff_u32 out_rgba_size,
                                sff_u16 *out_w, sff_u16 *out_h,
                                sff_u8 *work_buf, sff_u32 work_buf_size)
{
    if (!s || !blob || !out_rgba) return SFF_ERR_ARG;
    if (s->codec && s->codec->decode_png_rgba) {
        if (!s->codec->decode_png_rgba(s->codec->user, blob, blob_len,
                                       out_rgba, out_rgba_size, out_w, out_h)) {
            return SFF_ERR_CORRUPT;
        }
        return SFF_OK;
    }
    if (!work_buf || work_buf_size == 0u) return SFF_ERR_BUFFER_TOO_SMALL;
    if (!sff_png_builtin_decode_rgba(blob, blob_len,
                                     s->tolerant,
                                     work_buf, work_buf_size,
                                     out_rgba, out_rgba_size,
                                     out_w, out_h)) {
        return SFF_ERR_CORRUPT;
    }
    return SFF_OK;
}

static int sff__decode_v2_indexed(const SffFile *s, sff_u32 index,
                                  sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                  sff_u16 *out_w, sff_u16 *out_h,
                                  sff_u8 out_pal_rgb[768], int *out_has_palette,
                                  sff_u8 *raw_work, sff_u32 raw_work_size)
{
    const SffSpriteEntry *sp;
    const SffSpriteEntry *src_sp;
    const sff_u8 *blob;
    sff_u16 png_w;
    sff_u16 png_h;
    sff_u8 png_ct;
    sff_u32 need;
    sff_u8 *png_work;
    sff_u32 png_work_size;
    int rc;

    if (!s || index >= s->sprite_count || !out_pixels) return SFF_ERR_ARG;

    sp = &s->sprites[index];
    src_sp = sp;
    if (sp->raw_length == 0u && sp->owner_index != SFF_NO_INDEX && sp->owner_index < s->sprite_count) {
        const SffSpriteEntry *owner;
        owner = &s->sprites[sp->owner_index];
        if (owner->data_len > 0u) src_sp = owner;
    }

    if (sp->data_len == 0u) return SFF_ERR_CORRUPT;

    if (!sff__mul_u32((sff_u32)sp->w, (sff_u32)sp->h, &need)) return SFF_ERR_CORRUPT;
    if (sp->w != 0u && sp->h != 0u && out_pixels_size < need) return SFF_ERR_BUFFER_TOO_SMALL;

    if (!sff__blob_to_ptr_or_work(s, sp->data_ofs, sp->data_len, raw_work, raw_work_size, &blob)) {
        return SFF_ERR_BUFFER_TOO_SMALL;
    }

    if (out_has_palette) *out_has_palette = 0;

    png_work = raw_work;
    png_work_size = raw_work_size;
    if (png_work && blob == png_work) {
        if (png_work_size < sp->data_len) return SFF_ERR_BUFFER_TOO_SMALL;
        png_work += sp->data_len;
        png_work_size -= sp->data_len;
    }

    {
        const sff_u8 *png_blob;
        sff_u32 png_len;
        if (sff__png_locate_payload(blob, sp->data_len, &png_blob, &png_len) &&
            sff__png_peek(png_blob, png_len, &png_w, &png_h, &png_ct)) {
            if (png_ct != 3u) return SFF_ERR_NOT_INDEXED;
            rc = sff__decode_png_indexed(s, png_blob, png_len,
                                         out_pixels, out_pixels_size,
                                         out_w, out_h,
                                         out_pal_rgb, out_has_palette,
                                         png_work, png_work_size);
            if (rc != SFF_OK) return rc;

            if (out_pal_rgb && sff_read_palette_rgb(s, sp->palette_index, out_pal_rgb) == SFF_OK) {
                if (out_has_palette) *out_has_palette = 1;
            }
            return SFF_OK;
        }
    }

    if (sff_pcx_is_8bpp_rle(blob, sp->data_len)) {
        int has_pal;
        has_pal = 0;
        if (!sff_pcx_decode_8bpp_into(blob, sp->data_len,
                                      out_pixels, out_pixels_size,
                                      out_w, out_h,
                                      out_pal_rgb, &has_pal)) {
            return SFF_ERR_CORRUPT;
        }
        if (out_pal_rgb && !has_pal && sff_read_palette_rgb(s, sp->palette_index, out_pal_rgb) == SFF_OK) {
            has_pal = 1;
        }
        if (out_has_palette) *out_has_palette = has_pal;
        return SFF_OK;
    }

    if (src_sp->compression == SFF_COMP_PNG24 || src_sp->compression == SFF_COMP_PNG32) return SFF_ERR_NOT_INDEXED;
    if (sp->w == 0u || sp->h == 0u) return SFF_ERR_CORRUPT;

    if (src_sp->compression == SFF_COMP_RLE5 || src_sp->compression == SFF_COMP_LZ5) {
        if (src_sp->depth != 5u && src_sp->depth != 8u) return SFF_ERR_UNSUPPORTED_COMPRESSION;
    } else {
        if (src_sp->depth != 8u) return SFF_ERR_UNSUPPORTED_COMPRESSION;
    }

    memset(out_pixels, 0, (size_t)need);

    if (src_sp->compression == SFF_COMP_NONE) {
        sff_u32 copy;
        copy = sp->data_len;
        if (copy > need) copy = need;
        memcpy(out_pixels, blob, (size_t)copy);
    } else if (src_sp->compression == SFF_COMP_RLE8_SFF) {
        if (!sff_decomp_rle8_sff(blob, sp->data_len, sp->w, sp->h, out_pixels, need)) return SFF_ERR_CORRUPT;
    } else if (src_sp->compression == SFF_COMP_RLE5) {
        if (!sff_decomp_rle5(blob, sp->data_len, sp->w, sp->h, out_pixels, need)) return SFF_ERR_CORRUPT;
    } else if (src_sp->compression == SFF_COMP_LZ5) {
        if (!sff_decomp_lz5(blob, sp->data_len, sp->w, sp->h, out_pixels, need)) return SFF_ERR_CORRUPT;
    } else if (src_sp->compression == SFF_COMP_PNG8) {
        return SFF_ERR_CORRUPT;
    } else {
        return SFF_ERR_UNSUPPORTED_COMPRESSION;
    }

    if (out_w) *out_w = sp->w;
    if (out_h) *out_h = sp->h;

    if (out_pal_rgb && sff_read_palette_rgb(s, sp->palette_index, out_pal_rgb) == SFF_OK) {
        if (out_has_palette) *out_has_palette = 1;
    }

    return SFF_OK;
}

static int sff__decode_v1_indexed(const SffFile *s, sff_u32 index,
                                  sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                  sff_u16 *out_w, sff_u16 *out_h,
                                  sff_u8 out_pal_rgb[768], int *out_has_palette,
                                  sff_u8 *raw_work, sff_u32 raw_work_size)
{
    const SffSpriteEntry *sp;
    const sff_u8 *blob;
    sff_u16 w;
    sff_u16 h;
    sff_u8 png_ct;
    sff_u8 *png_work;
    sff_u32 png_work_size;
    int has_pal;
    int rc;

    sp = &s->sprites[index];
    if (sp->data_len == 0u) return SFF_ERR_CORRUPT;

    if (!sff__blob_to_ptr_or_work(s, sp->data_ofs, sp->data_len, raw_work, raw_work_size, &blob)) {
        return SFF_ERR_BUFFER_TOO_SMALL;
    }

    if (out_has_palette) *out_has_palette = 0;

    png_work = raw_work;
    png_work_size = raw_work_size;
    if (png_work && blob == png_work) {
        if (png_work_size < sp->data_len) return SFF_ERR_BUFFER_TOO_SMALL;
        png_work += sp->data_len;
        png_work_size -= sp->data_len;
    }

    if (sff__png_peek(blob, sp->data_len, &w, &h, &png_ct)) {
        if (png_ct != 3u) return SFF_ERR_NOT_INDEXED;
        rc = sff__decode_png_indexed(s, blob, sp->data_len,
                                     out_pixels, out_pixels_size,
                                     out_w, out_h,
                                     out_pal_rgb, out_has_palette,
                                     png_work, png_work_size);
        if (rc != SFF_OK) return rc;
        if (out_pal_rgb && (sp->same_palette != 0u || !(out_has_palette && *out_has_palette))) {
            if (sff__v1_get_cached_palette_rgb(s, index, out_pal_rgb)) {
                if (out_has_palette) *out_has_palette = 1;
            }
        }
        return SFF_OK;
    }

    if (!sff_pcx_peek_dims(blob, sp->data_len, &w, &h)) return SFF_ERR_UNSUPPORTED_COMPRESSION;
    if (out_pixels_size < (sff_u32)w * (sff_u32)h) return SFF_ERR_BUFFER_TOO_SMALL;

    has_pal = 0;
    if (!sff_pcx_decode_8bpp_into(blob, sp->data_len,
                                  out_pixels, out_pixels_size,
                                  out_w, out_h,
                                  out_pal_rgb, &has_pal)) {
        return SFF_ERR_CORRUPT;
    }

    if (out_pal_rgb && (sp->same_palette != 0u || !has_pal)) {
        if (sff__v1_get_cached_palette_rgb(s, index, out_pal_rgb)) {
            has_pal = 1;
        }
    }
    if (out_has_palette) *out_has_palette = has_pal;

    return SFF_OK;
}

int sff_decode_sprite_indexed_into(const SffFile *s, sff_u32 index,
                                   sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                   sff_u16 *out_w, sff_u16 *out_h,
                                   sff_u8 out_pal_rgb[768], int *out_has_palette,
                                   sff_u8 *work_buf, sff_u32 work_buf_size)
{
    if (!s || !out_pixels) return SFF_ERR_ARG;
    if (index >= s->sprite_count) return SFF_ERR_NO_SPRITE;
    if (out_w) *out_w = 0u;
    if (out_h) *out_h = 0u;
    if (out_has_palette) *out_has_palette = 0;

    if (s->kind == SFF_KIND_V1) {
        return sff__decode_v1_indexed(s, index,
                                      out_pixels, out_pixels_size,
                                      out_w, out_h,
                                      out_pal_rgb, out_has_palette,
                                      work_buf, work_buf_size);
    }
    if (s->kind == SFF_KIND_V2) {
        return sff__decode_v2_indexed(s, index,
                                      out_pixels, out_pixels_size,
                                      out_w, out_h,
                                      out_pal_rgb, out_has_palette,
                                      work_buf, work_buf_size);
    }
    return SFF_ERR_UNSUPPORTED_VERSION;
}

int sff_decode_sprite_rgba_into(const SffFile *s, sff_u32 index,
                                sff_u8 *out_rgba, sff_u32 out_rgba_size,
                                sff_u16 *out_w, sff_u16 *out_h,
                                sff_u8 *work_buf, sff_u32 work_buf_size)
{
    SffSpriteInfo info;
    int rc;

    if (!s || !out_rgba) return SFF_ERR_ARG;
    if (out_w) *out_w = 0u;
    if (out_h) *out_h = 0u;

    rc = sff_get_sprite_info(s, index, &info);
    if (rc != SFF_OK) return rc;

    /* Direct PNG RGBA/truecolor path */
    if (info.data_len > 0u) {
        sff_u8 small[40];
        const sff_u8 *blob;
        sff_u16 png_w;
        sff_u16 png_h;
        sff_u8 png_ct;
        sff_u8 *png_work;
        sff_u32 png_work_size;
        sff_u32 rgba_need;

        blob = sff__memory_ptr(s, info.data_ofs, info.data_len);
        png_work = work_buf;
        png_work_size = work_buf_size;
        if (!blob) {
            sff_u32 need;
            need = info.data_len;
            if (need > 33u) need = 33u;
            if (!sff__read_at(s, info.data_ofs, small, need)) return SFF_ERR_IO;
            blob = small;
        }

        {
            const sff_u8 *png_blob;
            sff_u32 png_len;
            sff_u32 peek_len;
            peek_len = (blob == small) ? (sff_u32)sizeof(small) : info.data_len;
            if (sff__png_locate_payload(blob, peek_len, &png_blob, &png_len) &&
                sff__png_peek(png_blob, png_len, &png_w, &png_h, &png_ct)) {
                if (png_ct == 2u || png_ct == 6u || png_ct == 0u) {
                    {
                        sff_u32 pix_need;
                        if (!sff__mul_u32((sff_u32)png_w, (sff_u32)png_h, &pix_need)) return SFF_ERR_CORRUPT;
                        if (!sff__mul_u32(pix_need, 4u, &rgba_need)) return SFF_ERR_CORRUPT;
                    }
                    if (out_rgba_size < rgba_need) return SFF_ERR_BUFFER_TOO_SMALL;

                    if (blob == small) {
                    if (!work_buf || work_buf_size < info.data_len) return SFF_ERR_BUFFER_TOO_SMALL;
                    if (!sff__read_at(s, info.data_ofs, work_buf, info.data_len)) return SFF_ERR_IO;
                    blob = work_buf;
                    if (png_work_size < info.data_len) return SFF_ERR_BUFFER_TOO_SMALL;
                    png_work = work_buf + info.data_len;
                    png_work_size = work_buf_size - info.data_len;
                    if (!sff__png_locate_payload(blob, info.data_len, &png_blob, &png_len)) {
                        png_blob = 0;
                    }
                }
                    if (png_blob) {
                        rc = sff__decode_png_rgba(s, png_blob, png_len, out_rgba, out_rgba_size, out_w, out_h, png_work, png_work_size);
                        if (rc == SFF_OK) return rc;
                    }
                }
            }
        }
    }

    /* Indexed -> RGBA path */
    {
        sff_u32 pix_need;
        sff_u32 rgba_need;
        sff_u8 pal[768];
        int has_pal;
        sff_u8 *idx_buf;
        sff_u8 *raw_work;
        sff_u32 raw_work_size;
        sff_u32 raw_need;

        if (info.w == 0u || info.h == 0u) return SFF_ERR_CORRUPT;
        if (!sff__mul_u32((sff_u32)info.w, (sff_u32)info.h, &pix_need)) return SFF_ERR_CORRUPT;
        if (!sff__mul_u32(pix_need, 4u, &rgba_need)) return SFF_ERR_CORRUPT;
        if (out_rgba_size < rgba_need) return SFF_ERR_BUFFER_TOO_SMALL;

        raw_need = 0u;
        if (s->source_kind != SFF_SOURCE_MEMORY) {
            raw_need = info.data_len;
        }

        if (work_buf && work_buf_size >= pix_need + raw_need) {
            idx_buf = work_buf;
            raw_work = work_buf + pix_need;
            raw_work_size = work_buf_size - pix_need;
        } else {
            if (out_rgba_size < pix_need + raw_need) return SFF_ERR_BUFFER_TOO_SMALL;
            idx_buf = out_rgba;
            raw_work = out_rgba + pix_need;
            raw_work_size = out_rgba_size - pix_need;
        }

        has_pal = 0;
        rc = sff_decode_sprite_indexed_into(s, index,
                                            idx_buf, pix_need,
                                            out_w, out_h,
                                            pal, &has_pal,
                                            raw_work, raw_work_size);
        if (rc != SFF_OK) return rc;

        if (!has_pal) {
            sff_u32 i;
            for (i = 0u; i < 256u; ++i) {
                pal[3u * i + 0u] = (sff_u8)i;
                pal[3u * i + 1u] = (sff_u8)i;
                pal[3u * i + 2u] = (sff_u8)i;
            }
        }

        sff__rgba_from_indexed(idx_buf, pix_need, pal, out_rgba);
        return SFF_OK;
    }
}

#include "eotdec.h"
#include <string.h>

static int rd_u8(FILE *fp, unsigned int *v) {
    int c = fgetc(fp);
    if (c == EOF) return EOTDEC_ERR_SHORT;
    *v = (unsigned int)(c & 255);
    return EOTDEC_OK;
}

static int rd_le16(FILE *fp, unsigned int *v) {
    unsigned int b0, b1;
    if (rd_u8(fp, &b0) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    if (rd_u8(fp, &b1) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    *v = (unsigned int)(b0 | (b1 << 8));
    return EOTDEC_OK;
}

static int rd_le32(FILE *fp, unsigned long *v) {
    unsigned int b0, b1, b2, b3;
    if (rd_u8(fp, &b0) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    if (rd_u8(fp, &b1) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    if (rd_u8(fp, &b2) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    if (rd_u8(fp, &b3) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    *v = ((unsigned long)b0) | ((unsigned long)b1 << 8) | ((unsigned long)b2 << 16) | ((unsigned long)b3 << 24);
    return EOTDEC_OK;
}

static int rd_be16_buf(const unsigned char *p) {
    return (int)(((unsigned int)p[0] << 8) | (unsigned int)p[1]);
}

static unsigned long rd_be32_buf(const unsigned char *p) {
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

static unsigned long file_tell_u32(FILE *fp) {
    long p = ftell(fp);
    if (p < 0) return 0UL;
    return (unsigned long)p;
}

static unsigned long file_size_u32(FILE *fp) {
    long cur, end;
    cur = ftell(fp);
    if (cur < 0) return 0UL;
    if (fseek(fp, 0L, SEEK_END) != 0) return 0UL;
    end = ftell(fp);
    if (fseek(fp, cur, SEEK_SET) != 0) return 0UL;
    if (end < 0) return 0UL;
    return (unsigned long)end;
}

static int seek_abs(FILE *fp, unsigned long off) {
    if (off > 0x7fffffffUL) return EOTDEC_ERR_RANGE;
    if (fseek(fp, (long)off, SEEK_SET) != 0) return EOTDEC_ERR_IO;
    return EOTDEC_OK;
}

static int skip_bytes(FILE *fp, unsigned long n) {
    unsigned long cur;
    cur = file_tell_u32(fp);
    return seek_abs(fp, cur + n);
}

static unsigned long calc_bytes_checksum(FILE *fp, unsigned long off, unsigned long len) {
    unsigned long i, sum;
    int c;
    if (seek_abs(fp, off) != EOTDEC_OK) return 0UL;
    sum = 0UL;
    for (i = 0; i < len; i++) {
        c = fgetc(fp);
        if (c == EOF) break;
        sum += (unsigned long)((unsigned int)c & 255U);
    }
    return sum ^ EOTDEC_CS_XORKEY;
}

static int read_utf16le_preview(FILE *fp, unsigned long nbytes, char *out) {
    unsigned long i;
    unsigned int lo, hi;
    unsigned int pos;
    pos = 0U;
    for (i = 0; i + 1UL < nbytes; i += 2UL) {
        if (rd_u8(fp, &lo) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
        if (rd_u8(fp, &hi) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
        if (pos < EOTDEC_MAX_TEXT_CHARS) {
            if (lo == 0U && hi == 0U) {
                out[pos++] = '|';
            } else if (hi == 0U && lo >= 32U && lo < 127U) {
                out[pos++] = (char)lo;
            } else if (hi == 0U && (lo == 9U || lo == 10U || lo == 13U)) {
                out[pos++] = ' ';
            } else {
                out[pos++] = '?';
            }
        }
    }
    if ((nbytes & 1UL) != 0UL) {
        unsigned int dummy;
        if (rd_u8(fp, &dummy) != EOTDEC_OK) return EOTDEC_ERR_SHORT;
    }
    out[pos] = '\0';
    return EOTDEC_OK;
}

static int read_name_field(FILE *fp, char *out) {
    unsigned int sz;
    int r;
    r = rd_le16(fp, &sz);
    if (r != EOTDEC_OK) return r;
    return read_utf16le_preview(fp, (unsigned long)sz, out);
}

static int read_padding(FILE *fp) {
    unsigned int pad;
    return rd_le16(fp, &pad);
}

const char *eotdec_errstr(int code) {
    switch (code) {
    case EOTDEC_OK: return "ok";
    case EOTDEC_ERR_IO: return "io error";
    case EOTDEC_ERR_SHORT: return "truncated file";
    case EOTDEC_ERR_BAD_MAGIC: return "bad EOT magic";
    case EOTDEC_ERR_BAD_VERSION: return "unsupported EOT version";
    case EOTDEC_ERR_BAD_SIZE: return "bad or inconsistent size";
    case EOTDEC_ERR_ROOT_CHECKSUM: return "root string checksum mismatch";
    case EOTDEC_ERR_COMPRESSED: return "MicroType Express compressed FontData: use mtx-unpack for LZCOMP/CTF blocks";
    case EOTDEC_ERR_UNSUPPORTED: return "unsupported feature";
    case EOTDEC_ERR_SFNT: return "invalid OpenType/TrueType sfnt payload";
    case EOTDEC_ERR_RANGE: return "offset/range too large";
    case EOTDEC_ERR_MTX_CORRUPT: return "corrupt or unsupported MTX/LZCOMP stream";
    case EOTDEC_ERR_MTX_LIMIT: return "MTX/LZCOMP stream exceeds static no-heap limits";
    default: return "unknown error";
    }
}

int eotdec_probe_file(FILE *fp, struct eotdec_info *o) {
    int r;
    unsigned int tmp16, i;
    unsigned long pos;

    if (fp == 0 || o == 0) return EOTDEC_ERR_IO;
    memset(o, 0, sizeof(*o));
    o->file_size = file_size_u32(fp);
    if (seek_abs(fp, 0UL) != EOTDEC_OK) return EOTDEC_ERR_IO;

    if ((r = rd_le32(fp, &o->eot_size)) != EOTDEC_OK) return r;
    if ((r = rd_le32(fp, &o->font_data_size)) != EOTDEC_OK) return r;
    if ((r = rd_le32(fp, &o->version)) != EOTDEC_OK) return r;
    if ((r = rd_le32(fp, &o->flags)) != EOTDEC_OK) return r;
    if (o->version != EOTDEC_VERSION_10000 && o->version != EOTDEC_VERSION_20001 && o->version != EOTDEC_VERSION_20002) return EOTDEC_ERR_BAD_VERSION;

    for (i = 0; i < 10U; i++) {
        if ((r = rd_u8(fp, &tmp16)) != EOTDEC_OK) return r;
    }
    if ((r = rd_u8(fp, &o->charset)) != EOTDEC_OK) return r;
    if ((r = rd_u8(fp, &o->italic)) != EOTDEC_OK) return r;
    if ((r = rd_le32(fp, &o->weight)) != EOTDEC_OK) return r;
    if ((r = rd_le16(fp, &o->fs_type)) != EOTDEC_OK) return r;
    if ((r = rd_le16(fp, &o->magic)) != EOTDEC_OK) return r;
    if (o->magic != EOTDEC_MAGIC_PL) return EOTDEC_ERR_BAD_MAGIC;

    for (i = 0; i < 4U; i++) {
        if ((r = rd_le32(fp, &o->unicode_range[i])) != EOTDEC_OK) return r;
    }
    for (i = 0; i < 2U; i++) {
        if ((r = rd_le32(fp, &o->codepage_range[i])) != EOTDEC_OK) return r;
    }
    if ((r = rd_le32(fp, &o->check_sum_adjustment)) != EOTDEC_OK) return r;
    /* reserved1..4 */
    for (i = 0; i < 4U; i++) {
        unsigned long dummy;
        if ((r = rd_le32(fp, &dummy)) != EOTDEC_OK) return r;
    }
    if ((r = read_padding(fp)) != EOTDEC_OK) return r;

    if ((r = read_name_field(fp, o->family)) != EOTDEC_OK) return r;
    if ((r = read_padding(fp)) != EOTDEC_OK) return r;
    if ((r = read_name_field(fp, o->style)) != EOTDEC_OK) return r;
    if ((r = read_padding(fp)) != EOTDEC_OK) return r;
    if ((r = read_name_field(fp, o->version_name)) != EOTDEC_OK) return r;
    if ((r = read_padding(fp)) != EOTDEC_OK) return r;
    if ((r = read_name_field(fp, o->full_name)) != EOTDEC_OK) return r;

    if (o->version == EOTDEC_VERSION_10000) {
        o->font_offset = file_tell_u32(fp);
    } else {
        if ((r = read_padding(fp)) != EOTDEC_OK) return r;
        if ((r = rd_le16(fp, &tmp16)) != EOTDEC_OK) return r;
        o->root_size = (unsigned long)tmp16;
        o->root_offset = file_tell_u32(fp);
        if ((r = read_utf16le_preview(fp, o->root_size, o->root_preview)) != EOTDEC_OK) return r;
        if (o->version == EOTDEC_VERSION_20001) {
            o->font_offset = file_tell_u32(fp);
        } else {
            if ((r = rd_le32(fp, &o->root_checksum_stored)) != EOTDEC_OK) return r;
            if ((r = rd_le32(fp, &o->eudc_code_page)) != EOTDEC_OK) return r;
            if ((r = read_padding(fp)) != EOTDEC_OK) return r;
            if ((r = rd_le16(fp, &tmp16)) != EOTDEC_OK) return r;
            if ((r = skip_bytes(fp, (unsigned long)tmp16)) != EOTDEC_OK) return r;
            if ((r = rd_le32(fp, &o->eudc_flags)) != EOTDEC_OK) return r;
            if ((r = rd_le32(fp, &o->eudc_size)) != EOTDEC_OK) return r;
            o->eudc_offset = file_tell_u32(fp);
            if ((r = skip_bytes(fp, o->eudc_size)) != EOTDEC_OK) return r;
            o->font_offset = file_tell_u32(fp);
            o->root_checksum_calc = calc_bytes_checksum(fp, o->root_offset, o->root_size);
        }
    }

    pos = o->font_offset + o->font_data_size;
    if (o->font_data_size == 0UL) return EOTDEC_ERR_BAD_SIZE;
    if (o->eot_size != 0UL && pos > o->eot_size) return EOTDEC_ERR_BAD_SIZE;
    if (o->file_size != 0UL && pos > o->file_size) return EOTDEC_ERR_BAD_SIZE;
    return EOTDEC_OK;
}

static int read_processed(FILE *fp, unsigned long off, unsigned char *buf, unsigned long len, unsigned long flags) {
    unsigned long i;
    if (seek_abs(fp, off) != EOTDEC_OK) return EOTDEC_ERR_IO;
    if (fread(buf, 1, (size_t)len, fp) != (size_t)len) return EOTDEC_ERR_SHORT;
    if ((flags & EOTDEC_FLAG_XORENCRYPTDATA) != 0UL) {
        for (i = 0; i < len; i++) buf[i] = (unsigned char)(buf[i] ^ EOTDEC_XORKEY);
    }
    return EOTDEC_OK;
}

int eotdec_extract_file(FILE *in, FILE *out, const struct eotdec_info *info, int strict) {
    static unsigned char chunk[EOTDEC_COPY_CHUNK];
    unsigned long left, n, i, off;
    size_t got;

    if (in == 0 || out == 0 || info == 0) return EOTDEC_ERR_IO;
    if (strict && info->version == EOTDEC_VERSION_20002 && info->root_size != 0UL && info->root_checksum_calc != info->root_checksum_stored) return EOTDEC_ERR_ROOT_CHECKSUM;
    if ((info->flags & EOTDEC_FLAG_TTCOMPRESSED) != 0UL) return EOTDEC_ERR_COMPRESSED;

    off = info->font_offset;
    left = info->font_data_size;
    if (seek_abs(in, off) != EOTDEC_OK) return EOTDEC_ERR_IO;
    while (left != 0UL) {
        n = left;
        if (n > (unsigned long)EOTDEC_COPY_CHUNK) n = (unsigned long)EOTDEC_COPY_CHUNK;
        got = fread(chunk, 1, (size_t)n, in);
        if (got != (size_t)n) return EOTDEC_ERR_SHORT;
        if ((info->flags & EOTDEC_FLAG_XORENCRYPTDATA) != 0UL) {
            for (i = 0; i < n; i++) chunk[i] = (unsigned char)(chunk[i] ^ EOTDEC_XORKEY);
        }
        if (fwrite(chunk, 1, (size_t)n, out) != (size_t)n) return EOTDEC_ERR_IO;
        left -= n;
    }
    return EOTDEC_OK;
}

int eotdec_read_sfnt_info(FILE *fp, const struct eotdec_info *info, struct eotdec_sfnt_info *sfnt) {
    unsigned char head[12];
    unsigned char rec[16];
    unsigned int n, i, j;
    unsigned long off;
    int r;

    if (fp == 0 || info == 0 || sfnt == 0) return EOTDEC_ERR_IO;
    memset(sfnt, 0, sizeof(*sfnt));
    if ((info->flags & EOTDEC_FLAG_TTCOMPRESSED) != 0UL) return EOTDEC_ERR_COMPRESSED;
    if (info->font_data_size < 12UL) return EOTDEC_ERR_SFNT;
    r = read_processed(fp, info->font_offset, head, 12UL, info->flags);
    if (r != EOTDEC_OK) return r;

    sfnt->sfnt_version = rd_be32_buf(head);
    sfnt->num_tables = (unsigned int)rd_be16_buf(head + 4);
    sfnt->search_range = (unsigned int)rd_be16_buf(head + 6);
    sfnt->entry_selector = (unsigned int)rd_be16_buf(head + 8);
    sfnt->range_shift = (unsigned int)rd_be16_buf(head + 10);

    if (!(sfnt->sfnt_version == 0x00010000UL || sfnt->sfnt_version == 0x4F54544FUL || sfnt->sfnt_version == 0x74727565UL || sfnt->sfnt_version == 0x74797031UL)) return EOTDEC_ERR_SFNT;
    if (sfnt->num_tables > EOTDEC_MAX_TABLES) return EOTDEC_ERR_RANGE;
    if (12UL + ((unsigned long)sfnt->num_tables * 16UL) > info->font_data_size) return EOTDEC_ERR_SFNT;

    off = info->font_offset + 12UL;
    n = sfnt->num_tables;
    for (i = 0; i < n; i++) {
        r = read_processed(fp, off + ((unsigned long)i * 16UL), rec, 16UL, info->flags);
        if (r != EOTDEC_OK) return r;
        for (j = 0; j < 4U; j++) sfnt->tables[i].tag[j] = (char)rec[j];
        sfnt->tables[i].tag[4] = '\0';
        sfnt->tables[i].checksum = rd_be32_buf(rec + 4);
        sfnt->tables[i].offset = rd_be32_buf(rec + 8);
        sfnt->tables[i].length = rd_be32_buf(rec + 12);
    }
    return EOTDEC_OK;
}

void eotdec_print_info(FILE *out, const struct eotdec_info *i) {
    if (out == 0 || i == 0) return;
    fprintf(out, "EOT info\n");
    fprintf(out, "  version:             0x%08lX\n", i->version);
    fprintf(out, "  eot_size:            %lu\n", i->eot_size);
    fprintf(out, "  file_size:           %lu\n", i->file_size);
    fprintf(out, "  font_data_size:      %lu\n", i->font_data_size);
    fprintf(out, "  font_offset:         %lu\n", i->font_offset);
    fprintf(out, "  flags:               0x%08lX\n", i->flags);
    fprintf(out, "    subset:            %s\n", (i->flags & EOTDEC_FLAG_SUBSET) ? "yes" : "no");
    fprintf(out, "    mtx_compressed:    %s\n", (i->flags & EOTDEC_FLAG_TTCOMPRESSED) ? "yes" : "no");
    fprintf(out, "    xor_encrypted:     %s\n", (i->flags & EOTDEC_FLAG_XORENCRYPTDATA) ? "yes" : "no");
    fprintf(out, "    web_object:        %s\n", (i->flags & EOTDEC_FLAG_WEBOBJECT) ? "yes" : "no");
    fprintf(out, "  magic:               0x%04X\n", i->magic);
    fprintf(out, "  charset:             %u\n", i->charset);
    fprintf(out, "  italic:              %u\n", i->italic);
    fprintf(out, "  weight:              %lu\n", i->weight);
    fprintf(out, "  fs_type:             0x%04X\n", i->fs_type);
    fprintf(out, "  checksum_adjustment: 0x%08lX\n", i->check_sum_adjustment);
    fprintf(out, "  family:              %s\n", i->family);
    fprintf(out, "  style:               %s\n", i->style);
    fprintf(out, "  version_name:        %s\n", i->version_name);
    fprintf(out, "  full_name:           %s\n", i->full_name);
    if (i->version != EOTDEC_VERSION_10000) {
        fprintf(out, "  root_offset:         %lu\n", i->root_offset);
        fprintf(out, "  root_size:           %lu\n", i->root_size);
        fprintf(out, "  root_preview:        %s\n", i->root_preview);
    }
    if (i->version == EOTDEC_VERSION_20002) {
        fprintf(out, "  root_checksum:       stored=0x%08lX calc=0x%08lX %s\n", i->root_checksum_stored, i->root_checksum_calc, (i->root_checksum_stored == i->root_checksum_calc) ? "ok" : "BAD");
        fprintf(out, "  eudc_code_page:      %lu\n", i->eudc_code_page);
        fprintf(out, "  eudc_flags:          0x%08lX\n", i->eudc_flags);
        fprintf(out, "  eudc_size:           %lu\n", i->eudc_size);
    }
}

void eotdec_print_sfnt(FILE *out, const struct eotdec_sfnt_info *s) {
    unsigned int i;
    if (out == 0 || s == 0) return;
    fprintf(out, "SFNT payload\n");
    fprintf(out, "  sfnt_version:  0x%08lX", s->sfnt_version);
    if (s->sfnt_version == 0x4F54544FUL) fprintf(out, " ('OTTO')");
    if (s->sfnt_version == 0x74727565UL) fprintf(out, " ('true')");
    if (s->sfnt_version == 0x74797031UL) fprintf(out, " ('typ1')");
    fprintf(out, "\n");
    fprintf(out, "  num_tables:    %u\n", s->num_tables);
    fprintf(out, "  search_range:  %u\n", s->search_range);
    fprintf(out, "  entry_selector:%u\n", s->entry_selector);
    fprintf(out, "  range_shift:   %u\n", s->range_shift);
    fprintf(out, "\n  tables:\n");
    for (i = 0; i < s->num_tables && i < EOTDEC_MAX_TABLES; i++) {
        fprintf(out, "    %-4s checksum=0x%08lX offset=%lu length=%lu\n", s->tables[i].tag, s->tables[i].checksum, s->tables[i].offset, s->tables[i].length);
    }
}

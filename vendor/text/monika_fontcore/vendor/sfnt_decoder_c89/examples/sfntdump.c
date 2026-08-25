#include <stdio.h>
#include "sfnt_decoder.h"

#ifndef SFNTDUMP_MAX_FILE
#define SFNTDUMP_MAX_FILE (32u * 1024u * 1024u)
#endif
#ifndef SFNTDUMP_MAX_POINTS
#define SFNTDUMP_MAX_POINTS 4096u
#endif

static unsigned char g_font[SFNTDUMP_MAX_FILE];
static sfnt_point g_points[SFNTDUMP_MAX_POINTS];

static void print_tag(sfnt_u32 tag)
{
    char s[5];
    sfnt_tag_to_chars(tag, s);
    printf("%s", s);
}

static int visit_component(void *user, const sfnt_component *c)
{
    (void)user;
    printf("    component gid=%u flags=0x%04x args=(%d,%d) matrix=[%d %d %d %d]/65536\n",
           (unsigned)c->glyph_index,
           (unsigned)c->flags,
           (int)c->arg1, (int)c->arg2,
           (int)c->a, (int)c->b, (int)c->c, (int)c->d);
    return SFNT_OK;
}

int main(int argc, char **argv)
{
    FILE *fp;
    long n;
    sfnt_face face;
    sfnt_cmap cmap;
    sfnt_u16 gid;
    sfnt_hmetric hm;
    sfnt_glyph_header gh;
    sfnt_outline outline;
    sfnt_os2_metrics os2;
    sfnt_u32 i;
    int r;

    if (argc < 2) {
        printf("usage: sfntdump font.ttf-or-otf [unicode_hex]\n");
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("could not open %s\n", argv[1]);
        return 1;
    }
    n = (long)fread(g_font, 1u, SFNTDUMP_MAX_FILE, fp);
    fclose(fp);
    if (n <= 0) {
        printf("empty or unreadable file\n");
        return 1;
    }

    r = sfnt_open(&face, g_font, (sfnt_u32)n);
    if (r != SFNT_OK) {
        printf("sfnt_open failed: %d\n", r);
        return 1;
    }

    printf("sfntVersion="); print_tag(face.sfnt_version); printf(" / 0x%08x\n", (unsigned)face.sfnt_version);
    printf("tables=%u glyphs=%u unitsPerEm=%u bbox=(%d,%d)-(%d,%d)\n",
           (unsigned)face.num_tables, (unsigned)face.num_glyphs, (unsigned)face.units_per_em,
           (int)face.x_min, (int)face.y_min, (int)face.x_max, (int)face.y_max);
    printf("hhea asc=%d desc=%d lineGap=%d hMetrics=%u\n",
           (int)face.ascender, (int)face.descender, (int)face.line_gap, (unsigned)face.num_h_metrics);

    printf("\nDirectory:\n");
    for (i = 0u; i < (sfnt_u32)face.num_tables; ++i) {
        printf("  "); print_tag(face.tables[i].tag);
        printf(" off=%u len=%u checksum=0x%08x",
               (unsigned)face.tables[i].offset,
               (unsigned)face.tables[i].length,
               (unsigned)face.tables[i].checksum);
        r = sfnt_validate_table_checksum(&face, face.tables[i].tag);
        printf(" %s\n", (r == SFNT_OK) ? "ok" : "checksum?" );
    }

    r = sfnt_select_cmap(&face, &cmap);
    if (r == SFNT_OK) {
        printf("\nSelected cmap: platform=%u encoding=%u format=%u length=%u\n",
               (unsigned)cmap.platform_id, (unsigned)cmap.encoding_id,
               (unsigned)cmap.format, (unsigned)cmap.length);
    } else {
        printf("\nNo supported cmap: %d\n", r);
    }

    gid = 0u;
    if (argc >= 3) {
        unsigned int cp = 0u;
        sscanf(argv[2], "%x", &cp);
        if (sfnt_lookup_glyph(&face, (sfnt_u32)cp, &gid) == SFNT_OK) {
            printf("U+%04X -> gid %u\n", cp, (unsigned)gid);
        }
    } else if (sfnt_lookup_glyph(&face, (sfnt_u32)'A', &gid) == SFNT_OK) {
        printf("U+0041 -> gid %u\n", (unsigned)gid);
    }

    if (gid != 0u && sfnt_get_hmetric(&face, gid, &hm) == SFNT_OK) {
        printf("gid %u hmetric advance=%u lsb=%d\n", (unsigned)gid, (unsigned)hm.advance_width, (int)hm.left_side_bearing);
    }

    if (gid != 0u && sfnt_get_glyph_header(&face, gid, &gh) == SFNT_OK) {
        printf("gid %u glyph contours=%d bbox=(%d,%d)-(%d,%d)\n",
               (unsigned)gid, (int)gh.number_of_contours,
               (int)gh.x_min, (int)gh.y_min, (int)gh.x_max, (int)gh.y_max);
        if (gh.number_of_contours > 0) {
            r = sfnt_decode_simple_glyph(&face, gid, g_points, (sfnt_u16)SFNTDUMP_MAX_POINTS, &outline);
            printf("  simple decode: r=%d points=%u contours=%u instr=%u\n",
                   r, (unsigned)outline.point_count, (unsigned)outline.contour_count,
                   (unsigned)outline.instruction_length);
        } else if (gh.number_of_contours < 0) {
            printf("  compound glyph:\n");
            (void)sfnt_visit_compound_glyph(&face, gid, visit_component, 0);
        }
    }

    if (sfnt_get_os2_metrics(&face, &os2) == SFNT_OK) {
        printf("OS/2 v%u weight=%u width=%u typoAsc=%d typoDesc=%d winAsc=%u winDesc=%u xHeight=%d capHeight=%d\n",
               (unsigned)os2.version, (unsigned)os2.us_weight_class, (unsigned)os2.us_width_class,
               (int)os2.s_typo_ascender, (int)os2.s_typo_descender,
               (unsigned)os2.us_win_ascent, (unsigned)os2.us_win_descent,
               (int)os2.sx_height, (int)os2.s_cap_height);
    }

    printf("name records: %u\n", (unsigned)sfnt_name_count(&face));
    return 0;
}

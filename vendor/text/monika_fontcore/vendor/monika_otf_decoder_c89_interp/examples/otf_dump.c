#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/otf_c89.h"

#ifndef OTF_DEMO_MAX_FONT_BYTES
#define OTF_DEMO_MAX_FONT_BYTES (32UL*1024UL*1024UL)
#endif
static unsigned char g_font[OTF_DEMO_MAX_FONT_BYTES];

static void print_tag(unsigned long t){ putchar((int)((t>>24)&255)); putchar((int)((t>>16)&255)); putchar((int)((t>>8)&255)); putchar((int)(t&255)); }
static unsigned long tag4(const char *s){ return OTF_TAG(s[0],s[1],s[2],s[3]); }

int main(int argc, char **argv){
    FILE *fp; unsigned long n; otf_font font; int e; char name[256]; unsigned short gid; otf_glyph_path path;
    if(argc<2){ printf("usage: otf_dump font.otf [U+hex] [axis=value ...]\nexample: otf_dump Cantarell-VF.otf 41 wght=700\n"); return 1; }
    fp=fopen(argv[1],"rb"); if(!fp){ printf("open failed\n"); return 1; }
    n=(unsigned long)fread(g_font,1,OTF_DEMO_MAX_FONT_BYTES,fp); fclose(fp);
    e=otf_parse(&font,g_font,n); if(e){ printf("parse error: %s (%d)\n",otf_errstr(e),e); return 2; }
    printf("sfnt: "); print_tag(font.sfnt_version); printf(" tables=%u upem=%u glyphs=%u\n",font.num_tables,font.units_per_em,font.num_glyphs);
    if(otf_get_name_ascii(&font,1,name,sizeof(name))==OTF_OK) printf("family: %s\n",name);
    if(otf_get_name_ascii(&font,4,name,sizeof(name))==OTF_OK) printf("full: %s\n",name);
    printf("hhea asc=%d desc=%d gap=%d hmetrics=%u\n",font.ascender,font.descender,font.line_gap,font.number_of_hmetrics);
    printf("OS/2 weight=%u width=%u typoAsc=%d typoDesc=%d\n",font.weight_class,font.width_class,font.os2_typo_ascender,font.os2_typo_descender);
    printf("CFF present=%d cff2=%d glyphs=%u charstringsOff=%lu fdCount=%u privateOff=%lu localSubrs=%u globalSubrs=%u vstoreItems=%u\n",font.cff.present,font.cff.is_cff2,font.cff.glyph_count,font.cff.top.charstrings_off,font.cff.fd_count,font.cff.top.private_off,font.cff.local_subr_index.count,font.cff.global_subr_index.count,font.cff.vstore_item_count);
    printf("HVAR=%d VVAR=%d MVAR=%d hvarData=%u vvarData=%u mvarRecords=%u\n",font.hvar.present,font.vvar.present,font.mvar.present,font.hvar.store.data_count,font.vvar.store.data_count,font.mvar.value_record_count);
    { unsigned short ai; for(ai=0; ai<font.var_axis_count; ai++){ printf("axis "); print_tag(font.var_axes[ai].tag); printf(" min=%ld default=%ld max=%ld user=%ld norm=%ld/65536\n",font.var_axes[ai].min_value>>16,font.var_axes[ai].default_value>>16,font.var_axes[ai].max_value>>16,font.var_axes[ai].user_value>>16,font.var_axes[ai].norm_value); } }
    if(argc>3){ int ai; for(ai=3; ai<argc; ai++){ char *eq=strchr(argv[ai],'='); if(eq && eq-argv[ai]>=4){ unsigned long tag=tag4(argv[ai]); long val=strtol(eq+1,0,10); e=otf_set_variation_axis_int(&font,tag,val); printf("set axis "); print_tag(tag); printf("=%ld -> %s (%d)\n",val,otf_errstr(e),e); } } }
    gid=otf_glyph_index_for_codepoint(&font, argc>=3 ? (unsigned long)strtoul(argv[2],0,16) : 0x41UL);
    printf("glyph for codepoint = %u\n",gid);
    e=otf_decode_glyph_path(&font,gid,&path); if(e){ printf("glyph decode error: %s (%d)\n",otf_errstr(e),e); return 3; }
    printf("path cmds=%u advance=%d lsb=%d advH=%d tsb=%d overflow=%d\n",path.count,path.advance_width,path.left_side_bearing,path.advance_height,path.top_side_bearing,path.overflow);
    { long d=0; if(otf_get_mvar_delta(&font, OTF_TAG('h','a','s','c'), &d)==OTF_OK) printf("MVAR hasc delta=%ld\n", d); }
    { unsigned short i, lim=path.count<24?path.count:24; for(i=0;i<lim;i++){ printf("%u op=%u x1=%d y1=%d x2=%d y2=%d x3=%d y3=%d\n",i,path.cmds[i].op,OTF_FIXED_TO_INT(path.cmds[i].x1),OTF_FIXED_TO_INT(path.cmds[i].y1),OTF_FIXED_TO_INT(path.cmds[i].x2),OTF_FIXED_TO_INT(path.cmds[i].y2),OTF_FIXED_TO_INT(path.cmds[i].x3),OTF_FIXED_TO_INT(path.cmds[i].y3)); } }
    return 0;
}

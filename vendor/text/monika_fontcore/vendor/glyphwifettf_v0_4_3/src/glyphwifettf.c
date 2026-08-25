#include "glyphwifettf.h"


#define GWT_ABS(a) ((a)<0?-(a):(a))
#define GWT_MIN(a,b) ((a)<(b)?(a):(b))
#define GWT_MAX(a,b) ((a)>(b)?(a):(b))

static unsigned short rd_u16(const unsigned char *p){ return (unsigned short)(((unsigned short)p[0]<<8)|p[1]); }
static short rd_s16(const unsigned char *p){ return (short)rd_u16(p); }
static unsigned long rd_u32(const unsigned char *p){ return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3]; }
static int in_range(const GWT_Font *f, unsigned long off, unsigned long len){ return f && off <= f->size && len <= f->size - off; }
static const unsigned char *ptr(const GWT_Font *f, unsigned long off, unsigned long len){ if(!in_range(f,off,len)) return 0; return f->data + off; }
#ifndef GWT_USE_LONG_LONG
#define GWT_USE_LONG_LONG 1
#endif

static gwt_fixed fp_mul(gwt_fixed a, gwt_fixed b){
#if GWT_USE_LONG_LONG
    return (gwt_fixed)(((long long)a * (long long)b) >> GWT_FP_SHIFT);
#else
    /* Strict-C89 fallback: avoids 64-bit syntax, intended for moderate 16.16 values. */
    int neg;
    unsigned long ua, ub, ah, al, bh, bl, r;
    neg = 0;
    if(a < 0){ ua = (unsigned long)(-a); neg = !neg; } else ua = (unsigned long)a;
    if(b < 0){ ub = (unsigned long)(-b); neg = !neg; } else ub = (unsigned long)b;
    ah = ua >> 16; al = ua & 0xFFFFUL;
    bh = ub >> 16; bl = ub & 0xFFFFUL;
    r = (ah * bh << 16) + (ah * bl) + (al * bh) + ((al * bl) >> 16);
    return neg ? -(gwt_fixed)r : (gwt_fixed)r;
#endif
}
static gwt_fixed fp_from_units(int v){ return ((gwt_fixed)v) << GWT_FP_SHIFT; }
static int fp_to_int_round(gwt_fixed v){ if(v >= 0) return (int)((v + GWT_FP_HALF) >> GWT_FP_SHIFT); return -(int)(((-v) + GWT_FP_HALF) >> GWT_FP_SHIFT); }

int gwt_find_table(const GWT_Font *font, unsigned long tag, GWT_Table *out_table){
    unsigned long off, n, i;
    const unsigned char *p;
    if(!font || !out_table) return GWT_ERR_BAD_ARG;
    off = 12; n = font->num_tables;
    if(!in_range(font, off, n * 16UL)) return GWT_ERR_BAD_FONT;
    for(i=0;i<n;i++){
        p = font->data + off + i*16UL;
        if(rd_u32(p) == tag){ out_table->tag=tag; out_table->offset=rd_u32(p+8); out_table->length=rd_u32(p+12); if(!in_range(font,out_table->offset,out_table->length)) return GWT_ERR_BAD_FONT; return GWT_OK; }
    }
    out_table->tag=0; out_table->offset=0; out_table->length=0;
    return GWT_ERR_TABLE_MISSING;
}

static int load_opt_table(GWT_Font *font, unsigned long tag, GWT_Table *tab){ int r = gwt_find_table(font, tag, tab); if(r!=GWT_OK){ tab->tag=0; tab->offset=0; tab->length=0; } return GWT_OK; }

static int cmap_scan(GWT_Font *font){
    const unsigned char *p; unsigned short num, i, platform, encoding, fmt; unsigned long suboff, abs;
    font->cmap_fmt4_offset = 0; font->cmap_fmt12_offset = 0;
    p = ptr(font, font->table_cmap.offset, 4); if(!p) return GWT_ERR_BAD_FONT;
    num = rd_u16(p+2);
    if(!ptr(font, font->table_cmap.offset+4, (unsigned long)num*8UL)) return GWT_ERR_BAD_FONT;
    for(i=0;i<num;i++){
        p = font->data + font->table_cmap.offset + 4UL + (unsigned long)i*8UL;
        platform = rd_u16(p); encoding = rd_u16(p+2); suboff = rd_u32(p+4); abs = font->table_cmap.offset + suboff;
        p = ptr(font, abs, 2); if(!p) continue; fmt = rd_u16(p);
        if(fmt == 12 && (platform == 3 || platform == 0)) font->cmap_fmt12_offset = abs;
        if(fmt == 4 && (platform == 3 || platform == 0) && font->cmap_fmt4_offset == 0) font->cmap_fmt4_offset = abs;
        (void)encoding;
    }
    if(font->cmap_fmt12_offset || font->cmap_fmt4_offset) return GWT_OK;
    return GWT_ERR_UNSUPPORTED;
}

int gwt_font_init(GWT_Font *font, const unsigned char *data, unsigned long size){
    const unsigned char *p; unsigned long sig;
    if(!font || !data || size < 12) return GWT_ERR_BAD_ARG;
    font->data=data; font->size=size; font->num_tables=0;
    p = data; sig = rd_u32(p);
    if(!(sig == 0x00010000UL || sig == GWT_TAG('t','r','u','e') || sig == GWT_TAG('t','t','c','f') || sig == GWT_TAG('O','T','T','O'))) return GWT_ERR_BAD_FONT;
    if(sig == GWT_TAG('t','t','c','f') || sig == GWT_TAG('O','T','T','O')) return GWT_ERR_UNSUPPORTED;
    font->num_tables = rd_u16(p+4);
    if(!in_range(font,12,(unsigned long)font->num_tables*16UL)) return GWT_ERR_BAD_FONT;
    if(gwt_find_table(font,GWT_TAG('h','e','a','d'),&font->table_head)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    if(gwt_find_table(font,GWT_TAG('m','a','x','p'),&font->table_maxp)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    if(gwt_find_table(font,GWT_TAG('h','h','e','a'),&font->table_hhea)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    if(gwt_find_table(font,GWT_TAG('h','m','t','x'),&font->table_hmtx)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    if(gwt_find_table(font,GWT_TAG('c','m','a','p'),&font->table_cmap)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    if(gwt_find_table(font,GWT_TAG('l','o','c','a'),&font->table_loca)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    if(gwt_find_table(font,GWT_TAG('g','l','y','f'),&font->table_glyf)!=GWT_OK) return GWT_ERR_TABLE_MISSING;
    load_opt_table(font,GWT_TAG('n','a','m','e'),&font->table_name); load_opt_table(font,GWT_TAG('k','e','r','n'),&font->table_kern);
    p = ptr(font,font->table_head.offset,54); if(!p) return GWT_ERR_BAD_FONT;
    font->units_per_em=rd_u16(p+18); font->x_min=rd_s16(p+36); font->y_min=rd_s16(p+38); font->x_max=rd_s16(p+40); font->y_max=rd_s16(p+42); font->index_to_loc_format=rd_s16(p+50);
    p = ptr(font,font->table_maxp.offset,6); if(!p) return GWT_ERR_BAD_FONT; font->num_glyphs=rd_u16(p+4);
    p = ptr(font,font->table_hhea.offset,36); if(!p) return GWT_ERR_BAD_FONT; font->ascent=rd_s16(p+4); font->descent=rd_s16(p+6); font->line_gap=rd_s16(p+8); font->num_hmetrics=rd_u16(p+34);
    return cmap_scan(font);
}

static int cmap4_lookup(const GWT_Font *font, unsigned long cp, unsigned short *gid){
    const unsigned char *p; unsigned short segCount, i, endc, startc, iddelta, ro; unsigned long endOff, startOff, deltaOff, roOff, glyphIndexOff;
    if(cp > 0xFFFFUL || !font->cmap_fmt4_offset) return GWT_ERR_UNSUPPORTED;
    p = ptr(font,font->cmap_fmt4_offset,16); if(!p) return GWT_ERR_BAD_FONT;
    segCount = (unsigned short)(rd_u16(p+6)/2);
    endOff = font->cmap_fmt4_offset + 14;
    startOff = endOff + (unsigned long)segCount*2UL + 2UL;
    deltaOff = startOff + (unsigned long)segCount*2UL;
    roOff = deltaOff + (unsigned long)segCount*2UL;
    if(!ptr(font, roOff, (unsigned long)segCount*2UL)) return GWT_ERR_BAD_FONT;
    for(i=0;i<segCount;i++){
        endc=rd_u16(font->data+endOff+(unsigned long)i*2UL); startc=rd_u16(font->data+startOff+(unsigned long)i*2UL);
        if(cp >= startc && cp <= endc){
            iddelta=rd_u16(font->data+deltaOff+(unsigned long)i*2UL); ro=rd_u16(font->data+roOff+(unsigned long)i*2UL);
            if(ro==0){ *gid=(unsigned short)((cp + iddelta) & 0xFFFFU); return GWT_OK; }
            glyphIndexOff = roOff + (unsigned long)i*2UL + ro + ((cp - startc) * 2UL);
            if(!ptr(font,glyphIndexOff,2)) return GWT_ERR_BAD_FONT;
            *gid=rd_u16(font->data+glyphIndexOff); if(*gid) *gid=(unsigned short)((*gid + iddelta)&0xFFFFU); return GWT_OK;
        }
    }
    *gid=0; return GWT_OK;
}

static int cmap12_lookup(const GWT_Font *font, unsigned long cp, unsigned short *gid){
    const unsigned char *p; unsigned long n, i, start, end, startGlyph;
    if(!font->cmap_fmt12_offset) return GWT_ERR_UNSUPPORTED;
    p = ptr(font,font->cmap_fmt12_offset,16); if(!p) return GWT_ERR_BAD_FONT; n=rd_u32(p+12);
    if(!ptr(font,font->cmap_fmt12_offset+16,n*12UL)) return GWT_ERR_BAD_FONT;
    for(i=0;i<n;i++){
        p = font->data + font->cmap_fmt12_offset + 16UL + i*12UL; start=rd_u32(p); end=rd_u32(p+4); startGlyph=rd_u32(p+8);
        if(cp>=start && cp<=end){ *gid=(unsigned short)(startGlyph + (cp-start)); return GWT_OK; }
    }
    *gid=0; return GWT_OK;
}

int gwt_codepoint_to_glyph(const GWT_Font *font, unsigned long codepoint, unsigned short *glyph_id){
    int r; if(!font||!glyph_id) return GWT_ERR_BAD_ARG;
    if(codepoint > 0xFFFFUL && font->cmap_fmt12_offset){ r=cmap12_lookup(font,codepoint,glyph_id); if(r==GWT_OK) return r; }
    r=cmap4_lookup(font,codepoint,glyph_id); if(r==GWT_OK) return r;
    return cmap12_lookup(font,codepoint,glyph_id);
}

static int glyph_offset(const GWT_Font *font, unsigned short gid, unsigned long *off, unsigned long *len){
    unsigned long a,b; const unsigned char *p;
    if(gid >= font->num_glyphs) return GWT_ERR_RANGE;
    if(font->index_to_loc_format==0){ p=ptr(font,font->table_loca.offset+(unsigned long)gid*2UL,4); if(!p)return GWT_ERR_BAD_FONT; a=(unsigned long)rd_u16(p)*2UL; b=(unsigned long)rd_u16(p+2)*2UL; }
    else { p=ptr(font,font->table_loca.offset+(unsigned long)gid*4UL,8); if(!p)return GWT_ERR_BAD_FONT; a=rd_u32(p); b=rd_u32(p+4); }
    if(b<a || a>font->table_glyf.length || b>font->table_glyf.length) return GWT_ERR_BAD_FONT;
    *off=font->table_glyf.offset+a; *len=b-a; return GWT_OK;
}

int gwt_get_glyph_metrics(const GWT_Font *font, unsigned short glyph_id, GWT_GlyphMetrics *m){
    unsigned long off,len,hOff; const unsigned char *p;
    if(!font||!m) return GWT_ERR_BAD_ARG;
    if(glyph_id>=font->num_glyphs) return GWT_ERR_RANGE;
    if(glyph_id < font->num_hmetrics) hOff = font->table_hmtx.offset + (unsigned long)glyph_id*4UL; else hOff = font->table_hmtx.offset + (unsigned long)(font->num_hmetrics-1U)*4UL;
    p=ptr(font,hOff,4); if(!p)return GWT_ERR_BAD_FONT; m->advance_width=rd_u16(p); m->left_side_bearing=rd_s16(p+2);
    if(glyph_id >= font->num_hmetrics){ p=ptr(font,font->table_hmtx.offset+(unsigned long)font->num_hmetrics*4UL+(unsigned long)(glyph_id-font->num_hmetrics)*2UL,2); if(p) m->left_side_bearing=rd_s16(p); }
    if(glyph_offset(font,glyph_id,&off,&len)!=GWT_OK || len<10){ m->x_min=m->y_min=m->x_max=m->y_max=0; return GWT_OK; }
    p=ptr(font,off,10); if(!p)return GWT_ERR_BAD_FONT; m->x_min=rd_s16(p+2); m->y_min=rd_s16(p+4); m->x_max=rd_s16(p+6); m->y_max=rd_s16(p+8); return GWT_OK;
}

int gwt_get_kerning(const GWT_Font *font, unsigned short left_gid, unsigned short right_gid, int *kern_units){
    const unsigned char *p; unsigned short nTables, i; unsigned long off;
    if(!font||!kern_units) return GWT_ERR_BAD_ARG;
    *kern_units=0;
    if(!font->table_kern.tag) return GWT_OK;
    p=ptr(font,font->table_kern.offset,4); if(!p)return GWT_ERR_BAD_FONT; nTables=rd_u16(p+2); off=font->table_kern.offset+4;
    for(i=0;i<nTables;i++){
        unsigned short length, coverage, nPairs; unsigned long pairOff, lo, hi, mid, key, cur;
        p=ptr(font,off,6); if(!p)return GWT_ERR_BAD_FONT; length=rd_u16(p+2); coverage=rd_u16(p+4);
        if((coverage & 0x00FFU)==0){ p=ptr(font,off+6,8); if(!p)return GWT_ERR_BAD_FONT; nPairs=rd_u16(p); pairOff=off+14; key=((unsigned long)left_gid<<16)|right_gid; lo=0; hi=nPairs;
            while(lo<hi){ mid=(lo+hi)/2; p=ptr(font,pairOff+mid*6UL,6); if(!p)return GWT_ERR_BAD_FONT; cur=((unsigned long)rd_u16(p)<<16)|rd_u16(p+2); if(cur<key)lo=mid+1; else hi=mid; }
            if(lo<nPairs){ p=ptr(font,pairOff+lo*6UL,6); if(!p)return GWT_ERR_BAD_FONT; cur=((unsigned long)rd_u16(p)<<16)|rd_u16(p+2); if(cur==key){ *kern_units=rd_s16(p+4); return GWT_OK; } }
        }
        off += length; if(off > font->table_kern.offset + font->table_kern.length) break;
    }
    return GWT_OK;
}

void gwt_outline_reset(GWT_Outline *o){ if(o){ o->point_count=0; o->contour_count=0; o->x_min=o->y_min=o->x_max=o->y_max=0; } }

static int add_point(GWT_Outline *o, gwt_fixed x, gwt_fixed y, unsigned char on, unsigned short contour){
    if(o->point_count>=o->max_points) return GWT_ERR_BUFFER_FULL;
    o->points[o->point_count].x=x; o->points[o->point_count].y=y; o->points[o->point_count].flags=(unsigned char)(on?GWT_POINT_ON_CURVE:0); o->points[o->point_count].contour=contour; o->point_count++; return GWT_OK;
}

static int add_contour(GWT_Outline *o, unsigned short first, unsigned short count){ if(o->contour_count>=o->max_contours)return GWT_ERR_BUFFER_FULL; o->contours[o->contour_count].first_point=first; o->contours[o->contour_count].point_count=count; o->contour_count++; return GWT_OK; }

typedef struct Mat2x3 { gwt_fixed xx, yx, xy, yy, dx, dy; } Mat2x3;
static void mat_identity(Mat2x3 *m){ m->xx=GWT_FP_ONE; m->yx=0; m->xy=0; m->yy=GWT_FP_ONE; m->dx=0; m->dy=0; }
static void mat_apply(const Mat2x3 *m, gwt_fixed x, gwt_fixed y, gwt_fixed *ox, gwt_fixed *oy){ *ox=fp_mul(m->xx,x)+fp_mul(m->xy,y)+m->dx; *oy=fp_mul(m->yx,x)+fp_mul(m->yy,y)+m->dy; }
static void mat_compose(const Mat2x3 *a, const Mat2x3 *b, Mat2x3 *out){ Mat2x3 r; r.xx=fp_mul(a->xx,b->xx)+fp_mul(a->xy,b->yx); r.xy=fp_mul(a->xx,b->xy)+fp_mul(a->xy,b->yy); r.yx=fp_mul(a->yx,b->xx)+fp_mul(a->yy,b->yx); r.yy=fp_mul(a->yx,b->xy)+fp_mul(a->yy,b->yy); r.dx=fp_mul(a->xx,b->dx)+fp_mul(a->xy,b->dy)+a->dx; r.dy=fp_mul(a->yx,b->dx)+fp_mul(a->yy,b->dy)+a->dy; *out=r; }
static gwt_fixed f2dot14(short v){ return ((gwt_fixed)v) << 2; }

static int decode_glyph_rec(const GWT_Font *font, unsigned short gid, GWT_Outline *o, const Mat2x3 *tr, int depth){
    unsigned long off,len,pos; const unsigned char *p; short nContours,xmin,ymin,xmax,ymax; unsigned short i,totalPts,firstAdded;
    if(depth>GWT_MAX_RECURSION) return GWT_ERR_RECURSION_LIMIT;
    if(glyph_offset(font,gid,&off,&len)!=GWT_OK) return GWT_ERR_RANGE;
    if(len==0) return GWT_OK;
    p=ptr(font,off,10);
    if(!p)return GWT_ERR_BAD_FONT;
    nContours=rd_s16(p); xmin=rd_s16(p+2); ymin=rd_s16(p+4); xmax=rd_s16(p+6); ymax=rd_s16(p+8); if(o->point_count==0){ o->x_min=xmin;o->y_min=ymin;o->x_max=xmax;o->y_max=ymax; }
    if(nContours>=0){
        unsigned short endPts[256]; unsigned short instrLen; unsigned char flags[4096]; short xs[4096], ys[4096]; unsigned short fcount; int x,y; unsigned char flag, repeat; gwt_fixed tx,ty; unsigned short cstart,cend;
        if(nContours>256) return GWT_ERR_BUFFER_FULL;
        pos=off+10; if(!ptr(font,pos,(unsigned long)nContours*2UL))return GWT_ERR_BAD_FONT; totalPts=0;
        for(i=0;i<(unsigned short)nContours;i++){ endPts[i]=rd_u16(font->data+pos+i*2UL); totalPts=(unsigned short)(endPts[i]+1U); }
        if(totalPts>4096) return GWT_ERR_BUFFER_FULL;
        pos += (unsigned long)nContours*2UL;
        p=ptr(font,pos,2);
        if(!p)return GWT_ERR_BAD_FONT;
        instrLen=rd_u16(p);
        pos+=2UL+instrLen;
        if(!ptr(font,pos,1))return GWT_ERR_BAD_FONT;
        fcount=0; while(fcount<totalPts){ p=ptr(font,pos,1); if(!p)return GWT_ERR_BAD_FONT; flag=*p; pos++; flags[fcount++]=flag; if(flag&8){ p=ptr(font,pos,1); if(!p)return GWT_ERR_BAD_FONT; repeat=*p; pos++; while(repeat-- && fcount<totalPts) flags[fcount++]=flag; } }
        x=0; for(i=0;i<totalPts;i++){ flag=flags[i]; if(flag&2){ p=ptr(font,pos,1); if(!p)return GWT_ERR_BAD_FONT; pos++; x += (flag&16)?(int)(*p):-(int)(*p); } else { if(flag&16){} else { p=ptr(font,pos,2); if(!p)return GWT_ERR_BAD_FONT; x += rd_s16(p); pos+=2; } } xs[i]=(short)x; }
        y=0; for(i=0;i<totalPts;i++){ flag=flags[i]; if(flag&4){ p=ptr(font,pos,1); if(!p)return GWT_ERR_BAD_FONT; pos++; y += (flag&32)?(int)(*p):-(int)(*p); } else { if(flag&32){} else { p=ptr(font,pos,2); if(!p)return GWT_ERR_BAD_FONT; y += rd_s16(p); pos+=2; } } ys[i]=(short)y; }
        firstAdded=o->point_count; cstart=0;
        for(i=0;i<(unsigned short)nContours;i++){
            unsigned short j, cnt; cend=endPts[i]; cnt=(unsigned short)(cend-cstart+1U); if(add_contour(o,o->point_count,cnt)!=GWT_OK)return GWT_ERR_BUFFER_FULL;
            for(j=cstart;j<=cend;j++){ mat_apply(tr,fp_from_units(xs[j]),fp_from_units(ys[j]),&tx,&ty); if(add_point(o,tx,ty,(unsigned char)(flags[j]&1U),i)!=GWT_OK)return GWT_ERR_BUFFER_FULL; }
            cstart=(unsigned short)(cend+1U);
        }
        (void)firstAdded; return GWT_OK;
    } else {
        unsigned short flags, comp_gid; int more=1; Mat2x3 local, combo;
        pos=off+10;
        while(more){
            int arg1,arg2; p=ptr(font,pos,4); if(!p)return GWT_ERR_BAD_FONT; flags=rd_u16(p); comp_gid=rd_u16(p+2); pos+=4; arg1=arg2=0; mat_identity(&local);
            if(flags&1){ p=ptr(font,pos,4); if(!p)return GWT_ERR_BAD_FONT; arg1=rd_s16(p); arg2=rd_s16(p+2); pos+=4; } else { p=ptr(font,pos,2); if(!p)return GWT_ERR_BAD_FONT; arg1=(signed char)p[0]; arg2=(signed char)p[1]; pos+=2; }
            if(flags&2){ local.dx=fp_from_units(arg1); local.dy=fp_from_units(arg2); }
            if(flags&8){ p=ptr(font,pos,2); if(!p)return GWT_ERR_BAD_FONT; local.xx=local.yy=f2dot14(rd_s16(p)); pos+=2; }
            else if(flags&64){ p=ptr(font,pos,4); if(!p)return GWT_ERR_BAD_FONT; local.xx=f2dot14(rd_s16(p)); local.yy=f2dot14(rd_s16(p+2)); pos+=4; }
            else if(flags&128){ p=ptr(font,pos,8); if(!p)return GWT_ERR_BAD_FONT; local.xx=f2dot14(rd_s16(p)); local.yx=f2dot14(rd_s16(p+2)); local.xy=f2dot14(rd_s16(p+4)); local.yy=f2dot14(rd_s16(p+6)); pos+=8; }
            mat_compose(tr,&local,&combo); if(decode_glyph_rec(font,comp_gid,o,&combo,depth+1)!=GWT_OK) return GWT_ERR_BUFFER_FULL;
            more = (flags & 32) ? 1 : 0;
            if(!(flags & 32)) break;
        }
        return GWT_OK;
    }
}

int gwt_decode_glyph_outline(const GWT_Font *font, unsigned short glyph_id, GWT_Outline *outline){ Mat2x3 id; if(!font||!outline||!outline->points||!outline->contours)return GWT_ERR_BAD_ARG; gwt_outline_reset(outline); mat_identity(&id); return decode_glyph_rec(font,glyph_id,outline,&id,0); }

static GWT_Point midp(GWT_Point a, GWT_Point b){ GWT_Point m; m.x=(a.x+b.x)/2; m.y=(a.y+b.y)/2; m.flags=GWT_POINT_ON_CURVE; m.contour=a.contour; return m; }
static int seg_add(GWT_SegmentBuffer *b, unsigned char type, GWT_Point a, GWT_Point c, GWT_Point d){ GWT_Segment *s; if(b->count>=b->max_segments)return GWT_ERR_BUFFER_FULL; s=&b->segments[b->count++]; s->type=type; s->x0=a.x; s->y0=a.y; s->x1=c.x; s->y1=c.y; s->x2=d.x; s->y2=d.y; return GWT_OK; }
int gwt_outline_to_segments(const GWT_Outline *o, GWT_SegmentBuffer *b){
    unsigned short ci; if(!o||!b||!b->segments)return GWT_ERR_BAD_ARG; b->count=0;
    for(ci=0;ci<o->contour_count;ci++){
        GWT_Contour c=o->contours[ci]; unsigned short n=c.point_count, k; GWT_Point first, cur, next, after;
        if(n==0) continue;
        first=o->points[c.first_point];
        if(!(first.flags&GWT_POINT_ON_CURVE)){ GWT_Point last=o->points[c.first_point+n-1]; first=(last.flags&GWT_POINT_ON_CURVE)?last:midp(last,first); }
        cur=first; if(seg_add(b,0,cur,cur,cur)!=GWT_OK)return GWT_ERR_BUFFER_FULL;
        k=0; while(k<n){ next=o->points[c.first_point+k]; if(k==0 && !(o->points[c.first_point].flags&GWT_POINT_ON_CURVE)){ k++; continue; }
            if(next.flags&GWT_POINT_ON_CURVE){ if(!(next.x==cur.x && next.y==cur.y)) if(seg_add(b,1,cur,next,next)!=GWT_OK)return GWT_ERR_BUFFER_FULL; cur=next; k++; }
            else { after = (k+1<n)?o->points[c.first_point+k+1]:first; if(!(after.flags&GWT_POINT_ON_CURVE)) after=midp(next,after); if(seg_add(b,2,cur,next,after)!=GWT_OK)return GWT_ERR_BUFFER_FULL; cur=after; k += (after.flags&GWT_POINT_ON_CURVE && k+1<n)?2:1; }
        }
        if(seg_add(b,3,cur,first,first)!=GWT_OK)return GWT_ERR_BUFFER_FULL;
    }
    return GWT_OK;
}

gwt_fixed gwt_make_scale(const GWT_Font *font, int pixel_size){
    if(!font || font->units_per_em == 0) return 0;
#if GWT_USE_LONG_LONG
    return (gwt_fixed)(((long long)pixel_size << GWT_FP_SHIFT) / font->units_per_em);
#else
    return (gwt_fixed)(((unsigned long)pixel_size << GWT_FP_SHIFT) / (unsigned long)font->units_per_em);
#endif
}
gwt_fixed gwt_units_to_pixels(gwt_fixed scale, int units){ return fp_mul(fp_from_units(units),scale); }

static int point_in_poly_fp(int px, int py, const GWT_Outline *o, gwt_fixed scale, int ox, int oy){
    int inside=0; unsigned short ci;
    for(ci=0;ci<o->contour_count;ci++){ GWT_Contour c=o->contours[ci]; unsigned short i,j; if(c.point_count<2)continue; j=(unsigned short)(c.point_count-1U); for(i=0;i<c.point_count;i++){ GWT_Point a=o->points[c.first_point+i], b=o->points[c.first_point+j]; int ax=ox+fp_to_int_round(fp_mul(a.x,scale)); int ay=oy-fp_to_int_round(fp_mul(a.y,scale)); int bx=ox+fp_to_int_round(fp_mul(b.x,scale)); int by=oy-fp_to_int_round(fp_mul(b.y,scale)); if(((ay>py)!=(by>py)) && (px < (bx-ax)*(py-ay)/(by-ay==0?1:by-ay)+ax)) inside=!inside; j=i; } }
    return inside;
}
int gwt_rasterize_outline(const GWT_Outline *outline, gwt_fixed scale, GWT_Bitmap *bmp, int origin_x_px, int origin_y_px, unsigned char fill_value){
    unsigned short x,y; if(!outline||!bmp||!bmp->pixels)return GWT_ERR_BAD_ARG;
    for(y=0;y<bmp->height;y++) for(x=0;x<bmp->width;x++) if(point_in_poly_fp((int)x,(int)y,outline,scale,origin_x_px,origin_y_px)) bmp->pixels[(unsigned long)y*bmp->stride+x]=fill_value;
    return GWT_OK;
}

void gwt_atlas_init(GWT_Atlas *a, unsigned char *pixels, unsigned short w, unsigned short h, unsigned short stride, GWT_AtlasGlyph *glyphs, unsigned short max_glyphs){ unsigned long i, n=(unsigned long)h*stride; if(!a)return; a->pixels=pixels; a->width=w; a->height=h; a->stride=stride; a->glyphs=glyphs; a->max_glyphs=max_glyphs; a->glyph_count=0; a->pen_x=1; a->pen_y=1; a->row_h=0; if(pixels) for(i=0;i<n;i++) pixels[i]=0; }
const GWT_AtlasGlyph *gwt_atlas_find(const GWT_Atlas *a, unsigned long cp){ unsigned short i; if(!a)return 0; for(i=0;i<a->glyph_count;i++) if(a->glyphs[i].codepoint==cp)return &a->glyphs[i]; return 0; }
int gwt_atlas_add_codepoint(const GWT_Font *font, GWT_Atlas *a, unsigned long cp, int pixel_size, GWT_Outline *scratch){
    unsigned short gid; GWT_GlyphMetrics m; gwt_fixed sc; int r,w,h,base,ox,oy; GWT_Bitmap sub; GWT_AtlasGlyph *ag;
    if(!font||!a||!scratch)return GWT_ERR_BAD_ARG;
    if(gwt_atlas_find(a,cp)) return GWT_OK;
    if(a->glyph_count>=a->max_glyphs)return GWT_ERR_BUFFER_FULL;
    r=gwt_codepoint_to_glyph(font,cp,&gid); if(r!=GWT_OK)return r; r=gwt_get_glyph_metrics(font,gid,&m); if(r!=GWT_OK)return r; r=gwt_decode_glyph_outline(font,gid,scratch); if(r!=GWT_OK)return r;
    sc=gwt_make_scale(font,pixel_size); w=fp_to_int_round(gwt_units_to_pixels(sc,m.x_max-m.x_min))+4; h=fp_to_int_round(gwt_units_to_pixels(sc,m.y_max-m.y_min))+4; if(w<1)w=1;if(h<1)h=1;
    if(a->pen_x + w + 1 > a->width){ a->pen_x=1; a->pen_y=(unsigned short)(a->pen_y+a->row_h+1); a->row_h=0; }
    if(a->pen_y + h + 1 > a->height) return GWT_ERR_BUFFER_FULL;
    if(h>a->row_h)a->row_h=(unsigned short)h;
    sub.pixels=a->pixels + (unsigned long)a->pen_y*a->stride + a->pen_x; sub.width=(unsigned short)w; sub.height=(unsigned short)h; sub.stride=a->stride; base=fp_to_int_round(gwt_units_to_pixels(sc,m.y_max))+2; ox=2-fp_to_int_round(gwt_units_to_pixels(sc,m.x_min)); oy=base; gwt_rasterize_outline(scratch,sc,&sub,ox,oy,255);
    ag=&a->glyphs[a->glyph_count++]; ag->codepoint=cp; ag->glyph_id=gid; ag->x=a->pen_x; ag->y=a->pen_y; ag->w=(unsigned short)w; ag->h=(unsigned short)h; ag->advance_px=fp_to_int_round(gwt_units_to_pixels(sc,m.advance_width)); ag->bearing_x_px=fp_to_int_round(gwt_units_to_pixels(sc,m.left_side_bearing)); ag->bearing_y_px=base; a->pen_x=(unsigned short)(a->pen_x+w+1); return GWT_OK;
}

unsigned long gwt_utf8_next(const char **text){ const unsigned char *s=(const unsigned char*)*text; unsigned long cp; if(!s||!*s)return 0; if(s[0]<0x80){ *text=(const char*)(s+1); return s[0]; } if((s[0]&0xE0)==0xC0 && (s[1]&0xC0)==0x80){ cp=((unsigned long)(s[0]&0x1F)<<6)|(s[1]&0x3F); *text=(const char*)(s+2); return cp; } if((s[0]&0xF0)==0xE0 && (s[1]&0xC0)==0x80 && (s[2]&0xC0)==0x80){ cp=((unsigned long)(s[0]&0x0F)<<12)|((unsigned long)(s[1]&0x3F)<<6)|(s[2]&0x3F); *text=(const char*)(s+3); return cp; } if((s[0]&0xF8)==0xF0 && (s[1]&0xC0)==0x80 && (s[2]&0xC0)==0x80 && (s[3]&0xC0)==0x80){ cp=((unsigned long)(s[0]&7)<<18)|((unsigned long)(s[1]&0x3F)<<12)|((unsigned long)(s[2]&0x3F)<<6)|(s[3]&0x3F); *text=(const char*)(s+4); return cp; } *text=(const char*)(s+1); return 0xFFFDUL; }
int gwt_layout_utf8(const GWT_Font *font, const char *text, int pixel_size, int x, int y, GWT_LayoutGlyph *out, unsigned short max_out, unsigned short *out_count){
    const char *p=text; unsigned long cp; unsigned short count=0, gid, prev=0; gwt_fixed sc; int pen=x, kern; GWT_GlyphMetrics m;
    if(!font||!text||!out||!out_count)return GWT_ERR_BAD_ARG;
    sc=gwt_make_scale(font,pixel_size);
    while((cp=gwt_utf8_next(&p))!=0){ if(cp=='\n'){ pen=x; y += pixel_size; prev=0; continue; } if(count>=max_out){ *out_count=count; return GWT_ERR_BUFFER_FULL; } if(gwt_codepoint_to_glyph(font,cp,&gid)!=GWT_OK) gid=0; kern=0; if(prev) gwt_get_kerning(font,prev,gid,&kern); pen += fp_to_int_round(gwt_units_to_pixels(sc,kern)); gwt_get_glyph_metrics(font,gid,&m); out[count].codepoint=cp; out[count].glyph_id=gid; out[count].x=pen; out[count].y=y; out[count].advance_px=fp_to_int_round(gwt_units_to_pixels(sc,m.advance_width)); pen += out[count].advance_px; prev=gid; count++; }
    *out_count=count; return GWT_OK;
}

/* ---- v0.3.0 utility/quality-of-life layer ---- */
const char *gwt_error_string(int code){
    switch(code){
        case GWT_OK: return "GWT_OK";
        case GWT_ERR_BAD_ARG: return "GWT_ERR_BAD_ARG";
        case GWT_ERR_BAD_FONT: return "GWT_ERR_BAD_FONT";
        case GWT_ERR_TABLE_MISSING: return "GWT_ERR_TABLE_MISSING";
        case GWT_ERR_UNSUPPORTED: return "GWT_ERR_UNSUPPORTED";
        case GWT_ERR_RANGE: return "GWT_ERR_RANGE";
        case GWT_ERR_BUFFER_FULL: return "GWT_ERR_BUFFER_FULL";
        case GWT_ERR_RECURSION_LIMIT: return "GWT_ERR_RECURSION_LIMIT";
        default: return "GWT_ERR_UNKNOWN";
    }
}

void gwt_bitmap_clear(GWT_Bitmap *bmp, unsigned char value){
    unsigned short y, x;
    if(!bmp || !bmp->pixels) return;
    for(y=0; y<bmp->height; y++){
        for(x=0; x<bmp->width; x++){
            bmp->pixels[(unsigned long)y * bmp->stride + x] = value;
        }
    }
}

static gwt_fixed fp_lerp(gwt_fixed a, gwt_fixed b, gwt_fixed t){
    return a + fp_mul(b - a, t);
}

static void point_to_screen_fp(GWT_Point p, gwt_fixed scale, int ox, int oy, gwt_fixed *x, gwt_fixed *y){
    *x = (((gwt_fixed)ox) << GWT_FP_SHIFT) + fp_mul(p.x, scale);
    *y = (((gwt_fixed)oy) << GWT_FP_SHIFT) - fp_mul(p.y, scale);
}

static int edge_cross_fp(gwt_fixed px, gwt_fixed py, gwt_fixed ax, gwt_fixed ay, gwt_fixed bx, gwt_fixed by){
    gwt_fixed xint;
    if(((ay > py) != (by > py))){
        if(by == ay) return 0;
        #if GWT_USE_LONG_LONG
        xint = ax + (gwt_fixed)(((long long)(bx - ax) * (long long)(py - ay)) / (long long)(by - ay));
#else
        {
            gwt_fixed dx, dy, ddy;
            dx = bx - ax;
            dy = py - ay;
            ddy = by - ay;
            xint = ax + (gwt_fixed)((dx / ddy) * dy + ((dx % ddy) * (dy >> 8)) / (ddy >> 8 ? (ddy >> 8) : 1));
        }
#endif
        if(px < xint) return 1;
    }
    return 0;
}

static int line_cross_scaled(gwt_fixed px, gwt_fixed py, GWT_Point a, GWT_Point b, gwt_fixed scale, int ox, int oy){
    gwt_fixed ax, ay, bx, by;
    point_to_screen_fp(a, scale, ox, oy, &ax, &ay);
    point_to_screen_fp(b, scale, ox, oy, &bx, &by);
    return edge_cross_fp(px, py, ax, ay, bx, by);
}

static int quad_cross_scaled(gwt_fixed px, gwt_fixed py, GWT_Point a, GWT_Point c, GWT_Point b, gwt_fixed scale, int ox, int oy){
    gwt_fixed ax, ay, cx, cy, bx, by;
    gwt_fixed lastx, lasty, qx, qy;
    gwt_fixed abx, aby, cbx, cby;
    unsigned short step;
    int crosses;
    point_to_screen_fp(a, scale, ox, oy, &ax, &ay);
    point_to_screen_fp(c, scale, ox, oy, &cx, &cy);
    point_to_screen_fp(b, scale, ox, oy, &bx, &by);
    lastx = ax;
    lasty = ay;
    crosses = 0;
    for(step=1; step<=8; step++){
        gwt_fixed t = (gwt_fixed)(((unsigned long)step << GWT_FP_SHIFT) / 8UL);
        abx = fp_lerp(ax, cx, t);
        aby = fp_lerp(ay, cy, t);
        cbx = fp_lerp(cx, bx, t);
        cby = fp_lerp(cy, by, t);
        qx = fp_lerp(abx, cbx, t);
        qy = fp_lerp(aby, cby, t);
        crosses ^= edge_cross_fp(px, py, lastx, lasty, qx, qy);
        lastx = qx;
        lasty = qy;
    }
    return crosses;
}

static int point_in_outline_quad_fp(gwt_fixed px, gwt_fixed py, const GWT_Outline *o, gwt_fixed scale, int ox, int oy){
    int inside;
    unsigned short ci;
    inside = 0;
    for(ci=0; ci<o->contour_count; ci++){
        GWT_Contour c;
        GWT_Point first, cur, next, after;
        unsigned short n, k;
        c = o->contours[ci];
        n = c.point_count;
        if(n < 2) continue;
        first = o->points[c.first_point];
        if(!(first.flags & GWT_POINT_ON_CURVE)){
            GWT_Point last = o->points[c.first_point + n - 1];
            if(last.flags & GWT_POINT_ON_CURVE) first = last;
            else first = midp(last, first);
        }
        cur = first;
        k = 0;
        while(k < n){
            next = o->points[c.first_point + k];
            if(k == 0 && !(o->points[c.first_point].flags & GWT_POINT_ON_CURVE)){
                k++;
                continue;
            }
            if(next.flags & GWT_POINT_ON_CURVE){
                inside ^= line_cross_scaled(px, py, cur, next, scale, ox, oy);
                cur = next;
                k++;
            } else {
                int consume_two;
                if(k + 1U < n){
                    after = o->points[c.first_point + k + 1U];
                    if(after.flags & GWT_POINT_ON_CURVE) consume_two = 1;
                    else { after = midp(next, after); consume_two = 0; }
                } else {
                    after = first;
                    consume_two = 0;
                }
                inside ^= quad_cross_scaled(px, py, cur, next, after, scale, ox, oy);
                cur = after;
                if(consume_two) k += 2U;
                else k += 1U;
            }
        }
        inside ^= line_cross_scaled(px, py, cur, first, scale, ox, oy);
    }
    return inside;
}

int gwt_rasterize_outline_aa(const GWT_Outline *outline, gwt_fixed scale, GWT_Bitmap *bmp, int origin_x_px, int origin_y_px, unsigned char fill_value, unsigned char samples_log2){
    unsigned short x, y, sx, sy;
    unsigned short samples_axis;
    unsigned short sample_count;
    if(!outline || !bmp || !bmp->pixels) return GWT_ERR_BAD_ARG;
    if(samples_log2 > 2) samples_log2 = 2; /* 0=1x, 1=2x2, 2=4x4 */
    samples_axis = (unsigned short)(1U << samples_log2);
    sample_count = (unsigned short)(samples_axis * samples_axis);
    for(y=0; y<bmp->height; y++){
        for(x=0; x<bmp->width; x++){
            unsigned short hits = 0;
            unsigned char value;
            for(sy=0; sy<samples_axis; sy++){
                for(sx=0; sx<samples_axis; sx++){
                    gwt_fixed px;
                    gwt_fixed py;
                    px = (((gwt_fixed)x) << GWT_FP_SHIFT) + (gwt_fixed)((((unsigned long)sx * 2UL + 1UL) << GWT_FP_SHIFT) / ((unsigned long)samples_axis * 2UL));
                    py = (((gwt_fixed)y) << GWT_FP_SHIFT) + (gwt_fixed)((((unsigned long)sy * 2UL + 1UL) << GWT_FP_SHIFT) / ((unsigned long)samples_axis * 2UL));
                    if(point_in_outline_quad_fp(px, py, outline, scale, origin_x_px, origin_y_px)) hits++;
                }
            }
            value = (unsigned char)(((unsigned int)fill_value * (unsigned int)hits) / (unsigned int)sample_count);
            if(value > bmp->pixels[(unsigned long)y * bmp->stride + x]){
                bmp->pixels[(unsigned long)y * bmp->stride + x] = value;
            }
        }
    }
    return GWT_OK;
}

int gwt_atlas_add_codepoint_aa(const GWT_Font *font, GWT_Atlas *a, unsigned long cp, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch){
    unsigned short gid;
    GWT_GlyphMetrics m;
    gwt_fixed sc;
    int r, w, h, base, ox, oy;
    GWT_Bitmap sub;
    GWT_AtlasGlyph *ag;
    if(!font || !a || !scratch) return GWT_ERR_BAD_ARG;
    if(gwt_atlas_find(a, cp)) return GWT_OK;
    if(a->glyph_count >= a->max_glyphs) return GWT_ERR_BUFFER_FULL;
    r = gwt_codepoint_to_glyph(font, cp, &gid);
    if(r != GWT_OK) return r;
    r = gwt_get_glyph_metrics(font, gid, &m);
    if(r != GWT_OK) return r;
    r = gwt_decode_glyph_outline(font, gid, scratch);
    if(r != GWT_OK) return r;
    sc = gwt_make_scale(font, pixel_size);
    w = fp_to_int_round(gwt_units_to_pixels(sc, m.x_max - m.x_min)) + 4;
    h = fp_to_int_round(gwt_units_to_pixels(sc, m.y_max - m.y_min)) + 4;
    if(w < 1) w = 1;
    if(h < 1) h = 1;
    if(a->pen_x + w + 1 > a->width){
        a->pen_x = 1;
        a->pen_y = (unsigned short)(a->pen_y + a->row_h + 1U);
        a->row_h = 0;
    }
    if(a->pen_y + h + 1 > a->height) return GWT_ERR_BUFFER_FULL;
    if(h > a->row_h) a->row_h = (unsigned short)h;
    sub.pixels = a->pixels + (unsigned long)a->pen_y * a->stride + a->pen_x;
    sub.width = (unsigned short)w;
    sub.height = (unsigned short)h;
    sub.stride = a->stride;
    base = fp_to_int_round(gwt_units_to_pixels(sc, m.y_max)) + 2;
    ox = 2 - fp_to_int_round(gwt_units_to_pixels(sc, m.x_min));
    oy = base;
    gwt_rasterize_outline_aa(scratch, sc, &sub, ox, oy, 255, samples_log2);
    ag = &a->glyphs[a->glyph_count++];
    ag->codepoint = cp;
    ag->glyph_id = gid;
    ag->x = a->pen_x;
    ag->y = a->pen_y;
    ag->w = (unsigned short)w;
    ag->h = (unsigned short)h;
    ag->advance_px = fp_to_int_round(gwt_units_to_pixels(sc, m.advance_width));
    ag->bearing_x_px = fp_to_int_round(gwt_units_to_pixels(sc, m.left_side_bearing));
    ag->bearing_y_px = base;
    a->pen_x = (unsigned short)(a->pen_x + w + 1);
    return GWT_OK;
}

int gwt_atlas_add_range(const GWT_Font *font, GWT_Atlas *atlas, unsigned long first_codepoint, unsigned long last_codepoint, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline){
    unsigned long cp;
    int r;
    if(!font || !atlas || !scratch_outline) return GWT_ERR_BAD_ARG;
    if(last_codepoint < first_codepoint) return GWT_ERR_BAD_ARG;
    for(cp=first_codepoint; cp<=last_codepoint; cp++){
        r = gwt_atlas_add_codepoint_aa(font, atlas, cp, pixel_size, samples_log2, scratch_outline);
        if(r != GWT_OK) return r;
        if(cp == 0xFFFFFFFFUL) break;
    }
    return GWT_OK;
}

int gwt_atlas_add_ascii(const GWT_Font *font, GWT_Atlas *atlas, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline){
    return gwt_atlas_add_range(font, atlas, 32UL, 126UL, pixel_size, samples_log2, scratch_outline);
}

int gwt_get_name_ascii(const GWT_Font *font, unsigned short name_id, char *out, unsigned short max_out){
    const unsigned char *p;
    unsigned short count, string_off, i;
    if(!font || !out || max_out == 0) return GWT_ERR_BAD_ARG;
    out[0] = 0;
    if(!font->table_name.tag) return GWT_ERR_TABLE_MISSING;
    p = ptr(font, font->table_name.offset, 6);
    if(!p) return GWT_ERR_BAD_FONT;
    count = rd_u16(p + 2);
    string_off = rd_u16(p + 4);
    if(!ptr(font, font->table_name.offset + 6UL, (unsigned long)count * 12UL)) return GWT_ERR_BAD_FONT;
    for(i=0; i<count; i++){
        const unsigned char *r;
        unsigned short platform, enc, lang, nid, len, off;
        unsigned long abs;
        r = font->data + font->table_name.offset + 6UL + (unsigned long)i * 12UL;
        platform = rd_u16(r);
        enc = rd_u16(r + 2);
        lang = rd_u16(r + 4);
        nid = rd_u16(r + 6);
        len = rd_u16(r + 8);
        off = rd_u16(r + 10);
        if(nid != name_id) continue;
        abs = font->table_name.offset + (unsigned long)string_off + (unsigned long)off;
        p = ptr(font, abs, len);
        if(!p) return GWT_ERR_BAD_FONT;
        if(platform == 3 || platform == 0){
            unsigned short j, oi;
            oi = 0;
            for(j=0; j+1U<len && oi+1U<max_out; j+=2U){
                unsigned short ch = rd_u16(p + j);
                out[oi++] = (char)((ch >= 32U && ch < 127U) ? ch : '?');
            }
            out[oi] = 0;
            (void)enc; (void)lang;
            return GWT_OK;
        } else if(platform == 1){
            unsigned short j, oi;
            oi = 0;
            for(j=0; j<len && oi+1U<max_out; j++){
                unsigned char ch = p[j];
                out[oi++] = (char)((ch >= 32U && ch < 127U) ? ch : '?');
            }
            out[oi] = 0;
            (void)enc; (void)lang;
            return GWT_OK;
        }
        (void)enc; (void)lang;
    }
    return GWT_ERR_RANGE;
}

int gwt_measure_utf8(const GWT_Font *font, const char *text, int pixel_size, int *out_width, int *out_height){
    const char *p;
    unsigned long cp;
    unsigned short gid, prev;
    gwt_fixed sc;
    int pen, line_w, max_w, lines, kern;
    GWT_GlyphMetrics m;
    if(!font || !text || !out_width || !out_height) return GWT_ERR_BAD_ARG;
    p = text;
    sc = gwt_make_scale(font, pixel_size);
    pen = 0;
    max_w = 0;
    lines = 1;
    prev = 0;
    while((cp = gwt_utf8_next(&p)) != 0){
        if(cp == '\n'){
            if(pen > max_w) max_w = pen;
            pen = 0;
            prev = 0;
            lines++;
            continue;
        }
        if(gwt_codepoint_to_glyph(font, cp, &gid) != GWT_OK) gid = 0;
        kern = 0;
        if(prev) gwt_get_kerning(font, prev, gid, &kern);
        pen += fp_to_int_round(gwt_units_to_pixels(sc, kern));
        if(gwt_get_glyph_metrics(font, gid, &m) == GWT_OK){
            pen += fp_to_int_round(gwt_units_to_pixels(sc, m.advance_width));
        }
        prev = gid;
    }
    if(pen > max_w) max_w = pen;
    line_w = max_w;
    *out_width = line_w;
    *out_height = lines * pixel_size;
    return GWT_OK;
}

int gwt_layout_box_utf8(const GWT_Font *font, const char *text, int pixel_size, int x, int y, int max_width_px, int line_height_px, unsigned short flags, GWT_LayoutGlyph *out, unsigned short max_out, unsigned short *out_count){
    const char *p;
    unsigned long cp;
    unsigned short count, gid, prev;
    gwt_fixed sc;
    int pen, kern;
    GWT_GlyphMetrics m;
    if(!font || !text || !out || !out_count) return GWT_ERR_BAD_ARG;
    if(line_height_px <= 0) line_height_px = pixel_size;
    sc = gwt_make_scale(font, pixel_size);
    p = text;
    count = 0;
    pen = x;
    prev = 0;
    while((cp = gwt_utf8_next(&p)) != 0){
        if(cp == '\n'){
            pen = x;
            y += line_height_px;
            prev = 0;
            continue;
        }
        if(gwt_codepoint_to_glyph(font, cp, &gid) != GWT_OK){
            if(flags & GWT_LAYOUT_SKIP_MISSING) continue;
            gid = 0;
        }
        if(gwt_get_glyph_metrics(font, gid, &m) != GWT_OK){
            if(flags & GWT_LAYOUT_SKIP_MISSING) continue;
            m.advance_width = font->units_per_em / 2;
        }
        kern = 0;
        if(prev) gwt_get_kerning(font, prev, gid, &kern);
        pen += fp_to_int_round(gwt_units_to_pixels(sc, kern));
        if(max_width_px > 0 && pen != x){
            int adv = fp_to_int_round(gwt_units_to_pixels(sc, m.advance_width));
            if((pen - x + adv) > max_width_px){
                pen = x;
                y += line_height_px;
                prev = 0;
            }
        }
        if(count >= max_out){
            *out_count = count;
            return GWT_ERR_BUFFER_FULL;
        }
        out[count].codepoint = cp;
        out[count].glyph_id = gid;
        out[count].x = pen;
        out[count].y = y;
        out[count].advance_px = fp_to_int_round(gwt_units_to_pixels(sc, m.advance_width));
        pen += out[count].advance_px;
        prev = gid;
        count++;
    }
    *out_count = count;
    (void)flags;
    return GWT_OK;
}

/* ---- v0.4.0 engine integration layer ---- */
void gwt_atlas_reset(GWT_Atlas *atlas, unsigned char clear_value){
    unsigned long i, n;
    if(!atlas) return;
    atlas->glyph_count = 0;
    atlas->pen_x = 1;
    atlas->pen_y = 1;
    atlas->row_h = 0;
    if(atlas->pixels){
        n = (unsigned long)atlas->height * atlas->stride;
        for(i=0; i<n; i++) atlas->pixels[i] = clear_value;
    }
}

int gwt_atlas_add_utf8(const GWT_Font *font, GWT_Atlas *atlas, const char *text, int pixel_size, unsigned char samples_log2, GWT_Outline *scratch_outline){
    const char *p;
    unsigned long cp;
    int r;
    if(!font || !atlas || !text || !scratch_outline) return GWT_ERR_BAD_ARG;
    p = text;
    while((cp = gwt_utf8_next(&p)) != 0){
        if(cp == '\n' || cp == '\r' || cp == '\t') continue;
        r = gwt_atlas_add_codepoint_aa(font, atlas, cp, pixel_size, samples_log2, scratch_outline);
        if(r != GWT_OK) return r;
    }
    return GWT_OK;
}

int gwt_build_text_quads_utf8(const GWT_Atlas *atlas, const GWT_LayoutGlyph *layout, unsigned short layout_count, GWT_TextQuad *out, unsigned short max_out, unsigned short *out_count){
    unsigned short i, count;
    const GWT_AtlasGlyph *ag;
    if(!atlas || !layout || !out || !out_count) return GWT_ERR_BAD_ARG;
    count = 0;
    for(i=0; i<layout_count; i++){
        ag = gwt_atlas_find(atlas, layout[i].codepoint);
        if(!ag) continue;
        if(count >= max_out){
            *out_count = count;
            return GWT_ERR_BUFFER_FULL;
        }
        out[count].codepoint = layout[i].codepoint;
        out[count].glyph_id = layout[i].glyph_id;
        out[count].dst_x = layout[i].x + ag->bearing_x_px;
        out[count].dst_y = layout[i].y - ag->bearing_y_px;
        out[count].src_x = ag->x;
        out[count].src_y = ag->y;
        out[count].w = ag->w;
        out[count].h = ag->h;
        out[count].advance_px = layout[i].advance_px;
        count++;
    }
    *out_count = count;
    return GWT_OK;
}

int gwt_build_text_quads_box_utf8(const GWT_Font *font, const GWT_Atlas *atlas, const char *text, int pixel_size, int x, int y, int max_width_px, int line_height_px, unsigned short flags, GWT_LayoutGlyph *layout_scratch, unsigned short max_layout, GWT_TextQuad *out, unsigned short max_out, unsigned short *out_count){
    unsigned short layout_count;
    int r;
    if(!font || !atlas || !text || !layout_scratch || !out || !out_count) return GWT_ERR_BAD_ARG;
    r = gwt_layout_box_utf8(font, text, pixel_size, x, y, max_width_px, line_height_px, flags, layout_scratch, max_layout, &layout_count);
    if(r != GWT_OK) return r;
    return gwt_build_text_quads_utf8(atlas, layout_scratch, layout_count, out, max_out, out_count);
}

int gwt_get_text_metrics(const GWT_Font *font, const char *text, int pixel_size, GWT_TextMetrics *out){
    int w, h;
    const char *p;
    unsigned long cp;
    int lines;
    gwt_fixed sc;
    if(!font || !text || !out) return GWT_ERR_BAD_ARG;
    if(gwt_measure_utf8(font, text, pixel_size, &w, &h) != GWT_OK) return GWT_ERR_BAD_FONT;
    lines = 1;
    p = text;
    while((cp = gwt_utf8_next(&p)) != 0){
        if(cp == '\n') lines++;
    }
    sc = gwt_make_scale(font, pixel_size);
    out->width = w;
    out->height = h;
    out->line_count = lines;
    out->ascent_px = fp_to_int_round(gwt_units_to_pixels(sc, font->ascent));
    out->descent_px = fp_to_int_round(gwt_units_to_pixels(sc, font->descent));
    out->line_gap_px = fp_to_int_round(gwt_units_to_pixels(sc, font->line_gap));
    return GWT_OK;
}

int gwt_bitmap_blit_alpha(GWT_Bitmap *dst, int dst_x, int dst_y, const unsigned char *src, unsigned short src_w, unsigned short src_h, unsigned short src_stride, unsigned char color){
    unsigned short x, y;
    if(!dst || !dst->pixels || !src) return GWT_ERR_BAD_ARG;
    for(y=0; y<src_h; y++){
        int dy = dst_y + (int)y;
        if(dy < 0 || dy >= (int)dst->height) continue;
        for(x=0; x<src_w; x++){
            int dx = dst_x + (int)x;
            unsigned char a;
            unsigned long di;
            if(dx < 0 || dx >= (int)dst->width) continue;
            a = src[(unsigned long)y * src_stride + x];
            if(a == 0) continue;
            di = (unsigned long)dy * dst->stride + (unsigned short)dx;
            /* alpha-over into an 8-bit mask/value buffer, no RGB ownership here */
            dst->pixels[di] = (unsigned char)(((unsigned int)dst->pixels[di] * (255U - (unsigned int)a) + (unsigned int)color * (unsigned int)a) / 255U);
        }
    }
    return GWT_OK;
}

int gwt_draw_text_bitmap_utf8(const GWT_Font *font, const GWT_Atlas *atlas, GWT_Bitmap *dst, const char *text, int pixel_size, int x, int y, unsigned char color, GWT_LayoutGlyph *layout_scratch, unsigned short max_layout){
    unsigned short count, i;
    int r;
    if(!font || !atlas || !dst || !text || !layout_scratch) return GWT_ERR_BAD_ARG;
    r = gwt_layout_utf8(font, text, pixel_size, x, y, layout_scratch, max_layout, &count);
    if(r != GWT_OK) return r;
    for(i=0; i<count; i++){
        const GWT_AtlasGlyph *ag;
        const unsigned char *src;
        int dx, dy;
        ag = gwt_atlas_find(atlas, layout_scratch[i].codepoint);
        if(!ag) continue;
        src = atlas->pixels + (unsigned long)ag->y * atlas->stride + ag->x;
        dx = layout_scratch[i].x + ag->bearing_x_px;
        dy = layout_scratch[i].y - ag->bearing_y_px;
        r = gwt_bitmap_blit_alpha(dst, dx, dy, src, ag->w, ag->h, atlas->stride, color);
        if(r != GWT_OK) return r;
    }
    return GWT_OK;
}

#include "otf_c89.h"

/* No malloc, no free, no heap. All work happens on caller-owned structs. */

static int r8(const otf_buf *b, unsigned long o, unsigned char *v){ if(!b||!v||o>=b->size)return OTF_ERR_RANGE; *v=b->data[o]; return OTF_OK; }
static int u16(const otf_buf *b, unsigned long o, unsigned short *v){ if(!b||!v||o+1UL>=b->size)return OTF_ERR_RANGE; *v=(unsigned short)(((unsigned short)b->data[o]<<8)|b->data[o+1]); return OTF_OK; }
static int s16(const otf_buf *b, unsigned long o, short *v){ unsigned short x; int e=u16(b,o,&x); if(e)return e; *v=(short)x; return OTF_OK; }
static int u32(const otf_buf *b, unsigned long o, unsigned long *v){ if(!b||!v||o+3UL>=b->size)return OTF_ERR_RANGE; *v=((unsigned long)b->data[o]<<24)|((unsigned long)b->data[o+1]<<16)|((unsigned long)b->data[o+2]<<8)|b->data[o+3]; return OTF_OK; }
static int check_range(const otf_buf *b, unsigned long o, unsigned long n){ if(!b)return OTF_ERR_BAD_ARG; if(o>b->size)return OTF_ERR_RANGE; if(n>b->size-o)return OTF_ERR_RANGE; return OTF_OK; }
static void zmem(void *p, unsigned long n){ unsigned char *q=(unsigned char*)p; while(n--)*q++=0; }
static long clamp_long(long v,long a,long b){ if(v<a)return a; if(v>b)return b; return v; }
static long f2dot14_to_fixed(short v){ return ((long)v) << 2; }
static long fixed_mul(long a,long b){ long r; int neg=0; if(a<0){a=-a;neg=!neg;} if(b<0){b=-b;neg=!neg;} r=(a>>8)*(b>>8); return neg?-r:r; }
static long fixed_div(long a,long b){ long q; int neg=0; if(b==0)return 0; if(a<0){a=-a;neg=!neg;} if(b<0){b=-b;neg=!neg;} q=(a/b)<<16; a=a%b; q += (a<<16)/b; return neg?-q:q; }
static long fixed_lerp(long x0,long y0,long x1,long y1,long x){ long den=x1-x0; if(den==0)return y0; return y0 + fixed_mul(fixed_div(x-x0,den), y1-y0); }

const char *otf_errstr(int code){
    switch(code){
    case OTF_OK:return "OK"; case OTF_ERR_BAD_ARG:return "bad argument"; case OTF_ERR_RANGE:return "range error";
    case OTF_ERR_BAD_MAGIC:return "bad magic"; case OTF_ERR_UNSUPPORTED:return "unsupported"; case OTF_ERR_OVERFLOW:return "static limit overflow";
    case OTF_ERR_MISSING_TABLE:return "missing table"; case OTF_ERR_BAD_CFF:return "bad CFF"; case OTF_ERR_BAD_CHARSTRING:return "bad charstring";
    default:return "unknown";
    }
}

const otf_table *otf_find_table(const otf_font *font, unsigned long tag){
    unsigned short i; if(!font)return 0; for(i=0;i<font->num_tables;i++) if(font->tables[i].tag==tag)return &font->tables[i]; return 0;
}
int otf_has_table(const otf_font *font, unsigned long tag){ return otf_find_table(font,tag)!=0; }
int otf_is_otf_cff(const otf_font *font){ return font && font->sfnt_version==OTF_TAG('O','T','T','O') && font->cff.present && !font->cff.is_cff2; }
int otf_is_otf_cff2(const otf_font *font){ return font && font->sfnt_version==OTF_TAG('O','T','T','O') && font->cff.present && font->cff.is_cff2; }

static int parse_head(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('h','e','a','d')); int e;
    if(!t)return OTF_ERR_MISSING_TABLE; if((e=check_range(&f->buf,t->offset,t->length))!=0)return e;
    if((e=u16(&f->buf,t->offset+18,&f->units_per_em))!=0)return e;
    if((e=s16(&f->buf,t->offset+50,&f->index_to_loc_format))!=0)return e;
    if((e=s16(&f->buf,t->offset+52,&f->glyph_data_format))!=0)return e;
    return OTF_OK;
}
static int parse_maxp(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('m','a','x','p')); int e;
    if(!t)return OTF_ERR_MISSING_TABLE; if((e=check_range(&f->buf,t->offset,t->length))!=0)return e;
    return u16(&f->buf,t->offset+4,&f->num_glyphs);
}
static int parse_hhea(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('h','h','e','a')); int e;
    if(!t)return OTF_ERR_MISSING_TABLE; if((e=check_range(&f->buf,t->offset,t->length))!=0)return e;
    if((e=s16(&f->buf,t->offset+4,&f->ascender))!=0)return e;
    if((e=s16(&f->buf,t->offset+6,&f->descender))!=0)return e;
    if((e=s16(&f->buf,t->offset+8,&f->line_gap))!=0)return e;
    return u16(&f->buf,t->offset+34,&f->number_of_hmetrics);
}

static int parse_vhea(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('v','h','e','a')); int e;
    if(!t){ f->number_of_vmetrics=0; return OTF_OK; }
    if((e=s16(&f->buf,t->offset+4,&f->v_ascender))!=0)return e;
    if((e=s16(&f->buf,t->offset+6,&f->v_descender))!=0)return e;
    if((e=s16(&f->buf,t->offset+8,&f->v_line_gap))!=0)return e;
    return u16(&f->buf,t->offset+34,&f->number_of_vmetrics);
}
static int parse_os2(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('O','S','/','2')); int e;
    if(!t)return OTF_OK; if((e=check_range(&f->buf,t->offset,t->length))!=0)return e;
    if((e=u16(&f->buf,t->offset+0,&f->os2_version))!=0)return e;
    if((e=u16(&f->buf,t->offset+4,&f->weight_class))!=0)return e;
    if((e=u16(&f->buf,t->offset+6,&f->width_class))!=0)return e;
    if(t->length>=72){
        if((e=s16(&f->buf,t->offset+68,&f->os2_typo_ascender))!=0)return e;
        if((e=s16(&f->buf,t->offset+70,&f->os2_typo_descender))!=0)return e;
        if((e=s16(&f->buf,t->offset+72,&f->os2_typo_line_gap))!=0)return e;
    }
    return OTF_OK;
}
static int parse_name(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('n','a','m','e')); unsigned short count, storage, i; int e;
    if(!t)return OTF_OK; if((e=check_range(&f->buf,t->offset,t->length))!=0)return e;
    if((e=u16(&f->buf,t->offset+2,&count))!=0)return e; if((e=u16(&f->buf,t->offset+4,&storage))!=0)return e;
    f->name_count = count>OTF_MAX_NAME_RECORDS ? OTF_MAX_NAME_RECORDS : count;
    f->name_storage_offset=t->offset+storage;
    for(i=0;i<f->name_count;i++){
        unsigned long ro=t->offset+6UL+(unsigned long)i*12UL;
        if((e=u16(&f->buf,ro+0,&f->names[i].platform_id))!=0)return e;
        if((e=u16(&f->buf,ro+2,&f->names[i].encoding_id))!=0)return e;
        if((e=u16(&f->buf,ro+4,&f->names[i].language_id))!=0)return e;
        if((e=u16(&f->buf,ro+6,&f->names[i].name_id))!=0)return e;
        if((e=u16(&f->buf,ro+8,&f->names[i].length))!=0)return e;
        if((e=u16(&f->buf,ro+10,&f->names[i].offset))!=0)return e;
    }
    return OTF_OK;
}

int otf_get_name_ascii(const otf_font *f, unsigned short name_id, char *out, unsigned short cap){
    unsigned short i, j, best=0xffff; unsigned long o; if(!f||!out||cap==0)return OTF_ERR_BAD_ARG; out[0]=0;
    for(i=0;i<f->name_count;i++) if(f->names[i].name_id==name_id){ best=i; if(f->names[i].platform_id==3)break; }
    if(best==0xffff)return OTF_ERR_MISSING_TABLE;
    o=f->name_storage_offset+f->names[best].offset;
    if(check_range(&f->buf,o,f->names[best].length))return OTF_ERR_RANGE;
    j=0;
    if(f->names[best].platform_id==3){
        unsigned short k; for(k=0;k+1<f->names[best].length && j+1<cap;k+=2){ unsigned char lo=f->buf.data[o+k+1]; out[j++]=(lo>=32&&lo<127)?(char)lo:'?'; }
    } else {
        unsigned short k; for(k=0;k<f->names[best].length && j+1<cap;k++){ unsigned char c=f->buf.data[o+k]; out[j++]=(c>=32&&c<127)?(char)c:'?'; }
    }
    out[j]=0; return OTF_OK;
}

static int parse_cmap4(otf_font *f, unsigned long off){
    unsigned short segX2, segCount, i; int e; unsigned long p;
    if((e=u16(&f->buf,off+6,&segX2))!=0)return e; segCount=(unsigned short)(segX2/2);
    if(segCount>OTF_MAX_CMAP4_SEGS)return OTF_ERR_OVERFLOW;
    f->cmap4.present=1; f->cmap4.seg_count=segCount; f->cmap4.table_offset=off;
    p=off+14;
    for(i=0;i<segCount;i++) if((e=u16(&f->buf,p+2UL*i,&f->cmap4.end_code[i]))!=0)return e;
    p += 2UL*segCount + 2UL;
    for(i=0;i<segCount;i++) if((e=u16(&f->buf,p+2UL*i,&f->cmap4.start_code[i]))!=0)return e;
    p += 2UL*segCount;
    for(i=0;i<segCount;i++) if((e=s16(&f->buf,p+2UL*i,&f->cmap4.id_delta[i]))!=0)return e;
    p += 2UL*segCount;
    for(i=0;i<segCount;i++){ f->cmap4.id_range_offset_pos[i]=p+2UL*i; if((e=u16(&f->buf,p+2UL*i,&f->cmap4.id_range_offset[i]))!=0)return e; }
    return OTF_OK;
}
static int parse_cmap(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('c','m','a','p')); unsigned short count,i; int e;
    if(!t)return OTF_ERR_MISSING_TABLE; if((e=check_range(&f->buf,t->offset,t->length))!=0)return e;
    if((e=u16(&f->buf,t->offset+2,&count))!=0)return e;
    f->cmap_count=count>OTF_MAX_CMAP_SUBTABLES?OTF_MAX_CMAP_SUBTABLES:count;
    for(i=0;i<f->cmap_count;i++){
        unsigned long rec=t->offset+4UL+(unsigned long)i*8UL, rel=0; unsigned short fmt=0;
        if((e=u16(&f->buf,rec,&f->cmaps[i].platform_id))!=0)return e;
        if((e=u16(&f->buf,rec+2,&f->cmaps[i].encoding_id))!=0)return e;
        if((e=u32(&f->buf,rec+4,&rel))!=0)return e;
        f->cmaps[i].offset=t->offset+rel;
        if((e=u16(&f->buf,f->cmaps[i].offset,&fmt))!=0)return e;
        f->cmaps[i].format=fmt;
    }
    /* Prefer Unicode full repertoire format 12; then BMP format 4. */
    for(i=0;i<f->cmap_count;i++) if(f->cmaps[i].format==12){ unsigned long ng; if(u32(&f->buf,f->cmaps[i].offset+12,&ng)==0){ f->cmap12_offset=f->cmaps[i].offset; f->cmap12_groups=ng; } }
    for(i=0;i<f->cmap_count;i++) if(f->cmaps[i].format==4) return parse_cmap4(f,f->cmaps[i].offset);
    return OTF_OK;
}

unsigned short otf_glyph_index_for_codepoint(const otf_font *f, unsigned long cp){
    if(!f)return 0;
    if(f->cmap12_offset){
        unsigned long lo=0, hi=f->cmap12_groups;
        while(lo<hi){
            unsigned long mid=(lo+hi)/2, p=f->cmap12_offset+16UL+mid*12UL, a,b,g;
            if(u32(&f->buf,p,&a)||u32(&f->buf,p+4,&b)||u32(&f->buf,p+8,&g))break;
            if(cp<a)hi=mid; else if(cp>b)lo=mid+1; else return (unsigned short)(g+(cp-a));
        }
    }
    if(cp<=0xffffUL && f->cmap4.present){
        unsigned short c=(unsigned short)cp, i;
        for(i=0;i<f->cmap4.seg_count;i++) if(c>=f->cmap4.start_code[i] && c<=f->cmap4.end_code[i]){
            if(f->cmap4.id_range_offset[i]==0) return (unsigned short)(c + f->cmap4.id_delta[i]);
            else {
                unsigned long glyph_pos=f->cmap4.id_range_offset_pos[i]+f->cmap4.id_range_offset[i]+2UL*(c-f->cmap4.start_code[i]);
                unsigned short glyph=0; if(u16(&f->buf,glyph_pos,&glyph))return 0; if(glyph)glyph=(unsigned short)(glyph+f->cmap4.id_delta[i]); return glyph;
            }
        }
    }
    return 0;
}

int otf_get_hmetric(const otf_font *f, unsigned short gid, unsigned short *adv, short *lsb){
    const otf_table *t; int e; unsigned long o; if(!f||!adv||!lsb)return OTF_ERR_BAD_ARG; t=otf_find_table(f,OTF_TAG('h','m','t','x')); if(!t)return OTF_ERR_MISSING_TABLE;
    if(gid < f->number_of_hmetrics){ o=t->offset+4UL*gid; if((e=u16(&f->buf,o,adv))!=0)return e; if((e=s16(&f->buf,o+2,lsb))!=0)return e; }
    else { o=t->offset+4UL*(f->number_of_hmetrics-1UL); if((e=u16(&f->buf,o,adv))!=0)return e; o=t->offset+4UL*f->number_of_hmetrics+2UL*(gid-f->number_of_hmetrics); if((e=s16(&f->buf,o,lsb))!=0)return e; }
    return OTF_OK;
}

int otf_get_vmetric(const otf_font *f, unsigned short gid, unsigned short *advh, short *tsb){
    const otf_table *t; int e; unsigned long o; if(!f||!advh||!tsb)return OTF_ERR_BAD_ARG; t=otf_find_table(f,OTF_TAG('v','m','t','x')); if(!t)return OTF_ERR_MISSING_TABLE;
    if(f->number_of_vmetrics==0)return OTF_ERR_MISSING_TABLE;
    if(gid < f->number_of_vmetrics){ o=t->offset+4UL*gid; if((e=u16(&f->buf,o,advh))!=0)return e; if((e=s16(&f->buf,o+2,tsb))!=0)return e; }
    else { o=t->offset+4UL*(f->number_of_vmetrics-1UL); if((e=u16(&f->buf,o,advh))!=0)return e; o=t->offset+4UL*f->number_of_vmetrics+2UL*(gid-f->number_of_vmetrics); if((e=s16(&f->buf,o,tsb))!=0)return e; }
    return OTF_OK;
}

static int cff_index_parse(const otf_buf *b, unsigned long base, unsigned long off, otf_cff_index *idx){
    int e; unsigned long abs=base+off, last; unsigned short count; unsigned char os;
    zmem(idx,sizeof(*idx)); idx->offset=off;
    if((e=u16(b,abs,&count))!=0)return e; idx->count=count;
    if(count==0){ idx->end=off+2; return OTF_OK; }
    if((e=r8(b,abs+2,&os))!=0)return e; if(os<1||os>4)return OTF_ERR_BAD_CFF; idx->off_size=os;
    idx->offsets_base=off+3; idx->data_base=idx->offsets_base+(unsigned long)(count+1)*os;
    /* read last offset to compute end */
    { unsigned long p=base+idx->offsets_base+(unsigned long)count*os, val=0; unsigned char k; for(k=0;k<os;k++){ unsigned char x; if((e=r8(b,p+k,&x))!=0)return e; val=(val<<8)|x; } last=val; }
    if(last==0)return OTF_ERR_BAD_CFF; idx->end=idx->data_base+last-1; return OTF_OK;
}

static int cff2_index_parse(const otf_buf *b, unsigned long base, unsigned long off, otf_cff_index *idx){
    int e; unsigned long abs=base+off, count32, last; unsigned char os;
    zmem(idx,sizeof(*idx)); idx->offset=off;
    if((e=u32(b,abs,&count32))!=0)return e;
    if(count32>65535UL)return OTF_ERR_OVERFLOW;
    idx->count=(unsigned short)count32;
    if(count32==0){ idx->end=off+4; return OTF_OK; }
    if((e=r8(b,abs+4,&os))!=0)return e; if(os<1||os>4)return OTF_ERR_BAD_CFF; idx->off_size=os;
    idx->offsets_base=off+5; idx->data_base=idx->offsets_base+(unsigned long)(count32+1UL)*os;
    { unsigned long q=base+idx->offsets_base+count32*os, val=0; unsigned char k; for(k=0;k<os;k++){ unsigned char x; if((e=r8(b,q+k,&x))!=0)return e; val=(val<<8)|x; } last=val; }
    if(last==0)return OTF_ERR_BAD_CFF; idx->end=idx->data_base+last-1; return OTF_OK;
}

static int cff_index_offset(const otf_buf *b, unsigned long base, const otf_cff_index *idx, unsigned short n, unsigned long *start, unsigned long *len){
    unsigned long a=0,c=0,p; unsigned char k; int e; if(!idx||n>=idx->count)return OTF_ERR_RANGE;
    p=base+idx->offsets_base+(unsigned long)n*idx->off_size; for(k=0;k<idx->off_size;k++){ unsigned char x; if((e=r8(b,p+k,&x))!=0)return e; a=(a<<8)|x; }
    p=base+idx->offsets_base+(unsigned long)(n+1)*idx->off_size; for(k=0;k<idx->off_size;k++){ unsigned char x; if((e=r8(b,p+k,&x))!=0)return e; c=(c<<8)|x; }
    if(a==0||c<a)return OTF_ERR_BAD_CFF; *start=base+idx->data_base+a-1; *len=c-a; return OTF_OK;
}

static int cff_parse_number(const otf_buf *b, unsigned long *p, unsigned long end, long *out){
    unsigned char v; int e; if(*p>=end)return OTF_ERR_RANGE; if((e=r8(b,*p,&v))!=0)return e; (*p)++;
    if(v>=32 && v<=246){ *out=(long)v-139L; return OTF_OK; }
    if(v>=247 && v<=250){ unsigned char w; if((e=r8(b,*p,&w))!=0)return e; (*p)++; *out=((long)v-247L)*256L+(long)w+108L; return OTF_OK; }
    if(v>=251 && v<=254){ unsigned char w; if((e=r8(b,*p,&w))!=0)return e; (*p)++; *out= -(((long)v-251L)*256L+(long)w+108L); return OTF_OK; }
    if(v==28){ short s; if((e=s16(b,*p,&s))!=0)return e; *p+=2; *out=s; return OTF_OK; }
    if(v==29){ unsigned long u; if((e=u32(b,*p,&u))!=0)return e; *p+=4; *out=(long)u; return OTF_OK; }
    if(v==30){ /* real number: unsupported without float; skip BCD, return 0 */
        unsigned char n; do { if((e=r8(b,*p,&n))!=0)return e; (*p)++; } while(((n&15)!=15) && ((n>>4)!=15) && *p<end); *out=0; return OTF_OK;
    }
    return OTF_ERR_BAD_CFF;
}

static int cff_parse_top_dict(const otf_buf *b, unsigned long start, unsigned long len, otf_cff_top *top){
    long st[OTF_MAX_CFF_STACK]; unsigned short sp=0; unsigned long p=start,end=start+len; int e;
    zmem(top,sizeof(*top)); top->charstring_type=2; top->charset_off=0; top->encoding_off=0;
    while(p<end){ unsigned char op; if((e=r8(b,p,&op))!=0)return e;
        if(op<=27 && op!=28){ p++; if(op==12){ unsigned char esc; if((e=r8(b,p,&esc))!=0)return e; p++; op=(unsigned char)(120+esc); }
            switch(op){
            case 15: if(sp>=1)top->charset_off=(unsigned long)st[sp-1]; break;
            case 16: if(sp>=1)top->encoding_off=(unsigned long)st[sp-1]; break;
            case 17: if(sp>=1)top->charstrings_off=(unsigned long)st[sp-1]; break;
            case 18: if(sp>=2){ top->private_size=(unsigned long)st[sp-2]; top->private_off=(unsigned long)st[sp-1]; } break;
            case 24: if(sp>=1)top->variation_store_off=(unsigned long)st[sp-1]; break;
            case 144: if(sp>=1)top->variation_store_off=(unsigned long)st[sp-1]; break; /* CFF2 12 24 vstore */
            case 123: if(sp>=3){ top->ros_registry_sid=(short)st[sp-3]; top->ros_ordering_sid=(short)st[sp-2]; top->ros_supplement=(short)st[sp-1]; top->is_cid=1; } break; /* 12 30 */
            case 156: if(sp>=1)top->fd_array_off=(unsigned long)st[sp-1]; break; /* 12 36 */
            case 157: if(sp>=1)top->fd_select_off=(unsigned long)st[sp-1]; break; /* 12 37 */
            default: break;
            } sp=0;
        } else {
            long num; if(sp>=OTF_MAX_CFF_STACK)return OTF_ERR_OVERFLOW; if((e=cff_parse_number(b,&p,end,&num))!=0)return e; st[sp++]=num;
        }
    }
    return top->charstrings_off?OTF_OK:OTF_ERR_BAD_CFF;
}
static int cff_parse_private_dict(const otf_buf *b, unsigned long start, unsigned long len, otf_cff_priv *pr){
    long st[OTF_MAX_CFF_STACK]; unsigned short sp=0; unsigned long p=start,end=start+len; int e;
    zmem(pr,sizeof(*pr)); pr->default_width_x=0; pr->nominal_width_x=0;
    while(p<end){ unsigned char op; if((e=r8(b,p,&op))!=0)return e;
        if(op<=27 && op!=28){ p++; if(op==12){ unsigned char esc; if((e=r8(b,p,&esc))!=0)return e; p++; op=(unsigned char)(120+esc); }
            switch(op){ case 19: if(sp>=1)pr->subrs_off=(unsigned long)st[sp-1]; break; case 20: if(sp>=1)pr->default_width_x=(short)st[sp-1]; break; case 21: if(sp>=1)pr->nominal_width_x=(short)st[sp-1]; break; case 22: if(sp>=1)pr->vsindex=(short)st[sp-1]; break; case 23: /* CFF2 Private blend: keep default values only. */ break; default: break; } sp=0;
        } else { long num; if(sp>=OTF_MAX_CFF_STACK)return OTF_ERR_OVERFLOW; if((e=cff_parse_number(b,&p,end,&num))!=0)return e; st[sp++]=num; }
    }
    return OTF_OK;
}

static int cff2_parse_fd_dict(const otf_buf *b, unsigned long start, unsigned long len, otf_cff_top *fd){
    long st[OTF_MAX_CFF_STACK]; unsigned short sp=0; unsigned long q=start,end=start+len; int e;
    zmem(fd,sizeof(*fd));
    while(q<end){ unsigned char op; if((e=r8(b,q,&op))!=0)return e;
        if(op<=27 && op!=28){ q++; if(op==12){ unsigned char esc; if((e=r8(b,q,&esc))!=0)return e; q++; op=(unsigned char)(120+esc); }
            switch(op){ case 18: if(sp>=2){ fd->private_size=(unsigned long)st[sp-2]; fd->private_off=(unsigned long)st[sp-1]; } break; default: break; }
            sp=0;
        } else { long num; if(sp>=OTF_MAX_CFF_STACK)return OTF_ERR_OVERFLOW; if((e=cff_parse_number(b,&q,end,&num))!=0)return e; st[sp++]=num; }
    }
    return OTF_OK;
}

static int cff2_fd_for_gid(const otf_font *f, unsigned short gid, unsigned short *fd){
    const otf_buf *b=&f->buf; unsigned long p=f->cff.table_offset+f->cff.top.fd_select_off; unsigned char fmt; int e;
    if(!fd)return OTF_ERR_BAD_ARG; *fd=0;
    if(f->cff.fd_count<=1 || f->cff.top.fd_select_off==0)return OTF_OK;
    if((e=r8(b,p,&fmt))!=0)return e; p++;
    if(fmt==0){ unsigned char v; if((e=r8(b,p+gid,&v))!=0)return e; *fd=v; return (*fd<f->cff.fd_count)?OTF_OK:OTF_ERR_BAD_CFF; }
    if(fmt==3){ unsigned short nr,i,first,next; unsigned char v; if((e=u16(b,p,&nr))!=0)return e; p+=2; for(i=0;i<nr;i++){ if((e=u16(b,p+(unsigned long)i*3UL,&first))!=0)return e; if((e=r8(b,p+(unsigned long)i*3UL+2,&v))!=0)return e; if(i+1<nr){ if((e=u16(b,p+(unsigned long)(i+1)*3UL,&next))!=0)return e; } else { if((e=u16(b,p+(unsigned long)nr*3UL,&next))!=0)return e; } if(gid>=first && gid<next){ *fd=v; return (*fd<f->cff.fd_count)?OTF_OK:OTF_ERR_BAD_CFF; }} return OTF_ERR_BAD_CFF; }
    if(fmt==4){ unsigned long nr,i,first,next; unsigned short v; if((e=u32(b,p,&nr))!=0)return e; p+=4; for(i=0;i<nr;i++){ if((e=u32(b,p+i*6UL,&first))!=0)return e; if((e=u16(b,p+i*6UL+4,&v))!=0)return e; if(i+1<nr){ if((e=u32(b,p+(i+1UL)*6UL,&next))!=0)return e; } else { if((e=u32(b,p+nr*6UL,&next))!=0)return e; } if((unsigned long)gid>=first && (unsigned long)gid<next){ *fd=v; return (*fd<f->cff.fd_count)?OTF_OK:OTF_ERR_BAD_CFF; }} return OTF_ERR_BAD_CFF; }
    return OTF_ERR_UNSUPPORTED;
}


static void otf_compute_normalized_axis(otf_font *f, unsigned short ai){
    otf_var_axis *a; long u,n;
    if(!f || ai>=f->var_axis_count)return;
    a=&f->var_axes[ai]; u=clamp_long(a->user_value,a->min_value,a->max_value);
    if(u==a->default_value)n=0;
    else if(u<a->default_value)n=-fixed_div(a->default_value-u,a->default_value-a->min_value);
    else n=fixed_div(u-a->default_value,a->max_value-a->default_value);
    n=clamp_long(n,-OTF_FIXED_ONE,OTF_FIXED_ONE);
    if(f->avar_map_count[ai]>0){
        long x=n; unsigned short i,mc=f->avar_map_count[ai]; long fx,tx,fx2,tx2;
        if(x<=f2dot14_to_fixed(f->avar_maps[ai][0].from_coord)) n=f2dot14_to_fixed(f->avar_maps[ai][0].to_coord);
        else if(x>=f2dot14_to_fixed(f->avar_maps[ai][mc-1].from_coord)) n=f2dot14_to_fixed(f->avar_maps[ai][mc-1].to_coord);
        else {
            for(i=0;i+1<mc;i++){
                fx=f2dot14_to_fixed(f->avar_maps[ai][i].from_coord); tx=f2dot14_to_fixed(f->avar_maps[ai][i].to_coord);
                fx2=f2dot14_to_fixed(f->avar_maps[ai][i+1].from_coord); tx2=f2dot14_to_fixed(f->avar_maps[ai][i+1].to_coord);
                if(x>=fx && x<=fx2){ n=fixed_lerp(fx,tx,fx2,tx2,x); break; }
            }
        }
        n=clamp_long(n,-OTF_FIXED_ONE,OTF_FIXED_ONE);
    }
    a->norm_value=n;
}

static int parse_fvar(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('f','v','a','r')); unsigned short off,csp,axisCount,axisSize,instCount,instSize,i; int e;
    if(!t)return OTF_OK;
    if((e=u16(&f->buf,t->offset+4,&off))!=0)return e;
    if((e=u16(&f->buf,t->offset+6,&csp))!=0)return e;
    if((e=u16(&f->buf,t->offset+8,&axisCount))!=0)return e;
    if((e=u16(&f->buf,t->offset+10,&axisSize))!=0)return e;
    if((e=u16(&f->buf,t->offset+12,&instCount))!=0)return e;
    if((e=u16(&f->buf,t->offset+14,&instSize))!=0)return e; (void)csp; (void)instCount; (void)instSize;
    if(axisCount>OTF_MAX_VAR_AXES)return OTF_ERR_OVERFLOW; if(axisSize<20)return OTF_ERR_BAD_CFF;
    f->var_axis_count=axisCount;
    for(i=0;i<axisCount;i++){
        unsigned long p=t->offset+(unsigned long)off+(unsigned long)i*axisSize,tag,minv,defv,maxv; unsigned short flags,nameid;
        if((e=u32(&f->buf,p,&tag))!=0)return e; if((e=u32(&f->buf,p+4,&minv))!=0)return e; if((e=u32(&f->buf,p+8,&defv))!=0)return e; if((e=u32(&f->buf,p+12,&maxv))!=0)return e; if((e=u16(&f->buf,p+16,&flags))!=0)return e; if((e=u16(&f->buf,p+18,&nameid))!=0)return e;
        f->var_axes[i].tag=tag; f->var_axes[i].min_value=(long)minv; f->var_axes[i].default_value=(long)defv; f->var_axes[i].max_value=(long)maxv; f->var_axes[i].user_value=(long)defv; f->var_axes[i].flags=flags; f->var_axes[i].name_id=nameid; f->var_axes[i].norm_value=0;
        otf_compute_normalized_axis(f,i);
    }
    return OTF_OK;
}

static int parse_avar(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('a','v','a','r')); unsigned short axisCount,i; unsigned long p; int e;
    if(!t || f->var_axis_count==0)return OTF_OK;
    if((e=u16(&f->buf,t->offset+4,&axisCount))!=0)return e; if(axisCount>f->var_axis_count)axisCount=f->var_axis_count;
    p=t->offset+6;
    for(i=0;i<axisCount;i++){
        unsigned short mc,j; if((e=u16(&f->buf,p,&mc))!=0)return e; p+=2;
        f->avar_map_count[i]=(mc>OTF_MAX_AVAR_MAPS_PER_AXIS)?OTF_MAX_AVAR_MAPS_PER_AXIS:mc;
        for(j=0;j<mc;j++){
            short from,to; if((e=s16(&f->buf,p,&from))!=0)return e; if((e=s16(&f->buf,p+2,&to))!=0)return e; p+=4;
            if(j<OTF_MAX_AVAR_MAPS_PER_AXIS){ f->avar_maps[i][j].from_coord=from; f->avar_maps[i][j].to_coord=to; }
        }
        otf_compute_normalized_axis(f,i);
    }
    return OTF_OK;
}

static long region_scalar_for_instance(const otf_font *f, unsigned short ri){
    long scalar=OTF_FIXED_ONE; unsigned short ai;
    if(!f || ri>=f->cff.vstore.region_count)return 0;
    for(ai=0; ai<f->var_axis_count && ai<f->cff.vstore.axis_count; ai++){
        long inst=f->var_axes[ai].norm_value;
        long start=f2dot14_to_fixed(f->cff.vstore.regions[ri].start[ai]);
        long peak=f2dot14_to_fixed(f->cff.vstore.regions[ri].peak[ai]);
        long end=f2dot14_to_fixed(f->cff.vstore.regions[ri].end[ai]);
        long as=OTF_FIXED_ONE;
        if(start>peak || peak>end) as=OTF_FIXED_ONE;
        else if(start<0 && end>0 && peak!=0) as=OTF_FIXED_ONE;
        else if(peak==0) as=OTF_FIXED_ONE;
        else if(inst<start || inst>end) as=0;
        else if(inst==peak) as=OTF_FIXED_ONE;
        else if(inst<peak) as=fixed_div(inst-start, peak-start);
        else as=fixed_div(end-inst, end-peak);
        if(as<0)as=0; if(as>OTF_FIXED_ONE)as=OTF_FIXED_ONE;
        scalar=fixed_mul(scalar,as); if(scalar==0)break;
    }
    return scalar;
}



static int ivs_parse_at(otf_font *f, unsigned long store_base, otf_item_var_store *ivs){
    const otf_buf *b=&f->buf; unsigned long regionListOff, dataOffBase, i; unsigned short fmt, axisCount, regionCount, dataCount; int e;
    zmem(ivs,sizeof(*ivs)); ivs->store_base=store_base;
    if((e=u16(b,store_base,&fmt))!=0)return e; if(fmt!=1)return OTF_ERR_UNSUPPORTED;
    if((e=u32(b,store_base+2,&regionListOff))!=0)return e;
    if((e=u16(b,store_base+6,&dataCount))!=0)return e; if(dataCount>OTF_MAX_IVS_DATA)return OTF_ERR_OVERFLOW;
    if((e=u16(b,store_base+regionListOff,&axisCount))!=0)return e;
    if((e=u16(b,store_base+regionListOff+2,&regionCount))!=0)return e;
    if(axisCount>OTF_MAX_VAR_AXES || regionCount>OTF_MAX_VAR_REGIONS)return OTF_ERR_OVERFLOW;
    ivs->present=1; ivs->axis_count=axisCount; ivs->region_count=regionCount; ivs->data_count=dataCount; dataOffBase=store_base+8;
    for(i=0;i<regionCount;i++){
        unsigned short ai; unsigned long rp=store_base+regionListOff+4UL+i*(unsigned long)axisCount*6UL;
        for(ai=0;ai<axisCount;ai++){
            short a,bv,c; if((e=s16(b,rp+(unsigned long)ai*6UL,&a))!=0)return e; if((e=s16(b,rp+(unsigned long)ai*6UL+2,&bv))!=0)return e; if((e=s16(b,rp+(unsigned long)ai*6UL+4,&c))!=0)return e;
            ivs->regions[i].start[ai]=a; ivs->regions[i].peak[ai]=bv; ivs->regions[i].end[ai]=c;
        }
    }
    for(i=0;i<dataCount;i++){
        unsigned long rel, da; unsigned short itemCount,wdc,ric,j;
        if((e=u32(b,dataOffBase+i*4UL,&rel))!=0)return e; ivs->data_offset[i]=rel; if(rel==0)continue;
        da=store_base+rel; if((e=u16(b,da,&itemCount))!=0)return e; if((e=u16(b,da+2,&wdc))!=0)return e; if((e=u16(b,da+4,&ric))!=0)return e;
        if(ric>OTF_MAX_ITEM_REGION_INDEXES)return OTF_ERR_OVERFLOW;
        ivs->item_count[i]=itemCount; ivs->word_delta_count[i]=wdc; ivs->region_index_count[i]=ric;
        for(j=0;j<ric;j++){ unsigned short ri; if((e=u16(b,da+6UL+(unsigned long)j*2UL,&ri))!=0)return e; if(ri>=regionCount)return OTF_ERR_BAD_CFF; ivs->region_index[i][j]=ri; }
    }
    return OTF_OK;
}

static long ivs_region_scalar_for_instance(const otf_font *f, const otf_item_var_store *ivs, unsigned short ri){
    long scalar=OTF_FIXED_ONE; unsigned short ai;
    if(!f || !ivs || ri>=ivs->region_count)return 0;
    for(ai=0; ai<f->var_axis_count && ai<ivs->axis_count; ai++){
        long inst=f->var_axes[ai].norm_value;
        long start=f2dot14_to_fixed(ivs->regions[ri].start[ai]);
        long peak=f2dot14_to_fixed(ivs->regions[ri].peak[ai]);
        long end=f2dot14_to_fixed(ivs->regions[ri].end[ai]);
        long as=OTF_FIXED_ONE;
        if(start>peak || peak>end) as=OTF_FIXED_ONE;
        else if(start<0 && end>0 && peak!=0) as=OTF_FIXED_ONE;
        else if(peak==0) as=OTF_FIXED_ONE;
        else if(inst<start || inst>end) as=0;
        else if(inst==peak) as=OTF_FIXED_ONE;
        else if(inst<peak) as=fixed_div(inst-start, peak-start);
        else as=fixed_div(end-inst, end-peak);
        if(as<0)as=0; if(as>OTF_FIXED_ONE)as=OTF_FIXED_ONE;
        scalar=fixed_mul(scalar,as); if(scalar==0)break;
    }
    return scalar;
}

static void ivs_update_scalars(const otf_font *f, otf_item_var_store *ivs){
    unsigned short i; if(!f||!ivs||!ivs->present)return; for(i=0;i<ivs->region_count;i++)ivs->region_scalar[i]=ivs_region_scalar_for_instance(f,ivs,i);
}

static int ivs_delta(const otf_font *f, const otf_item_var_store *ivs, unsigned short outer, unsigned short inner, long *delta_out){
    const otf_buf *b=&f->buf; unsigned long da,row,row_size,p; unsigned short wdc,wordCount,longWords,ric,j; long sum=0; int e;
    if(!delta_out)return OTF_ERR_BAD_ARG; *delta_out=0; if(!ivs||!ivs->present)return OTF_OK;
    if(outer==0xffffU && inner==0xffffU)return OTF_OK; if(outer>=ivs->data_count)return OTF_OK;
    if(ivs->data_offset[outer]==0 || inner>=ivs->item_count[outer])return OTF_OK;
    da=ivs->store_base+ivs->data_offset[outer]; wdc=ivs->word_delta_count[outer]; longWords=(unsigned short)((wdc&0x8000U)?1:0); wordCount=(unsigned short)(wdc&0x7fffU); ric=ivs->region_index_count[outer];
    if(wordCount>ric)return OTF_ERR_BAD_CFF;
    row_size=(longWords?2UL:1UL)*((unsigned long)ric+(unsigned long)wordCount);
    row=da+6UL+(unsigned long)ric*2UL+(unsigned long)inner*row_size;
    p=row;
    for(j=0;j<ric;j++){
        long d=0,adj; unsigned short ri=ivs->region_index[outer][j];
        if(j<wordCount){ if(longWords){ unsigned long u; if((e=u32(b,p,&u))!=0)return e; d=(long)u; p+=4; } else { short sv; if((e=s16(b,p,&sv))!=0)return e; d=sv; p+=2; } }
        else { if(longWords){ short sv; if((e=s16(b,p,&sv))!=0)return e; d=sv; p+=2; } else { unsigned char uc; if((e=r8(b,p,&uc))!=0)return e; d=(signed char)uc; p+=1; } }
        adj=fixed_mul(d<<16, ivs->region_scalar[ri]);
        if(adj>=0)sum += (adj+32768L)>>16; else sum -= ((-adj+32768L)>>16);
    }
    *delta_out=sum; return OTF_OK;
}

static int delta_map_lookup(const otf_font *f, unsigned long map_abs, unsigned long index, unsigned short *outer, unsigned short *inner){
    unsigned char fmt,ef; unsigned long count,data,entry=0,pos,i; unsigned short count16; unsigned char entrySize,innerBits; int e;
    if(!outer||!inner)return OTF_ERR_BAD_ARG; *outer=0; *inner=(unsigned short)index;
    if(map_abs==0)return OTF_OK;
    if((e=r8(&f->buf,map_abs,&fmt))!=0)return e; if((e=r8(&f->buf,map_abs+1,&ef))!=0)return e;
    if(fmt==0){ if((e=u16(&f->buf,map_abs+2,&count16))!=0)return e; count=count16; data=map_abs+4; }
    else if(fmt==1){ if((e=u32(&f->buf,map_abs+2,&count))!=0)return e; data=map_abs+6; }
    else return OTF_ERR_UNSUPPORTED;
    if(count==0){ *outer=0xffffU; *inner=0xffffU; return OTF_OK; }
    if(index>=count)index=count-1;
    entrySize=(unsigned char)(((ef&0x30U)>>4)+1U); innerBits=(unsigned char)((ef&0x0fU)+1U); pos=data+index*(unsigned long)entrySize;
    for(i=0;i<entrySize;i++){ unsigned char x; if((e=r8(&f->buf,pos+i,&x))!=0)return e; entry=(entry<<8)|x; }
    *outer=(unsigned short)(entry >> innerBits); *inner=(unsigned short)(entry & ((1UL<<innerBits)-1UL)); return OTF_OK;
}

static int parse_hvar(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('H','V','A','R')); unsigned short maj,min; unsigned long ivo,awo,lso,rso; int e;
    zmem(&f->hvar,sizeof(f->hvar)); if(!t)return OTF_OK; if((e=u16(&f->buf,t->offset,&maj))!=0)return e; if((e=u16(&f->buf,t->offset+2,&min))!=0)return e; if(maj!=1)return OTF_ERR_UNSUPPORTED; (void)min;
    if((e=u32(&f->buf,t->offset+4,&ivo))!=0)return e; if((e=u32(&f->buf,t->offset+8,&awo))!=0)return e; if((e=u32(&f->buf,t->offset+12,&lso))!=0)return e; if((e=u32(&f->buf,t->offset+16,&rso))!=0)return e;
    f->hvar.present=1; f->hvar.table_offset=t->offset; f->hvar.table_length=t->length; f->hvar.map_advance=awo?t->offset+awo:0; f->hvar.map_side1=lso?t->offset+lso:0; f->hvar.map_side2=rso?t->offset+rso:0;
    if(ivo){ if((e=ivs_parse_at(f,t->offset+ivo,&f->hvar.store))!=0)return e; ivs_update_scalars(f,&f->hvar.store); }
    return OTF_OK;
}

static int parse_vvar(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('V','V','A','R')); unsigned short maj,min; unsigned long ivo,aho,tso,bso,vo; int e;
    zmem(&f->vvar,sizeof(f->vvar)); if(!t)return OTF_OK; if((e=u16(&f->buf,t->offset,&maj))!=0)return e; if((e=u16(&f->buf,t->offset+2,&min))!=0)return e; if(maj!=1)return OTF_ERR_UNSUPPORTED; (void)min;
    if((e=u32(&f->buf,t->offset+4,&ivo))!=0)return e; if((e=u32(&f->buf,t->offset+8,&aho))!=0)return e; if((e=u32(&f->buf,t->offset+12,&tso))!=0)return e; if((e=u32(&f->buf,t->offset+16,&bso))!=0)return e; if((e=u32(&f->buf,t->offset+20,&vo))!=0)return e;
    f->vvar.present=1; f->vvar.table_offset=t->offset; f->vvar.table_length=t->length; f->vvar.map_advance=aho?t->offset+aho:0; f->vvar.map_side1=tso?t->offset+tso:0; f->vvar.map_side2=bso?t->offset+bso:0; f->vvar.map_vorg=vo?t->offset+vo:0;
    if(ivo){ if((e=ivs_parse_at(f,t->offset+ivo,&f->vvar.store))!=0)return e; ivs_update_scalars(f,&f->vvar.store); }
    return OTF_OK;
}

static int parse_mvar(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('M','V','A','R')); unsigned short maj,min,res,rs,rc,ivo,i; int e;
    zmem(&f->mvar,sizeof(f->mvar)); if(!t)return OTF_OK; if((e=u16(&f->buf,t->offset,&maj))!=0)return e; if((e=u16(&f->buf,t->offset+2,&min))!=0)return e; if(maj!=1)return OTF_ERR_UNSUPPORTED; (void)min;
    if((e=u16(&f->buf,t->offset+4,&res))!=0)return e; if((e=u16(&f->buf,t->offset+6,&rs))!=0)return e; if((e=u16(&f->buf,t->offset+8,&rc))!=0)return e; if((e=u16(&f->buf,t->offset+10,&ivo))!=0)return e; (void)res;
    if(rs<8)return OTF_ERR_BAD_CFF; if(rc>OTF_MAX_MVAR_RECORDS)return OTF_ERR_OVERFLOW; f->mvar.present=1; f->mvar.value_record_size=rs; f->mvar.value_record_count=rc;
    if(ivo){ if((e=ivs_parse_at(f,t->offset+ivo,&f->mvar.store))!=0)return e; ivs_update_scalars(f,&f->mvar.store); }
    for(i=0;i<rc;i++){ unsigned long q=t->offset+12UL+(unsigned long)i*rs,tag; if((e=u32(&f->buf,q,&tag))!=0)return e; if((e=u16(&f->buf,q+4,&f->mvar.records[i].outer_index))!=0)return e; if((e=u16(&f->buf,q+6,&f->mvar.records[i].inner_index))!=0)return e; f->mvar.records[i].value_tag=tag; }
    return OTF_OK;
}

int otf_get_mvar_delta(const otf_font *f, unsigned long value_tag, long *delta_out){
    unsigned short i; if(!f||!delta_out)return OTF_ERR_BAD_ARG; *delta_out=0; if(!f->mvar.present)return OTF_OK;
    for(i=0;i<f->mvar.value_record_count;i++) if(f->mvar.records[i].value_tag==value_tag) return ivs_delta(f,&f->mvar.store,f->mvar.records[i].outer_index,f->mvar.records[i].inner_index,delta_out);
    return OTF_OK;
}

int otf_get_hmetric_var(const otf_font *f, unsigned short gid, unsigned short *adv, short *lsb){
    unsigned short a; short s; int e; long d=0; if(!f||!adv||!lsb)return OTF_ERR_BAD_ARG; if((e=otf_get_hmetric(f,gid,&a,&s))!=0)return e; if(f->hvar.present){ unsigned short o,i; if((e=delta_map_lookup(f,f->hvar.map_advance,gid,&o,&i))!=0)return e; if((e=ivs_delta(f,&f->hvar.store,o,i,&d))!=0)return e; a=(unsigned short)clamp_long((long)a+d,0,65535L); if(f->hvar.map_side1){ if((e=delta_map_lookup(f,f->hvar.map_side1,gid,&o,&i))!=0)return e; if((e=ivs_delta(f,&f->hvar.store,o,i,&d))!=0)return e; s=(short)clamp_long((long)s+d,-32768L,32767L); }} *adv=a; *lsb=s; return OTF_OK;
}

int otf_get_vmetric_var(const otf_font *f, unsigned short gid, unsigned short *advh, short *tsb){
    unsigned short a; short s; int e; long d=0; if(!f||!advh||!tsb)return OTF_ERR_BAD_ARG; if((e=otf_get_vmetric(f,gid,&a,&s))!=0)return e; if(f->vvar.present){ unsigned short o,i; if((e=delta_map_lookup(f,f->vvar.map_advance,gid,&o,&i))!=0)return e; if((e=ivs_delta(f,&f->vvar.store,o,i,&d))!=0)return e; a=(unsigned short)clamp_long((long)a+d,0,65535L); if(f->vvar.map_side1){ if((e=delta_map_lookup(f,f->vvar.map_side1,gid,&o,&i))!=0)return e; if((e=ivs_delta(f,&f->vvar.store,o,i,&d))!=0)return e; s=(short)clamp_long((long)s+d,-32768L,32767L); }} *advh=a; *tsb=s; return OTF_OK;
}

int otf_get_vorg_var(const otf_font *f, unsigned short gid, short default_y_origin, short *y_origin){
    int e; long d=0; unsigned short o,i; if(!f||!y_origin)return OTF_ERR_BAD_ARG; *y_origin=default_y_origin; if(!f->vvar.present || !f->vvar.map_vorg)return OTF_OK; if((e=delta_map_lookup(f,f->vvar.map_vorg,gid,&o,&i))!=0)return e; if((e=ivs_delta(f,&f->vvar.store,o,i,&d))!=0)return e; *y_origin=(short)clamp_long((long)default_y_origin+d,-32768L,32767L); return OTF_OK;
}

int otf_update_cff2_variation_scalars(otf_font *f){
    unsigned short item,j; if(!f)return OTF_ERR_BAD_ARG;
    for(item=0; item<f->cff.vstore.item_count; item++){
        for(j=0; j<f->cff.vstore.item_region_count[item] && j<OTF_MAX_ITEM_REGION_INDEXES; j++){
            unsigned short ri=f->cff.vstore.item_region_index[item][j];
            f->cff.vstore.item_scalar[item][j]=region_scalar_for_instance(f,ri);
        }
    }
    ivs_update_scalars(f,&f->hvar.store); ivs_update_scalars(f,&f->vvar.store); ivs_update_scalars(f,&f->mvar.store);
    return OTF_OK;
}

void otf_reset_variations(otf_font *f){
    unsigned short i; if(!f)return; for(i=0;i<f->var_axis_count;i++){ f->var_axes[i].user_value=f->var_axes[i].default_value; otf_compute_normalized_axis(f,i); } (void)otf_update_cff2_variation_scalars(f);
}

int otf_set_variation_axis(otf_font *f, unsigned long axis_tag, long user_value_16_16){
    unsigned short i; if(!f)return OTF_ERR_BAD_ARG; for(i=0;i<f->var_axis_count;i++) if(f->var_axes[i].tag==axis_tag){ f->var_axes[i].user_value=user_value_16_16; otf_compute_normalized_axis(f,i); return otf_update_cff2_variation_scalars(f); } return OTF_ERR_RANGE;
}
int otf_set_variation_axis_int(otf_font *f, unsigned long axis_tag, long user_value_integer){ return otf_set_variation_axis(f,axis_tag,user_value_integer<<16); }

static int cff2_parse_vstore(otf_font *f){
    const otf_buf *b=&f->buf; unsigned long base=f->cff.table_offset, vs=f->cff.top.variation_store_off, ivs, regionListOff, dataOffBase, i; unsigned short length, fmt, axisCount, regionCount, dataCount; int e;
    zmem(&f->cff.vstore,sizeof(f->cff.vstore)); f->cff.vstore_item_count=0;
    if(vs==0)return OTF_OK;
    if((e=u16(b,base+vs,&length))!=0)return e; ivs=base+vs+2UL; (void)length;
    if((e=u16(b,ivs,&fmt))!=0)return e; if(fmt!=1)return OTF_ERR_UNSUPPORTED;
    if((e=u32(b,ivs+2,&regionListOff))!=0)return e;
    if((e=u16(b,ivs+6,&dataCount))!=0)return e; if(dataCount>OTF_MAX_CFF_VSTORE_ITEMS)return OTF_ERR_OVERFLOW;
    f->cff.vstore_item_count=(unsigned short)dataCount; f->cff.vstore.item_count=(unsigned short)dataCount; dataOffBase=ivs+8;
    if((e=u16(b,ivs+regionListOff,&axisCount))!=0)return e;
    if((e=u16(b,ivs+regionListOff+2,&regionCount))!=0)return e;
    if(axisCount>OTF_MAX_VAR_AXES)return OTF_ERR_OVERFLOW;
    if(regionCount>OTF_MAX_VAR_REGIONS)return OTF_ERR_OVERFLOW;
    f->cff.vstore.axis_count=axisCount; f->cff.vstore.region_count=regionCount;
    for(i=0;i<regionCount;i++){
        unsigned short ai; unsigned long rp=ivs+regionListOff+4UL+i*(unsigned long)axisCount*6UL;
        for(ai=0;ai<axisCount;ai++){
            short a,bv,c; if((e=s16(b,rp+(unsigned long)ai*6UL,&a))!=0)return e; if((e=s16(b,rp+(unsigned long)ai*6UL+2,&bv))!=0)return e; if((e=s16(b,rp+(unsigned long)ai*6UL+4,&c))!=0)return e;
            f->cff.vstore.regions[i].start[ai]=a; f->cff.vstore.regions[i].peak[ai]=bv; f->cff.vstore.regions[i].end[ai]=c;
        }
    }
    for(i=0;i<dataCount;i++){
        unsigned long dataRel=0, dataAbs; unsigned short itemCount, wordDeltaCount, regionIndexCount,j;
        if((e=u32(b,dataOffBase+i*4UL,&dataRel))!=0)return e;
        if(dataRel==0){ f->cff.vstore_region_count[i]=0; continue; }
        dataAbs=ivs+dataRel;
        if((e=u16(b,dataAbs,&itemCount))!=0)return e; if((e=u16(b,dataAbs+2,&wordDeltaCount))!=0)return e; if((e=u16(b,dataAbs+4,&regionIndexCount))!=0)return e; (void)itemCount; (void)wordDeltaCount;
        if(regionIndexCount>OTF_MAX_ITEM_REGION_INDEXES)return OTF_ERR_OVERFLOW;
        f->cff.vstore_region_count[i]=regionIndexCount; f->cff.vstore.item_region_count[i]=regionIndexCount;
        for(j=0;j<regionIndexCount;j++){ unsigned short ri; if((e=u16(b,dataAbs+6UL+(unsigned long)j*2UL,&ri))!=0)return e; if(ri>=regionCount)return OTF_ERR_BAD_CFF; f->cff.vstore.item_region_index[i][j]=ri; }
    }
    return otf_update_cff2_variation_scalars(f);
}

static int parse_cff2(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('C','F','F','2')); unsigned long s,l,off; unsigned short i; int e;
    if(!t)return OTF_ERR_MISSING_TABLE;
    f->cff.present=1; f->cff.is_cff2=1; f->cff.table_offset=t->offset; f->cff.table_length=t->length;
    if((e=r8(&f->buf,t->offset+0,&f->cff.major))!=0)return e; if((e=r8(&f->buf,t->offset+1,&f->cff.minor))!=0)return e; if((e=r8(&f->buf,t->offset+2,&f->cff.hdr_size))!=0)return e; if((e=u16(&f->buf,t->offset+3,&f->cff.cff2_top_dict_size))!=0)return e;
    if(f->cff.major!=2)return OTF_ERR_UNSUPPORTED;
    if((e=cff_parse_top_dict(&f->buf,t->offset+f->cff.hdr_size,f->cff.cff2_top_dict_size,&f->cff.top))!=0)return e;
    off=(unsigned long)f->cff.hdr_size+(unsigned long)f->cff.cff2_top_dict_size;
    if((e=cff2_index_parse(&f->buf,t->offset,off,&f->cff.global_subr_index))!=0)return e;
    if((e=cff2_index_parse(&f->buf,t->offset,f->cff.top.charstrings_off,&f->cff.charstrings_index))!=0)return e;
    f->cff.glyph_count=f->cff.charstrings_index.count;
    if(f->cff.glyph_count!=f->num_glyphs)return OTF_ERR_BAD_CFF;
    if((e=cff2_index_parse(&f->buf,t->offset,f->cff.top.fd_array_off,&f->cff.fd_array_index))!=0)return e;
    if(f->cff.fd_array_index.count==0)return OTF_ERR_BAD_CFF;
    if(f->cff.fd_array_index.count>OTF_MAX_CFF_FD)return OTF_ERR_OVERFLOW;
    f->cff.fd_count=f->cff.fd_array_index.count;
    for(i=0;i<f->cff.fd_count;i++){
        if((e=cff_index_offset(&f->buf,t->offset,&f->cff.fd_array_index,i,&s,&l))!=0)return e;
        if((e=cff2_parse_fd_dict(&f->buf,s,l,&f->cff.fd_top[i]))!=0)return e;
        if(f->cff.fd_top[i].private_size || f->cff.fd_top[i].private_off){
            if((e=cff_parse_private_dict(&f->buf,t->offset+f->cff.fd_top[i].private_off,f->cff.fd_top[i].private_size,&f->cff.fd_priv[i]))!=0)return e;
            if(f->cff.fd_priv[i].subrs_off){ unsigned long loff=f->cff.fd_top[i].private_off+f->cff.fd_priv[i].subrs_off; if((e=cff2_index_parse(&f->buf,t->offset,loff,&f->cff.fd_local_subr_index[i]))!=0)return e; }
        }
    }
    if(f->cff.top.variation_store_off){ if((e=cff2_parse_vstore(f))!=0)return e; }
    f->cff.priv=f->cff.fd_priv[0]; f->cff.local_subr_index=f->cff.fd_local_subr_index[0];
    return OTF_OK;
}

static int parse_cff(otf_font *f){
    const otf_table *t=otf_find_table(f,OTF_TAG('C','F','F',' ')); unsigned long off=0; unsigned long s,l; int e;
    if(!t){ if(otf_find_table(f,OTF_TAG('C','F','F','2')))return parse_cff2(f); return OTF_ERR_MISSING_TABLE; }
    f->cff.present=1; f->cff.table_offset=t->offset; f->cff.table_length=t->length;
    if((e=r8(&f->buf,t->offset+0,&f->cff.major))!=0)return e; if((e=r8(&f->buf,t->offset+1,&f->cff.minor))!=0)return e; if((e=r8(&f->buf,t->offset+2,&f->cff.hdr_size))!=0)return e; if((e=r8(&f->buf,t->offset+3,&f->cff.off_size))!=0)return e;
    if(f->cff.major!=1)return OTF_ERR_UNSUPPORTED;
    off=f->cff.hdr_size;
    if((e=cff_index_parse(&f->buf,t->offset,off,&f->cff.name_index))!=0)return e; off=f->cff.name_index.end;
    if((e=cff_index_parse(&f->buf,t->offset,off,&f->cff.top_index))!=0)return e; off=f->cff.top_index.end;
    if((e=cff_index_parse(&f->buf,t->offset,off,&f->cff.string_index))!=0)return e; off=f->cff.string_index.end;
    if((e=cff_index_parse(&f->buf,t->offset,off,&f->cff.global_subr_index))!=0)return e; off=f->cff.global_subr_index.end;
    if(f->cff.top_index.count<1)return OTF_ERR_BAD_CFF;
    if((e=cff_index_offset(&f->buf,t->offset,&f->cff.top_index,0,&s,&l))!=0)return e;
    if((e=cff_parse_top_dict(&f->buf,s,l,&f->cff.top))!=0)return e;
    if((e=cff_index_parse(&f->buf,t->offset,f->cff.top.charstrings_off,&f->cff.charstrings_index))!=0)return e;
    f->cff.glyph_count=f->cff.charstrings_index.count;
    if(f->cff.top.private_size && f->cff.top.private_off){
        if((e=cff_parse_private_dict(&f->buf,t->offset+f->cff.top.private_off,f->cff.top.private_size,&f->cff.priv))!=0)return e;
        if(f->cff.priv.subrs_off){ unsigned long loff=f->cff.top.private_off+f->cff.priv.subrs_off; if((e=cff_index_parse(&f->buf,t->offset,loff,&f->cff.local_subr_index))!=0)return e; }
    }
    return OTF_OK;
}

int otf_parse(otf_font *f, const unsigned char *data, unsigned long size){
    unsigned short i; int e; if(!f||!data||size<12)return OTF_ERR_BAD_ARG; zmem(f,sizeof(*f)); f->buf.data=data; f->buf.size=size;
    if((e=u32(&f->buf,0,&f->sfnt_version))!=0)return e;
    if(!(f->sfnt_version==OTF_TAG('O','T','T','O') || f->sfnt_version==0x00010000UL))return OTF_ERR_BAD_MAGIC;
    if((e=u16(&f->buf,4,&f->num_tables))!=0)return e; if(f->num_tables>OTF_MAX_TABLES)return OTF_ERR_OVERFLOW;
    for(i=0;i<f->num_tables;i++){
        unsigned long o=12UL+(unsigned long)i*16UL;
        if((e=u32(&f->buf,o,&f->tables[i].tag))!=0)return e; if((e=u32(&f->buf,o+4,&f->tables[i].checksum))!=0)return e; if((e=u32(&f->buf,o+8,&f->tables[i].offset))!=0)return e; if((e=u32(&f->buf,o+12,&f->tables[i].length))!=0)return e;
        if((e=check_range(&f->buf,f->tables[i].offset,f->tables[i].length))!=0)return e;
    }
    if((e=parse_head(f))!=0)return e; if((e=parse_maxp(f))!=0)return e; if((e=parse_hhea(f))!=0)return e; if((e=parse_vhea(f))!=0)return e; if((e=parse_os2(f))!=0)return e; if((e=parse_name(f))!=0)return e; if((e=parse_cmap(f))!=0)return e;
    if((e=parse_fvar(f))!=0)return e; if((e=parse_avar(f))!=0)return e;
    if((e=parse_hvar(f))!=0)return e; if((e=parse_vvar(f))!=0)return e; if((e=parse_mvar(f))!=0)return e;
    if(f->sfnt_version==OTF_TAG('O','T','T','O')){ if((e=parse_cff(f))!=0)return e; }
    return OTF_OK;
}

static int subr_bias(unsigned short n){ if(n<1240)return 107; if(n<33900)return 1131; return 32768; }

typedef struct cs_ctx_s {
    const otf_font *font;
    otf_glyph_path *path;
    long stack[OTF_MAX_CFF_STACK]; unsigned short sp;
    long x,y; int have_move; int hint_count; int width_done; int depth;
    const otf_cff_index *local_subrs; unsigned short active_item; unsigned short active_regions;
} cs_ctx;
static int path_add(cs_ctx *c, unsigned char op, long x1,long y1,long x2,long y2,long x3,long y3){
    otf_glyph_path *p=c->path; otf_path_cmd *cmd; if(p->count>=OTF_MAX_PATH_CMDS){p->overflow=1; return OTF_ERR_OVERFLOW;} cmd=&p->cmds[p->count++]; cmd->op=op; cmd->x1=OTF_FIXED_FROM_INT(x1); cmd->y1=OTF_FIXED_FROM_INT(y1); cmd->x2=OTF_FIXED_FROM_INT(x2); cmd->y2=OTF_FIXED_FROM_INT(y2); cmd->x3=OTF_FIXED_FROM_INT(x3); cmd->y3=OTF_FIXED_FROM_INT(y3); return OTF_OK;
}
static void clear_stack(cs_ctx *c){ c->sp=0; }
static int cs_number(const otf_buf *b, unsigned long *p, unsigned long end, long *out){
    unsigned char v; int e;if(*p>=end)return OTF_ERR_RANGE; if((e=r8(b,*p,&v))!=0)return e; (*p)++;
    if(v==28){ short s; if((e=s16(b,*p,&s))!=0)return e; *p+=2; *out=s; return OTF_OK; }
    if(v>=32 && v<=246){ *out=(long)v-139L; return OTF_OK; }
    if(v>=247 && v<=250){ unsigned char w; if((e=r8(b,*p,&w))!=0)return e; (*p)++; *out=(v-247L)*256L+w+108L; return OTF_OK; }
    if(v>=251 && v<=254){ unsigned char w; if((e=r8(b,*p,&w))!=0)return e; (*p)++; *out=-((v-251L)*256L+w+108L); return OTF_OK; }
    if(v==255){ unsigned long q; if((e=u32(b,*p,&q))!=0)return e; *p+=4; *out=(long)(q>>16); return OTF_OK; } /* 16.16 -> int grid */
    return OTF_ERR_BAD_CHARSTRING;
}
static void cs_maybe_width(cs_ctx *c, int odd){
    if(c->font->cff.is_cff2){ c->width_done=1; return; }
    if(!c->width_done && odd && c->sp>0){ c->path->advance_width=(short)(c->font->cff.priv.nominal_width_x + c->stack[0]);
        { unsigned short i; for(i=1;i<c->sp;i++) c->stack[i-1]=c->stack[i]; c->sp--; }
    }
    c->width_done=1;
}
static int run_charstring(cs_ctx *c, unsigned long start, unsigned long len);
static int call_subr(cs_ctx *c, const otf_cff_index *idx, int global){
    long n; int bias; unsigned long s,l; int e; if(!idx || idx->count==0)return OTF_ERR_BAD_CHARSTRING; if(c->sp<1)return OTF_ERR_BAD_CHARSTRING; n=c->stack[--c->sp]; bias=subr_bias(idx->count); n+=bias; if(n<0 || n>=idx->count)return OTF_ERR_BAD_CHARSTRING; if(c->depth>=OTF_MAX_SUBR_DEPTH)return OTF_ERR_OVERFLOW; if((e=cff_index_offset(&c->font->buf,c->font->cff.table_offset,idx,(unsigned short)n,&s,&l))!=0)return e; c->depth++; e=run_charstring(c,s,l); c->depth--; (void)global; return e;
}
static int run_charstring(cs_ctx *c, unsigned long start, unsigned long len){
    const otf_buf *b=&c->font->buf; unsigned long p=start,end=start+len; int e;
    while(p<end){ unsigned char op; if((e=r8(b,p,&op))!=0)return e; p++;
        if(op==28 || op>=32){ long num; p--; if(c->sp>=OTF_MAX_CFF_STACK)return OTF_ERR_OVERFLOW; if((e=cs_number(b,&p,end,&num))!=0)return e; c->stack[c->sp++]=num; continue; }
        if(op==12){ unsigned char esc; if((e=r8(b,p,&esc))!=0)return e; p++; op=(unsigned char)(120+esc); }
        switch(op){
        case 1: case 3: cs_maybe_width(c,(c->sp&1)); c->hint_count+=(int)(c->sp/2); clear_stack(c); break;
        case 15: if(c->font->cff.is_cff2){ long vi; if(c->sp<1)return OTF_ERR_BAD_CHARSTRING; vi=c->stack[--c->sp]; if(vi>=0 && vi<c->font->cff.vstore_item_count){ c->active_item=(unsigned short)vi; c->active_regions=c->font->cff.vstore.item_region_count[c->active_item]; } clear_stack(c); break; } clear_stack(c); break;
        case 16: if(c->font->cff.is_cff2){ long n,k,total,base; unsigned short i,j; if(c->sp<1)return OTF_ERR_BAD_CHARSTRING; n=c->stack[c->sp-1]; if(n<0)return OTF_ERR_BAD_CHARSTRING; k=(long)c->active_regions; total=n+n*k+1L; if(total>(long)c->sp)return OTF_ERR_BAD_CHARSTRING; base=(long)c->sp-total; for(i=0;i<(unsigned short)n;i++){ long v=c->stack[base+i]; for(j=0;j<(unsigned short)k;j++){ long delta=c->stack[base+n+i*(unsigned short)k+j]; long sc=c->font->cff.vstore.item_scalar[c->active_item][j]; long adj=fixed_mul(delta<<16,sc); if(adj>=0)v += (adj + 32768L) >> 16; else v += -(((-adj) + 32768L) >> 16); } c->stack[base+i]=v; } c->sp=(unsigned short)(base+n); break; } clear_stack(c); break;
        case 18: case 23: cs_maybe_width(c,(c->sp&1)); c->hint_count+=(int)(c->sp/2); clear_stack(c); { unsigned long bytes=(unsigned long)((c->hint_count+7)/8); if(p+bytes>end)return OTF_ERR_RANGE; p+=bytes; } break;
        case 4: cs_maybe_width(c,(c->sp&1)); if(c->sp<1)return OTF_ERR_BAD_CHARSTRING; c->y+=c->stack[c->sp-1]; if((e=path_add(c,OTF_PATH_MOVE,c->x,c->y,0,0,0,0))!=0)return e; c->have_move=1; clear_stack(c); break;
        case 5: { unsigned short i; for(i=0;i+1<c->sp;i+=2){ c->x+=c->stack[i]; c->y+=c->stack[i+1]; if((e=path_add(c,OTF_PATH_LINE,c->x,c->y,0,0,0,0))!=0)return e; } clear_stack(c); } break;
        case 6: { unsigned short i; int horiz=1; for(i=0;i<c->sp;i++){ if(horiz)c->x+=c->stack[i]; else c->y+=c->stack[i]; if((e=path_add(c,OTF_PATH_LINE,c->x,c->y,0,0,0,0))!=0)return e; horiz=!horiz; } clear_stack(c); } break;
        case 7: { unsigned short i; int vert=1; for(i=0;i<c->sp;i++){ if(vert)c->y+=c->stack[i]; else c->x+=c->stack[i]; if((e=path_add(c,OTF_PATH_LINE,c->x,c->y,0,0,0,0))!=0)return e; vert=!vert; } clear_stack(c); } break;
        case 8: { unsigned short i; for(i=0;i+5<c->sp;i+=6){ long x1=c->x+c->stack[i], y1=c->y+c->stack[i+1], x2=x1+c->stack[i+2], y2=y1+c->stack[i+3]; c->x=x2+c->stack[i+4]; c->y=y2+c->stack[i+5]; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,c->x,c->y))!=0)return e; } clear_stack(c); } break;
        case 10: if((e=call_subr(c,c->local_subrs?c->local_subrs:&c->font->cff.local_subr_index,0))!=0)return e; break;
        case 11: return OTF_OK;
        case 14: if(c->sp>=4){} if((e=path_add(c,OTF_PATH_CLOSE,c->x,c->y,0,0,0,0))!=0)return e; clear_stack(c); return OTF_OK;
        case 21: cs_maybe_width(c,(c->sp&1)); if(c->sp<2)return OTF_ERR_BAD_CHARSTRING; c->x+=c->stack[c->sp-2]; c->y+=c->stack[c->sp-1]; if((e=path_add(c,OTF_PATH_MOVE,c->x,c->y,0,0,0,0))!=0)return e; c->have_move=1; clear_stack(c); break;
        case 22: cs_maybe_width(c,(c->sp&1)); if(c->sp<1)return OTF_ERR_BAD_CHARSTRING; c->x+=c->stack[c->sp-1]; if((e=path_add(c,OTF_PATH_MOVE,c->x,c->y,0,0,0,0))!=0)return e; c->have_move=1; clear_stack(c); break;
        case 24: { unsigned short i; for(i=0;i+7<c->sp;i+=6){ if(i+7>=c->sp)break; { long x1=c->x+c->stack[i], y1=c->y+c->stack[i+1], x2=x1+c->stack[i+2], y2=y1+c->stack[i+3]; c->x=x2+c->stack[i+4]; c->y=y2+c->stack[i+5]; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,c->x,c->y))!=0)return e; }} if(c->sp>=2){ c->x+=c->stack[c->sp-2]; c->y+=c->stack[c->sp-1]; if((e=path_add(c,OTF_PATH_LINE,c->x,c->y,0,0,0,0))!=0)return e; } clear_stack(c); } break;
        case 25: { unsigned short i=0; while(i+7<c->sp){ c->x+=c->stack[i]; c->y+=c->stack[i+1]; if((e=path_add(c,OTF_PATH_LINE,c->x,c->y,0,0,0,0))!=0)return e; i+=2; } if(i+5<c->sp){ long x1=c->x+c->stack[i], y1=c->y+c->stack[i+1], x2=x1+c->stack[i+2], y2=y1+c->stack[i+3]; c->x=x2+c->stack[i+4]; c->y=y2+c->stack[i+5]; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,c->x,c->y))!=0)return e; } clear_stack(c); } break;
        case 26: { unsigned short i=0; cs_maybe_width(c,(c->sp&1)==0); while(i+3<c->sp){ long x1=c->x, y1=c->y+c->stack[i], x2=x1+c->stack[i+1], y2=y1+c->stack[i+2]; c->x=x2+c->stack[i+3]; c->y=y2; i+=4; if(i==c->sp-1){ c->x+=c->stack[i]; i++; } if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,c->x,c->y))!=0)return e; } clear_stack(c); } break;
        case 27: { unsigned short i=0; cs_maybe_width(c,(c->sp&1)==0); while(i+3<c->sp){ long x1=c->x+c->stack[i], y1=c->y, x2=x1+c->stack[i+1], y2=y1+c->stack[i+2]; c->x=x2+c->stack[i+3]; c->y=y2; i+=4; if(i==c->sp-1){ c->y+=c->stack[i]; i++; } if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,c->x,c->y))!=0)return e; } clear_stack(c); } break;
        case 29: if((e=call_subr(c,&c->font->cff.global_subr_index,1))!=0)return e; break;
        case 30: case 31: { unsigned short i=0; int hv=(op==31); while(i+3<c->sp){ long x1,y1,x2,y2; if(hv){ x1=c->x+c->stack[i]; y1=c->y; x2=x1+c->stack[i+1]; y2=y1+c->stack[i+2]; c->x=x2; c->y=y2+c->stack[i+3]; } else { x1=c->x; y1=c->y+c->stack[i]; x2=x1+c->stack[i+1]; y2=y1+c->stack[i+2]; c->x=x2+c->stack[i+3]; c->y=y2; } i+=4; hv=!hv; if(i==c->sp-1){ if(op==31)c->x+=c->stack[i]; else c->y+=c->stack[i]; i++; } if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,c->x,c->y))!=0)return e; } clear_stack(c); } break;
        case 154: /* flex */ if(c->sp>=13){ long x1=c->x+c->stack[0],y1=c->y+c->stack[1],x2=x1+c->stack[2],y2=y1+c->stack[3],x3=x2+c->stack[4],y3=y2+c->stack[5]; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,x3,y3))!=0)return e; { long x4=x3+c->stack[6],y4=y3+c->stack[7],x5=x4+c->stack[8],y5=y4+c->stack[9]; c->x=x5+c->stack[10]; c->y=y5+c->stack[11]; if((e=path_add(c,OTF_PATH_CUBIC,x4,y4,x5,y5,c->x,c->y))!=0)return e; }} clear_stack(c); break;
        case 155: /* hflex */ if(c->sp>=7){ long x1=c->x+c->stack[0],y1=c->y,x2=x1+c->stack[1],y2=y1+c->stack[2],x3=x2+c->stack[3],y3=y2; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,x3,y3))!=0)return e; { long x4=x3+c->stack[4],y4=y3,x5=x4+c->stack[5],y5=c->y; c->x=x5+c->stack[6]; c->y=y5; if((e=path_add(c,OTF_PATH_CUBIC,x4,y4,x5,y5,c->x,c->y))!=0)return e; }} clear_stack(c); break;
        case 156: /* hflex1 */ if(c->sp>=9){ long x1=c->x+c->stack[0],y1=c->y+c->stack[1],x2=x1+c->stack[2],y2=y1+c->stack[3],x3=x2+c->stack[4],y3=y2; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,x3,y3))!=0)return e; { long x4=x3+c->stack[5],y4=y3,x5=x4+c->stack[6],y5=y4+c->stack[7]; c->x=x5+c->stack[8]; c->y=c->y; if((e=path_add(c,OTF_PATH_CUBIC,x4,y4,x5,y5,c->x,c->y))!=0)return e; }} clear_stack(c); break;
        case 157: /* flex1 */ if(c->sp>=11){ long dx=0,dy=0; unsigned short i; for(i=0;i<10;i+=2){ dx+=c->stack[i]; dy+=c->stack[i+1]; } { long x1=c->x+c->stack[0],y1=c->y+c->stack[1],x2=x1+c->stack[2],y2=y1+c->stack[3],x3=x2+c->stack[4],y3=y2+c->stack[5]; if((e=path_add(c,OTF_PATH_CUBIC,x1,y1,x2,y2,x3,y3))!=0)return e; { long x4=x3+c->stack[6],y4=y3+c->stack[7],x5=x4+c->stack[8],y5=y4+c->stack[9]; if(dx<0)dx=-dx; if(dy<0)dy=-dy; if(dx>dy){ c->x=x5+c->stack[10]; c->y=y5; } else { c->x=x5; c->y=y5+c->stack[10]; } if((e=path_add(c,OTF_PATH_CUBIC,x4,y4,x5,y5,c->x,c->y))!=0)return e; }}} clear_stack(c); break;
        default: clear_stack(c); break;
        }
    }
    return OTF_OK;
}

int otf_decode_glyph_path(const otf_font *f, unsigned short gid, otf_glyph_path *path){
    unsigned long s,l; int e; cs_ctx c; unsigned short adv=0, fd=0; short lsb=0; if(!f||!path)return OTF_ERR_BAD_ARG; zmem(path,sizeof(*path));
    if(!f->cff.present)return OTF_ERR_UNSUPPORTED; if(gid>=f->cff.charstrings_index.count)return OTF_ERR_RANGE;
    (void)otf_get_hmetric_var(f,gid,&adv,&lsb); path->advance_width=(short)adv; path->left_side_bearing=lsb; path->nominal_width=f->cff.priv.nominal_width_x; { unsigned short ah=0; short ts=0; if(otf_get_vmetric_var(f,gid,&ah,&ts)==OTF_OK){ path->advance_height=(short)ah; path->top_side_bearing=ts; }}
    if((e=cff_index_offset(&f->buf,f->cff.table_offset,&f->cff.charstrings_index,gid,&s,&l))!=0)return e;
    zmem(&c,sizeof(c)); c.font=f; c.path=path; c.path->advance_width=(short)adv; c.width_done=0; c.local_subrs=&f->cff.local_subr_index; c.active_item=0; c.active_regions=0;
    if(f->cff.is_cff2){ if((e=cff2_fd_for_gid(f,gid,&fd))!=0)return e; c.local_subrs=&f->cff.fd_local_subr_index[fd]; c.active_item=0; if(f->cff.fd_priv[fd].vsindex>=0 && f->cff.fd_priv[fd].vsindex<f->cff.vstore_item_count)c.active_item=(unsigned short)f->cff.fd_priv[fd].vsindex; c.active_regions=f->cff.vstore.item_region_count[c.active_item]; c.width_done=1; }
    if((e=run_charstring(&c,s,l))!=0)return e;
    return path->overflow?OTF_ERR_OVERFLOW:OTF_OK;
}


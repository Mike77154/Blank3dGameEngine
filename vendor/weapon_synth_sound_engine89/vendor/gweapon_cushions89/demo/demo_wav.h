#ifndef DEMO_WAV_H
#define DEMO_WAV_H
#include <stdio.h>
static void dw_u16(FILE *f, unsigned int v) { fputc((int)(v & 255U),f); fputc((int)((v>>8)&255U),f); }
static void dw_u32(FILE *f, unsigned long v) { fputc((int)(v&255UL),f); fputc((int)((v>>8)&255UL),f); fputc((int)((v>>16)&255UL),f); fputc((int)((v>>24)&255UL),f); }
#ifndef DEMO_WAV_STEREO_ONLY
static int dw_write(const char *path, const signed short *s, unsigned long n, unsigned int rate)
{
    FILE *f; unsigned long i; unsigned long bytes=n*2UL;
    f=fopen(path,"wb"); if(!f) return 0;
    fwrite("RIFF",1,4,f); dw_u32(f,36UL+bytes); fwrite("WAVEfmt ",1,8,f);
    dw_u32(f,16UL); dw_u16(f,1U); dw_u16(f,1U); dw_u32(f,rate);
    dw_u32(f,(unsigned long)rate*2UL); dw_u16(f,2U); dw_u16(f,16U);
    fwrite("data",1,4,f); dw_u32(f,bytes);
    for(i=0UL;i<n;++i) dw_u16(f,(unsigned int)(unsigned short)s[i]);
    fclose(f); return 1;
}
#endif
#ifndef DEMO_WAV_MONO_ONLY
static int dw_write_stereo(const char *path, const signed short *s, unsigned long frames, unsigned int rate)
{
    FILE *f; unsigned long i; unsigned long samples=frames*2UL; unsigned long bytes=samples*2UL;
    f=fopen(path,"wb"); if(!f) return 0;
    fwrite("RIFF",1,4,f); dw_u32(f,36UL+bytes); fwrite("WAVEfmt ",1,8,f);
    dw_u32(f,16UL); dw_u16(f,1U); dw_u16(f,2U); dw_u32(f,rate);
    dw_u32(f,(unsigned long)rate*4UL); dw_u16(f,4U); dw_u16(f,16U);
    fwrite("data",1,4,f); dw_u32(f,bytes);
    for(i=0UL;i<samples;++i) dw_u16(f,(unsigned int)(unsigned short)s[i]);
    fclose(f); return 1;
}
#endif

#endif

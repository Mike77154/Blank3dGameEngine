#include "goldie_audio89.h"
#include <stdio.h>
#include <string.h>

static unsigned char g_wav[48];
static short g_pcm[8];

static void put_u16(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v & 255U);
    p[1] = (unsigned char)((v >> 8) & 255U);
}

static void put_u32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v & 255UL);
    p[1] = (unsigned char)((v >> 8) & 255UL);
    p[2] = (unsigned char)((v >> 16) & 255UL);
    p[3] = (unsigned char)((v >> 24) & 255UL);
}

int main(void)
{
    goldie_audio89_pcm_view pcm;
    int rc;
    memset(g_wav, 0, sizeof(g_wav));
    memcpy(g_wav + 0, "RIFF", 4);
    put_u32(g_wav + 4, 40UL);
    memcpy(g_wav + 8, "WAVE", 4);
    memcpy(g_wav + 12, "fmt ", 4);
    put_u32(g_wav + 16, 16UL);
    put_u16(g_wav + 20, 1U);
    put_u16(g_wav + 22, 1U);
    put_u32(g_wav + 24, 8000UL);
    put_u32(g_wav + 28, 8000UL);
    put_u16(g_wav + 32, 1U);
    put_u16(g_wav + 34, 8U);
    memcpy(g_wav + 36, "data", 4);
    put_u32(g_wav + 40, 4UL);
    g_wav[44] = 0U;
    g_wav[45] = 64U;
    g_wav[46] = 128U;
    g_wav[47] = 255U;
    rc = goldie_audio89_decode_wav_memory(g_wav, (unsigned long)sizeof(g_wav),
                                          g_pcm, 8UL, &pcm);
    if (rc != GOLDIE_AUDIO89_OK) {
        printf("wav decode failed: %s\n", goldie_audio89_result_string(rc));
        return 1;
    }
    printf("Goldie WAV smoke: %lu Hz %u ch %lu frames -> %d %d %d %d\n",
           pcm.sample_rate, pcm.channels, pcm.frame_count,
           (int)g_pcm[0], (int)g_pcm[1], (int)g_pcm[2], (int)g_pcm[3]);
    if (pcm.frame_count != 4UL || pcm.channels != 1U || pcm.sample_rate != 8000UL) return 2;
    if (g_pcm[0] != (short)-32768 || g_pcm[2] != (short)0) return 3;
    return 0;
}

#include <stdio.h>
#include <string.h>

static unsigned long fnv1a_file(const char *path, int *ok)
{
    FILE *f;
    int ch;
    unsigned long h;
    f = fopen(path, "rb");
    if (!f) { *ok = 0; return 0UL; }
    h = 2166136261UL;
    while ((ch = fgetc(f)) != EOF) {
        h ^= (unsigned long)(unsigned char)ch;
        h *= 16777619UL;
    }
    fclose(f);
    *ok = 1;
    return h;
}

static int same_file(const char *a, const char *b, long *diff_at)
{
    FILE *fa;
    FILE *fb;
    long pos;
    int ca;
    int cb;
    fa = fopen(a, "rb");
    fb = fopen(b, "rb");
    if (!fa || !fb) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        *diff_at = -1L;
        return 0;
    }
    pos = 0L;
    for (;;) {
        ca = fgetc(fa);
        cb = fgetc(fb);
        if (ca != cb) { *diff_at = pos; fclose(fa); fclose(fb); return 0; }
        if (ca == EOF) break;
        ++pos;
    }
    fclose(fa);
    fclose(fb);
    *diff_at = -1L;
    return 1;
}

int main(int argc, char **argv)
{
    int i;
    int visual_fail;
    int command_fail;
    char a[512];
    char b[512];
    long diff;
    unsigned long corpus_old;
    unsigned long corpus_new;
    if (argc < 3) return 2;
    visual_fail = 0;
    command_fail = 0;
    corpus_old = 2166136261UL;
    corpus_new = 2166136261UL;
    for (i = 0; i < 192; ++i) {
        int ok1;
        int ok2;
        unsigned long h1;
        unsigned long h2;
        sprintf(a, "%s/%03d.ppm", argv[1], i);
        sprintf(b, "%s/%03d.ppm", argv[2], i);
        if (!same_file(a, b, &diff)) {
            printf("VISUAL_FAIL %03d byte=%ld\n", i, diff);
            ++visual_fail;
        }
        h1 = fnv1a_file(a, &ok1); h2 = fnv1a_file(b, &ok2);
        if (ok1) { corpus_old ^= h1; corpus_old *= 16777619UL; }
        if (ok2) { corpus_new ^= h2; corpus_new *= 16777619UL; }
        sprintf(a, "%s/%03d.cmd", argv[1], i);
        sprintf(b, "%s/%03d.cmd", argv[2], i);
        if (!same_file(a, b, &diff)) {
            printf("COMMAND_FAIL %03d byte=%ld\n", i, diff);
            ++command_fail;
        }
    }
    printf("golden_presets=192 visual_byte_failures=%d command_byte_failures=%d\n", visual_fail, command_fail);
    printf("visual_corpus_fnv_old=%lu visual_corpus_fnv_ini=%lu\n", corpus_old, corpus_new);
    return (visual_fail == 0 && command_fail == 0) ? 0 : 1;
}

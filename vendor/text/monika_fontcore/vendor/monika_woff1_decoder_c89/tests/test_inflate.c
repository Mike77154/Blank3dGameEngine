#include <stdio.h>
#include <string.h>
#include "woff1_inflate.h"

static unsigned char out[2048];

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static unsigned int from_hex(const char *hex, unsigned char *buf)
{
    unsigned int i;
    unsigned int n;
    n = 0;
    for (i = 0; hex[i] != '\0' && hex[i + 1] != '\0'; i += 2) {
        buf[n] = (unsigned char)((hexval(hex[i]) << 4) | hexval(hex[i + 1]));
        n++;
    }
    return n;
}

static int test_one(const char *hex, const unsigned char *plain, unsigned int plain_len)
{
    unsigned char src[512];
    unsigned int src_len;
    woff1_u32 got;
    int r;

    src_len = from_hex(hex, src);
    got = 0;
    memset(out, 0, sizeof(out));
    r = woff1_inflate_zlib(src, (woff1_u32)src_len, out, (woff1_u32)sizeof(out), &got, 1);
    if (r != WOFF1_INF_OK) {
        printf("inflate failed: %s\n", woff1_inflate_error_string(r));
        return 0;
    }
    if (got != (woff1_u32)plain_len) {
        printf("bad length got %lu expected %u\n", (unsigned long)got, plain_len);
        return 0;
    }
    if (memcmp(out, plain, plain_len) != 0) {
        printf("bad bytes\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    unsigned char dyn[300] = {
        101, 50, 99, 32, 100, 90, 89, 90, 122, 32, 100, 90, 97, 122, 88, 51, 97, 89, 32, 32, 50, 100, 120, 97, 97, 97, 49, 97, 122, 32, 88, 97, 48, 32, 89, 90, 49, 32, 121, 32, 32, 89, 32, 97, 88, 49, 100, 32, 32, 100, 120, 48, 88, 48, 32, 32, 32, 50, 90, 48, 122, 50, 98, 90, 32, 122, 88, 32, 121, 49, 121, 99, 89, 48, 100, 32, 48, 122, 121, 90, 97, 90, 98, 32, 51, 50, 50, 122, 32, 32, 48, 32, 97, 32, 49, 49, 32, 122, 48, 121, 50, 121, 89, 32, 49, 51, 97, 122, 48, 101, 48, 49, 32, 88, 98, 90, 121, 50, 49, 32, 48, 88, 90, 121, 88, 121, 97, 49, 49, 51, 51, 120, 89, 51, 97, 32, 32, 49, 50, 32, 99, 49, 32, 98, 99, 99, 97, 89, 97, 32, 32, 32, 100, 51, 32, 121, 32, 99, 32, 32, 32, 48, 32, 32, 32, 89, 120, 90, 90, 100, 97, 32, 122, 120, 88, 32, 32, 100, 32, 48, 32, 51, 88, 97, 32, 97, 122, 101, 98, 32, 89, 48, 88, 49, 32, 48, 89, 32, 48, 97, 122, 50, 120, 88, 98, 32, 101, 32, 98, 32, 99, 99, 32, 32, 32, 88, 50, 32, 101, 97, 49, 98, 50, 32, 50, 89, 32, 51, 48, 98, 122, 32, 121, 100, 32, 50, 88, 50, 32, 90, 100, 122, 32, 48, 90, 97, 120, 51, 122, 32, 97, 32, 32, 120, 50, 101, 120, 88, 32, 32, 100, 122, 49, 121, 49, 90, 49, 32, 99, 98, 99, 101, 32, 32, 49, 32, 32, 120, 51, 48, 32, 121, 120, 120, 100, 32, 32, 51, 90, 101, 50, 49, 100, 120, 98, 88, 99, 122, 101, 101
    };
    unsigned char aaa[1000];
    unsigned int i;
    const unsigned char hello[] = "hello hello hello hello";

    for (i = 0; i < 1000; ++i) aaa[i] = (unsigned char)'a';

    if (!test_one("78dacb48cdc9c957c8402701680308b1", hello, 23)) return 1;
    if (!test_one("78da1d4fb9110431086b4525f03483c9c0760f86ea8fbd841924d07365e3f8f29e196d1a0b90f32282a3614158ce2860218c0f701e19618e9c5ad2d186e2da8b0ea8cbc3132ad20021c08ca6925a608da64b0c4b2f6190795905b3ea5b1a000b3623f78e351b8e8ee9c62733decffd04fad91083a805a26f62918dd40245cbb3c445627f5f26b8c1299005a56cd4810ce8a7411e4f7bd2e1c9fd4b36174fcb9dfb4e90c19550ef4d5bf52b7c5edaee7b7fbf755495", dyn, 300)) return 1;
    if (!test_one("78da4b4c1c05a360140c770000f9d87af8", aaa, 1000)) return 1;
    if (!test_one("78da030000000001", (const unsigned char *)"", 0)) return 1;

    printf("inflate tests ok\n");
    return 0;
}

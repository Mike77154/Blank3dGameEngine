#include <stdio.h>

#include "qoi89.h"

int main(void) {
    qoi89_desc desc;
    unsigned char pixels[8];
    unsigned char encoded[64];
    unsigned char decoded[8];
    size_t encoded_len;
    size_t decoded_len;
    qoi89_desc decoded_desc;
    qoi89_status st;

    desc.width = 2ul;
    desc.height = 1ul;
    desc.channels = 4u;
    desc.colorspace = QOI89_SRGB;

    pixels[0] = 255u; pixels[1] = 0u;   pixels[2] = 0u;   pixels[3] = 255u;
    pixels[4] = 0u;   pixels[5] = 255u; pixels[6] = 0u;   pixels[7] = 255u;

    st = qoi89_encode(pixels, sizeof(pixels), &desc, encoded, sizeof(encoded), &encoded_len);
    if (st != QOI89_OK) {
        printf("encode failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    st = qoi89_decode(encoded, encoded_len, 0u, decoded, sizeof(decoded), &decoded_desc, &decoded_len);
    if (st != QOI89_OK) {
        printf("decode failed: %s\n", qoi89_status_string(st));
        return 1;
    }

    printf("encoded_len=%lu decoded_len=%lu channels=%u\n",
        (unsigned long)encoded_len,
        (unsigned long)decoded_len,
        (unsigned int)decoded_desc.channels
    );

    return 0;
}

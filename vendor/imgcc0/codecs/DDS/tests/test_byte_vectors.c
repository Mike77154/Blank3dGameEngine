#include <stdio.h>
#include <string.h>
#include "giffany_dds/gdds.h"
#include "golden_vectors.h"

typedef struct vector_case {
    gdds_format format;
    const char* name;
    const gdds_u8* expected;
    gdds_size expected_size;
} vector_case;

static void fill_source(gdds_u8* p) {
    gdds_u32 i;
    for (i = 0u; i < 16u; ++i) {
        p[i * 4u + 0u] = (gdds_u8)(i * 13u + 7u);
        p[i * 4u + 1u] = (gdds_u8)(i * 29u + 3u);
        p[i * 4u + 2u] = (gdds_u8)(i * 47u + 11u);
        p[i * 4u + 3u] = (gdds_u8)(i * 17u + 19u);
    }
}

int main(void) {
    gdds_u8 pixels[64];
    gdds_u32 i;
    vector_case cases[] = {
        { GDDS_FORMAT_RGBA8, "rgba8", golden_rgba8, (gdds_size)sizeof(golden_rgba8) },
        { GDDS_FORMAT_BGRA8, "bgra8", golden_bgra8, (gdds_size)sizeof(golden_bgra8) },
        { GDDS_FORMAT_DXT1, "dxt1", golden_dxt1, (gdds_size)sizeof(golden_dxt1) },
        { GDDS_FORMAT_DXT3, "dxt3", golden_dxt3, (gdds_size)sizeof(golden_dxt3) },
        { GDDS_FORMAT_DXT5, "dxt5", golden_dxt5, (gdds_size)sizeof(golden_dxt5) },
        { GDDS_FORMAT_RGBA8_SRGB, "rgba8s", golden_rgba8s, (gdds_size)sizeof(golden_rgba8s) },
        { GDDS_FORMAT_BGRA8_SRGB, "bgra8s", golden_bgra8s, (gdds_size)sizeof(golden_bgra8s) },
        { GDDS_FORMAT_DXT1_SRGB, "dxt1s", golden_dxt1s, (gdds_size)sizeof(golden_dxt1s) },
        { GDDS_FORMAT_DXT3_SRGB, "dxt3s", golden_dxt3s, (gdds_size)sizeof(golden_dxt3s) },
        { GDDS_FORMAT_DXT5_SRGB, "dxt5s", golden_dxt5s, (gdds_size)sizeof(golden_dxt5s) },
        { GDDS_FORMAT_BC4_UNORM, "bc4", golden_bc4, (gdds_size)sizeof(golden_bc4) },
        { GDDS_FORMAT_BC5_UNORM, "bc5", golden_bc5, (gdds_size)sizeof(golden_bc5) },
        { GDDS_FORMAT_BC4_SNORM, "bc4s", golden_bc4s, (gdds_size)sizeof(golden_bc4s) },
        { GDDS_FORMAT_BC5_SNORM, "bc5s", golden_bc5s, (gdds_size)sizeof(golden_bc5s) }
    };
    fill_source(pixels);
    for (i = 0u; i < (gdds_u32)(sizeof(cases) / sizeof(cases[0])); ++i) {
        gdds_encode_options options = gdds_encode_options_default(cases[i].format);
        gdds_buffer encoded;
        gdds_result rc;
        memset(&encoded, 0, sizeof(encoded));
        rc = gdds_encode_memory_rgba8(pixels, 4u, 4u, &options, &encoded);
        if (rc != GDDS_RESULT_OK) {
            printf("encode failed: %s (%s)\n", cases[i].name, gdds_result_string(rc));
            return 1;
        }
        if (encoded.size != cases[i].expected_size ||
            memcmp(encoded.data, cases[i].expected, cases[i].expected_size) != 0) {
            printf("byte mismatch: %s\n", cases[i].name);
            return 2;
        }
        gdds_buffer_release(&encoded);
    }
    puts("golden byte vectors: ok");
    return 0;
}

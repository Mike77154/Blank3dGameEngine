#include <bmp/bmp.h>
#include <string.h>

#ifndef BMP_EXPORT
#error "BMP_EXPORT must be defined"
#endif
#ifndef BMP_NO_EXPORT
#error "BMP_NO_EXPORT must be defined"
#endif
#ifndef BMP_DEPRECATED
#error "BMP_DEPRECATED must be defined"
#endif
#ifndef BMP_DEPRECATED_EXPORT
#error "BMP_DEPRECATED_EXPORT must be defined"
#endif
#ifndef BMP_DEPRECATED_NO_EXPORT
#error "BMP_DEPRECATED_NO_EXPORT must be defined"
#endif
#ifndef BMP_NO_DEPRECATED
#error "BMP_NO_DEPRECATED must be defined"
#endif
#ifndef BMP_WARN_UNUSED_RESULT
#error "BMP_WARN_UNUSED_RESULT must be defined"
#endif
#ifndef BMP_ATTR_ACCESS_RO_2
#error "BMP_ATTR_ACCESS_RO_2 must be defined"
#endif
#ifndef BMP_ATTR_ACCESS_WO_2
#error "BMP_ATTR_ACCESS_WO_2 must be defined"
#endif

int main(void)
{
    static const bmp_u8 rgba[4] = { 1U, 2U, 3U, 255U };
    bmp_u8 output[128];
    bmp_u8 workspace[16];
    bmp_u32 output_size = 0U;
    bmp_encode_options opt;
    int rc;

    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = bmp_encode_rgba32_into(rgba, 1U, 1U, 4U, &opt,
                                output, (bmp_u32)sizeof(output), &output_size,
                                workspace, (bmp_u32)sizeof(workspace));
    if (rc != BMP_OK || output_size == 0U) return 1;

    return strcmp(bmp_version_string(), "1.16.0") == 0 &&
           bmp_version_number() == 11600U &&
           BMP_VERSION_NUMBER == 11600U ? 0 : 1;
}

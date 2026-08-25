#include <pcx/pcx.h>
#include <string.h>

#ifndef PCX_EXPORT
#error "PCX_EXPORT must be defined"
#endif
#ifndef PCX_NO_EXPORT
#error "PCX_NO_EXPORT must be defined"
#endif
#ifndef PCX_DEPRECATED
#error "PCX_DEPRECATED must be defined"
#endif
#ifndef PCX_WARN_UNUSED_RESULT
#error "PCX_WARN_UNUSED_RESULT must be defined"
#endif

int main(void)
{
    PCXImage img;
    static pcx_u8 pixels[12];
    pcx_image_init(&img);
    if (pcx_image_use_buffer(&img, 2, 2, 3, pixels) != PCX_OK)
    {
        return 1;
    }
    pcx_image_release(&img);
    return strcmp(pcx_version_string(), "1.16.0") == 0 &&
           pcx_version_number() == 11600U &&
           PCX_VERSION_NUMBER == 11600U ? 0 : 1;
}

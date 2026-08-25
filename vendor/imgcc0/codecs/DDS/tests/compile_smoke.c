#include "giffany_dds.h"

int main(void) {
    gdds_info info = {0};
    gdds_mip_info mip = {0};
    gdds_image image = {0};
    gdds_buffer buffer = {0};
    gdds_mip_level_rgba8 level = {0};
    gdds_generated_mipchain generated = {0};
    gdds_mipmap_options mip_options = gdds_mipmap_options_default();
    gdds_encode_options enc = gdds_encode_options_default(GDDS_FORMAT_DXT5_SRGB);
    gdds_u32 mip_count = gdds_calc_full_mip_count_2d(8u, 8u);
    (void)info;
    (void)mip;
    (void)image;
    (void)buffer;
    (void)level;
    (void)generated;
    (void)mip_options;
    mip_options.color_space = GDDS_MIP_COLOR_SPACE_SRGB;
    (void)enc;
    (void)mip_count;
    (void)gdds_format_is_srgb(enc.format);
    return 0;
}

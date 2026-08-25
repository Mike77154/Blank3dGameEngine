#include "gdds_internal.h"

gdds_u8 gdds__static_decode_rgba[GDDS_STATIC_RGBA_CAPACITY];
gdds_u8 gdds__static_encode_dds[GDDS_STATIC_DDS_CAPACITY];
gdds_u8 gdds__static_normal_rgba[GDDS_STATIC_RGBA_CAPACITY];
gdds_u8 gdds__static_mip_rgba[GDDS_STATIC_MIP_CAPACITY];
gdds_generated_mip_level gdds__static_mip_levels[GDDS_MAX_MIP_LEVELS];
gdds_mip_level_rgba8 gdds__static_encode_levels[GDDS_MAX_MIP_LEVELS];

const char* gdds_result_string(gdds_result result) {
    switch (result) {
        case GDDS_RESULT_OK: return "ok";
        case GDDS_RESULT_INVALID_ARGUMENT: return "invalid argument";
        case GDDS_RESULT_NOT_DDS: return "not a DDS file";
        case GDDS_RESULT_UNSUPPORTED: return "unsupported DDS variant";
        case GDDS_RESULT_TRUNCATED: return "truncated file";
        case GDDS_RESULT_ALLOC: return "static storage capacity exceeded";
        default: return "unknown error";
    }
}

int gdds_format_is_srgb(gdds_format format) {
    return gdds__format_is_srgb(format);
}

int gdds_format_is_snorm(gdds_format format) {
    return gdds__format_is_snorm(format);
}

static int gdds__normal_map_convention_is_y_up(gdds_normal_map_convention convention) {
    switch (convention) {
        case GDDS_NORMAL_MAP_CONVENTION_OPENGL:
        case GDDS_NORMAL_MAP_CONVENTION_GLTF:
            return 1;
        case GDDS_NORMAL_MAP_CONVENTION_DIRECTX:
        default:
            return 0;
    }
}

const char* gdds_normal_map_convention_string(gdds_normal_map_convention convention) {
    switch (convention) {
        case GDDS_NORMAL_MAP_CONVENTION_DIRECTX: return "directx";
        case GDDS_NORMAL_MAP_CONVENTION_OPENGL: return "opengl";
        case GDDS_NORMAL_MAP_CONVENTION_GLTF: return "gltf";
        default: return "unknown";
    }
}

int gdds_normal_map_convention_needs_y_flip(gdds_normal_map_convention source,
                                            gdds_normal_map_convention target) {
    return gdds__normal_map_convention_is_y_up(source) != gdds__normal_map_convention_is_y_up(target);
}

gdds_encode_options gdds_encode_options_default(gdds_format format) {
    gdds_encode_options options;
    options.format = format;
    return options;
}

gdds_parse_options gdds_parse_options_default(void) {
    gdds_parse_options options;
    options.mode = GDDS_PARSE_MODE_PERMISSIVE;
    return options;
}

gdds_mipmap_options gdds_mipmap_options_default(void) {
    gdds_mipmap_options options;
    options.mip_count = 0u;
    options.filter = GDDS_MIP_FILTER_BOX;
    options.alpha_filter = GDDS_MIP_ALPHA_FILTER_STRAIGHT;
    options.color_space = GDDS_MIP_COLOR_SPACE_AUTO;
    return options;
}

gdds_normal_map_decode_options gdds_normal_map_decode_options_default(void) {
    gdds_normal_map_decode_options options;
    options.reconstruct_z = 1;
    options.normalize = 1;
    options.invert_y = 0;
    options.stored_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    options.output_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    return options;
}

gdds_normal_map_encode_options gdds_normal_map_encode_options_default(void) {
    gdds_normal_map_encode_options options;
    options.input_layout = GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB;
    options.normalize = 1;
    options.invert_y = 0;
    options.input_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    options.output_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    options.output_format = GDDS_FORMAT_BC5_UNORM;
    return options;
}

gdds_u32 gdds_calc_full_mip_count_2d(gdds_u32 width, gdds_u32 height) {
    if (width == 0u || height == 0u) return 0u;
    return gdds__max_mip_count_2d(width, height);
}

void gdds_image_release(gdds_image* image) {
    if (!image) return;
    memset(image, 0, sizeof(*image));
}

void gdds_buffer_release(gdds_buffer* buffer) {
    if (!buffer) return;
    memset(buffer, 0, sizeof(*buffer));
}

void gdds_generated_mipchain_release(gdds_generated_mipchain* mipchain) {
    if (!mipchain) return;
    memset(mipchain, 0, sizeof(*mipchain));
}

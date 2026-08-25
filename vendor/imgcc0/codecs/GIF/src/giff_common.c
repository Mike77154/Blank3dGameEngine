#include "giff_internal.h"

giff_u32 giff_align_up(giff_u32 value, giff_u32 align)
{
    giff_u32 mask;

    if (align <= 1u) {
        return value;
    }

    mask = align - 1u;
    return (value + mask) & (~mask);
}

void giff_mem_zero(void* ptr, giff_u32 size)
{
    if (ptr != 0 && size != 0u) {
        memset(ptr, 0, (size_t)size);
    }
}

void giff_mem_copy(void* dst, const void* src, giff_u32 size)
{
    if (dst != 0 && src != 0 && size != 0u) {
        memcpy(dst, src, (size_t)size);
    }
}

void giff_mem_move(void* dst, const void* src, giff_u32 size)
{
    if (dst != 0 && src != 0 && size != 0u) {
        memmove(dst, src, (size_t)size);
    }
}

giff_result giff_ws_take(
    giff_ws_cursor* ws,
    giff_u32 bytes,
    giff_u32 align,
    void** out_ptr
)
{
    giff_u32 offset;

    if (ws == 0 || out_ptr == 0) {
        return GIFF_E_INVALID_ARGUMENT;
    }

    offset = giff_align_up(ws->used, align);
    if (offset > ws->size) {
        return GIFF_E_NO_WORKSPACE;
    }
    if (bytes > (ws->size - offset)) {
        return GIFF_E_NO_WORKSPACE;
    }

    *out_ptr = (void*)(ws->base + offset);
    ws->used = offset + bytes;
    return GIFF_OK;
}

const char* giff_result_string(giff_result code)
{
    switch (code) {
        case GIFF_OK: return "GIFF_OK";
        case GIFF_E_NEED_MORE_INPUT: return "GIFF_E_NEED_MORE_INPUT";
        case GIFF_E_DONE: return "GIFF_E_DONE";
        case GIFF_E_INVALID_ARGUMENT: return "GIFF_E_INVALID_ARGUMENT";
        case GIFF_E_INVALID_SIGNATURE: return "GIFF_E_INVALID_SIGNATURE";
        case GIFF_E_UNSUPPORTED_VERSION: return "GIFF_E_UNSUPPORTED_VERSION";
        case GIFF_E_BAD_DIMENSIONS: return "GIFF_E_BAD_DIMENSIONS";
        case GIFF_E_BAD_COLOR_TABLE: return "GIFF_E_BAD_COLOR_TABLE";
        case GIFF_E_BAD_BLOCK: return "GIFF_E_BAD_BLOCK";
        case GIFF_E_BAD_LZW_CODE: return "GIFF_E_BAD_LZW_CODE";
        case GIFF_E_TRUNCATED: return "GIFF_E_TRUNCATED";
        case GIFF_E_NO_WORKSPACE: return "GIFF_E_NO_WORKSPACE";
        case GIFF_E_OUTPUT_OVERFLOW: return "GIFF_E_OUTPUT_OVERFLOW";
        case GIFF_E_UNSUPPORTED: return "GIFF_E_UNSUPPORTED";
        case GIFF_E_INTERNAL: return "GIFF_E_INTERNAL";
        case GIFF_E_IO: return "GIFF_E_IO";
        default: return "GIFF_E_UNKNOWN";
    }
}

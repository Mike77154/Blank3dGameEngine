#include "mwinfont89.h"
#include <string.h>

static int CALLBACK mwinfont89_enum_callback(
    const LOGFONTA *logical_font,
    const TEXTMETRICA *metric,
    DWORD font_type,
    LPARAM parameter)
{
    int *found;
    (void)logical_font;
    (void)metric;
    (void)font_type;
    found = (int *)parameter;
    *found = 1;
    return 0;
}

static int mwinfont89_face_exists(HDC dc, const char *face)
{
    LOGFONTA logical_font;
    int found;

    if (dc == (HDC)0 || face == (const char *)0 || face[0] == '\0') {
        return 0;
    }
    ZeroMemory(&logical_font, sizeof(logical_font));
    logical_font.lfCharSet = DEFAULT_CHARSET;
    lstrcpynA(logical_font.lfFaceName, face, LF_FACESIZE);
    found = 0;
    EnumFontFamiliesExA(
        dc,
        &logical_font,
        (FONTENUMPROCA)mwinfont89_enum_callback,
        (LPARAM)&found,
        0
    );
    return found;
}

static int mwinfont89_family(int family)
{
    switch (family) {
    case MFONT89_FAMILY_ROMAN: return FF_ROMAN;
    case MFONT89_FAMILY_SWISS: return FF_SWISS;
    case MFONT89_FAMILY_MODERN: return FF_MODERN;
    default: return FF_DONTCARE;
    }
}

static mfont89_handle mwinfont89_acquire(
    void *user,
    const mfont89_request *request)
{
    mwinfont89_state *state;
    const char *selected_face;
    HFONT font;
    int i;
    int slot;

    state = (mwinfont89_state *)user;
    if (state == (mwinfont89_state *)0 ||
        request == (const mfont89_request *)0) {
        return (mfont89_handle)GetStockObject(SYSTEM_FONT);
    }
    selected_face = "";
    for (i = 0; i < request->face_count; ++i) {
        if (mwinfont89_face_exists(state->lookup_dc, request->faces[i])) {
            selected_face = request->faces[i];
            break;
        }
    }
    font = CreateFontA(
        -request->pixel_height,
        0,
        0,
        0,
        request->weight,
        request->italic ? TRUE : FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | mwinfont89_family(request->family),
        selected_face
    );
    if (font == (HFONT)0) {
        return (mfont89_handle)GetStockObject(SYSTEM_FONT);
    }
    if (state->count >= MWINFONT89_MAX_HANDLES) {
        DeleteObject(font);
        return (mfont89_handle)GetStockObject(SYSTEM_FONT);
    }
    slot = state->count;
    state->handles[slot] = font;
    state->owned[slot] = 1;
    state->count += 1;
    return (mfont89_handle)font;
}

static void mwinfont89_release_all_provider(void *user)
{
    mwinfont89_shutdown((mwinfont89_state *)user);
}

void mwinfont89_init(mwinfont89_state *state, HDC lookup_dc)
{
    int i;

    if (state == (mwinfont89_state *)0) {
        return;
    }
    state->lookup_dc = lookup_dc;
    state->count = 0;
    for (i = 0; i < MWINFONT89_MAX_HANDLES; ++i) {
        state->handles[i] = (HFONT)0;
        state->owned[i] = 0;
    }
}

void mwinfont89_set_lookup_dc(mwinfont89_state *state, HDC lookup_dc)
{
    if (state != (mwinfont89_state *)0) {
        state->lookup_dc = lookup_dc;
    }
}

mfont89_provider mwinfont89_make_provider(mwinfont89_state *state)
{
    mfont89_provider provider;
    provider.user = state;
    provider.acquire = mwinfont89_acquire;
    provider.release_all = mwinfont89_release_all_provider;
    return provider;
}

void mwinfont89_shutdown(mwinfont89_state *state)
{
    int i;

    if (state == (mwinfont89_state *)0) {
        return;
    }
    for (i = 0; i < state->count; ++i) {
        if (state->owned[i] && state->handles[i] != (HFONT)0) {
            DeleteObject(state->handles[i]);
        }
        state->handles[i] = (HFONT)0;
        state->owned[i] = 0;
    }
    state->count = 0;
    state->lookup_dc = (HDC)0;
}

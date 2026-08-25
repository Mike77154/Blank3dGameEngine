#include "mwingdi89.h"
#include <string.h>

static COLORREF mwingdi89_color(mcanvas89_color color)
{
    return RGB(color.red, color.green, color.blue);
}

static void mwingdi89_fill_rect(
    void *user,
    const mcanvas89_rect *source,
    mcanvas89_color color)
{
    mwingdi89_state *state;
    RECT rect;

    state = (mwingdi89_state *)user;
    if (state == (mwingdi89_state *)0 || state->dc == (HDC)0) {
        return;
    }
    rect.left = source->left;
    rect.top = source->top;
    rect.right = source->right;
    rect.bottom = source->bottom;
    SetDCBrushColor(state->dc, mwingdi89_color(color));
    FillRect(state->dc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
}

static HFONT mwingdi89_transform_font(
    HFONT base_font,
    int scale_q8,
    int rotation_tenths)
{
    LOGFONTA logical_font;
    HFONT transformed;

    if (base_font == (HFONT)0 ||
        (scale_q8 == 256 && rotation_tenths == 0)) {
        return (HFONT)0;
    }
    if (GetObjectA(base_font, sizeof(logical_font), &logical_font) == 0) {
        return (HFONT)0;
    }
    if (scale_q8 <= 0) {
        scale_q8 = 1;
    }
    logical_font.lfHeight = (logical_font.lfHeight * scale_q8) / 256;
    if (logical_font.lfHeight == 0) {
        logical_font.lfHeight = -1;
    }
    logical_font.lfEscapement = rotation_tenths;
    logical_font.lfOrientation = rotation_tenths;
    transformed = CreateFontIndirectA(&logical_font);
    return transformed;
}

static HFONT mwingdi89_select_style_font(
    HDC dc,
    const mtext89_style *style,
    HFONT *temporary,
    HFONT *old_font)
{
    HFONT base_font;
    HFONT selected_font;

    base_font = (HFONT)style->font;
    if (base_font == (HFONT)0) {
        base_font = (HFONT)GetStockObject(SYSTEM_FONT);
    }
    *temporary = mwingdi89_transform_font(
        base_font,
        style->scale_q8 > 0 ? style->scale_q8 : 256,
        style->rotation_tenths
    );
    selected_font = *temporary != (HFONT)0 ? *temporary : base_font;
    *old_font = (HFONT)SelectObject(dc, selected_font);
    return selected_font;
}

static void mwingdi89_restore_style_font(
    HDC dc,
    HFONT old_font,
    HFONT temporary)
{
    SelectObject(dc, old_font);
    if (temporary != (HFONT)0) {
        DeleteObject(temporary);
    }
}

static int mwingdi89_measure_text(
    void *user,
    const char *text,
    const mtext89_style *style,
    mtext89_extent *extent)
{
    mwingdi89_state *state;
    HFONT temporary;
    HFONT old_font;
    int old_extra;
    SIZE size;
    int length;

    state = (mwingdi89_state *)user;
    if (state == (mwingdi89_state *)0 || state->dc == (HDC)0 ||
        text == (const char *)0 || style == (const mtext89_style *)0 ||
        extent == (mtext89_extent *)0) {
        return 0;
    }
    length = (int)strlen(text);
    temporary = (HFONT)0;
    old_font = (HFONT)0;
    mwingdi89_select_style_font(
        state->dc,
        style,
        &temporary,
        &old_font
    );
    old_extra = SetTextCharacterExtra(state->dc, style->character_extra);
    size.cx = 0;
    size.cy = 0;
    if (!GetTextExtentPoint32A(state->dc, text, length, &size)) {
        SetTextCharacterExtra(state->dc, old_extra);
        mwingdi89_restore_style_font(state->dc, old_font, temporary);
        return 0;
    }
    SetTextCharacterExtra(state->dc, old_extra);
    mwingdi89_restore_style_font(state->dc, old_font, temporary);
    extent->width = size.cx;
    extent->height = size.cy;
    return 1;
}

static void mwingdi89_draw_text(
    void *user,
    const mcanvas89_rect *source,
    const char *text,
    const mtext89_style *style)
{
    mwingdi89_state *state;
    RECT rect;
    HFONT temporary;
    HFONT old_font;
    int old_extra;
    UINT flags;
    SIZE extent;
    int x;
    int y;
    int length;

    state = (mwingdi89_state *)user;
    if (state == (mwingdi89_state *)0 || state->dc == (HDC)0) {
        return;
    }
    rect.left = source->left;
    rect.top = source->top;
    rect.right = source->right;
    rect.bottom = source->bottom;
    flags = DT_SINGLELINE | DT_VCENTER;
    if (style->align == MTEXT89_ALIGN_CENTER) {
        flags |= DT_CENTER;
    } else if (style->align == MTEXT89_ALIGN_RIGHT) {
        flags |= DT_RIGHT;
    } else {
        flags |= DT_LEFT;
    }

    SetBkMode(state->dc, TRANSPARENT);
    SetTextColor(state->dc, mwingdi89_color(style->color));
    temporary = (HFONT)0;
    old_font = (HFONT)0;
    mwingdi89_select_style_font(
        state->dc,
        style,
        &temporary,
        &old_font
    );
    old_extra = SetTextCharacterExtra(state->dc, style->character_extra);

    if (style->rotation_tenths == 0) {
        DrawTextA(state->dc, text, -1, &rect, flags);
    } else {
        length = (int)strlen(text);
        extent.cx = 0;
        extent.cy = 0;
        GetTextExtentPoint32A(state->dc, text, length, &extent);
        if (style->align == MTEXT89_ALIGN_CENTER) {
            x = (source->left + source->right - extent.cx) / 2;
        } else if (style->align == MTEXT89_ALIGN_RIGHT) {
            x = source->right - extent.cx;
        } else {
            x = source->left;
        }
        y = (source->top + source->bottom - extent.cy) / 2;
        TextOutA(state->dc, x, y, text, length);
    }

    SetTextCharacterExtra(state->dc, old_extra);
    mwingdi89_restore_style_font(state->dc, old_font, temporary);
}

void mwingdi89_init(mwingdi89_state *state, HDC dc)
{
    if (state != (mwingdi89_state *)0) {
        state->dc = dc;
    }
}

mcanvas89_provider mwingdi89_canvas_provider(mwingdi89_state *state)
{
    mcanvas89_provider provider;
    provider.user = state;
    provider.fill_rect = mwingdi89_fill_rect;
    return provider;
}

mtext89_provider mwingdi89_text_provider(mwingdi89_state *state)
{
    mtext89_provider provider;
    provider.user = state;
    provider.draw_text = mwingdi89_draw_text;
    provider.measure_text = mwingdi89_measure_text;
    return provider;
}

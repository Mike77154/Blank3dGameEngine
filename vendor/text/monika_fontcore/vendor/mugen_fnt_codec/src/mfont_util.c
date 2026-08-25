#include "mfont_internal.h"

mft_u16 mft_read_le16(const mft_u8 *p)
{
    return (mft_u16)((mft_u16)p[0] | ((mft_u16)p[1] << 8));
}

mft_u16 mft_read_be16(const mft_u8 *p)
{
    return (mft_u16)(((mft_u16)p[0] << 8) | (mft_u16)p[1]);
}

mft_u32 mft_read_le32(const mft_u8 *p)
{
    return (mft_u32)p[0] |
           ((mft_u32)p[1] << 8) |
           ((mft_u32)p[2] << 16) |
           ((mft_u32)p[3] << 24);
}

mft_u32 mft_read_be32(const mft_u8 *p)
{
    return ((mft_u32)p[0] << 24) |
           ((mft_u32)p[1] << 16) |
           ((mft_u32)p[2] << 8) |
           (mft_u32)p[3];
}

void mft_write_le16(mft_u8 *p, mft_u16 v)
{
    p[0] = (mft_u8)(v & 255U);
    p[1] = (mft_u8)((v >> 8) & 255U);
}

void mft_write_le32(mft_u8 *p, mft_u32 v)
{
    p[0] = (mft_u8)(v & 255UL);
    p[1] = (mft_u8)((v >> 8) & 255UL);
    p[2] = (mft_u8)((v >> 16) & 255UL);
    p[3] = (mft_u8)((v >> 24) & 255UL);
}

void mft_write_be32(mft_u8 *p, mft_u32 v)
{
    p[0] = (mft_u8)((v >> 24) & 255UL);
    p[1] = (mft_u8)((v >> 16) & 255UL);
    p[2] = (mft_u8)((v >> 8) & 255UL);
    p[3] = (mft_u8)(v & 255UL);
}

void mft_memzero(void *p, mft_u32 size)
{
    memset(p, 0, (size_t)size);
}

int mft_ascii_ieq(const char *a, const char *b)
{
    unsigned char ca;
    unsigned char cb;
    if (a == 0 || b == 0) {
        return 0;
    }
    while (*a != 0 && *b != 0) {
        ca = (unsigned char)*a;
        cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (unsigned char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (unsigned char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
        ++a;
        ++b;
    }
    return (*a == 0 && *b == 0) ? 1 : 0;
}

char *mft_trim(char *s)
{
    char *e;
    if (s == 0) {
        return s;
    }
    while (*s != 0 && isspace((unsigned char)*s)) {
        ++s;
    }
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) {
        --e;
    }
    *e = 0;
    return s;
}

int mft_parse_int(const char *s, mft_s32 *out_value)
{
    long sign;
    long value;
    int base;
    int digit;
    int any;

    if (s == 0 || out_value == 0) {
        return 0;
    }
    while (*s != 0 && isspace((unsigned char)*s)) {
        ++s;
    }
    sign = 1;
    if (*s == '-') {
        sign = -1;
        ++s;
    } else if (*s == '+') {
        ++s;
    }
    base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    }
    value = 0;
    any = 0;
    while (*s != 0) {
        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (base == 16 && *s >= 'a' && *s <= 'f') {
            digit = *s - 'a' + 10;
        } else if (base == 16 && *s >= 'A' && *s <= 'F') {
            digit = *s - 'A' + 10;
        } else {
            break;
        }
        if (digit >= base) {
            return 0;
        }
        value = (value * base) + digit;
        any = 1;
        ++s;
    }
    while (*s != 0 && isspace((unsigned char)*s)) {
        ++s;
    }
    if (!any || *s != 0) {
        return 0;
    }
    *out_value = (mft_s32)(value * sign);
    return 1;
}

int mft_parse_char_token(const char *token, mft_u8 *out_code)
{
    mft_s32 value;
    size_t len;
    if (token == 0 || out_code == 0) {
        return 0;
    }
    len = strlen(token);
    if (len == 1U) {
        *out_code = (mft_u8)token[0];
        return 1;
    }
    if (len == 3U && ((token[0] == '\'' && token[2] == '\'') ||
                      (token[0] == '"' && token[2] == '"'))) {
        *out_code = (mft_u8)token[1];
        return 1;
    }
    if (len == 4U && token[0] == '\\' && (token[1] == 'x' || token[1] == 'X')) {
        if (mft_parse_int(token + 1, &value)) {
            *out_code = (mft_u8)(value & 255);
            return 1;
        }
    }
    if (mft_parse_int(token, &value)) {
        if (value < 0 || value > 255) {
            return 0;
        }
        *out_code = (mft_u8)value;
        return 1;
    }
    *out_code = (mft_u8)token[0];
    return 1;
}

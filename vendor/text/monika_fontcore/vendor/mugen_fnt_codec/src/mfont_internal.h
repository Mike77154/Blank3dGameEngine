#ifndef MFT_MFONT_INTERNAL_H
#define MFT_MFONT_INTERNAL_H

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "mfont.h"
#include "zlfont.h"
#include "fntpng.h"
#include "pcxfnt.h"
#include "fntbump.h"

#define MFT_COUNTOF(a) ((mft_u32)(sizeof(a) / sizeof((a)[0])))

mft_u16 mft_read_le16(const mft_u8 *p);
mft_u16 mft_read_be16(const mft_u8 *p);
mft_u32 mft_read_le32(const mft_u8 *p);
mft_u32 mft_read_be32(const mft_u8 *p);
void mft_write_le16(mft_u8 *p, mft_u16 v);
void mft_write_le32(mft_u8 *p, mft_u32 v);
void mft_write_be32(mft_u8 *p, mft_u32 v);
void mft_memzero(void *p, mft_u32 size);
int mft_ascii_ieq(const char *a, const char *b);
char *mft_trim(char *s);
int mft_parse_int(const char *s, mft_s32 *out_value);
int mft_parse_char_token(const char *token, mft_u8 *out_code);

#endif

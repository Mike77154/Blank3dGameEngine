#include "sff_endian.h"

sff_u16 sff_rd_le_u16(const sff_u8 *p)
{
    return (sff_u16)((sff_u16)p[0] | (sff_u16)((sff_u16)p[1] << 8));
}

sff_s16 sff_rd_le_s16(const sff_u8 *p)
{
    return (sff_s16)sff_rd_le_u16(p);
}

sff_u32 sff_rd_le_u32(const sff_u8 *p)
{
    return (sff_u32)((sff_u32)p[0] |
                     ((sff_u32)p[1] << 8) |
                     ((sff_u32)p[2] << 16) |
                     ((sff_u32)p[3] << 24));
}

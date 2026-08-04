#ifndef SFF_ENDIAN_H
#define SFF_ENDIAN_H

#include "sff_types.h"

sff_u16 sff_rd_le_u16(const sff_u8 *p);
sff_s16 sff_rd_le_s16(const sff_u8 *p);
sff_u32 sff_rd_le_u32(const sff_u8 *p);

#endif /* SFF_ENDIAN_H */

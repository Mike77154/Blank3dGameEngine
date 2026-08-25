#ifndef GWP_DEC_VP8_FRAME_H_
#define GWP_DEC_VP8_FRAME_H_

#include "../webp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWPVP8FrameTag {
  GWPBool key_frame;
  GWPu32 version;
  GWPBool show_frame;
  GWPu32 first_partition_size;
} GWPVP8FrameTag;

typedef struct GWPVP8KeyFrameHeader {
  GWPu32 width;
  GWPu32 height;
  GWPu32 horizontal_scale;
  GWPu32 vertical_scale;
} GWPVP8KeyFrameHeader;

typedef struct GWPVP8PartitionInfo {
  GWPData uncompressed_chunk;
  GWPData first_partition;
} GWPVP8PartitionInfo;

typedef struct GWPVP8FrameHeader {
  GWPVP8FrameTag tag;
  GWPVP8KeyFrameHeader key;
  GWPVP8PartitionInfo partitions;
} GWPVP8FrameHeader;

GWPStatusCode GWPVP8ParseFrameHeader(const GWPu8* data,
                                     GWPu32 data_size,
                                     GWPVP8FrameHeader* header);

#ifdef __cplusplus
}    /* extern "C" */
#endif

#endif  /* GWP_DEC_VP8_FRAME_H_ */

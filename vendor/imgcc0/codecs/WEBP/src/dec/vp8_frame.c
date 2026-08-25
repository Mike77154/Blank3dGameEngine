#include "vp8_frame.h"

#include "../utils/endian.h"
#include "../utils/common.h"

GWPStatusCode GWPVP8ParseFrameHeader(const GWPu8* data,
                                     GWPu32 data_size,
                                     GWPVP8FrameHeader* header) {
  GWPu32 tag;
  GWPu32 header_size;
  GWPu32 total_needed;
  GWPu32 tmp;
  if (data == 0 || header == 0) return GWP_STATUS_INVALID_PARAM;
  GWPZero(header, (GWPu32)sizeof(*header));
  if (data_size < 3u) return GWP_STATUS_TRUNCATED_DATA;

  tag = ((GWPu32)data[0]) | ((GWPu32)data[1] << 8) | ((GWPu32)data[2] << 16);
  header->tag.key_frame = ((tag & 1u) == 0u) ? GWP_TRUE : GWP_FALSE;
  header->tag.version = (tag >> 1) & 0x7u;
  header->tag.show_frame = ((tag >> 4) & 1u) ? GWP_TRUE : GWP_FALSE;
  header->tag.first_partition_size = (tag >> 5) & 0x7ffffu;

  header_size = header->tag.key_frame ? 10u : 3u;
  if (data_size < header_size) return GWP_STATUS_TRUNCATED_DATA;

  if (!header->tag.key_frame) {
    return GWP_STATUS_UNSUPPORTED_FEATURE;
  }

  if (data[3] != 0x9du || data[4] != 0x01u || data[5] != 0x2au) {
    return GWP_STATUS_BITSTREAM_ERROR;
  }

  tmp = GWPReadLE16(data + 6);
  header->key.width = tmp & 0x3fffu;
  header->key.horizontal_scale = (tmp >> 14) & 0x3u;
  tmp = GWPReadLE16(data + 8);
  header->key.height = tmp & 0x3fffu;
  header->key.vertical_scale = (tmp >> 14) & 0x3u;

  if (header->key.width == 0u || header->key.height == 0u) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }
  if (header->key.width > GWP_MAX_IMAGE_WIDTH ||
      header->key.height > GWP_MAX_IMAGE_HEIGHT) {
    return GWP_STATUS_BAD_DIMENSIONS;
  }

  if (!GWPAddU32(header_size, header->tag.first_partition_size, &total_needed)) {
    return GWP_STATUS_LIMIT_EXCEEDED;
  }
  if (total_needed > data_size) return GWP_STATUS_TRUNCATED_DATA;

  header->partitions.uncompressed_chunk.bytes = data;
  header->partitions.uncompressed_chunk.size = header_size;
  header->partitions.first_partition.bytes = data + header_size;
  header->partitions.first_partition.size = header->tag.first_partition_size;
  return GWP_STATUS_OK;
}

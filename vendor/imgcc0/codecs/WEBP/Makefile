CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Wno-unused-function
CPPFLAGS ?=
INCLUDES = -Isrc

UTILS_OBJS = \
	src/utils/common.o \
	src/utils/arena.o \
	src/utils/bit_reader.o \
	src/utils/huffman.o

DEMUX_OBJS = \
	src/demux/demux.o \
	src/demux/anim_decode.o

DEC_OBJS = \
	src/dec/webp_dec.o \
	src/dec/alpha_dec.o \
	src/dec/vp8_bool.o \
	src/dec/vp8_frame.o \
	src/dec/vp8_probdata.o \
	src/dec/vp8_tree.o \
	src/dec/vp8_entropy.o \
	src/dec/vp8_modes.o \
	src/dec/vp8_quant.o \
	src/dec/vp8_tokens.o \
	src/dec/vp8_residual.o \
	src/dec/vp8_transform.o \
	src/dec/vp8_recon.o \
	src/dec/vp8_transform.o \
	src/dec/vp8_filter.o \
	src/dec/vp8_yuv.o \
	src/dec/vp8_picture.o \
	src/dec/vp8_dec.o \
	src/dec/vp8l_dec.o

ENC_OBJS = \
	src/dec/vp8_probdata.o \
	src/dec/vp8_recon.o \
	src/dec/vp8_transform.o \
	src/enc/bit_writer.o \
	src/enc/vp8_bool_enc.o \
	src/enc/vp8_tree_enc.o \
	src/enc/vp8_tokens_enc.o \
	src/enc/vp8_native_enc.o \
	src/enc/huffman_enc.o \
	src/enc/vp8l_enc.o \
	src/enc/vp8_lossy_plan.o \
	src/enc/vp8_intra_emit.o \
	src/enc/tool_bridge.o \
	src/enc/webp_enc.o

MUX_OBJS = \
	src/mux/mux.o \
	src/mux/anim_encode.o

ALL_OBJS = $(UTILS_OBJS) $(DEMUX_OBJS) $(DEC_OBJS) $(ENC_OBJS) $(MUX_OBJS)

all: src/libgiffywebpdecoder.a src/libgiffywebpenc.a src/demux/libgiffywebpdemux.a src/mux/libgiffywebpmux.a \
	examples/gwpinfo examples/gwpdecode examples/gwpvp8probe examples/gwpvp8kernels examples/gwpdumpyuv \
	examples/gwpanimdump examples/gwpanimux examples/gwpencode examples/gwpanimframes examples/gwpvp8plan examples/gwpvp8emit examples/gwpvp8bitstream

src/libgiffywebpdecoder.a: $(UTILS_OBJS) $(DEMUX_OBJS) $(DEC_OBJS)
	$(AR) rcs $@ $^

src/libgiffywebpenc.a: $(UTILS_OBJS) $(ENC_OBJS) src/dec/vp8_quant.o
	$(AR) rcs $@ $^

src/demux/libgiffywebpdemux.a: $(UTILS_OBJS) $(DEMUX_OBJS)
	$(AR) rcs $@ $^

src/mux/libgiffywebpmux.a: $(UTILS_OBJS) $(DEMUX_OBJS) $(ENC_OBJS) $(MUX_OBJS) src/dec/vp8_quant.o
	$(AR) rcs $@ $^

examples/gwpinfo: examples/gwpinfo.o src/libgiffywebpdecoder.a
	$(CC) $(CFLAGS) -o $@ examples/gwpinfo.o src/libgiffywebpdecoder.a

examples/gwpdecode: examples/gwpdecode.o src/libgiffywebpdecoder.a
	$(CC) $(CFLAGS) -o $@ examples/gwpdecode.o src/libgiffywebpdecoder.a

examples/gwpvp8probe: examples/gwpvp8probe.o src/libgiffywebpdecoder.a
	$(CC) $(CFLAGS) -o $@ examples/gwpvp8probe.o src/libgiffywebpdecoder.a

examples/gwpvp8kernels: examples/gwpvp8kernels.o src/libgiffywebpdecoder.a
	$(CC) $(CFLAGS) -o $@ examples/gwpvp8kernels.o src/libgiffywebpdecoder.a

examples/gwpdumpyuv: examples/gwpdumpyuv.o src/libgiffywebpdecoder.a
	$(CC) $(CFLAGS) -o $@ examples/gwpdumpyuv.o src/libgiffywebpdecoder.a

examples/gwpanimdump: examples/gwpanimdump.o src/libgiffywebpdecoder.a
	$(CC) $(CFLAGS) -o $@ examples/gwpanimdump.o src/libgiffywebpdecoder.a

examples/gwpanimux: examples/gwpanimux.o src/mux/libgiffywebpmux.a
	$(CC) $(CFLAGS) -o $@ examples/gwpanimux.o src/mux/libgiffywebpmux.a

examples/gwpencode: examples/gwpencode.o src/libgiffywebpenc.a
	$(CC) $(CFLAGS) -o $@ examples/gwpencode.o src/libgiffywebpenc.a

examples/gwpanimframes: examples/gwpanimframes.o src/mux/libgiffywebpmux.a
	$(CC) $(CFLAGS) -o $@ examples/gwpanimframes.o src/mux/libgiffywebpmux.a

examples/gwpvp8plan: examples/gwpvp8plan.o src/libgiffywebpenc.a
	$(CC) $(CFLAGS) -o $@ examples/gwpvp8plan.o src/libgiffywebpenc.a

examples/gwpvp8emit: examples/gwpvp8emit.o src/libgiffywebpenc.a
	$(CC) $(CFLAGS) -o $@ examples/gwpvp8emit.o src/libgiffywebpenc.a

examples/gwpvp8bitstream: examples/gwpvp8bitstream.o src/libgiffywebpenc.a
	$(CC) $(CFLAGS) -o $@ examples/gwpvp8bitstream.o src/libgiffywebpenc.a

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(ALL_OBJS) examples/*.o examples/gwpinfo examples/gwpdecode examples/gwpvp8probe \
	      examples/gwpvp8kernels examples/gwpdumpyuv examples/gwpanimdump examples/gwpanimux \
	      examples/gwpencode examples/gwpanimframes examples/gwpvp8plan examples/gwpvp8emit examples/gwpvp8bitstream src/libgiffywebpdecoder.a src/libgiffywebpenc.a \
	      src/demux/libgiffywebpdemux.a src/mux/libgiffywebpmux.a

.PHONY: all clean
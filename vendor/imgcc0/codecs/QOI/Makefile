CC ?= cc
AR ?= ar
RANLIB ?= ranlib
CFLAGS ?= -O2 -std=c89 -Wall -Wextra -pedantic
CPPFLAGS ?= -Iinclude -Isrc -Itests -Itests/third_party

LIB = libqoi89.a

LIB_OBJS = \
	src/qoi89_util.o \
	src/qoi89_desc.o \
	src/qoi89_header.o \
	src/qoi89_size.o \
	src/qoi89_encode.o \
	src/qoi89_decode.o \
	src/qoi89_stream_encode.o \
	src/qoi89_stream_decode.o \
	src/qoi89_io.o \
	src/qoi89_pipeline.o

TEST_COMMON_OBJS = tests/test_common.o
TEST_CORE = test_qoi89
TEST_CORE_OBJS = tests/test_qoi89.o
TEST_STREAM = test_qoi89_stream
TEST_STREAM_OBJS = tests/test_qoi89_stream.o
TEST_PIPE = test_qoi89_pipeline
TEST_PIPE_OBJS = tests/test_qoi89_pipeline.o
TEST_IO = test_qoi89_io
TEST_IO_OBJS = tests/test_qoi89_io.o
TEST_REF = test_qoi89_ref
TEST_REF_OBJS = tests/test_qoi89_ref.o
EXAMPLE = example_memory
EXAMPLE_OBJS = examples/example_memory.o
EXAMPLE_STREAM = example_streaming
EXAMPLE_STREAM_OBJS = examples/example_streaming.o
EXAMPLE_IO = example_callbacks
EXAMPLE_IO_OBJS = examples/example_callbacks.o
EXAMPLE_PIPE = example_pipeline
EXAMPLE_PIPE_OBJS = examples/example_pipeline.o

.PHONY: all clean test

all: $(LIB) $(TEST_CORE) $(TEST_STREAM) $(TEST_PIPE) $(TEST_IO) $(TEST_REF) \
	$(EXAMPLE) $(EXAMPLE_STREAM) $(EXAMPLE_IO) $(EXAMPLE_PIPE)

$(LIB): $(LIB_OBJS)
	$(AR) rcs $@ $(LIB_OBJS)
	$(RANLIB) $@

src/qoi89_util.o: src/qoi89_util.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_desc.o: src/qoi89_desc.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_header.o: src/qoi89_header.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_size.o: src/qoi89_size.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_encode.o: src/qoi89_encode.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_decode.o: src/qoi89_decode.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_stream_encode.o: src/qoi89_stream_encode.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_stream_decode.o: src/qoi89_stream_decode.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_io.o: src/qoi89_io.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

src/qoi89_pipeline.o: src/qoi89_pipeline.c include/qoi89.h src/qoi89_internal.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/test_common.o: tests/test_common.c tests/test_common.h include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/test_qoi89.o: tests/test_qoi89.c tests/test_common.h include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/test_qoi89_stream.o: tests/test_qoi89_stream.c tests/test_common.h include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/test_qoi89_pipeline.o: tests/test_qoi89_pipeline.c tests/test_common.h include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/test_qoi89_io.o: tests/test_qoi89_io.c tests/test_common.h include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

tests/test_qoi89_ref.o: tests/test_qoi89_ref.c tests/test_common.h include/qoi89.h tests/third_party/qoi_ref.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

examples/example_memory.o: examples/example_memory.c include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

examples/example_streaming.o: examples/example_streaming.c include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

examples/example_callbacks.o: examples/example_callbacks.c include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

examples/example_pipeline.o: examples/example_pipeline.c include/qoi89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TEST_CORE): $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_CORE_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_CORE_OBJS)

$(TEST_STREAM): $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_STREAM_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_STREAM_OBJS)

$(TEST_PIPE): $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_PIPE_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_PIPE_OBJS)

$(TEST_IO): $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_IO_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_IO_OBJS)

$(TEST_REF): $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_REF_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_REF_OBJS)

$(EXAMPLE): $(LIB_OBJS) $(EXAMPLE_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(EXAMPLE_OBJS)

$(EXAMPLE_STREAM): $(LIB_OBJS) $(EXAMPLE_STREAM_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(EXAMPLE_STREAM_OBJS)

$(EXAMPLE_IO): $(LIB_OBJS) $(EXAMPLE_IO_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(EXAMPLE_IO_OBJS)

$(EXAMPLE_PIPE): $(LIB_OBJS) $(EXAMPLE_PIPE_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OBJS) $(EXAMPLE_PIPE_OBJS)

test: $(TEST_CORE) $(TEST_STREAM) $(TEST_PIPE) $(TEST_IO) $(TEST_REF)
	./$(TEST_CORE)
	./$(TEST_STREAM)
	./$(TEST_PIPE)
	./$(TEST_IO)
	./$(TEST_REF)

clean:
	rm -f $(LIB_OBJS) $(TEST_COMMON_OBJS) $(TEST_CORE_OBJS) $(TEST_STREAM_OBJS) $(TEST_PIPE_OBJS) \
		$(TEST_IO_OBJS) $(TEST_REF_OBJS) $(EXAMPLE_OBJS) $(EXAMPLE_STREAM_OBJS) $(EXAMPLE_IO_OBJS) \
		$(EXAMPLE_PIPE_OBJS) $(LIB) $(TEST_CORE) $(TEST_STREAM) $(TEST_PIPE) $(TEST_IO) $(TEST_REF) \
		$(EXAMPLE) $(EXAMPLE_STREAM) $(EXAMPLE_IO) $(EXAMPLE_PIPE)

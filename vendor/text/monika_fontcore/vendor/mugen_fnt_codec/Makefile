CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -O2
INCLUDES := -Iinclude -Isrc -Ithird_party/puff
OBJDIR := build/obj
LIBDIR := build/lib
BINDIR := build/bin

CORE_OBJS := \
	$(OBJDIR)/mfont_core.o \
	$(OBJDIR)/mfont_text.o \
	$(OBJDIR)/mfont_util.o

CODEC_OBJS := \
	$(OBJDIR)/zlfont.o \
	$(OBJDIR)/fntpng.o \
	$(OBJDIR)/pcxfnt.o \
	$(OBJDIR)/fntbump.o \
	$(OBJDIR)/puff.o

TOOL_OBJS := $(OBJDIR)/mfonttool.o

ALL_OBJS := $(CORE_OBJS) $(CODEC_OBJS)

.PHONY: all clean dirs

all: dirs \
	$(LIBDIR)/libzlfont.a \
	$(LIBDIR)/libfntpng.a \
	$(LIBDIR)/libpcxfnt.a \
	$(LIBDIR)/libfntbump.a \
	$(LIBDIR)/libmfont.a \
	$(BINDIR)/mfonttool

dirs:
	@mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR)

$(OBJDIR)/%.o: src/%.c | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/mfonttool.o: tools/mfonttool.c | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/puff.o: third_party/puff/puff.c | dirs
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIBDIR)/libzlfont.a: $(OBJDIR)/zlfont.o $(OBJDIR)/puff.o | dirs
	$(AR) rcs $@ $^

$(LIBDIR)/libfntpng.a: $(OBJDIR)/fntpng.o | dirs
	$(AR) rcs $@ $^

$(LIBDIR)/libpcxfnt.a: $(OBJDIR)/pcxfnt.o | dirs
	$(AR) rcs $@ $^

$(LIBDIR)/libfntbump.a: $(OBJDIR)/fntbump.o | dirs
	$(AR) rcs $@ $^

$(LIBDIR)/libmfont.a: $(ALL_OBJS) | dirs
	$(AR) rcs $@ $^

$(BINDIR)/mfonttool: $(ALL_OBJS) $(TOOL_OBJS) | dirs
	$(CC) $(CFLAGS) $(TOOL_OBJS) $(ALL_OBJS) -o $@

clean:
	rm -rf build

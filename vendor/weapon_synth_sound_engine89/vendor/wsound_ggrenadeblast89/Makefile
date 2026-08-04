CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
ARFLAGS ?= rcs

SRC = \
	src/wsound_gtypes89.c \
	src/wsound_geq6_89.c \
	src/wsound_gdist89.c \
	src/wsound_gchorus89.c \
	src/wsound_greverb89.c \
	src/wsound_ggrenadeblast89.c

OBJ = $(SRC:.c=.o)

all: libwsound_ggrenadeblast89.a test_c89 test_transients test_speaker_safe render_previews

libwsound_ggrenadeblast89.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

test_c89: tests/test_c89.c libwsound_ggrenadeblast89.a
	$(CC) $(CFLAGS) -o $@ tests/test_c89.c libwsound_ggrenadeblast89.a

test_transients: tests/test_transients.c libwsound_ggrenadeblast89.a
	$(CC) $(CFLAGS) -o $@ tests/test_transients.c libwsound_ggrenadeblast89.a

test_speaker_safe: tests/test_speaker_safe.c libwsound_ggrenadeblast89.a
	$(CC) $(CFLAGS) -o $@ tests/test_speaker_safe.c libwsound_ggrenadeblast89.a

render_previews: demo/render_previews.c libwsound_ggrenadeblast89.a
	$(CC) $(CFLAGS) -o $@ demo/render_previews.c libwsound_ggrenadeblast89.a

check: test_c89 test_transients test_speaker_safe
	./test_c89
	./test_transients
	./test_speaker_safe
	@if nm -u libwsound_ggrenadeblast89.a | grep -E 'malloc|realloc|free|calloc|pow|sin|cos|exp|sqrt'; then \
		echo "forbidden external symbol found"; exit 1; \
	else \
		echo "forbidden external symbol scan: clean"; \
	fi
	@if grep -R -n -E '\b(float|double|malloc|realloc|calloc|free)\b' include src; then \
		echo "forbidden token found in core"; exit 1; \
	else \
		echo "forbidden core token scan: clean"; \
	fi

previews: render_previews
	./render_previews

clean:
	rm -f $(OBJ) libwsound_ggrenadeblast89.a test_c89 test_transients test_speaker_safe render_previews

.PHONY: all check previews clean

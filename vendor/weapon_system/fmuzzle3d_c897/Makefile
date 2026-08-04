CC = gcc
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror -O2

all: demo_render test_fmuzzle89

demo_render: fmuzzle89.c fmuzzle89.h demo_render.c
	$(CC) $(CFLAGS) fmuzzle89.c demo_render.c -o demo_render

test_fmuzzle89: fmuzzle89.c fmuzzle89.h test_fmuzzle89.c
	$(CC) $(CFLAGS) fmuzzle89.c test_fmuzzle89.c -o test_fmuzzle89

preview: demo_render
	mkdir -p preview_frames
	./demo_render

test: test_fmuzzle89
	./test_fmuzzle89

clean:
	rm -f demo_render test_fmuzzle89
	rm -f preview_frames/frame_*.ppm

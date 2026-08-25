CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -O2
INCLUDES = -Iinclude
CORE = src/gskybox89.c
ASSETS = src/gskybox89_assets.c

all: demo_null_backend test_build demo_preview_ppm demo_asset_mapper demo_asset_preview_ppm

demo_null_backend: $(CORE) demo/demo_null_backend.c include/gskybox89.h
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) demo/demo_null_backend.c -o demo_null_backend

test_build: $(CORE) $(ASSETS) tests/test_build.c include/gskybox89.h include/gskybox89_assets.h
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) $(ASSETS) tests/test_build.c -o test_build

demo_preview_ppm: $(CORE) demo/demo_preview_ppm.c include/gskybox89.h
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) demo/demo_preview_ppm.c -o demo_preview_ppm

demo_asset_mapper: $(CORE) $(ASSETS) demo/demo_asset_mapper.c include/gskybox89.h include/gskybox89_assets.h
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) $(ASSETS) demo/demo_asset_mapper.c -o demo_asset_mapper

demo_asset_preview_ppm: $(CORE) $(ASSETS) demo/demo_asset_preview_ppm.c include/gskybox89.h include/gskybox89_assets.h
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) $(ASSETS) demo/demo_asset_preview_ppm.c -o demo_asset_preview_ppm

check: test_build demo_null_backend demo_asset_mapper
	./test_build
	./demo_null_backend
	./demo_asset_mapper skyft.tga skybk.tga skyrt.tga skylf.tga skyup.tga skydn.tga px.png nx.png py.png ny.png pz.png nz.png

clean:
	rm -f demo_null_backend test_build demo_preview_ppm demo_asset_mapper demo_asset_preview_ppm \
	      demo_null_backend.exe test_build.exe demo_preview_ppm.exe demo_asset_mapper.exe demo_asset_preview_ppm.exe \
	      preview_gskybox89.ppm preview_gskybox89.png preview_asset_*.ppm preview_asset_*.png

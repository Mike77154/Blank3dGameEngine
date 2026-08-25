CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude

BUNDLE_INCLUDES = \
	-Igscopebundle89/include -Igscopeprovider89/include -Igscopeini89/include \
	-Igscopeanim89/include \
	-Imodules/gscopepaint89/include -Imodules/gscopevector89/include \
	-Imodules/gscoperaster89/include -Imodules/gscopebars89/include \
	-Imodules/gscopepresets89/include -Igsniperhud89/include \
	-Igtelescopiczoom89/include -Iinclude

ALL_LIBS = libgscopeini89.a libgscopeanim89.a libgtelescopiczoom89.a libgsway89.a \
	libgsniperhud89.a libgaimquery89.a libgscopeprovider89.a libgscopebundle89.a

all: $(ALL_LIBS) demo_split

gscopeini89.o: gscopeini89/src/gscopeini89.c
	$(CC) $(CFLAGS) -c $< -o $@

gscopeanim89.o: gscopeanim89/src/gscopeanim89.c
	$(CC) $(CFLAGS) $(BUNDLE_INCLUDES) -c $< -o $@

gtelescopiczoom89.o: gtelescopiczoom89/src/gtelescopiczoom89.c
	$(CC) $(CFLAGS) -c $< -o $@

gsway89.o: gsway89/src/gsway89.c
	$(CC) $(CFLAGS) -c $< -o $@

gsniperhud89.o: gsniperhud89/src/gsniperhud89.c
	$(CC) $(CFLAGS) -c $< -o $@

gaimquery89.o: gaimquery89/src/gaimquery89.c
	$(CC) $(CFLAGS) -c $< -o $@

gscopeprovider89.o: gscopeprovider89/src/gscopeprovider89.c
	$(CC) $(CFLAGS) $(BUNDLE_INCLUDES) -c $< -o $@

gscopebundle89.o: gscopebundle89/src/gscopebundle89.c
	$(CC) $(CFLAGS) $(BUNDLE_INCLUDES) -c $< -o $@

libgscopeini89.a: gscopeini89.o
	$(AR) rcs $@ $<

libgscopeanim89.a: gscopeanim89.o
	$(AR) rcs $@ $<

libgtelescopiczoom89.a: gtelescopiczoom89.o
	$(AR) rcs $@ $<

libgsway89.a: gsway89.o
	$(AR) rcs $@ $<

libgsniperhud89.a: gsniperhud89.o
	$(AR) rcs $@ $<

libgaimquery89.a: gaimquery89.o
	$(AR) rcs $@ $<

libgscopeprovider89.a: gscopeprovider89.o
	$(AR) rcs $@ $<

libgscopebundle89.a: gscopebundle89.o
	$(AR) rcs $@ $<

demo_split: examples/demo_split.c libgscopeini89.a libgtelescopiczoom89.a \
	libgsway89.a libgsniperhud89.a libgaimquery89.a
	$(CC) $(CFLAGS) examples/demo_split.c \
		libgtelescopiczoom89.a libgsway89.a \
		libgsniperhud89.a libgaimquery89.a libgscopeini89.a -o $@

provider_bundle_smoke: tests/provider_bundle_smoke.c
	$(CC) $(CFLAGS) $(BUNDLE_INCLUDES) \
		gscopeini89/src/gscopeini89.c \
		gscopeanim89/src/gscopeanim89.c \
		modules/gscopepaint89/src/gscopepaint89.c \
		modules/gscopevector89/src/gscopevector89.c \
		modules/gscoperaster89/src/gscoperaster89.c \
		modules/gscopebars89/src/gscopebars89.c \
		modules/gscopepresets89/src/gscopepresets89.c \
		gsniperhud89/src/gsniperhud89.c \
		gtelescopiczoom89/src/gtelescopiczoom89.c \
		gscopeprovider89/src/gscopeprovider89.c \
		gscopebundle89/src/gscopebundle89.c \
		tests/provider_bundle_smoke.c -o tests/provider_bundle_smoke

provider_host_example: examples/provider_host_example.c
	$(CC) $(CFLAGS) $(BUNDLE_INCLUDES) \
		gscopeini89/src/gscopeini89.c \
		gscopeanim89/src/gscopeanim89.c \
		modules/gscopepaint89/src/gscopepaint89.c \
		modules/gscopevector89/src/gscopevector89.c \
		modules/gscoperaster89/src/gscoperaster89.c \
		modules/gscopebars89/src/gscopebars89.c \
		modules/gscopepresets89/src/gscopepresets89.c \
		gsniperhud89/src/gsniperhud89.c \
		gtelescopiczoom89/src/gtelescopiczoom89.c \
		gscopeprovider89/src/gscopeprovider89.c \
		gscopebundle89/src/gscopebundle89.c \
		examples/provider_host_example.c -o examples/provider_host_example

animation_smoke: tests/animation_smoke.c
	$(CC) $(CFLAGS) $(BUNDLE_INCLUDES) \
		gscopeini89/src/gscopeini89.c \
		gscopeanim89/src/gscopeanim89.c \
		modules/gscopepaint89/src/gscopepaint89.c \
		modules/gscopevector89/src/gscopevector89.c \
		modules/gscopepresets89/src/gscopepresets89.c \
		tests/animation_smoke.c -o tests/animation_smoke

golden_master:
	mkdir -p tests/golden/out_legacy tests/golden/out_ini
	$(CC) $(CFLAGS) -Itests/golden/legacy \
		tests/golden/legacy/gscopepresets89_legacy.c \
		tests/golden/golden_preview_runtime.c tests/golden/golden_preview_main.c \
		-o tests/golden/golden_legacy
	$(CC) $(CFLAGS) -DGOLDEN_RECIPE_BUILD \
		-Imodules/gscopepresets89/include -Imodules/gscopepresets89/deps \
		modules/gscopepresets89/src/gscopepresets89.c \
		tests/golden/golden_preview_runtime.c tests/golden/golden_preview_main.c \
		-o tests/golden/golden_ini
	$(CC) $(CFLAGS) tests/golden/golden_compare.c -o tests/golden/golden_compare
	./tests/golden/golden_legacy tests/golden/out_legacy
	./tests/golden/golden_ini tests/golden/out_ini config/reticles/catalog.ini
	./tests/golden/golden_compare tests/golden/out_legacy tests/golden/out_ini | tee tests/GOLDEN_MASTER_V0_4.txt

test: all provider_bundle_smoke animation_smoke golden_master
	./demo_split >/dev/null
	$(MAKE) -C modules/gscopepresets89 test
	./tests/provider_bundle_smoke
	./tests/animation_smoke

clean:
	rm -f *.o *.a demo_split demo_split.exe
	rm -f tests/provider_bundle_smoke tests/provider_bundle_smoke.exe tests/animation_smoke tests/animation_smoke.exe
	rm -f tests/golden/golden_legacy tests/golden/golden_ini tests/golden/golden_compare
	rm -f examples/provider_host_example examples/provider_host_example.exe
	$(MAKE) -C modules/gscopepresets89 clean >/dev/null 2>&1 || true

.PHONY: all test clean provider_bundle_smoke provider_host_example animation_smoke golden_master

CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2

EXP = vendor/wsound_expansion_pack89_v1_2_sendfix
WORLD = vendor/wsound_weapon_world89_v1_0
ACOUSTIC = vendor/wsound_acoustic_phase12_89
PHASE3 = vendor/wsound_phase3_89

INCLUDES = \
 -Iinclude \
 -Ivendor/gpaah89/include \
 -Ivendor/chuecka89/include \
 -Ivendor/gpump89/include \
 -Ivendor/shotpumpkin89/include \
 -Ivendor/gweaponfoley89/include \
 -Ivendor/gklek89/include \
 -Ivendor/wmagazine89/include \
 -Ivendor/grocketwhistle89 \
 -Ivendor/grocketspin89/include \
 -Ivendor/ggunmach89/include \
 -Ivendor/gguntuberotator89/include \
 -Ivendor/ggatlingwhistle89/include \
 -Ivendor/gshotgunsequence89/include \
 -Ivendor/gshotguneq89/include \
 -Ivendor/gvoice89/include \
 -Ivendor/gweaponvoice89/include \
 -Ivendor/gtinkle89/include \
 -Ivendor/gfire89/include \
 -Ivendor/gbulletair89/include \
 -Ivendor/wsound_ggrenadeblast89/include \
 -Ivendor/wsound_rocketblast89/include \
 -Ivendor/gweapon_cushions89/gweaponbody89/include \
 -Ivendor/gweapon_cushions89/gmuzzlegas89/include \
 -Ivendor/gweapon_cushions89/gballisticcrack89/include \
 -Ivendor/gweapon_cushions89/glatetail89/include \
 -Ivendor/gweapon_cushions89/gcinemathump89/include \
 -I$(EXP)/wsoundmuzzledevice89/include \
 -I$(EXP)/wsoundaero89/include \
 -I$(EXP)/wsoundfriction89/include \
 -I$(EXP)/wsoundparticles89/include \
 -I$(EXP)/wsoundammo89/include \
 -I$(EXP)/wsoundbeltfeed89/include \
 -I$(EXP)/wsoundoutdoor89/include \
 -I$(EXP)/wsoundportal89/include \
 -I$(EXP)/wsounddoppler89/include \
 -I$(EXP)/wsoundspatial89/include \
 -I$(EXP)/wsoundthermal89/include \
 -I$(EXP)/wsoundlistener89/include \
 -I$(EXP)/wsoundmask89/include \
 -I$(WORLD)/include \
 -I$(WORLD)/wsounddna89/include \
 -I$(WORLD)/wsoundreceiver89/include \
 -I$(WORLD)/wsoundprojectile89/include \
 -I$(WORLD)/wsoundprop89/include \
 -I$(WORLD)/wsoundroom89/include \
 -I$(WORLD)/wsoundaction89/include \
 -I$(WORLD)/wsoundimpact89/include \
 -I$(WORLD)/wsoundricochet89/include \
 -I$(WORLD)/wsoundcombatbus89/include \
 -I$(ACOUSTIC)/include \
 -I$(PHASE3)/include

BASE_SYNTH_SOURCES = \
 vendor/gpaah89/src/gpaah89.c \
 vendor/chuecka89/src/chuecka89.c \
 vendor/gpump89/src/gpump89.c \
 vendor/shotpumpkin89/src/shotpumpkin89.c \
 vendor/gweaponfoley89/src/gweaponfoley89.c \
 vendor/gklek89/src/gklek89.c \
 vendor/wmagazine89/src/wmagazine89.c \
 vendor/grocketwhistle89/grocketwhistle89.c \
 vendor/grocketspin89/src/grocketspin89.c \
 vendor/ggunmach89/src/ggunmach89.c \
 vendor/gguntuberotator89/src/gguntuberotator89.c \
 vendor/ggatlingwhistle89/src/ggatlingwhistle89.c \
 vendor/gweapon_cushions89/gweaponbody89/src/gweaponbody89.c \
 vendor/gweapon_cushions89/gmuzzlegas89/src/gmuzzlegas89.c \
 vendor/gweapon_cushions89/gballisticcrack89/src/gballisticcrack89.c \
 vendor/gweapon_cushions89/glatetail89/src/glatetail89.c \
 vendor/gweapon_cushions89/gcinemathump89/src/gcinemathump89.c \
 vendor/gshotgunsequence89/src/gshotgunsequence89.c \
 vendor/gshotguneq89/src/gshotguneq89.c \
 vendor/gtinkle89/src/gtinkle89.c \
 vendor/gfire89/src/gfire89.c \
 vendor/gfire89/src/gfire89_fx.c

VOICE_SOURCES = \
 vendor/gvoice89/src/gvoice89.c \
 vendor/gweaponvoice89/src/gweaponvoice89.c

BASE_BRIDGE_SOURCES = \
 src/gsynthreport89.c \
 src/gsynthsoundengine89.c

ROCKETBLAST_SOURCES = \
 vendor/wsound_rocketblast89/src/wsrb89_dsp.c \
 vendor/wsound_rocketblast89/src/wsrb89_presets.c \
 vendor/wsound_rocketblast89/src/wsound_rocketblast89.c

ORDNANCE_SOURCES = \
 vendor/gbulletair89/src/gbulletair89.c \
 vendor/wsound_ggrenadeblast89/src/wsound_gtypes89.c \
 vendor/wsound_ggrenadeblast89/src/wsound_geq6_89.c \
 vendor/wsound_ggrenadeblast89/src/wsound_gdist89.c \
 vendor/wsound_ggrenadeblast89/src/wsound_gchorus89.c \
 vendor/wsound_ggrenadeblast89/src/wsound_greverb89.c \
 vendor/wsound_ggrenadeblast89/src/wsound_ggrenadeblast89.c \
 $(ROCKETBLAST_SOURCES) \
 src/gsynthordnance89.c

EXPANSION_SOURCES = \
 $(EXP)/wsoundaero89/src/wsoundaero89.c \
 $(EXP)/wsoundammo89/src/wsoundammo89.c \
 $(EXP)/wsoundbeltfeed89/src/wsoundbeltfeed89.c \
 $(EXP)/wsounddoppler89/src/wsounddoppler89.c \
 $(EXP)/wsoundfriction89/src/wsoundfriction89.c \
 $(EXP)/wsoundlistener89/src/wsoundlistener89.c \
 $(EXP)/wsoundmask89/src/wsoundmask89.c \
 $(EXP)/wsoundmuzzledevice89/src/wsoundmuzzledevice89.c \
 $(EXP)/wsoundoutdoor89/src/wsoundoutdoor89.c \
 $(EXP)/wsoundparticles89/src/wsoundparticles89.c \
 $(EXP)/wsoundportal89/src/wsoundportal89.c \
 $(EXP)/wsoundspatial89/src/wsoundspatial89.c \
 $(EXP)/wsoundthermal89/src/wsoundthermal89.c \
 src/gsynthsoundexpansion89.c

WORLD_SOURCES = \
 $(WORLD)/wsoundaction89/src/wsoundaction89.c \
 $(WORLD)/wsoundcombatbus89/src/wsoundcombatbus89.c \
 $(WORLD)/wsounddna89/src/wsounddna89.c \
 $(WORLD)/wsoundimpact89/src/wsoundimpact89.c \
 $(WORLD)/wsoundprojectile89/src/wsoundprojectile89.c \
 $(WORLD)/wsoundprop89/src/wsoundprop89.c \
 $(WORLD)/wsoundreceiver89/src/wsoundreceiver89.c \
 $(WORLD)/wsoundricochet89/src/wsoundricochet89.c \
 $(WORLD)/wsoundroom89/src/wsoundroom89.c \
 $(ACOUSTIC)/src/wsoundacoustic89.c \
 $(PHASE3)/src/wsoundmetrics89.c \
 src/gsynthworld89.c

UNIFIED_FACADE_SOURCE = src/weapon_synth_sound_engine89.c
ALL_CORE_SOURCES = $(BASE_SYNTH_SOURCES) $(VOICE_SOURCES) $(BASE_BRIDGE_SOURCES) \
                   $(ORDNANCE_SOURCES) $(EXPANSION_SOURCES) $(WORLD_SOURCES) \
                   $(UNIFIED_FACADE_SOURCE)

LEGACY_REALTIME_TARGET = build/render_realtime_showcase
LEGACY_ORDNANCE_TARGET = build/render_ordnance_showcase
UNIFIED_PREVIEW_TARGET = build/render_unified_event_showcase
PHASE12_PREVIEW_TARGET = build/render_phase12_showcase
PHASE3_PREVIEW_TARGET = build/render_phase3_showcase
PUMP_LAYER_PREVIEW_TARGET = build/render_pump_layer_showcase
SHOTGUN_MASTER_PREVIEW_TARGET = build/render_shotgun_master_showcase
WEAPON_SEQUENCE_PREVIEW_TARGET = build/render_complete_weapon_sequences
ROCKET_WHISTLE_V12_PREVIEW_TARGET = build/render_rocket_whistle_v12_showcase
ROCKET_GATLING_PREVIEW_TARGET = build/render_rocket_spin_gatling_showcase
BASE_TEST_TARGET = build/test_gsynthsoundengine89
ORDNANCE_TEST_TARGET = build/test_gsynthordnance89
EXPANSION_TEST_TARGET = build/test_gsynthsoundexpansion89
UNIFIED_TEST_TARGET = build/test_weapon_synth_sound_engine89
PHASE12_TEST_TARGET = build/test_phase12_acoustics
PHASE3_TEST_TARGET = build/test_phase3_dna_metrics
PUMP_LAYER_TEST_TARGET = build/test_pump_layers
SHOTGUN_MASTER_TEST_TARGET = build/test_shotgun_master
WEAPON_HIERARCHY_TEST_TARGET = build/test_weapon_mix_hierarchy
MAGAZINE_WHISTLE_TEST_TARGET = build/test_magazine_whistle_integration
ROCKET_WHISTLE_VENDOR_TEST_TARGET = build/test_grocketwhistle89_v12
ROCKET_GATLING_TEST_TARGET = build/test_rocket_spin_gatling_integration
EXAMPLE_TARGET = build/game_engine_event_bridge.o
SHOTGUN_TEK_EXAMPLE_TARGET = build/shotgun_tek_provider_bridge.o

LIB_TARGET = build/libweapon_synth_sound_engine89.a
REPORT_LIB_TARGET = build/libgsynthreport89.a
BASE_BRIDGE_TARGET = build/libgsynthsoundengine89.a
ORDNANCE_BRIDGE_TARGET = build/libgsynthordnance89.a
EXPANSION_BRIDGE_TARGET = build/libgsynthsoundexpansion89.a
WORLD_BRIDGE_TARGET = build/libgsynthworld89.a
EVENT_FACADE_TARGET = build/libweapon_synth_event89.a

# Keep archive object paths deliberately short.  The previous layout mirrored the
# complete vendored source path below build/libobjs/.  On Win32/MinGW32 that can
# exceed legacy MAX_PATH when Blank3D itself lives in a long project directory
# (for example wsoundmuzzledevice89.o exceeded 260 characters).
#
# ALL_CORE_SOURCES currently has unique basenames; each flattened object gets an
# explicit dependency rule below, so the source path remains unambiguous while
# ar.exe only receives short build/libobjs/<basename>.o paths.
obj_for = build/libobjs/$(notdir $(patsubst %.c,%.o,$(1)))

LIB_OBJECTS = $(foreach s,$(ALL_CORE_SOURCES),$(call obj_for,$(s)))
REPORT_OBJECTS = $(foreach s,$(VOICE_SOURCES) $(BASE_SYNTH_SOURCES) src/gsynthreport89.c,$(call obj_for,$(s)))
BASE_BRIDGE_OBJECTS = $(foreach s,$(VOICE_SOURCES) vendor/gtinkle89/src/gtinkle89.c vendor/gfire89/src/gfire89.c src/gsynthsoundengine89.c,$(call obj_for,$(s)))
ORDNANCE_BRIDGE_OBJECTS = $(foreach s,$(VOICE_SOURCES) $(ORDNANCE_SOURCES),$(call obj_for,$(s)))
EXPANSION_BRIDGE_OBJECTS = $(foreach s,$(EXPANSION_SOURCES),$(call obj_for,$(s)))
WORLD_BRIDGE_OBJECTS = $(foreach s,$(VOICE_SOURCES) $(WORLD_SOURCES),$(call obj_for,$(s)))
EVENT_FACADE_OBJECTS = $(foreach s,$(UNIFIED_FACADE_SOURCE),$(call obj_for,$(s)))

# Flattening depends on unique source basenames.  Fail loudly if a future vendor
# introduces a duplicate instead of silently archiving the wrong object.
ifneq ($(words $(LIB_OBJECTS)),$(words $(sort $(LIB_OBJECTS))))
$(error weapon_synth_sound_engine89: duplicate source basenames cannot be flattened safely)
endif

# Generate one explicit rule per source.  This avoids reconstructing a source
# path from a flattened object basename and keeps the archive prerequisites
# correct under GNU make -j as well as serial builds.
define make_libobj_rule
$(call obj_for,$(1)): $(1)
	mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@
endef

$(foreach s,$(ALL_CORE_SOURCES),$(eval $(call make_libobj_rule,$(s))))

all: libs demos tests examples

libs: $(LIB_TARGET) $(REPORT_LIB_TARGET) $(BASE_BRIDGE_TARGET) \
      $(ORDNANCE_BRIDGE_TARGET) $(EXPANSION_BRIDGE_TARGET) \
      $(WORLD_BRIDGE_TARGET) $(EVENT_FACADE_TARGET)

demos: $(LEGACY_REALTIME_TARGET) $(LEGACY_ORDNANCE_TARGET) $(UNIFIED_PREVIEW_TARGET) $(PHASE12_PREVIEW_TARGET) $(PHASE3_PREVIEW_TARGET) $(PUMP_LAYER_PREVIEW_TARGET) $(SHOTGUN_MASTER_PREVIEW_TARGET) $(WEAPON_SEQUENCE_PREVIEW_TARGET) $(ROCKET_WHISTLE_V12_PREVIEW_TARGET) $(ROCKET_GATLING_PREVIEW_TARGET)

tests: $(BASE_TEST_TARGET) $(ORDNANCE_TEST_TARGET) $(EXPANSION_TEST_TARGET) $(UNIFIED_TEST_TARGET) $(PHASE12_TEST_TARGET) $(PHASE3_TEST_TARGET) $(PUMP_LAYER_TEST_TARGET) $(SHOTGUN_MASTER_TEST_TARGET) $(WEAPON_HIERARCHY_TEST_TARGET) $(MAGAZINE_WHISTLE_TEST_TARGET) $(ROCKET_WHISTLE_VENDOR_TEST_TARGET) $(ROCKET_GATLING_TEST_TARGET)

examples: $(EXAMPLE_TARGET) $(SHOTGUN_TEK_EXAMPLE_TARGET)

$(LEGACY_REALTIME_TARGET): src/render_realtime_showcase.c $(BASE_SYNTH_SOURCES) $(VOICE_SOURCES) src/gsynthsoundengine89.c $(ROCKETBLAST_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(WEAPON_SEQUENCE_PREVIEW_TARGET): src/render_complete_weapon_sequences.c $(BASE_SYNTH_SOURCES) $(VOICE_SOURCES) src/gsynthsoundengine89.c $(ORDNANCE_SOURCES) $(EXP)/wsoundammo89/src/wsoundammo89.c $(EXP)/wsoundbeltfeed89/src/wsoundbeltfeed89.c $(WORLD)/wsoundreceiver89/src/wsoundreceiver89.c
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(ROCKET_WHISTLE_V12_PREVIEW_TARGET): src/render_rocket_whistle_v12_showcase.c $(LIB_TARGET)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_TARGET) -o $@

$(ROCKET_GATLING_PREVIEW_TARGET): src/render_rocket_spin_gatling_showcase.c $(LIB_TARGET)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_TARGET) -o $@

$(LEGACY_ORDNANCE_TARGET): src/render_ordnance_showcase.c $(BASE_SYNTH_SOURCES) $(VOICE_SOURCES) src/gsynthsoundengine89.c $(ORDNANCE_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(UNIFIED_PREVIEW_TARGET): src/render_unified_event_showcase.c $(ALL_CORE_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@


$(PHASE12_PREVIEW_TARGET): src/render_phase12_showcase.c $(ALL_CORE_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(PHASE3_PREVIEW_TARGET): src/render_phase3_showcase.c $(ALL_CORE_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(PUMP_LAYER_PREVIEW_TARGET): src/render_pump_layer_showcase.c $(BASE_SYNTH_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(SHOTGUN_MASTER_PREVIEW_TARGET): src/render_shotgun_master_showcase.c $(BASE_SYNTH_SOURCES)
	mkdir -p build audio
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(BASE_TEST_TARGET): tests/test_gsynthsoundengine89.c $(BASE_SYNTH_SOURCES) $(VOICE_SOURCES) src/gsynthsoundengine89.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(ORDNANCE_TEST_TARGET): tests/test_gsynthordnance89.c $(VOICE_SOURCES) $(ORDNANCE_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(EXPANSION_TEST_TARGET): tests/test_gsynthsoundexpansion89.c $(EXPANSION_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(UNIFIED_TEST_TARGET): tests/test_weapon_synth_sound_engine89.c $(ALL_CORE_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@


$(PHASE12_TEST_TARGET): tests/test_phase12_acoustics.c $(ALL_CORE_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(PHASE3_TEST_TARGET): tests/test_phase3_dna_metrics.c $(ALL_CORE_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(PUMP_LAYER_TEST_TARGET): tests/test_pump_layers.c $(BASE_SYNTH_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(SHOTGUN_MASTER_TEST_TARGET): tests/test_shotgun_master.c $(BASE_SYNTH_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(EXAMPLE_TARGET): examples/game_engine_event_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SHOTGUN_TEK_EXAMPLE_TARGET): examples/shotgun_tek_provider_bridge.c
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_TARGET): $(LIB_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

$(REPORT_LIB_TARGET): $(REPORT_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

$(BASE_BRIDGE_TARGET): $(BASE_BRIDGE_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

$(ORDNANCE_BRIDGE_TARGET): $(ORDNANCE_BRIDGE_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

$(EXPANSION_BRIDGE_TARGET): $(EXPANSION_BRIDGE_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

$(WORLD_BRIDGE_TARGET): $(WORLD_BRIDGE_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

$(EVENT_FACADE_TARGET): $(EVENT_FACADE_OBJECTS)
	mkdir -p build
	$(AR) rcs $@ $^

render: demos
	./$(LEGACY_REALTIME_TARGET)
	./$(LEGACY_ORDNANCE_TARGET)
	./$(UNIFIED_PREVIEW_TARGET)
	./$(PHASE12_PREVIEW_TARGET)
	./$(PHASE3_PREVIEW_TARGET)
	./$(PUMP_LAYER_PREVIEW_TARGET)
	./$(SHOTGUN_MASTER_PREVIEW_TARGET)
	./$(WEAPON_SEQUENCE_PREVIEW_TARGET)
	./$(ROCKET_WHISTLE_V12_PREVIEW_TARGET)
	./$(ROCKET_GATLING_PREVIEW_TARGET)

test: tests
	./$(BASE_TEST_TARGET)
	./$(ORDNANCE_TEST_TARGET)
	./$(EXPANSION_TEST_TARGET)
	./$(UNIFIED_TEST_TARGET)
	./$(PHASE12_TEST_TARGET)
	./$(PHASE3_TEST_TARGET)
	./$(PUMP_LAYER_TEST_TARGET)
	./$(SHOTGUN_MASTER_TEST_TARGET)
	./$(WEAPON_HIERARCHY_TEST_TARGET)
	./$(MAGAZINE_WHISTLE_TEST_TARGET)
	./$(ROCKET_WHISTLE_VENDOR_TEST_TARGET)
	./$(ROCKET_GATLING_TEST_TARGET)

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-std=c89 -pedantic -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined' tests
	ASAN_OPTIONS=detect_leaks=0 ./$(BASE_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(ORDNANCE_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(EXPANSION_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(UNIFIED_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(PHASE12_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(PHASE3_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(PUMP_LAYER_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(SHOTGUN_MASTER_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(WEAPON_HIERARCHY_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(MAGAZINE_WHISTLE_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(ROCKET_WHISTLE_VENDOR_TEST_TARGET)
	ASAN_OPTIONS=detect_leaks=0 ./$(ROCKET_GATLING_TEST_TARGET)

$(WEAPON_HIERARCHY_TEST_TARGET): tests/test_weapon_mix_hierarchy.c $(VOICE_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(MAGAZINE_WHISTLE_TEST_TARGET): tests/test_magazine_whistle_integration.c $(ALL_CORE_SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(ROCKET_WHISTLE_VENDOR_TEST_TARGET): vendor/grocketwhistle89/test_api.c vendor/grocketwhistle89/grocketwhistle89.c
	mkdir -p build
	$(CC) $(CFLAGS) -Ivendor/grocketwhistle89 $^ -o $@

clean:
	rm -rf build

.PHONY: all libs demos tests examples render test sanitize clean

$(ROCKET_GATLING_TEST_TARGET): tests/test_rocket_spin_gatling_integration.c $(LIB_TARGET)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB_TARGET) -o $@

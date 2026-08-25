# Blank3D full-stack integrated runner - MSYS2 MinGW32
# C89 + fixed point + caller-owned/static storage.

# Command-line assignments still override these (`make CC=clang`), while the
# explicit defaults avoid GNU make's built-in `CC=cc` on MSYS2.
CC = gcc
AR = ar
TARGET = blank3d.exe
DEBUG_TARGET = blank3d_debug.exe
VPHYSICS_DEMO_TARGET = blank3d_vphysics_demo.exe
RELEASE_RSP = .blank3d_release.rsp
DEBUG_RSP = .blank3d_debug.rsp
VPHYSICS_DEMO_RSP = .blank3d_vphysics_demo.rsp
SYNTAX_RSP = .blank3d_syntax.rsp

WEAPON_MANAGER = vendor/weapon_system/gweapon89_manager_v2_super_agnostic_provider_bus/gweapon89_manager
BOLT_ROOT = vendor/weapon_system/bolt3d_ballistics_c89/bolt3d_ballistics_c89
AOI_TRAIL_ROOT = vendor/weapon_system/aoi_trail3d89/aoi_trail3d89
SCOPE_ROOT = vendor/weapon_system/gscope89
SCOPE_INI_ROOT = $(SCOPE_ROOT)/gscopeini89
ZOOM_ROOT = $(SCOPE_ROOT)/gtelescopiczoom89
SWAY_ROOT = $(SCOPE_ROOT)/gsway89
SNIPER_HUD_ROOT = $(SCOPE_ROOT)/gsniperhud89
AIM_ROOT = $(SCOPE_ROOT)/gaimquery89
SCOPE_PROVIDER_ROOT = $(SCOPE_ROOT)/gscopeprovider89
SCOPE_BUNDLE_ROOT = $(SCOPE_ROOT)/gscopebundle89
SCOPE_ANIM_ROOT = $(SCOPE_ROOT)/gscopeanim89
SCOPE_PAINT_ROOT = $(SCOPE_ROOT)/modules/gscopepaint89
SCOPE_VECTOR_ROOT = $(SCOPE_ROOT)/modules/gscopevector89
SCOPE_RASTER_ROOT = $(SCOPE_ROOT)/modules/gscoperaster89
SCOPE_BARS_ROOT = $(SCOPE_ROOT)/modules/gscopebars89
SCOPE_PRESETS_ROOT = $(SCOPE_ROOT)/modules/gscopepresets89
CROSSHAIR_ROOT = vendor/weapon_system/gcrosshair89
CROSSHAIR_CORE_ROOT = $(CROSSHAIR_ROOT)/gcrosshair_core89
CROSSHAIR_PARAMS_ROOT = $(CROSSHAIR_ROOT)/gcrosshair_params89
CROSSHAIR_BASE_ROOT = $(CROSSHAIR_ROOT)/gcrosshair_base89
CROSSHAIR_PROVIDER_ROOT = $(CROSSHAIR_ROOT)/gcrosshair_provider89
CROSSHAIR_ANIM_ROOT = $(CROSSHAIR_ROOT)/gcrosshair_anim89
CROSSHAIR_RUNTIME_ROOT = $(CROSSHAIR_ROOT)/gcrosshair_runtime89
CAMERANAKU_ROOT = vendor/cameranaku89
GTRIGGER89_ROOT = vendor/weapon_system/gtrigger89
GPPA89_ROOT = vendor/weapon_system/gplayerprojectileaim89
GPS89_ROOT = vendor/weapon_system/gprojectilespawn89
SATELLABORNER89_ROOT = vendor/weapon_system/Satellaborner89
TELESEARCHER89_ROOT = vendor/weapon_system/Telesearcher89
EXPANDIBLEFIRE89_ROOT = vendor/weapon_system/expandiblefire89
BULLETSPIN89_ROOT = vendor/weapon_system/bulletspin89
BULLETCIRCLE89_ROOT = vendor/weapon_system/bulletcircle89
BULLETINLINE89_ROOT = vendor/weapon_system/bulletinline89
MORETHANONE89_ROOT = vendor/weapon_system/morethanone89
IMGCC0_ROOT = vendor/imgcc0
SPRITEPLANE89_ROOT = vendor/spriteplane89
SPRITEASSET89_ROOT = vendor/spriteasset89
SPRITEVERBS89_ROOT = vendor/spriteverbs89
ASSETROUTE89_ROOT = vendor/assetroute89
STATICSPRITE89_ROOT = vendor/staticsprite89
IMAGESEQUENCER89_ROOT = vendor/imagesequencer89
RENLIST89_ROOT = vendor/renlist89
TILECELL89_ROOT = vendor/tilecell89
GMSPRITESTRIP89_ROOT = vendor/gmspritestrip89
PROJECTILEVISUAL2D89_ROOT = vendor/weapon_system/projectilevisual2d89
GPROJ2D89_ROOT = vendor/weapon_system/gproj2d89
GSKYBOX89_ROOT = vendor/gskybox89
SKYBOXRECIPE89_ROOT = vendor/skyboxrecipe89
RT_TIME89_ROOT = vendor/rt_time89
TIMECLOCKER89_ROOT = vendor/timeclocker89
TICKOCLOCK89_ROOT = vendor/tickoclock89
TIMEVERBS89_ROOT = vendor/timeverbs89
GENWINCONFIGC89_ROOT = vendor/genwinconfigc89
GENERALVIDEOCONFIGC89_ROOT = vendor/generalvideoconfigc89
GAMEPLAYSCREENSIZEC89_ROOT = vendor/gameplayscreensizec89
GWINVRBS89_ROOT = vendor/gwinvrbs89
GENVIDVERBS89_ROOT = vendor/genvidverbs89
GMPLYSS89_ROOT = vendor/gmplyss89
HOWM89_ROOT = vendor/howlund_window_maker89
WINDOWSWINDOW89_ROOT = vendor/windowswindow89
PRIMITIVE2D89_ROOT = vendor/primitive2d_database89
MOUNT89_ROOT = vendor/3d_mounting_system89
GVEHICLE89_ROOT = vendor/gvehicle89
GVEHPOS89_ROOT = vendor/gvehpos89
GCR89_ROOT = vendor/weapon_system/gcasingruntime89
GWS89_ROOT = vendor/weapon_system/gweaponsnapshot89
GWEAPONMODULES89_ROOT = vendor/weapon_system/gweaponmodules89
GWEAPONBALLISTICS89_ROOT = vendor/weapon_system/gweaponballistics89
GWEAPONAIM89_ROOT = vendor/weapon_system/gweaponaim89
GSHOTGUN89_ROOT = vendor/weapon_system/gshotgun89
GWEAPONCROSSHAIR89_ROOT = vendor/weapon_system/gweaponcrosshair89
GWEAPONSNIPER89_ROOT = vendor/weapon_system/gweaponsniper89
GWEAPONPRESENTATION89_ROOT = vendor/weapon_system/gweaponpresentation89
GMECHANICALWEAPON89_ROOT = vendor/weapon_system/gmechanicalweapon89
GWEAPONIO89_ROOT = vendor/weapon_system/gweaponio89
GWEAPONLOADOUT89_ROOT = vendor/weapon_system/gweaponloadout89
GPLAYERFIRERAY89_ROOT = vendor/weapon_system/gplayerfireray89
GFIRE_FRAME89_ROOT = vendor/weapon_system/gfireframe89
GBOLTWEAPON89_ROOT = vendor/weapon_system/gboltweapon89
GWEAPONPROFILEIO89_ROOT = vendor/weapon_system/gweaponprofileio89
SYNTH_ROOT = vendor/weapon_synth_sound_engine89
GOLDIE_ROOT = vendor/audio/goldie_audio89
GOLDIE_RAWMIX_ROOT = $(GOLDIE_ROOT)/vendor/rawmix_phase5_dawstyle_giffany/rawmix
GOLDIE_KNM_ROOT = $(GOLDIE_ROOT)/vendor/knm_audio_refactor_cc0_v030_phase3_plus/KNM_audio_refactor_phase3
FONTCORE_ROOT = vendor/text/monika_fontcore
DDSL2_ROOT = vendor/ddsl2
FPIL_ROOT = vendor/fpil
RPYL_ROOT = vendor/rpyl
GFO_ROOT = vendor/gfo
VERTICAL_COMMON_ROOT = vendor/vertical_motion89_common
FLY89_ROOT = vendor/fly89
JUMP89_ROOT = vendor/jump89
AIRDIVER89_ROOT = vendor/airdiver89
MOVEMENTBASEVERBS89_ROOT = vendor/3d_movementbaseverbs89
INVARIANTS89_ROOT = vendor/invariantSpecialoperations_89
INPUT_HOOK_ROOT = vendor/input_hook89
INPUT_SCANNER_ROOT = vendor/input_scanner89
INPUT_KEYS89_ROOT = vendor/input_keys89
POLLS89_ROOT = vendor/polls89
SCANEMU89_ROOT = vendor/scanemu89

B3D_CC_MACHINE := $(strip $(shell $(CC) -dumpmachine 2>/dev/null))
INPUT_FOURHEAD_COMMON_SOURCES = \
 src/blank3d_input_platform.c \
 $(INPUT_HOOK_ROOT)/input_hook_backend_polls89.c \
 $(POLLS89_ROOT)/key_pc.c \
 $(POLLS89_ROOT)/polls_input_keys89.c

ifneq (,$(findstring mingw,$(B3D_CC_MACHINE)))
ifneq (,$(findstring x86_64,$(B3D_CC_MACHINE)))
INPUT_FOURHEAD_NATIVE_SOURCE = $(POLLS89_ROOT)/by_system_backend/win64/winpckeys_backend.c
else
INPUT_FOURHEAD_NATIVE_SOURCE = $(POLLS89_ROOT)/by_system_backend/win32/winpckeys_backend.c
endif
INPUT_FOURHEAD_PLATFORM_SOURCES = \
 $(INPUT_FOURHEAD_NATIVE_SOURCE) \
 $(INPUT_HOOK_ROOT)/input_hook_backend_polls89_winpckeys.c
else ifneq (,$(findstring linux,$(B3D_CC_MACHINE)))
ifneq (,$(filter x86_64% aarch64% arm64%,$(B3D_CC_MACHINE)))
INPUT_FOURHEAD_NATIVE_SOURCE = $(POLLS89_ROOT)/by_system_backend/linux64/linuxpckeys_backend.c
else
INPUT_FOURHEAD_NATIVE_SOURCE = $(POLLS89_ROOT)/by_system_backend/linux32/linuxpckeys_backend.c
endif
INPUT_FOURHEAD_PLATFORM_SOURCES = \
 $(INPUT_FOURHEAD_NATIVE_SOURCE) \
 $(INPUT_HOOK_ROOT)/input_hook_backend_polls89_linuxpckeys.c
else ifneq (,$(findstring darwin,$(B3D_CC_MACHINE)))
ifneq (,$(filter x86_64% aarch64% arm64%,$(B3D_CC_MACHINE)))
INPUT_FOURHEAD_NATIVE_SOURCE = $(POLLS89_ROOT)/by_system_backend/mac64/macpckeys_backend.c
else
INPUT_FOURHEAD_NATIVE_SOURCE = $(POLLS89_ROOT)/by_system_backend/mac32/macpckeys_backend.c
endif
INPUT_FOURHEAD_PLATFORM_SOURCES = \
 $(INPUT_FOURHEAD_NATIVE_SOURCE) \
 $(INPUT_HOOK_ROOT)/input_hook_backend_polls89_macpckeys.c
else
INPUT_FOURHEAD_NATIVE_SOURCE =
INPUT_FOURHEAD_PLATFORM_SOURCES =
endif

INPUT_FOURHEAD_SOURCES = $(INPUT_FOURHEAD_COMMON_SOURCES) $(INPUT_FOURHEAD_PLATFORM_SOURCES)
CYCLER89_ROOT = vendor/cycler89
GAUTOMOTION_ROOT = vendor/gautomotion89
GLOCO89_ROOT = vendor/gloco89
GK3D_ROOT = vendor/3dkin_gf
GAMEVERBS89_ROOT = vendor/gameverbs89
CONDOR_EVACT89_ROOT = vendor/condor_evact89
CONTACT_TRIGGER89_ROOT = vendor/3D_contact_trigger89
PBB_ITEM89_ROOT = vendor/pbb_item_system_c89
PBB_CONTACT_WEAPON89_ROOT = vendor/pbb_contact_weapon_bridge89
GAIRLUNGE89_ROOT = vendor/gairlunge89
GGROUNDLANCE89_ROOT = vendor/ggroundlance89
ECG_ROOT = vendor/ecg_embedded_c89
BIGHUD_ROOT = vendor/bigvaderhudder
GEDER_ROOT = vendor/geder_truth_gate_pipeline
NPC_EYES_ROOT = vendor/3d_npc_eyes
ENLIGHTENER_ROOT = vendor/enlightenerai
NATIONALMECANICANIMAL89_ROOT = vendor/nationalmecanicanimal89
GATTACH89_ROOT = vendor/gattach89
ACTOR_SYSTEM89_ROOT = vendor/actor_system89
CLASS_MANAGER89_ROOT = vendor/class_manager89
VAR_MANAGER89_ROOT = vendor/var_manager89
VAR_DSL89_ROOT = vendor/var_dsl89
VAR_RUNTIME89_ROOT = vendor/var_runtime89
EQUIPMENT_SYSTEM89_ROOT = vendor/equipment_system89
THING_SYSTEM89_ROOT = vendor/thing_system89
ECS89_ROOT = vendor/ecs89
WORLD3D89_ROOT = vendor/world3d89
SCENE3D89_ROOT = vendor/scene3d89
VPHYSICS_ROOT = vendor/vphysics_total_solver_c89
GFACTION89_ROOT = vendor/gfaction89
KATANA89_ROOT = vendor/katana89
PDC3D_ROOT = vendor/physical_damage_collision3d
SYNTH_LIB = $(SYNTH_ROOT)/build/libweapon_synth_sound_engine89.a
SYNTH_STAMP = $(SYNTH_ROOT)/build/.blank3d_toolchain
IMGCC0_LIB = $(IMGCC0_ROOT)/build/libimgcc0.a
IMGCC0_STAMP = $(IMGCC0_ROOT)/build/.blank3d_toolchain
# A packaged static library may have been built by another compiler/CPU.
# MinGW cannot link Linux ELF objects even though GNU ar can list them.
SYNTH_TOOLCHAIN_ID := $(strip $(shell $(CC) -dumpmachine 2>/dev/null))-$(strip $(shell $(CC) -dumpfullversion -dumpversion 2>/dev/null))
# Include the static codec ABI mode in the stamp. This forces a clean rebuild
# when moving from DLL-import decorated BMP/PCX objects to static linkage.
IMGCC0_STATIC_ABI_REV = bmp-pcx-static-v1
IMGCC0_TOOLCHAIN_ID := $(SYNTH_TOOLCHAIN_ID)-$(IMGCC0_STATIC_ABI_REV)

CORE_INCLUDES = -Isrc -Ivendor \
 -Ivendor/gamlib3d -Ivendor/gamlib3d/math_helpers -Ivendor/weapon_system/g3dweaponzeroing89/include -Ivendor/weapon_system/gweaponlaunch89/include \
 -I$(GTRIGGER89_ROOT)/include -I$(GPPA89_ROOT)/include -I$(GPS89_ROOT)/include \
 -I$(SATELLABORNER89_ROOT)/include -I$(TELESEARCHER89_ROOT)/include -I$(EXPANDIBLEFIRE89_ROOT)/include \
 -I$(BULLETSPIN89_ROOT)/include -I$(BULLETCIRCLE89_ROOT)/include -I$(BULLETINLINE89_ROOT)/include -I$(MORETHANONE89_ROOT)/include \
 -I$(IMGCC0_ROOT)/include -I$(SPRITEPLANE89_ROOT)/include \
 -I$(SPRITEASSET89_ROOT)/include -I$(SPRITEASSET89_ROOT)/adapters/imgcc0 \
 -I$(SPRITEVERBS89_ROOT)/include -I$(SPRITEVERBS89_ROOT)/adapters/aseprite \
 -I$(SPRITEVERBS89_ROOT)/adapters/assetroute89 -I$(SPRITEVERBS89_ROOT)/adapters/ddsl2 \
 -I$(ASSETROUTE89_ROOT)/include -I$(ASSETROUTE89_ROOT)/providers/posix -I$(ASSETROUTE89_ROOT)/providers/win32 \
 -I$(STATICSPRITE89_ROOT)/include -I$(IMAGESEQUENCER89_ROOT)/include \
 -I$(RENLIST89_ROOT)/include -I$(RENLIST89_ROOT)/adapters/imagesequencer89 \
 -I$(TILECELL89_ROOT)/include -I$(GMSPRITESTRIP89_ROOT)/include -I$(PROJECTILEVISUAL2D89_ROOT)/include -I$(GPROJ2D89_ROOT)/include \
 -I$(GSKYBOX89_ROOT)/include -I$(SKYBOXRECIPE89_ROOT)/include \
 -I$(RT_TIME89_ROOT) -I$(TIMECLOCKER89_ROOT)/include \
 -I$(TICKOCLOCK89_ROOT)/include -I$(TIMEVERBS89_ROOT)/include \
 -I$(GENWINCONFIGC89_ROOT)/include -I$(GENERALVIDEOCONFIGC89_ROOT)/include \
 -I$(GAMEPLAYSCREENSIZEC89_ROOT)/include -I$(GWINVRBS89_ROOT)/include \
 -I$(GENVIDVERBS89_ROOT)/include -I$(GMPLYSS89_ROOT)/include \
 -I$(HOWM89_ROOT)/include -I$(HOWM89_ROOT)/adapters/genwinconfigc89 \
 -I$(WINDOWSWINDOW89_ROOT)/include -I$(PRIMITIVE2D89_ROOT)/include \
 -I$(MOUNT89_ROOT)/include -I$(GVEHICLE89_ROOT)/include \
 -I$(GVEHICLE89_ROOT)/vendor/vehicleprovider89/include \
 -I$(GVEHICLE89_ROOT)/vendor/vehiclephysics89/include \
 -I$(GVEHICLE89_ROOT)/vendor/carmovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/motorcyclemovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/busmovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/tankmovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/watermovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/airmovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/rotorcraftmovement89/include \
 -I$(GVEHICLE89_ROOT)/vendor/spacemovement89/include \
 -I$(GVEHPOS89_ROOT)/include \
 -I$(GCR89_ROOT)/include -I$(GWS89_ROOT)/include \
 -I$(GWEAPONMODULES89_ROOT)/include -I$(GWEAPONBALLISTICS89_ROOT)/include -I$(GWEAPONAIM89_ROOT)/include \
 -I$(GSHOTGUN89_ROOT)/include -I$(GWEAPONCROSSHAIR89_ROOT)/include -I$(GWEAPONSNIPER89_ROOT)/include \
 -I$(GWEAPONPRESENTATION89_ROOT)/include -I$(GMECHANICALWEAPON89_ROOT)/include -I$(GWEAPONIO89_ROOT)/include \
 -I$(GWEAPONLOADOUT89_ROOT)/include -I$(GPLAYERFIRERAY89_ROOT)/include -I$(GFIRE_FRAME89_ROOT)/include \
 -I$(GBOLTWEAPON89_ROOT)/include -I$(GWEAPONPROFILEIO89_ROOT)/include \
 -Ivendor/soquete3d -Ivendor/giffany_shapes3d \
 -Ivendor/numsys/include -Ivendor/flags -Ivendor/gkinventory/include \
 -I$(VAR_MANAGER89_ROOT)/include -I$(VAR_DSL89_ROOT)/include -I$(VAR_RUNTIME89_ROOT)/include \
 -Ivendor/gbar89/include -Ivendor/conf_total -I$(WEAPON_MANAGER)/src \
 -Ivendor/weapon_system/gbulletmesh89/gbulletmesh89/src \
 -Ivendor/weapon_system/rocketmeshes/rocketmeshes/include -Ivendor/weapon_system/ghandgrenade3d89/ghandgrenade3d89/include \
 -I$(BOLT_ROOT)/include -I$(AOI_TRAIL_ROOT)/include \
 -I$(SCOPE_ROOT)/include -I$(SCOPE_INI_ROOT)/include \
 -I$(ZOOM_ROOT)/include -I$(SWAY_ROOT)/include \
 -I$(SNIPER_HUD_ROOT)/include -I$(AIM_ROOT)/include \
 -I$(SCOPE_PROVIDER_ROOT)/include -I$(SCOPE_BUNDLE_ROOT)/include \
 -I$(SCOPE_ANIM_ROOT)/include \
 -I$(SCOPE_PAINT_ROOT)/include -I$(SCOPE_VECTOR_ROOT)/include \
 -I$(SCOPE_RASTER_ROOT)/include -I$(SCOPE_BARS_ROOT)/include \
 -I$(SCOPE_PRESETS_ROOT)/include \
 -I$(CROSSHAIR_CORE_ROOT)/include -I$(CROSSHAIR_PARAMS_ROOT)/include \
 -I$(CROSSHAIR_BASE_ROOT)/include -I$(CROSSHAIR_PROVIDER_ROOT)/include \
 -I$(CROSSHAIR_ANIM_ROOT)/include -I$(CROSSHAIR_RUNTIME_ROOT)/include \
 -I$(CAMERANAKU_ROOT)/include -I$(GFO_ROOT)/include \
 -I$(VERTICAL_COMMON_ROOT)/include -I$(FLY89_ROOT)/include \
 -I$(JUMP89_ROOT)/include -I$(AIRDIVER89_ROOT)/include \
 -I$(MOVEMENTBASEVERBS89_ROOT)/include -I$(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d \
 -I$(INVARIANTS89_ROOT)/include -I$(INVARIANTS89_ROOT)/adapters/ddsl2 -I$(INVARIANTS89_ROOT)/adapters/flags89 \
 -I$(INPUT_HOOK_ROOT) -I$(INPUT_SCANNER_ROOT) -I$(INPUT_KEYS89_ROOT)/include -I$(POLLS89_ROOT) \
 -I$(SCANEMU89_ROOT)/include -I$(CYCLER89_ROOT)/include \
 -I$(GAUTOMOTION_ROOT)/include -I$(GLOCO89_ROOT)/include \
 -I$(GK3D_ROOT)/include -I$(GAMEVERBS89_ROOT)/include -I$(CONDOR_EVACT89_ROOT)/include \
 -I$(CONTACT_TRIGGER89_ROOT)/include -I$(PBB_ITEM89_ROOT)/include -I$(PBB_CONTACT_WEAPON89_ROOT)/include \
 -I$(GAIRLUNGE89_ROOT)/include -I$(GGROUNDLANCE89_ROOT)/include \
 -I$(DDSL2_ROOT) -I$(FPIL_ROOT)/fpi_core \
 -I$(RPYL_ROOT)/API -I$(RPYL_ROOT)/program -I$(RPYL_ROOT)/common \
 -I$(RPYL_ROOT)/config -I$(RPYL_ROOT)/util -I$(RPYL_ROOT)/span \
 -I$(RPYL_ROOT)/error -I$(RPYL_ROOT)/io -I$(RPYL_ROOT)/stream \
 -I$(RPYL_ROOT)/arena -I$(RPYL_ROOT)/alloc -I$(RPYL_ROOT)/types \
 -I$(RPYL_ROOT)/value -I$(RPYL_ROOT)/token -I$(RPYL_ROOT)/lexer \
 -I$(RPYL_ROOT)/parser -I$(RPYL_ROOT)/ast -I$(RPYL_ROOT)/semantics \
 -I$(RPYL_ROOT)/symtab -I$(RPYL_ROOT)/polysym -I$(RPYL_ROOT)/registry \
 -I$(RPYL_ROOT)/store -I$(RPYL_ROOT)/IR -I$(RPYL_ROOT)/opcodes \
 -I$(RPYL_ROOT)/bytecode -I$(RPYL_ROOT)/VM -I$(RPYL_ROOT)/runtime \
 -I$(RPYL_ROOT)/builtins -I$(RPYL_ROOT)/stdlib_propia \
 -I$(RPYL_ROOT)/warper -I$(RPYL_ROOT)/transpiler -I$(RPYL_ROOT)/compiler \
 -Ivendor/collision/ccs -Ivendor/collision/sicol \
 -I$(ECG_ROOT)/include -I$(BIGHUD_ROOT)/include \
 -I$(GEDER_ROOT)/include \
 -I$(NPC_EYES_ROOT)/include -I$(ENLIGHTENER_ROOT)/include \
 -I$(NATIONALMECANICANIMAL89_ROOT)/include -I$(GATTACH89_ROOT)/include \
 -I$(ACTOR_SYSTEM89_ROOT)/include -I$(CLASS_MANAGER89_ROOT)/include -I$(EQUIPMENT_SYSTEM89_ROOT)/include \
 -I$(THING_SYSTEM89_ROOT)/include -I$(ECS89_ROOT)/include \
 -I$(WORLD3D89_ROOT)/include -I$(SCENE3D89_ROOT)/include \
 -I$(VPHYSICS_ROOT)/include -I$(GFACTION89_ROOT)/include \
 -I$(KATANA89_ROOT)/include -I$(PDC3D_ROOT)/include

SYNTH_INCLUDES = \
 -I$(SYNTH_ROOT)/include \
 -I$(SYNTH_ROOT)/vendor/gpaah89/include \
 -I$(SYNTH_ROOT)/vendor/chuecka89/include \
 -I$(SYNTH_ROOT)/vendor/gpump89/include \
 -I$(SYNTH_ROOT)/vendor/shotpumpkin89/include \
 -I$(SYNTH_ROOT)/vendor/gweaponfoley89/include \
 -I$(SYNTH_ROOT)/vendor/gklek89/include \
 -I$(SYNTH_ROOT)/vendor/wmagazine89/include \
 -I$(SYNTH_ROOT)/vendor/grocketwhistle89 \
 -I$(SYNTH_ROOT)/vendor/grocketspin89/include \
 -I$(SYNTH_ROOT)/vendor/ggunmach89/include \
 -I$(SYNTH_ROOT)/vendor/gguntuberotator89/include \
 -I$(SYNTH_ROOT)/vendor/ggatlingwhistle89/include \
 -I$(SYNTH_ROOT)/vendor/gshotgunsequence89/include \
 -I$(SYNTH_ROOT)/vendor/gshotguneq89/include \
 -I$(SYNTH_ROOT)/vendor/gvoice89/include \
 -I$(SYNTH_ROOT)/vendor/gweaponvoice89/include \
 -I$(SYNTH_ROOT)/vendor/gtinkle89/include \
 -I$(SYNTH_ROOT)/vendor/gfire89/include \
 -I$(SYNTH_ROOT)/vendor/gbulletair89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_ggrenadeblast89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_rocketblast89/include \
 -I$(SYNTH_ROOT)/vendor/gweapon_cushions89/gweaponbody89/include \
 -I$(SYNTH_ROOT)/vendor/gweapon_cushions89/gmuzzlegas89/include \
 -I$(SYNTH_ROOT)/vendor/gweapon_cushions89/gballisticcrack89/include \
 -I$(SYNTH_ROOT)/vendor/gweapon_cushions89/glatetail89/include \
 -I$(SYNTH_ROOT)/vendor/gweapon_cushions89/gcinemathump89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundmuzzledevice89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundaero89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundfriction89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundparticles89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundammo89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundbeltfeed89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundoutdoor89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundportal89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsounddoppler89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundspatial89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundthermal89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundlistener89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundmask89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsounddna89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundreceiver89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundprojectile89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundprop89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundroom89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundaction89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundimpact89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundricochet89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_weapon_world89_v1_0/wsoundcombatbus89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_acoustic_phase12_89/include \
 -I$(SYNTH_ROOT)/vendor/wsound_phase3_89/include

GOLDIE_INCLUDES = \
 -I$(GOLDIE_ROOT)/runtime/include \
 -I$(GOLDIE_RAWMIX_ROOT)/include \
 -I$(GOLDIE_KNM_ROOT)/KNM_audio \
 -I$(GOLDIE_ROOT)/vendor/audiocodecs/mwav89/include \
 -I$(GOLDIE_ROOT)/vendor/audio/mpcm89/include \
 -I$(GOLDIE_ROOT)/vendor/audiocodecs/mp3_frame89/include \
 -I$(GOLDIE_ROOT)/vendor/audiocodecs/mp3_acm_codec89/include

FONTCORE_INCLUDES = \
 -I$(FONTCORE_ROOT)/include \
 -I$(FONTCORE_ROOT)/vendor/sfnt_decoder_c89/include \
 -I$(FONTCORE_ROOT)/vendor/glyphwifettf_v0_4_3/include \
 -I$(FONTCORE_ROOT)/vendor/monika_otf_decoder_c89_interp/src \
 -I$(FONTCORE_ROOT)/vendor/monika_woff1_decoder_c89/src \
 -I$(FONTCORE_ROOT)/vendor/monika_woff2_c89_decoder/include \
 -I$(FONTCORE_ROOT)/vendor/gmyy_spritefont_decoder_v3 \
 -I$(FONTCORE_ROOT)/vendor/mugen_fnt_codec/include \
 -I$(FONTCORE_ROOT)/vendor/giffy_hb_c89_v0_20/include

INCLUDES = $(CORE_INCLUDES) $(SYNTH_INCLUDES) $(GOLDIE_INCLUDES) $(FONTCORE_INCLUDES)

DDSL2_SOURCES = \
 $(DDSL2_ROOT)/API/ddsl_api.c \
 $(DDSL2_ROOT)/IR/ir.c \
 $(DDSL2_ROOT)/Transpiler/transpile.c \
 $(DDSL2_ROOT)/VM/vm.c \
 $(DDSL2_ROOT)/alloc/alloc.c \
 $(DDSL2_ROOT)/arena/arena.c \
 $(DDSL2_ROOT)/ast/ast.c \
 $(DDSL2_ROOT)/built-ins/builtins.c \
 $(DDSL2_ROOT)/bytecode/bytecode.c \
 $(DDSL2_ROOT)/common/strview.c \
 $(DDSL2_ROOT)/compiler/compiler.c \
 $(DDSL2_ROOT)/error/error.c \
 $(DDSL2_ROOT)/io/io.c \
 $(DDSL2_ROOT)/lexer/lexer.c \
 $(DDSL2_ROOT)/opcodes/opcodes.c \
 $(DDSL2_ROOT)/parser/parser.c \
 $(DDSL2_ROOT)/polysym/polysym.c \
 $(DDSL2_ROOT)/program/program.c \
 $(DDSL2_ROOT)/registry/registry.c \
 $(DDSL2_ROOT)/runtime/runtime.c \
 $(DDSL2_ROOT)/semantics/semantics.c \
 $(DDSL2_ROOT)/span/span.c \
 $(DDSL2_ROOT)/stdlib/stdlib.c \
 $(DDSL2_ROOT)/store/store.c \
 $(DDSL2_ROOT)/stream/stream.c \
 $(DDSL2_ROOT)/symtab/symtab.c \
 $(DDSL2_ROOT)/token/token.c \
 $(DDSL2_ROOT)/types/fixed.c \
 $(DDSL2_ROOT)/util/util.c \
 $(DDSL2_ROOT)/value/value.c \
 $(DDSL2_ROOT)/warper/warper.c

FPIL_SOURCES = \
 $(FPIL_ROOT)/fpi_core/fpi_alloc.c \
 $(FPIL_ROOT)/fpi_core/fpi_api.c \
 $(FPIL_ROOT)/fpi_core/fpi_arena.c \
 $(FPIL_ROOT)/fpi_core/fpi_ast.c \
 $(FPIL_ROOT)/fpi_core/fpi_bytecode.c \
 $(FPIL_ROOT)/fpi_core/fpi_compiler.c \
 $(FPIL_ROOT)/fpi_core/fpi_error.c \
 $(FPIL_ROOT)/fpi_core/fpi_ext.c \
 $(FPIL_ROOT)/fpi_core/fpi_io.c \
 $(FPIL_ROOT)/fpi_core/fpi_ir.c \
 $(FPIL_ROOT)/fpi_core/fpi_lexer.c \
 $(FPIL_ROOT)/fpi_core/fpi_opcodes.c \
 $(FPIL_ROOT)/fpi_core/fpi_parser.c \
 $(FPIL_ROOT)/fpi_core/fpi_polysym.c \
 $(FPIL_ROOT)/fpi_core/fpi_program.c \
 $(FPIL_ROOT)/fpi_core/fpi_registry.c \
 $(FPIL_ROOT)/fpi_core/fpi_runtime.c \
 $(FPIL_ROOT)/fpi_core/fpi_semantics.c \
 $(FPIL_ROOT)/fpi_core/fpi_span.c \
 $(FPIL_ROOT)/fpi_core/fpi_store.c \
 $(FPIL_ROOT)/fpi_core/fpi_stream.c \
 $(FPIL_ROOT)/fpi_core/fpi_symtab.c \
 $(FPIL_ROOT)/fpi_core/fpi_token.c \
 $(FPIL_ROOT)/fpi_core/fpi_transpiler.c \
 $(FPIL_ROOT)/fpi_core/fpi_util.c \
 $(FPIL_ROOT)/fpi_core/fpi_value.c \
 $(FPIL_ROOT)/fpi_core/fpi_vm.c \
 $(FPIL_ROOT)/fpi_core/fpi_warper.c

RPYL_SOURCES = \
 $(RPYL_ROOT)/common/rpyl_common.c \
 $(RPYL_ROOT)/span/rpyl_span.c \
 $(RPYL_ROOT)/error/rpyl_error.c \
 $(RPYL_ROOT)/io/rpyl_io.c \
 $(RPYL_ROOT)/symtab/rpyl_symtab.c \
 $(RPYL_ROOT)/registry/rpyl_registry.c \
 $(RPYL_ROOT)/store/rpyl_store.c \
 $(RPYL_ROOT)/IR/rpyl_ir.c \
 $(RPYL_ROOT)/opcodes/rpyl_opcodes.c \
 $(RPYL_ROOT)/builtins/rpyl_builtins.c \
 $(RPYL_ROOT)/semantics/rpyl_semantics.c \
 $(RPYL_ROOT)/compiler/rpyl_compiler.c \
 $(RPYL_ROOT)/warper/rpyl_warper.c \
 $(RPYL_ROOT)/program/rpyl.c \
 $(RPYL_ROOT)/alloc/rpyl_alloc.c \
 $(RPYL_ROOT)/arena/rpyl_arena.c \
 $(RPYL_ROOT)/ast/rpyl_ast.c \
 $(RPYL_ROOT)/bytecode/rpyl_bytecode.c \
 $(RPYL_ROOT)/stdlib_propia/rpyl_extlang.c \
 $(RPYL_ROOT)/value/rpyl_fixed.c \
 $(RPYL_ROOT)/lexer/rpyl_lexer.c \
 $(RPYL_ROOT)/parser/rpyl_parser.c \
 $(RPYL_ROOT)/polysym/rpyl_polysym.c \
 $(RPYL_ROOT)/runtime/rpyl_runtime.c \
 $(RPYL_ROOT)/VM/rpyl_vm.c \
 $(RPYL_ROOT)/transpiler/rpyl_transpile.c \
 $(RPYL_ROOT)/value/rpyl_value.c

LANGUAGE_SOURCES = $(DDSL2_SOURCES) $(FPIL_SOURCES) $(RPYL_SOURCES)

GFO_SOURCES = \
 $(GFO_ROOT)/src/gfo_ast.c \
 $(GFO_ROOT)/src/gfo_bc.c \
 $(GFO_ROOT)/src/gfo_common.c \
 $(GFO_ROOT)/src/gfo_ir.c \
 $(GFO_ROOT)/src/gfo_lexer.c \
 $(GFO_ROOT)/src/gfo_parser.c \
 $(GFO_ROOT)/src/gfo_runtime.c \
 $(GFO_ROOT)/src/gfo_sym.c \
 $(GFO_ROOT)/src/gfo_transpile.c


BIGHUD_SOURCES = \
 $(BIGHUD_ROOT)/src/bvh_api.c \
 $(BIGHUD_ROOT)/src/bvh_arena.c \
 $(BIGHUD_ROOT)/src/bvh_ast.c \
 $(BIGHUD_ROOT)/src/bvh_bytecode.c \
 $(BIGHUD_ROOT)/src/bvh_common.c \
 $(BIGHUD_ROOT)/src/bvh_io.c \
 $(BIGHUD_ROOT)/src/bvh_ir.c \
 $(BIGHUD_ROOT)/src/bvh_lexer.c \
 $(BIGHUD_ROOT)/src/bvh_parser.c \
 $(BIGHUD_ROOT)/src/bvh_polysym.c \
 $(BIGHUD_ROOT)/src/bvh_runtime.c \
 $(BIGHUD_ROOT)/src/bvh_token.c \
 $(BIGHUD_ROOT)/src/bvh_transpile.c \
 $(BIGHUD_ROOT)/src/bvh_value.c \
 $(BIGHUD_ROOT)/src/bvh_vm.c

GEDER_SOURCES = $(GEDER_ROOT)/src/geder_truth_gate.c
NPC_EYES_SOURCES = $(NPC_EYES_ROOT)/src/3d_npc_eyes.c
ENLIGHTENER_SOURCES = $(wildcard $(ENLIGHTENER_ROOT)/src/*.c)
NATIONALMECANICANIMAL89_SOURCES = $(NATIONALMECANICANIMAL89_ROOT)/src/nationalmecanicanimal89.c
GATTACH89_SOURCES = $(GATTACH89_ROOT)/src/gattach89.c $(GATTACH89_ROOT)/src/gattach89_profiles.c $(GATTACH89_ROOT)/src/gattach89_render_bridge.c
VPHYSICS_SOURCES = $(wildcard $(VPHYSICS_ROOT)/src/*.c)
GFACTION89_SOURCES = $(wildcard $(GFACTION89_ROOT)/src/*.c)

ECG_SOURCES = \
 $(ECG_ROOT)/src/ecg_fixed.c \
 $(ECG_ROOT)/src/ecg_font5x7.c \
 $(ECG_ROOT)/src/ecg_hud.c \
 $(ECG_ROOT)/src/ecg_profiles.c \
 $(ECG_ROOT)/src/ecg_renderer.c \
 $(ECG_ROOT)/src/ecg_surface.c \
 $(ECG_ROOT)/src/ecg_bighud.c

PDC3D_MELEE_SOURCES = \
 $(PDC3D_ROOT)/src/vendor/hurtbox3d.c \
 $(PDC3D_ROOT)/src/vendor/hitbox3d.c \
 $(PDC3D_ROOT)/src/vendor/melee3d.c

CROSSHAIR_SOURCES = \
 $(CROSSHAIR_CORE_ROOT)/src/gcrosshair_core89.c \
 $(CROSSHAIR_PARAMS_ROOT)/src/gcrosshair_params89.c \
 $(CROSSHAIR_BASE_ROOT)/src/gcrosshair_base89.c \
 $(CROSSHAIR_BASE_ROOT)/src/gcrosshair_recipe89.c \
 $(CROSSHAIR_PROVIDER_ROOT)/src/gcrosshair_provider89.c \
 $(CROSSHAIR_ANIM_ROOT)/src/gcrosshair_anim89.c \
 $(CROSSHAIR_RUNTIME_ROOT)/src/gcrosshair_runtime89.c

COLLISION_SOURCES =  vendor/collision/ccs/ccs_fixed.c  vendor/collision/ccs/ccs_math.c  vendor/collision/ccs/ccs_geom.c  vendor/collision/ccs/ccs_collision.c  vendor/collision/ccs/ccs_shapes.c  vendor/collision/ccs/ccs_narrow.c  vendor/collision/ccs/ccs_sat.c  vendor/collision/ccs/ccs_contact.c  vendor/collision/ccs/ccs_manifold.c  vendor/collision/ccs/ccs_raycast.c  vendor/collision/ccs/ccs_dispatch.c  vendor/collision/ccs/ccs_resolve.c  vendor/collision/ccs/ccs_broad_sweep.c  vendor/collision/ccs/ccs_broad_grid3d.c  vendor/collision/ccs/ccs_world.c  vendor/collision/ccs/ccs_shapecast.c  vendor/collision/ccs/ccs_plane.c  vendor/collision/ccs/ccs_triangle.c  vendor/collision/ccs/ccs_convex.c  vendor/collision/ccs/ccs_compound.c  vendor/collision/ccs/ccs_grid.c  vendor/collision/ccs/ccs_heightfield.c  vendor/collision/ccs/ccs_trimesh.c  vendor/collision/ccs/ccs_aabbtree.c  vendor/collision/ccs/ccs_trace_box.c  vendor/collision/sicol/sicol_math_fx.c  vendor/collision/sicol/sicol_shape.c  vendor/collision/sicol/sicol_mesh.c  vendor/collision/sicol/sicol_convex.c  vendor/collision/sicol/sicol_gjk.c  vendor/collision/sicol/sicol_cast.c  vendor/collision/sicol/sicol_broadphase.c  vendor/collision/sicol/sicol_narrowphase.c  vendor/collision/sicol/sicol_raycast.c  vendor/collision/sicol/sicol_world.c  vendor/collision/sicol/sicol_solver.c

WEAPON_HOSTED_VENDOR_SOURCES = \
 $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
 $(GWEAPONBALLISTICS89_ROOT)/src/gweaponballistics89.c \
 $(GWEAPONAIM89_ROOT)/src/gweaponaim89.c \
 $(GSHOTGUN89_ROOT)/src/gshotgun89.c \
 $(GWEAPONCROSSHAIR89_ROOT)/src/gweaponcrosshair89.c \
 $(GWEAPONSNIPER89_ROOT)/src/gweaponsniper89.c \
 $(GWEAPONPRESENTATION89_ROOT)/src/gweaponpresentation89.c \
 $(GMECHANICALWEAPON89_ROOT)/src/gmechanicalweapon89.c \
 $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
 $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c \
 $(GPLAYERFIRERAY89_ROOT)/src/gplayerfireray89.c \
 $(GFIRE_FRAME89_ROOT)/src/gfireframe89.c \
 $(GBOLTWEAPON89_ROOT)/src/gboltweapon89.c \
 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c

ITEM_CONTACT_SOURCES = \
 $(CONTACT_TRIGGER89_ROOT)/src/3d_contact_trigger89.c \
 $(wildcard $(PBB_ITEM89_ROOT)/src/*.c) \
 $(PBB_CONTACT_WEAPON89_ROOT)/src/pbb_contact_weapon_bridge89.c

GVEHICLE89_SOURCES = $(wildcard $(GVEHICLE89_ROOT)/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/vehicleprovider89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/vehiclephysics89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/carmovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/motorcyclemovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/busmovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/tankmovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/watermovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/airmovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/rotorcraftmovement89/src/*.c) \
 $(wildcard $(GVEHICLE89_ROOT)/vendor/spacemovement89/src/*.c)
GVEHPOS89_SOURCES = $(GVEHPOS89_ROOT)/src/gvpos.c

GOLDIE_SOURCES = \
 $(GOLDIE_ROOT)/runtime/src/goldie_audio89.c \
 $(GOLDIE_RAWMIX_ROOT)/src/rawmix.c \
 $(GOLDIE_KNM_ROOT)/KNM_audio/knm_hwr_audio.c \
 $(GOLDIE_KNM_ROOT)/KNM_audio/mnk_core.c \
 $(GOLDIE_KNM_ROOT)/knm_backends/mka_audio_null.c \
 $(GOLDIE_ROOT)/vendor/audiocodecs/mwav89/src/mwav89.c \
 $(GOLDIE_ROOT)/vendor/audio/mpcm89/src/mpcm89.c \
 $(GOLDIE_ROOT)/vendor/audiocodecs/mp3_frame89/src/mp3_frame89.c \
 $(GOLDIE_ROOT)/vendor/audiocodecs/mp3_acm_codec89/src/mp3_acm_codec89.c

FONTCORE_SOURCES = \
 $(FONTCORE_ROOT)/src/monika_fontcore.c \
 $(FONTCORE_ROOT)/vendor/sfnt_decoder_c89/src/sfnt_decoder.c \
 $(FONTCORE_ROOT)/vendor/glyphwifettf_v0_4_3/src/glyphwifettf.c \
 $(FONTCORE_ROOT)/vendor/glyphwifettf_v0_4_3/src/glyphwifettf_svg.c \
 $(FONTCORE_ROOT)/vendor/monika_otf_decoder_c89_interp/src/otf_c89.c \
 $(FONTCORE_ROOT)/vendor/monika_woff1_decoder_c89/src/woff1_decoder.c \
 $(FONTCORE_ROOT)/vendor/monika_woff1_decoder_c89/src/woff1_inflate.c \
 $(FONTCORE_ROOT)/vendor/monika_woff2_c89_decoder/src/w2f_core.c \
 $(FONTCORE_ROOT)/vendor/monika_woff2_c89_decoder/src/w2f_brotli_stub.c \
 $(FONTCORE_ROOT)/vendor/monika_woff2_c89_decoder/src/w2f_brotli_google_static.c \
 $(FONTCORE_ROOT)/vendor/gmyy_spritefont_decoder_v3/gmyy_spritefont.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/mfont_core.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/mfont_text.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/mfont_util.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/zlfont.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/pcxfnt.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/fntbump.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/src/fntpng.c \
 $(FONTCORE_ROOT)/vendor/mugen_fnt_codec/third_party/puff/puff.c \
 $(FONTCORE_ROOT)/vendor/giffy_hb_c89_v0_20/src/giffy_hb.c

SOURCES = src/monika_blank3d.c \
          src/blank3d_time89.c \
          src/blank3d_timeverbs89.c \
          $(RT_TIME89_ROOT)/rt_time.c \
          $(TIMECLOCKER89_ROOT)/src/timeclocker.c \
          $(TICKOCLOCK89_ROOT)/src/tickoclock89.c \
          $(TIMEVERBS89_ROOT)/src/timeverbs89.c \
          src/blank3d_display_stack89.c \
          src/blank3d_window_win32.c \
          $(HOWM89_ROOT)/src/howm89.c \
          $(HOWM89_ROOT)/adapters/genwinconfigc89/howm89_genwinconfigc89.c \
          $(WINDOWSWINDOW89_ROOT)/src/windowswindow89.c \
          src/blank3d_video_gl89.c \
          $(GENWINCONFIGC89_ROOT)/src/genwinconfigc89.c \
          $(GENERALVIDEOCONFIGC89_ROOT)/src/generalvideoconfigc89.c \
          $(GAMEPLAYSCREENSIZEC89_ROOT)/src/gameplayscreensizec89.c \
          $(GWINVRBS89_ROOT)/src/gwinvrbs89.c \
          $(GENVIDVERBS89_ROOT)/src/genvidverbs89.c \
          $(GMPLYSS89_ROOT)/src/gmplyss89.c \
          src/blank3d_image_assets.c \
          src/blank3d_image_gl.c \
          src/blank3d_spriteplanes.c \
          src/blank3d_muzzle_image.c \
          src/blank3d_muzzle_light.c \
          src/blank3d_flamethrower_billboard89.c \
          src/blank3d_projectile_sprite89.c \
          src/blank3d_projectilevisual2d89_bridge.c \
          $(PROJECTILEVISUAL2D89_ROOT)/src/projectilevisual2d89.c \
          src/blank3d_skybox89.c \
          src/blank3d_skybox89_gl.c \
          src/blank3d_skybox_recipe89.c \
          $(SKYBOXRECIPE89_ROOT)/src/skyboxrecipe89.c \
          $(GSKYBOX89_ROOT)/src/gskybox89.c \
          $(GSKYBOX89_ROOT)/src/gskybox89_assets.c \
          src/blank3d_sprite_runtime89.c \
          $(STATICSPRITE89_ROOT)/src/staticsprite89.c \
          $(IMAGESEQUENCER89_ROOT)/src/imagesequencer89.c \
          $(RENLIST89_ROOT)/src/renlist89.c \
          $(RENLIST89_ROOT)/adapters/imagesequencer89/renlist89_imagesequencer89.c \
          $(TILECELL89_ROOT)/src/tilecell89.c \
          $(GMSPRITESTRIP89_ROOT)/src/gmspritestrip89.c \
          $(SPRITEPLANE89_ROOT)/src/spriteplane89.c \
          $(SPRITEASSET89_ROOT)/src/spriteasset89.c \
          $(SPRITEASSET89_ROOT)/src/spriteasset89_softblit.c \
          $(SPRITEASSET89_ROOT)/src/spriteasset89_renlist89.c \
          $(SPRITEASSET89_ROOT)/adapters/imgcc0/spriteasset89_imgcc0.c \
          $(SPRITEVERBS89_ROOT)/src/spriteverbs89.c \
          $(SPRITEVERBS89_ROOT)/adapters/aseprite/spriteverbs89_aseprite.c \
          $(SPRITEVERBS89_ROOT)/adapters/assetroute89/spriteverbs89_assetroute.c \
          $(SPRITEVERBS89_ROOT)/adapters/ddsl2/spriteverbs89_ddsl2.c \
          $(ASSETROUTE89_ROOT)/src/assetroute89.c \
          $(ASSETROUTE89_ROOT)/providers/posix/assetroute89_posix.c \
          $(ASSETROUTE89_ROOT)/providers/win32/assetroute89_win32.c \
          src/blank3d_mount_vehicle.c \
          src/blank3d_vehicle_system.c \
          $(MOUNT89_ROOT)/src/mount89.c \
          $(GVEHICLE89_SOURCES) \
          $(GVEHPOS89_SOURCES) \
          src/blank3d_faction.c \
          $(GFACTION89_SOURCES) \
          src/blank3d_input.c \
          $(INPUT_FOURHEAD_SOURCES) \
          src/blank3d_ddsl_input.c \
          src/blank3d_list_cycle.c \
          $(CYCLER89_ROOT)/src/cycler89.c \
          src/blank3d_automotion.c \
          src/blank3d_gloco.c \
          src/blank3d_gloco_profile_ini.c \
          src/blank3d_kinverbs.c \
          $(GK3D_ROOT)/src/gk3d.c \
          $(GAMEVERBS89_ROOT)/src/gameverbs89.c \
          src/blank3d_condor.c \
          $(CONDOR_EVACT89_ROOT)/src/condor_evact89.c \
          $(ITEM_CONTACT_SOURCES) \
          $(GLOCO89_ROOT)/src/gloco89.c \
          $(GLOCO89_ROOT)/src/gloco89_bridge.c \
          $(GLOCO89_ROOT)/src/gloco89_profiles.c \
          src/blank3d_motion_attack.c \
          src/blank3d_truth_gate.c \
          src/blank3d_perception.c \
          src/blank3d_perception_ini.c \
          src/blank3d_attachment.c \
          src/blank3d_weapon_presentation.c \
          src/blank3d_mechanical_weapon.c \
          src/blank3d_actor_equipment.c \
          $(EQUIPMENT_SYSTEM89_ROOT)/src/equipment_system89.c \
          $(ACTOR_SYSTEM89_ROOT)/src/actor_system89.c \
          src/blank3d_classes.c \
          $(CLASS_MANAGER89_ROOT)/src/cm89.c \
          src/blank3d_runtime_spine.c \
          $(THING_SYSTEM89_ROOT)/src/thing_system89.c \
          $(ECS89_ROOT)/src/ecs89.c \
          $(WORLD3D89_ROOT)/src/w3d89.c \
          $(WORLD3D89_ROOT)/src/w3d89_arena.c \
          $(SCENE3D89_ROOT)/src/sm3d_scene.c \
          $(SCENE3D89_ROOT)/src/sm3d_arena.c \
          src/blank3d_katana_mesh.c \
          src/blank3d_katana_melee.c \
          $(KATANA89_ROOT)/src/katana89.c \
          $(PDC3D_MELEE_SOURCES) \
          src/blank3d_vphysics.c \
          src/blank3d_casing_physics.c \
          $(GATTACH89_SOURCES) \
          $(NATIONALMECANICANIMAL89_SOURCES) \
          $(VPHYSICS_SOURCES) \
          $(GEDER_SOURCES) \
          $(NPC_EYES_SOURCES) \
          $(ENLIGHTENER_SOURCES) \
          $(GAIRLUNGE89_ROOT)/src/gairlunge89.c \
          $(GGROUNDLANCE89_ROOT)/src/ggroundlance89.c \
          $(GAUTOMOTION_ROOT)/src/gmove89_types.c \
          $(GAUTOMOTION_ROOT)/src/gmove89_math.c \
          $(GAUTOMOTION_ROOT)/src/gautmove89.c \
          $(GAUTOMOTION_ROOT)/src/gmovepattern89.c \
          $(GAUTOMOTION_ROOT)/src/gmovesequence89.c \
          $(INPUT_HOOK_ROOT)/input_hook.c \
          $(INPUT_SCANNER_ROOT)/input_scanner.c \
          $(INPUT_KEYS89_ROOT)/src/input_keys89.c \
          $(SCANEMU89_ROOT)/src/scanemu89.c \
          src/blank3d_vertical_axis.c \
          $(FLY89_ROOT)/src/fly89.c \
          $(JUMP89_ROOT)/src/jump89.c \
          $(AIRDIVER89_ROOT)/src/airdiver89.c \
          $(MOVEMENTBASEVERBS89_ROOT)/src/3d_movementbaseverbs89.c \
          $(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d/3d_movementbaseverbs89_gamlib3d.c \
          $(INVARIANTS89_ROOT)/src/invariantSpecialoperations_89.c \
          $(INVARIANTS89_ROOT)/adapters/ddsl2/invariantSpecialoperations_89_ddsl2.c \
          $(INVARIANTS89_ROOT)/adapters/flags89/invariantSpecialoperations_89_flags89.c \
          src/engine_bridge.c \
          src/blank3d_systems.c \
          src/blank3d_pickups.c \
          src/blank3d_variables.c \
          $(VAR_MANAGER89_ROOT)/src/var_manager89.c \
          $(VAR_DSL89_ROOT)/src/var_dsl89.c \
          $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
          src/blank3d_weapon_host_io.c \
          $(WEAPON_HOSTED_VENDOR_SOURCES) \
          src/blank3d_weapon_ini.c \
          src/blank3d_weapon_loadout.c \
          src/blank3d_npc_inventory.c \
          src/blank3d_languages.c \
          src/blank3d_objects.c \
          src/blank3d_audio.c \
          src/blank3d_goldie_audio89.c \
          $(GOLDIE_SOURCES) \
          src/blank3d_text89.c \
          $(FONTCORE_SOURCES) \
          src/blank3d_config.c \
          src/blank3d_crosshair.c \
          $(CROSSHAIR_SOURCES) \
          src/blank3d_hud.c \
          src/blank3d_gproj_ammo_gbar.c \
          src/blank3d_numbar.c \
          src/blank3d_bighud.c \
          src/blank3d_ecg_vitals.c \
          src/blank3d_collision.c \
          src/blank3d_projectile_mesh.c \
          src/blank3d_weapon_modules.c \
          src/blank3d_ballistics.c vendor/weapon_system/g3dweaponzeroing89/src/g3dweaponzeroing89.c vendor/weapon_system/gweaponlaunch89/src/gweaponlaunch89.c \
          $(GTRIGGER89_ROOT)/src/gtrigger89.c \
          $(GPPA89_ROOT)/src/gplayerprojectileaim89.c \
          $(GPS89_ROOT)/src/gprojectilespawn89.c \
          $(SATELLABORNER89_ROOT)/src/satellaborner89.c \
          $(TELESEARCHER89_ROOT)/src/telesearcher89.c \
          $(EXPANDIBLEFIRE89_ROOT)/src/expandiblefire89.c \
          $(BULLETSPIN89_ROOT)/src/bulletspin89.c \
          $(BULLETCIRCLE89_ROOT)/src/bulletcircle89.c \
          $(BULLETINLINE89_ROOT)/src/bulletinline89.c \
          $(MORETHANONE89_ROOT)/src/morethanone89.c \
          $(GCR89_ROOT)/src/gcasingruntime89.c \
          $(GWS89_ROOT)/src/gweaponsnapshot89.c \
          src/blank3d_universal_aim.c \
          src/blank3d_bolt.c \
          src/blank3d_trails.c \
          src/blank3d_sniper.c \
          src/blank3d_camera_profiles.c \
          src/blank3d_cameranaku.c \
          src/blank3d_fire_frame_sync.c \
          src/blank3d_shotgun.c \
          src/blank3d_player_fire_ray.c \
          src/blank3d_player_projectile_aim.c \
          $(COLLISION_SOURCES) \
          vendor/gamlib3d/gamlib3d_camera.c \
          vendor/gamlib3d/gamlib3d_scalar.c \
          vendor/gamlib3d/gamlib3d_transform.c \
          vendor/gamlib3d/math_helpers/gamlib3d_math.c \
          vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
          vendor/soquete3d/soquete3d.c \
          vendor/giffany_shapes3d/g3d_shapes.c \
          vendor/numsys/src/numsys.c \
          vendor/flags/flags_fx.c \
          vendor/flags/flags_pool.c \
          vendor/flags/flags_util.c \
          vendor/flags/flags_value.c \
          vendor/flags/flagstore.c \
          vendor/gkinventory/src/gkinv.c \
          vendor/gkinventory/src/gkinv_fixed.c \
          vendor/gbar89/src/gbar89.c \
          vendor/gbar89/src/gbar89_bighud.c \
          $(GPROJ2D89_ROOT)/src/gproj2d89.c \
          vendor/conf_total/conf_total.c \
          vendor/weapon_system/gbulletmesh89/gbulletmesh89/src/gbulletmesh89.c \
          vendor/weapon_system/rocketmeshes/rocketmeshes/src/rocketmeshes.c \
          vendor/weapon_system/ghandgrenade3d89/ghandgrenade3d89/src/ghandgrenade3d89.c \
          $(BOLT_ROOT)/src/b3d_fixed.c \
          $(BOLT_ROOT)/src/b3d_vec3.c \
          $(BOLT_ROOT)/src/b3d_transform.c \
          $(BOLT_ROOT)/src/b3d_collision.c \
          $(BOLT_ROOT)/src/b3d_events.c \
          $(BOLT_ROOT)/src/b3d_projectile.c \
          $(BOLT_ROOT)/src/b3d_solver.c \
          $(BOLT_ROOT)/src/b3d_world.c \
          $(AOI_TRAIL_ROOT)/src/trail3d89.c \
          $(AOI_TRAIL_ROOT)/src/trail3d89_profiles.c \
          $(SCOPE_INI_ROOT)/src/gscopeini89.c \
          $(ZOOM_ROOT)/src/gtelescopiczoom89.c \
          $(SWAY_ROOT)/src/gsway89.c \
          $(SNIPER_HUD_ROOT)/src/gsniperhud89.c \
          $(AIM_ROOT)/src/gaimquery89.c \
          $(SCOPE_PAINT_ROOT)/src/gscopepaint89.c \
          $(SCOPE_VECTOR_ROOT)/src/gscopevector89.c \
          $(SCOPE_RASTER_ROOT)/src/gscoperaster89.c \
          $(SCOPE_BARS_ROOT)/src/gscopebars89.c \
          $(SCOPE_PRESETS_ROOT)/src/gscopepresets89.c \
          $(SCOPE_ANIM_ROOT)/src/gscopeanim89.c \
          $(SCOPE_PROVIDER_ROOT)/src/gscopeprovider89.c \
          $(SCOPE_BUNDLE_ROOT)/src/gscopebundle89.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_profiles.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_adapter.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_collision.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_spring_arm.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_target_group.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_composer.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_virtual.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_manager.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_zones.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_lens_ext.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_freelook.c \
          $(CAMERANAKU_ROOT)/src/cameranaku89_shake_ext.c \
          $(WEAPON_MANAGER)/src/gweapon89.c \
          $(ECG_SOURCES) \
          $(BIGHUD_SOURCES) \
          $(LANGUAGE_SOURCES) $(GFO_SOURCES)

LIMITS = \
 -DNS_MAX_TYPES=16 -DNS_MAX_VALUES=128 -DNS_MAX_THRESHOLDS=32 \
 -DNS_MAX_EVENTS=64 -DNS_MAX_TEMPLATES=8 -DNS_MAX_TEMPLATE_ITEMS=32 \
 -DNS_MAX_MODIFIERS=64 -DNS_MAX_DERIVED=16 -DNS_MAX_BINDINGS=16 \
 -DGWP89_MAX_WEAPONS=16 -DGWP89_MAX_USERS=32 -DGWP89_MAX_EVENTS=128 \
 -DGWP89_MAX_AMMO_TYPES=16 -DGWP89_MAX_PROVIDERS=24 \
 -DCM89_ENABLE_INTERNAL_INSTANCES=0 -DCM89_MAX_CLASSES=64 \
 -DCM89_MAX_BASES=4 -DCM89_MAX_MRO=16 \
 -DCM89_MAX_CLASS_MEMBERS=32 -DCM89_NAME_MAX=32 \
 -DVM89_MAX_GLOBALS=64 -DVM89_MAX_INSTANCES=128 \
 -DVM89_MAX_INSTANCE_VARS=24 -DVM89_MAX_LOCAL_FRAMES=16 \
 -DVM89_MAX_LOCALS_PER_FRAME=32 \
 -DGLOCO_MAX_ACTORS=64 -DGLOCO_MAX_PROFILES=64 \
 -DGVERB89_MAX_ENTRIES=256 -DGWT_USE_LONG_LONG=0

CFLAGS_COMMON = -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(INCLUDES)
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g
SYNTH_CFLAGS = -std=c89 -pedantic -Wall -Wextra -O2
LDLIBS = $(SYNTH_LIB) $(IMGCC0_LIB) -lopengl32 -lwinmm -lmsacm32 -lgdi32 -luser32 -lkernel32

.PHONY: all release debug run run-debug clean rebuild audit test test-core test-imgcc0-vendor test-spriteplane-vendor test-image-stack test-muzzle-image-pipeline test-morethanone-vendor test-buster-morethanone test-infinite-weapon-ammo test-special-reload test-item-contact-weapon test-item-contact-blank3d test-uzi-pickup test-health-pickup test-expandiblefire-vendor test-flamethrower test-flamethrower-stack test-flamethrower-billboard test-bulletspin-vendor test-bulletcircle-vendor test-bulletinline-vendor test-gatling-geometry-vendors test-gatling-geometry-stack \
        test-config test-audio-host test-collision test-projectile-meshes \
        test-weapon-modules test-crosshair-runtime test-crosshair-image-hybrid test-bolt-slingshot test-sniper-stack test-aoi-trail \
        test-camera-profile-catalog test-cameranaku-provider test-automatic-fire-frame-lock test-aim-convergence test-player-physical-projectile-aim test-shotgun-runtime \
        test-weapon-ini test-languages test-actor-weapon-isolation \
        test-npc-weapon-inventory test-vertical-motion test-vertical-vendors \
        test-class-manager-vendor test-gloco-vendor test-gloco-provider-stack test-gloco-ground-sweep test-3dkin-gf test-gameverbs89 test-condor-evact89 test-condor-bridge test-3dkin-gameverb-bridge test-var-runtime-vendor test-variables-bridge test-gfo-var-authoring test-class-object-spine test-runtime-object-spine test-input-stack test-input-fourhead-vocab test-input-fourhead-chain test-invariants-vendor test-perception-ini test-cycler-stack test-cycler-vendor test-automotion test-gautomotion-vendor test-motion-attacks test-motion-q16-win32 test-motion-attack-vendors test-ecg-vitals test-bighud test-numbar test-gbar-v04-bighud test-truth-gate test-geder-vendor test-perception-stack test-npc-eyes-vendor test-enlightener-vendor test-nationalmecanicanimal-vendor test-gattach-vendor test-attachment-stack test-mechanical-weapon test-actor-equipment test-runtime-spine test-thing-system-vendor test-actor-system-vendor test-equipment-system-vendor test-vphysics-vendor test-vphysics-provider test-casing-physics test-gfaction-vendor test-faction-bridge test-katana-melee test-rt-time89-vendor test-timeclocker89-vendor test-tickoclock89-vendor test-timeverbs89-vendor test-time-stack89 test-genwinconfigc89-vendor test-generalvideoconfigc89-vendor test-gameplayscreensizec89-vendor test-gwinvrbs89-vendor test-genvidverbs89-vendor test-gmplyss89-vendor test-display-sextet89 test-gameverb-runtime-capacity89 test-visual-feedback-authority89 test-howm89-vendor test-howm89-genwin-adapter test-windowswindow89-backend test-primitive2d89-vendor audit-window-authority89 audit-display-sextet89 audit-time-stack89 audit-link-response89 syntax-check syntax-check-input-win32 help FORCE

all: release
release: $(TARGET)
debug: $(DEBUG_TARGET)

physics-demo: $(VPHYSICS_DEMO_TARGET)

FORCE:

$(SYNTH_LIB): FORCE
	@current='$(SYNTH_TOOLCHAIN_ID)'; \
	stamp='$(SYNTH_STAMP)'; \
	if test ! -f "$$stamp" || test "x$$(cat "$$stamp" 2>/dev/null)" != "x$$current"; then \
		echo "[Blank3D] Rebuilding weapon synth for toolchain $$current"; \
		rm -rf '$(SYNTH_ROOT)/build'; \
	fi
	$(MAKE) -C $(SYNTH_ROOT) CC="$(CC)" AR="$(AR)" CFLAGS="$(SYNTH_CFLAGS)" build/libweapon_synth_sound_engine89.a
	@printf '%s\n' '$(SYNTH_TOOLCHAIN_ID)' > '$(SYNTH_STAMP)'

$(IMGCC0_LIB): FORCE
	@current='$(IMGCC0_TOOLCHAIN_ID)'; \
	stamp='$(IMGCC0_STAMP)'; \
	if test ! -f "$$stamp" || test "x$$(cat "$$stamp" 2>/dev/null)" != "x$$current"; then \
		echo "[Blank3D] Rebuilding imgcc0 for toolchain $$current"; \
		rm -rf '$(IMGCC0_ROOT)/build'; \
	fi
	$(MAKE) -C $(IMGCC0_ROOT) CC="$(CC)" AR="$(AR)" CFLAGS="-std=c89 -pedantic-errors -O2 -w" build/libimgcc0.a
	@printf '%s\n' '$(IMGCC0_TOOLCHAIN_ID)' > '$(IMGCC0_STAMP)'

# The full Blank3D command line is larger than the Win32/MSYS2 process
# argument limit.  Let GNU make write GCC response files directly instead of
# expanding hundreds of -I flags and source paths through the shell.  GCC
# expands @file itself, so the process command line stays tiny while the
# actual compiler/linker inputs remain unchanged.
$(TARGET): $(SOURCES) $(SYNTH_LIB) $(IMGCC0_LIB) Makefile
	$(file >$(RELEASE_RSP),$(CFLAGS_RELEASE) -mwindows $(SOURCES) $(LDLIBS) -o $@)
	$(CC) @$(RELEASE_RSP)

$(DEBUG_TARGET): $(SOURCES) $(SYNTH_LIB) $(IMGCC0_LIB) Makefile
	$(file >$(DEBUG_RSP),$(CFLAGS_DEBUG) $(SOURCES) $(LDLIBS) -o $@)
	$(CC) @$(DEBUG_RSP)

$(VPHYSICS_DEMO_TARGET): $(SOURCES) $(SYNTH_LIB) $(IMGCC0_LIB) Makefile
	$(file >$(VPHYSICS_DEMO_RSP),$(CFLAGS_DEBUG) -DB3D_VPHYSICS_DEMO=1 -DB3D_VPHYSICS_DEBUG_DRAW=1 $(SOURCES) $(LDLIBS) -o $@)
	$(CC) @$(VPHYSICS_DEMO_RSP)

run: release
	./$(TARGET)

run-debug: debug
	./$(DEBUG_TARGET)

test-satellaborner-vendor:
	$(MAKE) -C $(SATELLABORNER89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-telesearcher-vendor:
	$(MAKE) -C $(TELESEARCHER89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-sattele-vendors: test-satellaborner-vendor test-telesearcher-vendor

test-sattele-stack:
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror \
	 -I$(SATELLABORNER89_ROOT)/include -I$(TELESEARCHER89_ROOT)/include \
	 tests/test_sattele_projectile_stack.c \
	 $(SATELLABORNER89_ROOT)/src/satellaborner89.c \
	 $(TELESEARCHER89_ROOT)/src/telesearcher89.c \
	 -o tests/test_sattele_projectile_stack
	./tests/test_sattele_projectile_stack

test-morethanone-vendor:
	$(MAKE) -C $(MORETHANONE89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-infinite-weapon-ammo:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_infinite_weapon_ammo.c src/blank3d_weapon_ini.c \
	 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_infinite_weapon_ammo
	./tests/test_infinite_weapon_ammo

test-buster-morethanone:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_buster_morethanone.c src/blank3d_weapon_ini.c \
	 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 $(GTRIGGER89_ROOT)/src/gtrigger89.c $(MORETHANONE89_ROOT)/src/morethanone89.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_buster_morethanone
	./tests/test_buster_morethanone

test-bulletspin-vendor:
	$(MAKE) -C $(BULLETSPIN89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-bulletcircle-vendor:
	$(MAKE) -C $(BULLETCIRCLE89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-bulletinline-vendor:
	$(MAKE) -C $(BULLETINLINE89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-gatling-geometry-vendors: test-bulletspin-vendor test-bulletcircle-vendor test-bulletinline-vendor

test-gatling-geometry-stack:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_gatling_geometry_stack.c src/blank3d_weapon_ini.c \
	 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 $(BULLETSPIN89_ROOT)/src/bulletspin89.c \
	 $(BULLETCIRCLE89_ROOT)/src/bulletcircle89.c \
	 $(BULLETINLINE89_ROOT)/src/bulletinline89.c \
	 -o tests/test_gatling_geometry_stack
	./tests/test_gatling_geometry_stack

test-expandiblefire-vendor:
	$(MAKE) -C $(EXPANDIBLEFIRE89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror"

test-flamethrower-stack:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_flamethrower_expand.c src/blank3d_weapon_ini.c \
	 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c $(EXPANDIBLEFIRE89_ROOT)/src/expandiblefire89.c \
	 -o tests/test_flamethrower_expand
	./tests/test_flamethrower_expand

test-flamethrower: test-expandiblefire-vendor test-weapon-ini test-projectile-meshes test-flamethrower-stack test-flamethrower-billboard
	@echo "Blank3D ID14 flamethrower + expandiblefire89: PASS"


test-flamethrower-billboard: $(IMGCC0_LIB)
	mkdir -p build/tests
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) \
	 $(CORE_INCLUDES) \
	 tests/test_flamethrower_billboard.c \
	 src/blank3d_flamethrower_billboard89.c src/blank3d_image_assets.c \
	 $(ASSETROUTE89_ROOT)/src/assetroute89.c $(IMGCC0_LIB) \
	 -o build/tests/test_flamethrower_billboard
	./build/tests/test_flamethrower_billboard

test-katana-melee:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_katana_melee.c src/blank3d_katana_mesh.c \
	 src/blank3d_katana_melee.c $(KATANA89_ROOT)/src/katana89.c \
	 vendor/giffany_shapes3d/g3d_shapes.c vendor/soquete3d/soquete3d.c \
	 $(NATIONALMECANICANIMAL89_SOURCES) $(PDC3D_MELEE_SOURCES) \
	 -o tests/test_katana_melee
	./tests/test_katana_melee

test-nationalmecanicanimal-vendor:
	$(MAKE) -C $(NATIONALMECANICANIMAL89_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror -Iinclude"

test-gattach-vendor:
	$(MAKE) -C $(GATTACH89_ROOT) clean check CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Werror -Iinclude"

test-vphysics-vendor:
	$(MAKE) -C $(VPHYSICS_ROOT) clean strict test CC="$(CC)"

test-attachment-stack:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror \
	 -Isrc -Ivendor/soquete3d -I$(GATTACH89_ROOT)/include \
	 tests/test_attachment_stack.c src/blank3d_attachment.c \
	 $(GATTACH89_SOURCES) vendor/soquete3d/soquete3d.c \
	 -o tests/test_attachment_stack
	./tests/test_attachment_stack

test-mechanical-weapon:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(CORE_INCLUDES) \
	 tests/test_mechanical_weapon.c src/blank3d_mechanical_weapon.c \
	 $(GMECHANICALWEAPON89_ROOT)/src/gmechanicalweapon89.c \
	 src/blank3d_weapon_presentation.c $(GWEAPONPRESENTATION89_ROOT)/src/gweaponpresentation89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 $(NATIONALMECANICANIMAL89_SOURCES) $(GATTACH89_ROOT)/src/gattach89.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_mechanical_weapon
	./tests/test_mechanical_weapon

test-actor-system-vendor:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror \
	 -I$(ACTOR_SYSTEM89_ROOT)/include \
	 $(ACTOR_SYSTEM89_ROOT)/tests/test_actor_system89.c \
	 $(ACTOR_SYSTEM89_ROOT)/src/actor_system89.c \
	 -o tests/test_actor_system89
	./tests/test_actor_system89

test-equipment-system-vendor:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror \
	 -I$(EQUIPMENT_SYSTEM89_ROOT)/include \
	 $(EQUIPMENT_SYSTEM89_ROOT)/tests/test_equipment_system89.c \
	 $(EQUIPMENT_SYSTEM89_ROOT)/src/equipment_system89.c \
	 -o tests/test_equipment_system89
	./tests/test_equipment_system89

# GWeapon -> presentation -> GAttach -> per-actor NationalMecanicanimal89.
test-actor-equipment:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(CORE_INCLUDES) \
	 tests/test_actor_equipment.c src/blank3d_actor_equipment.c \
	 $(EQUIPMENT_SYSTEM89_ROOT)/src/equipment_system89.c \
	 src/blank3d_attachment.c src/blank3d_weapon_presentation.c \
	 $(GWEAPONPRESENTATION89_ROOT)/src/gweaponpresentation89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 src/blank3d_mechanical_weapon.c $(GMECHANICALWEAPON89_ROOT)/src/gmechanicalweapon89.c \
	 $(GATTACH89_SOURCES) $(NATIONALMECANICANIMAL89_SOURCES) \
	 vendor/soquete3d/soquete3d.c $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_actor_equipment
	./tests/test_actor_equipment

test-ecg-vitals:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Isrc -I$(ECG_ROOT)/include \
	 tests/test_ecg_vitals.c src/blank3d_ecg_vitals.c $(ECG_SOURCES) \
	 -o tests/test_ecg_vitals
	./tests/test_ecg_vitals

test-truth-gate:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Isrc -I$(GEDER_ROOT)/include \
	 tests/test_truth_gate.c src/blank3d_truth_gate.c $(GEDER_SOURCES) \
	 -o tests/test_truth_gate
	./tests/test_truth_gate

test-geder-vendor:
	$(MAKE) -C $(GEDER_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Iinclude"

# Socketer -> NPC Eyes -> EnlightenerAI -> GEDER portable integration test.
test-perception-stack:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_perception_stack.c src/blank3d_perception.c \
	 $(NPC_EYES_SOURCES) $(ENLIGHTENER_SOURCES) \
	 vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/soquete3d/soquete3d.c -o tests/test_perception_stack
	./tests/test_perception_stack


# Loads GEDER/NPC Eyes/Enlightener settings from entity INI files.
test-perception-ini:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_perception_ini.c src/blank3d_perception_ini.c \
	 src/blank3d_perception.c src/blank3d_truth_gate.c \
	 $(GEDER_SOURCES) $(NPC_EYES_SOURCES) $(ENLIGHTENER_SOURCES) \
	 vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/soquete3d/soquete3d.c -o tests/test_perception_ini
	./tests/test_perception_ini

test-npc-eyes-vendor:
	$(MAKE) -C $(NPC_EYES_ROOT) clean test CC="$(CC)" \
	 CFLAGS="-std=c89 -pedantic -Wall -Wextra -Iinclude"

test-enlightener-vendor:
	$(MAKE) -C $(ENLIGHTENER_ROOT) clean test CC="$(CC)"

# Portable parser/bridge test for .bighud-driven ECG, bars and counters.
test-bighud:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Isrc -Ivendor/gbar89/include \
	 -I$(ECG_ROOT)/include -I$(BIGHUD_ROOT)/include \
	 tests/test_bighud.c src/blank3d_bighud.c src/blank3d_ecg_vitals.c \
	 vendor/gbar89/src/gbar89.c vendor/gbar89/src/gbar89_bighud.c \
	 $(ECG_SOURCES) $(BIGHUD_SOURCES) -o tests/test_bighud
	./tests/test_bighud

# GFO -> INI orchestrator -> reusable .bhud preset integration.
test-numbar:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 -I$(ECG_ROOT)/include -I$(BIGHUD_ROOT)/include \
	 tests/test_numbar.c src/blank3d_numbar.c src/blank3d_bighud.c \
	 src/blank3d_ecg_vitals.c vendor/gbar89/src/gbar89.c \
	 vendor/gbar89/src/gbar89_bighud.c $(ECG_SOURCES) $(BIGHUD_SOURCES) \
	 -o tests/test_numbar
	./tests/test_numbar

# Exhaustive GBar89 v0.4 surface test through BigVaderHudder.

# Visual/parser regression for the explicit BVHUD -> GProj ammo renderer.
test-gproj-bvhud-visual:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic -Isrc -Ivendor/gbar89/include \
	 -Ivendor/ecg_embedded_c89/include -Ivendor/bigvaderhudder/include \
	 -I$(GPROJ2D89_ROOT)/include tests/test_gproj_bvhud_visual.c \
	 src/blank3d_bighud.c src/blank3d_ecg_vitals.c src/blank3d_gproj_ammo_gbar.c \
	 vendor/gbar89/src/gbar89.c vendor/gbar89/src/gbar89_bighud.c \
	 $(GPROJ2D89_ROOT)/src/gproj2d89.c $(ECG_SOURCES) $(BIGHUD_SOURCES) \
	 -o tests/test_gproj_bvhud_visual
	./tests/test_gproj_bvhud_visual

test-gbar-v04-bighud:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Isrc -Ivendor/gbar89/include \
	 -I$(ECG_ROOT)/include -I$(BIGHUD_ROOT)/include \
	 tests/test_gbar_v04_bighud.c src/blank3d_bighud.c src/blank3d_ecg_vitals.c \
	 vendor/gbar89/src/gbar89.c vendor/gbar89/src/gbar89_bighud.c \
	 $(ECG_SOURCES) $(BIGHUD_SOURCES) -o tests/test_gbar_v04_bighud
	./tests/test_gbar_v04_bighud

# Portable integration test; useful on Linux/macOS too (does not build Win32/OpenGL/audio shell).
test-core:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_systems.c src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
     src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
     src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
     src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_systems
	./tests/test_systems

# Player runner trigger -> GWeapon reload regression for special single-shot weapons.
test-special-reload:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_special_reload.c src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GTRIGGER89_ROOT)/src/gtrigger89.c vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c vendor/flags/flags_pool.c \
	 vendor/flags/flags_util.c vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_special_reload
	./tests/test_special_reload

# Strict standalone CT89 -> PBB -> weapon-provider bridge regression.
test-item-contact-weapon:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) \
	 -I$(CONTACT_TRIGGER89_ROOT)/include -I$(PBB_ITEM89_ROOT)/include -I$(PBB_CONTACT_WEAPON89_ROOT)/include \
	 $(ITEM_CONTACT_SOURCES) $(PBB_CONTACT_WEAPON89_ROOT)/tests/test_pbb_contact_weapon_bridge89.c \
	 -o tests/test_item_contact_weapon
	./tests/test_item_contact_weapon


# Portable Blank3D host integration: frame loop + GKInventory + GWP89.
test-item-contact-blank3d:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_item_contact_blank3d.c src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_item_contact_blank3d
	./tests/test_item_contact_blank3d

# Reproduces the exact player/NPC coupling around an empty NPC magazine.
test-actor-weapon-isolation:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_actor_weapon_isolation.c src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_actor_weapon_isolation
	./tests/test_actor_weapon_isolation


# Verifies per-NPC ownership, private ammo, per-weapon clips and runtime equip
# by arbitrary names/IDs loaded entirely from INI profiles.
test-npc-weapon-inventory:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_npc_weapon_inventory.c src/blank3d_npc_inventory.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_npc_weapon_inventory
	./tests/test_npc_weapon_inventory

test-config:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_config.c src/blank3d_config.c vendor/conf_total/conf_total.c \
	 -o tests/test_config
	./tests/test_config

test-collision:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_collision.c src/blank3d_collision.c $(COLLISION_SOURCES) \
	 -o tests/test_collision
	./tests/test_collision


test-vphysics-provider:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_vphysics_provider_stack.c src/blank3d_vphysics.c \
	 src/blank3d_collision.c $(VPHYSICS_SOURCES) $(COLLISION_SOURCES) \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 -o tests/test_vphysics_provider_stack
	./tests/test_vphysics_provider_stack

test-casing-physics:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_casing_vphysics.c src/blank3d_casing_physics.c \
	 src/blank3d_vphysics.c src/blank3d_collision.c \
	 $(VPHYSICS_SOURCES) $(COLLISION_SOURCES) \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 -o tests/test_casing_vphysics
	./tests/test_casing_vphysics

test-projectile-meshes:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_projectile_meshes.c src/blank3d_projectile_mesh.c \
	 vendor/giffany_shapes3d/g3d_shapes.c \
	 vendor/weapon_system/gbulletmesh89/gbulletmesh89/src/gbulletmesh89.c \
	 vendor/weapon_system/rocketmeshes/rocketmeshes/src/rocketmeshes.c \
	 vendor/weapon_system/ghandgrenade3d89/ghandgrenade3d89/src/ghandgrenade3d89.c \
	 -o tests/test_projectile_meshes
	./tests/test_projectile_meshes

test-weapon-modules:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_weapon_modules.c src/blank3d_weapon_modules.c \
	 $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 -o tests/test_weapon_modules
	./tests/test_weapon_modules

test-bolt-slingshot:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_bolt_slingshot.c src/blank3d_bolt.c \
	 $(GBOLTWEAPON89_ROOT)/src/gboltweapon89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c src/blank3d_collision.c \
	 $(BOLT_ROOT)/src/b3d_fixed.c $(BOLT_ROOT)/src/b3d_vec3.c \
	 $(BOLT_ROOT)/src/b3d_transform.c $(BOLT_ROOT)/src/b3d_collision.c \
	 $(BOLT_ROOT)/src/b3d_events.c $(BOLT_ROOT)/src/b3d_projectile.c \
	 $(BOLT_ROOT)/src/b3d_solver.c $(BOLT_ROOT)/src/b3d_world.c \
	 $(COLLISION_SOURCES) $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_bolt_slingshot
	./tests/test_bolt_slingshot


test-sniper-stack:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_sniper_stack.c src/blank3d_sniper.c $(GWEAPONSNIPER89_ROOT)/src/gweaponsniper89.c \
	 $(SCOPE_INI_ROOT)/src/gscopeini89.c \
	 $(ZOOM_ROOT)/src/gtelescopiczoom89.c \
	 $(SWAY_ROOT)/src/gsway89.c \
	 $(SNIPER_HUD_ROOT)/src/gsniperhud89.c \
	 $(AIM_ROOT)/src/gaimquery89.c \
	 $(SCOPE_PAINT_ROOT)/src/gscopepaint89.c \
	 $(SCOPE_VECTOR_ROOT)/src/gscopevector89.c \
	 $(SCOPE_RASTER_ROOT)/src/gscoperaster89.c \
	 $(SCOPE_BARS_ROOT)/src/gscopebars89.c \
	 $(SCOPE_PRESETS_ROOT)/src/gscopepresets89.c \
	 $(SCOPE_ANIM_ROOT)/src/gscopeanim89.c \
	 $(SCOPE_PROVIDER_ROOT)/src/gscopeprovider89.c \
	 $(SCOPE_BUNDLE_ROOT)/src/gscopebundle89.c \
	 -o tests/test_sniper_stack
	./tests/test_sniper_stack


test-aoi-trail:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_aoi_trail.c \
	 $(AOI_TRAIL_ROOT)/src/trail3d89.c \
	 $(AOI_TRAIL_ROOT)/src/trail3d89_profiles.c \
	 -o tests/test_aoi_trail
	./tests/test_aoi_trail



test-camera-profile-catalog:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_camera_profile_catalog.c src/blank3d_camera_profiles.c \
	 vendor/conf_total/conf_total.c -o tests/test_camera_profile_catalog
	./tests/test_camera_profile_catalog

test-cameranaku-provider:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_cameranaku_provider.c src/blank3d_camera_profiles.c src/blank3d_cameranaku.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89_profiles.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c vendor/conf_total/conf_total.c \
	 -o tests/test_cameranaku_provider
	./tests/test_cameranaku_provider


test-player-camera-fire-ray:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_player_camera_fire_ray.c src/blank3d_player_fire_ray.c \
	 $(GPLAYERFIRERAY89_ROOT)/src/gplayerfireray89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 -o tests/test_player_camera_fire_ray
	./tests/test_player_camera_fire_ray


test-automatic-fire-frame-lock:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_automatic_fire_frame_lock.c \
	 src/blank3d_fire_frame_sync.c $(GFIRE_FRAME89_ROOT)/src/gfireframe89.c src/blank3d_camera_profiles.c src/blank3d_cameranaku.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89_profiles.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c vendor/conf_total/conf_total.c \
	 -o tests/test_automatic_fire_frame_lock
	./tests/test_automatic_fire_frame_lock


test-player-physical-projectile-aim:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_player_physical_projectile_aim.c \
	 src/blank3d_player_projectile_aim.c src/blank3d_player_fire_ray.c \
	 $(GPLAYERFIRERAY89_ROOT)/src/gplayerfireray89.c \
	 $(GPPA89_ROOT)/src/gplayerprojectileaim89.c \
	 src/blank3d_universal_aim.c $(GWEAPONAIM89_ROOT)/src/gweaponaim89.c \
	 src/blank3d_ballistics.c $(GWEAPONBALLISTICS89_ROOT)/src/gweaponballistics89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/weapon_system/g3dweaponzeroing89/src/g3dweaponzeroing89.c vendor/weapon_system/gweaponlaunch89/src/gweaponlaunch89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/soquete3d/soquete3d.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_player_physical_projectile_aim
	./tests/test_player_physical_projectile_aim


test-weapon-launch-vendor:
	$(CC) $(CFLAGS) $(INCLUDES) -Werror \
	 vendor/weapon_system/gweaponlaunch89/tests/test_gweaponlaunch89.c \
	 vendor/weapon_system/gweaponlaunch89/src/gweaponlaunch89.c \
	 vendor/weapon_system/g3dweaponzeroing89/src/g3dweaponzeroing89.c \
	 vendor/weapon_system/gweapon89_manager_v2_super_agnostic_provider_bus/gweapon89_manager/src/gweapon89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 -o tests/test_gweaponlaunch89
	./tests/test_gweaponlaunch89

test-aim-convergence:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_aim_convergence.c src/blank3d_ballistics.c $(GWEAPONBALLISTICS89_ROOT)/src/gweaponballistics89.c vendor/weapon_system/g3dweaponzeroing89/src/g3dweaponzeroing89.c vendor/weapon_system/gweaponlaunch89/src/gweaponlaunch89.c \
	 src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_aim_convergence
	./tests/test_aim_convergence

test-universal-weapon-aim:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_universal_weapon_aim.c src/blank3d_universal_aim.c $(GWEAPONAIM89_ROOT)/src/gweaponaim89.c \
	 src/blank3d_ballistics.c $(GWEAPONBALLISTICS89_ROOT)/src/gweaponballistics89.c src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/weapon_system/g3dweaponzeroing89/src/g3dweaponzeroing89.c vendor/weapon_system/gweaponlaunch89/src/gweaponlaunch89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/soquete3d/soquete3d.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_universal_weapon_aim
	./tests/test_universal_weapon_aim


test-shotgun-runtime:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_shotgun_runtime.c src/blank3d_shotgun.c $(GSHOTGUN89_ROOT)/src/gshotgun89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_shotgun_runtime
	./tests/test_shotgun_runtime


test-crosshair-runtime:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(CORE_INCLUDES) \
	 tests/test_crosshair_runtime.c src/blank3d_crosshair.c $(GWEAPONCROSSHAIR89_ROOT)/src/gweaponcrosshair89.c \
	 $(CROSSHAIR_SOURCES) -o tests/test_crosshair_runtime
	./tests/test_crosshair_runtime

.PHONY: test-crosshair-image-hybrid
test-crosshair-image-hybrid:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(CORE_INCLUDES) \
	 tests/test_crosshair_image_hybrid.c src/blank3d_crosshair.c $(GWEAPONCROSSHAIR89_ROOT)/src/gweaponcrosshair89.c \
	 $(CROSSHAIR_SOURCES) -o build/tests/test_crosshair_image_hybrid
	./build/tests/test_crosshair_image_hybrid

test-weapon-ini:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_weapon_ini.c src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_weapon_ini
	./tests/test_weapon_ini

test-vertical-motion:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_vertical_motion.c src/blank3d_vertical_axis.c \
	 $(FLY89_ROOT)/src/fly89.c $(JUMP89_ROOT)/src/jump89.c \
	 $(AIRDIVER89_ROOT)/src/airdiver89.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 -o tests/test_vertical_motion
	./tests/test_vertical_motion

test-vertical-vendors:
	$(MAKE) -C $(FLY89_ROOT) clean all
	$(MAKE) -C $(JUMP89_ROOT) clean all
	$(MAKE) -C $(AIRDIVER89_ROOT) clean all


test-cycler-stack:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_cycler89.c src/blank3d_list_cycle.c \
	 $(CYCLER89_ROOT)/src/cycler89.c -o tests/test_cycler89
	./tests/test_cycler89


test-cycler-vendor:
	$(MAKE) -C $(CYCLER89_ROOT) clean all


test-input-stack:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_input_stack.c src/blank3d_input.c src/blank3d_ddsl_input.c \
	 $(INPUT_HOOK_ROOT)/input_hook.c $(INPUT_SCANNER_ROOT)/input_scanner.c \
	 $(INPUT_KEYS89_ROOT)/src/input_keys89.c $(SCANEMU89_ROOT)/src/scanemu89.c \
	 -o tests/test_input_stack
	./tests/test_input_stack

test-input-keys-vendor:
	$(MAKE) -C $(INPUT_KEYS89_ROOT) clean all test audit

test-input-fourhead-vocab:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(CORE_INCLUDES) \
	 tests/test_input_fourhead_vocab.c \
	 $(INPUT_KEYS89_ROOT)/src/input_keys89.c $(POLLS89_ROOT)/key_pc.c \
	 $(POLLS89_ROOT)/polls_input_keys89.c $(INPUT_HOOK_ROOT)/input_hook.c \
	 $(INPUT_SCANNER_ROOT)/input_scanner.c $(INPUT_SCANNER_ROOT)/input_backend_keyboard.c \
	 -o tests/test_input_fourhead_vocab
	./tests/test_input_fourhead_vocab

test-input-fourhead-chain:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(CORE_INCLUDES) \
	 tests/test_input_fourhead_chain.c \
	 $(INPUT_KEYS89_ROOT)/src/input_keys89.c $(INPUT_HOOK_ROOT)/input_hook.c \
	 $(INPUT_HOOK_ROOT)/input_hook_backend_polls89.c $(INPUT_SCANNER_ROOT)/input_scanner.c \
	 -o tests/test_input_fourhead_chain
	./tests/test_input_fourhead_chain



test-automotion:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_automotion.c src/blank3d_automotion.c \
	 $(GAUTOMOTION_ROOT)/src/gmove89_types.c \
	 $(GAUTOMOTION_ROOT)/src/gmove89_math.c \
	 $(GAUTOMOTION_ROOT)/src/gautmove89.c \
	 $(GAUTOMOTION_ROOT)/src/gmovepattern89.c \
	 vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 -o tests/test_automotion
	./tests/test_automotion


test-gautomotion-vendor:
	$(MAKE) -C $(GAUTOMOTION_ROOT) clean test

test-motion-attacks:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_motion_attacks.c src/blank3d_motion_attack.c \
	 $(GAIRLUNGE89_ROOT)/src/gairlunge89.c \
	 $(GGROUNDLANCE89_ROOT)/src/ggroundlance89.c \
	 vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 -o tests/test_motion_attacks
	./tests/test_motion_attacks


test-motion-q16-win32:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 -DGAL_FX_TYPE="signed int" -DGGL_FX_TYPE="signed int" \
	 tests/test_motion_q16_win32.c \
	 $(GAIRLUNGE89_ROOT)/src/gairlunge89.c \
	 $(GGROUNDLANCE89_ROOT)/src/ggroundlance89.c \
	 -o tests/test_motion_q16_win32
	./tests/test_motion_q16_win32


test-motion-attack-vendors:
	$(MAKE) -C $(GAIRLUNGE89_ROOT) clean all
	$(MAKE) -C $(GGROUNDLANCE89_ROOT) clean all

test-languages:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_languages.c src/blank3d_languages.c src/blank3d_ddsl_input.c \
	 $(INVARIANTS89_ROOT)/src/invariantSpecialoperations_89.c \
	 $(INVARIANTS89_ROOT)/adapters/ddsl2/invariantSpecialoperations_89_ddsl2.c \
	 $(INVARIANTS89_ROOT)/adapters/flags89/invariantSpecialoperations_89_flags89.c \
	 vendor/flags/flags_fx.c vendor/flags/flags_pool.c vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 $(GAMEVERBS89_ROOT)/src/gameverbs89.c \
	 $(LANGUAGE_SOURCES) $(GFO_SOURCES) -o tests/test_languages
	./tests/test_languages

# Exercises the complete synth host without requiring a real Windows audio
# device. The WinMM shim is test-only; the release build uses real winmm.dll.
test-audio-host: $(SYNTH_LIB)
	$(CC) -Itests/platform_stubs $(CFLAGS_COMMON) \
	 src/blank3d_audio.c src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c tests/platform_stubs/winmm_stub.c \
	 tests/test_audio_host.c $(WEAPON_MANAGER)/src/gweapon89.c \
	 $(SYNTH_LIB) -o tests/test_audio_host
	./tests/test_audio_host

test-thing-system-vendor:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic -I$(THING_SYSTEM89_ROOT)/include \
	 $(THING_SYSTEM89_ROOT)/src/thing_system89.c $(THING_SYSTEM89_ROOT)/tests/test_thing_system89.c \
	 -o tests/test_thing_system89_ctx
	./tests/test_thing_system89_ctx

test-runtime-spine:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic \
	 -Isrc -I$(THING_SYSTEM89_ROOT)/include -I$(ECS89_ROOT)/include \
	 -I$(WORLD3D89_ROOT)/include -I$(SCENE3D89_ROOT)/include \
	 src/blank3d_runtime_spine.c $(THING_SYSTEM89_ROOT)/src/thing_system89.c \
	 $(ECS89_ROOT)/src/ecs89.c $(WORLD3D89_ROOT)/src/w3d89.c \
	 $(WORLD3D89_ROOT)/src/w3d89_arena.c $(SCENE3D89_ROOT)/src/sm3d_scene.c \
	 $(SCENE3D89_ROOT)/src/sm3d_arena.c tests/test_runtime_spine.c \
	 -o tests/test_runtime_spine
	./tests/test_runtime_spine

test-class-manager-vendor:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic -I$(CLASS_MANAGER89_ROOT)/include \
	 $(CLASS_MANAGER89_ROOT)/src/cm89.c $(CLASS_MANAGER89_ROOT)/tests/test_cm89.c \
	 -o tests/test_cm89_full
	./tests/test_cm89_full
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) -I$(CLASS_MANAGER89_ROOT)/include \
	 $(CLASS_MANAGER89_ROOT)/src/cm89.c $(CLASS_MANAGER89_ROOT)/tests/test_cm89_bound.c \
	 -o tests/test_cm89_bound
	./tests/test_cm89_bound


test-runtime-object-spine:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) \
	 -Isrc -I$(CLASS_MANAGER89_ROOT)/include -I$(GFO_ROOT)/include -I$(THING_SYSTEM89_ROOT)/include -I$(ECS89_ROOT)/include \
	 -I$(WORLD3D89_ROOT)/include -I$(SCENE3D89_ROOT)/include \
	 src/blank3d_classes.c $(CLASS_MANAGER89_ROOT)/src/cm89.c \
	 src/blank3d_runtime_spine.c src/blank3d_objects.c $(GFO_SOURCES) \
	 $(THING_SYSTEM89_ROOT)/src/thing_system89.c $(ECS89_ROOT)/src/ecs89.c \
	 $(WORLD3D89_ROOT)/src/w3d89.c $(WORLD3D89_ROOT)/src/w3d89_arena.c \
	 $(SCENE3D89_ROOT)/src/sm3d_scene.c $(SCENE3D89_ROOT)/src/sm3d_arena.c \
	 tests/test_runtime_object_spine.c -o tests/test_runtime_object_spine
	./tests/test_runtime_object_spine


test-class-object-spine:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) \
	 -Isrc -I$(CLASS_MANAGER89_ROOT)/include -I$(GFO_ROOT)/include \
	 src/blank3d_classes.c $(CLASS_MANAGER89_ROOT)/src/cm89.c \
	 src/blank3d_objects.c $(GFO_SOURCES) tests/test_class_object_spine.c \
	 -o tests/test_class_object_spine
	./tests/test_class_object_spine

test-gfaction-vendor:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic -I$(GFACTION89_ROOT)/include \
	 $(GFACTION89_SOURCES) $(GFACTION89_ROOT)/demo/demo_zombie_rampage.c \
	 -o tests/gfaction_demo_rampage
	(cd $(GFACTION89_ROOT) && ../../tests/gfaction_demo_rampage)
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic -I$(GFACTION89_ROOT)/include \
	 $(GFACTION89_SOURCES) $(GFACTION89_ROOT)/demo/demo_faction_war.c \
	 -o tests/gfaction_demo_war
	(cd $(GFACTION89_ROOT) && ../../tests/gfaction_demo_war)


test-faction-bridge:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(CORE_INCLUDES) \
	 tests/test_faction_bridge.c src/blank3d_faction.c $(GFACTION89_SOURCES) \
	 -o tests/test_faction_bridge
	./tests/test_faction_bridge

test-var-runtime-vendor:
	mkdir -p build/tests
	$(CC) $(CFLAGS_COMMON) -Werror \
	 $(VAR_MANAGER89_ROOT)/src/var_manager89.c \
	 $(VAR_DSL89_ROOT)/src/var_dsl89.c \
	 $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
	 $(VAR_RUNTIME89_ROOT)/tests/test_var_runtime89.c \
	 -o build/tests/test_var_runtime89
	./build/tests/test_var_runtime89

test-gloco-vendor:
	$(MAKE) -C $(GLOCO89_ROOT) clean test-engine-ready

test-gloco-provider-stack:
	mkdir -p build/tests
	$(CC) $(CFLAGS_COMMON) -Werror \
	 tests/test_gloco_provider_stack.c src/blank3d_gloco.c src/blank3d_gloco_profile_ini.c \
	 $(GLOCO89_ROOT)/src/gloco89.c $(GLOCO89_ROOT)/src/gloco89_bridge.c $(GLOCO89_ROOT)/src/gloco89_profiles.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/src/3d_movementbaseverbs89.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d/3d_movementbaseverbs89_gamlib3d.c \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 src/blank3d_variables.c $(VAR_MANAGER89_ROOT)/src/var_manager89.c $(VAR_DSL89_ROOT)/src/var_dsl89.c $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c vendor/flags/flags_pool.c vendor/flags/flags_util.c vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 -o build/tests/test_gloco_provider_stack
	./build/tests/test_gloco_provider_stack

# Regression for a real Collision floor contact returning normal -Y.
# Horizontal locomotion must not classify that floor contact as a wall.
test-gloco-ground-sweep:
	mkdir -p build/tests
	$(CC) $(CFLAGS_COMMON) -Werror \
	 tests/test_gloco_ground_sweep.c src/blank3d_gloco.c src/blank3d_gloco_profile_ini.c \
	 src/blank3d_collision.c $(COLLISION_SOURCES) \
	 $(GLOCO89_ROOT)/src/gloco89.c $(GLOCO89_ROOT)/src/gloco89_bridge.c $(GLOCO89_ROOT)/src/gloco89_profiles.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/src/3d_movementbaseverbs89.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d/3d_movementbaseverbs89_gamlib3d.c \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 src/blank3d_variables.c $(VAR_MANAGER89_ROOT)/src/var_manager89.c $(VAR_DSL89_ROOT)/src/var_dsl89.c $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c vendor/flags/flags_pool.c vendor/flags/flags_util.c vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 -o build/tests/test_gloco_ground_sweep
	./build/tests/test_gloco_ground_sweep


test-gameverbs89:
	$(MAKE) -C $(GAMEVERBS89_ROOT) clean test


test-condor-evact89:
	$(MAKE) -C $(CONDOR_EVACT89_ROOT) clean test audit


test-condor-bridge:
	mkdir -p build/tests
	$(CC) $(CFLAGS_COMMON) -Werror \
	 tests/test_condor_bridge.c src/blank3d_condor.c \
	 $(CONDOR_EVACT89_ROOT)/src/condor_evact89.c \
	 $(GAMEVERBS89_ROOT)/src/gameverbs89.c \
	 src/blank3d_variables.c $(VAR_MANAGER89_ROOT)/src/var_manager89.c \
	 $(VAR_DSL89_ROOT)/src/var_dsl89.c $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c vendor/flags/flags_pool.c \
	 vendor/flags/flags_util.c vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 $(INPUT_SCANNER_ROOT)/input_scanner.c \
	 -o build/tests/test_condor_bridge
	./build/tests/test_condor_bridge


test-3dkin-gf:
	$(MAKE) -C $(GK3D_ROOT) clean verify


test-3dkin-gameverb-bridge:
	mkdir -p build/tests
	$(CC) $(CFLAGS_COMMON) -Werror \
	 tests/test_3dkin_gameverb_bridge.c src/blank3d_kinverbs.c \
	 $(GAMEVERBS89_ROOT)/src/gameverbs89.c $(GK3D_ROOT)/src/gk3d.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/src/3d_movementbaseverbs89.c \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 -o build/tests/test_3dkin_gameverb_bridge
	./build/tests/test_3dkin_gameverb_bridge

test-variables-bridge:
	$(CC) $(CFLAGS_COMMON) -Werror \
	 tests/test_variables_bridge.c src/blank3d_variables.c \
	 $(VAR_MANAGER89_ROOT)/src/var_manager89.c \
	 $(VAR_DSL89_ROOT)/src/var_dsl89.c \
	 $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_value.c \
	 vendor/flags/flagstore.c -o tests/test_variables_bridge
	./tests/test_variables_bridge

# Parses every active source against small Win32/OpenGL declarations. This is
# useful on non-Windows hosts; it does not replace a final MinGW link.
test-mount-vehicle:
	$(CC) $(CFLAGS_COMMON) src/blank3d_mount_vehicle.c $(MOUNT89_ROOT)/src/mount89.c \
	 $(GVEHICLE89_SOURCES) $(GVEHPOS89_SOURCES) \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 tests/test_mount_vehicle.c -o tests/test_mount_vehicle
	./tests/test_mount_vehicle

.PHONY: test-vehicle-ini-yard
test-vehicle-ini-yard:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(INCLUDES) \
	 src/blank3d_vehicle_system.c src/blank3d_mount_vehicle.c $(MOUNT89_ROOT)/src/mount89.c \
	 $(GVEHICLE89_SOURCES) $(GVEHPOS89_SOURCES) \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 tests/test_vehicle_ini_yard.c -o tests/test_vehicle_ini_yard
	./tests/test_vehicle_ini_yard

test-imgcc0-vendor:
	$(MAKE) -C $(IMGCC0_ROOT) CC="$(CC)" AR="$(AR)" CFLAGS="-std=c89 -pedantic-errors -O2 -w" build/libimgcc0.a


test-spriteplane-vendor:
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror -I$(SPRITEPLANE89_ROOT)/include \
	 $(SPRITEPLANE89_ROOT)/src/spriteplane89.c $(SPRITEPLANE89_ROOT)/demo/demo_spriteplane89.c \
	 -o build/tests/spriteplane89_demo
	./build/tests/spriteplane89_demo


test-image-stack: $(IMGCC0_LIB)
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror \
	 -I$(IMGCC0_ROOT)/include -I$(SPRITEPLANE89_ROOT)/include -I$(ASSETROUTE89_ROOT)/include -Isrc \
	 tests/test_image_stack.c src/blank3d_image_assets.c src/blank3d_spriteplanes.c \
	 $(SPRITEPLANE89_ROOT)/src/spriteplane89.c $(ASSETROUTE89_ROOT)/src/assetroute89.c $(IMGCC0_LIB) \
	 -o build/tests/test_image_stack
	./build/tests/test_image_stack

.PHONY: test-muzzle-image-pipeline
test-muzzle-image-pipeline: $(IMGCC0_LIB)
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_muzzle_image_pipeline.c src/blank3d_weapon_ini.c \
	 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c $(WEAPON_MANAGER)/src/gweapon89.c \
	 src/blank3d_image_assets.c src/blank3d_spriteplanes.c src/blank3d_muzzle_image.c src/blank3d_muzzle_light.c \
	 $(SPRITEPLANE89_ROOT)/src/spriteplane89.c $(ASSETROUTE89_ROOT)/src/assetroute89.c $(IMGCC0_LIB) \
	 -o build/tests/test_muzzle_image_pipeline
	./build/tests/test_muzzle_image_pipeline

.PHONY: test-pistol-muzzle-flash
test-pistol-muzzle-flash: $(IMGCC0_LIB)
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic -Wall -Wextra -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_pistol_muzzle_flash.c src/blank3d_weapon_ini.c \
	 $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 $(GWEAPONIO89_ROOT)/src/gweaponio89.c src/blank3d_weapon_host_io.c $(WEAPON_MANAGER)/src/gweapon89.c \
	 src/blank3d_image_assets.c src/blank3d_spriteplanes.c src/blank3d_muzzle_image.c src/blank3d_muzzle_light.c \
	 $(SPRITEPLANE89_ROOT)/src/spriteplane89.c $(ASSETROUTE89_ROOT)/src/assetroute89.c $(IMGCC0_LIB) \
	 -o build/tests/test_pistol_muzzle_flash
	./build/tests/test_pistol_muzzle_flash

.PHONY: test-staticsprite89-vendor test-imagesequencer89-vendor test-renlist89-vendor test-tilecell89-vendor test-gmspritestrip89-vendor test-sprite-microvendors89 test-projectile-sprite-modes89

test-staticsprite89-vendor:
	$(MAKE) -C $(STATICSPRITE89_ROOT) CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude" test audit

test-imagesequencer89-vendor:
	$(MAKE) -C $(IMAGESEQUENCER89_ROOT) CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude" test audit

test-renlist89-vendor:
	$(MAKE) -C $(RENLIST89_ROOT) CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude" test audit

test-tilecell89-vendor:
	$(MAKE) -C $(TILECELL89_ROOT) CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude" test audit

test-gmspritestrip89-vendor:
	$(MAKE) -C $(GMSPRITESTRIP89_ROOT) clean all CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror"
	./$(GMSPRITESTRIP89_ROOT)/test_gmspritestrip89

test-sprite-microvendors89: test-staticsprite89-vendor test-imagesequencer89-vendor test-renlist89-vendor test-tilecell89-vendor test-gmspritestrip89-vendor
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 -I$(STATICSPRITE89_ROOT)/include -I$(IMAGESEQUENCER89_ROOT)/include \
	 -I$(RENLIST89_ROOT)/include -I$(TILECELL89_ROOT)/include \
	 -I$(RENLIST89_ROOT)/adapters/imagesequencer89 \
	 -I$(TILECELL89_ROOT)/adapters/imagesequencer89 \
	 -I$(IMAGESEQUENCER89_ROOT)/adapters/staticsprite89 \
	 $(STATICSPRITE89_ROOT)/src/staticsprite89.c \
	 $(IMAGESEQUENCER89_ROOT)/src/imagesequencer89.c \
	 $(RENLIST89_ROOT)/src/renlist89.c \
	 $(TILECELL89_ROOT)/src/tilecell89.c \
	 $(RENLIST89_ROOT)/adapters/imagesequencer89/renlist89_imagesequencer89.c \
	 $(TILECELL89_ROOT)/adapters/imagesequencer89/tilecell89_imagesequencer89.c \
	 $(IMAGESEQUENCER89_ROOT)/adapters/staticsprite89/imagesequencer89_staticsprite89.c \
	 tests/test_sprite_microvendors89.c -o build/tests/test_sprite_microvendors89
	./build/tests/test_sprite_microvendors89

.PHONY: test-spriteasset89-vendor test-spriteverbs89-vendor test-assetroute89-vendor test-sprite-runtime89 syntax-check-sprite-runtime89-win32
test-spriteasset89-vendor:
	$(MAKE) -C $(SPRITEASSET89_ROOT) CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror -O2"

# SpriteVerbs89's upstream Makefile assumes SpriteAsset89 is a sibling.
# Blank3D owns the vendor graph explicitly, so compile the integration layout here.
test-spriteverbs89-vendor:
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 -I$(SPRITEVERBS89_ROOT)/include -I$(SPRITEVERBS89_ROOT)/adapters/aseprite \
	 -I$(SPRITEASSET89_ROOT)/include \
	 $(SPRITEVERBS89_ROOT)/src/spriteverbs89.c \
	 $(SPRITEVERBS89_ROOT)/adapters/aseprite/spriteverbs89_aseprite.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89.c \
	 $(SPRITEVERBS89_ROOT)/tests/test_spriteverbs89.c -o build/tests/test_spriteverbs89
	./build/tests/test_spriteverbs89

test-assetroute89-vendor:
	$(MAKE) -C $(ASSETROUTE89_ROOT) CC="$(CC)" CFLAGS="-std=c89 -pedantic-errors -Wall -Wextra -Werror -O2"

test-sprite-runtime89: $(IMGCC0_LIB)
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 -Isrc -I$(IMGCC0_ROOT)/include -I$(SPRITEASSET89_ROOT)/include \
	 -I$(SPRITEVERBS89_ROOT)/include -I$(SPRITEVERBS89_ROOT)/adapters/aseprite \
	 -I$(SPRITEVERBS89_ROOT)/adapters/assetroute89 \
	 -I$(ASSETROUTE89_ROOT)/include -I$(ASSETROUTE89_ROOT)/providers/posix \
	 -I$(ASSETROUTE89_ROOT)/providers/win32 \
	 tests/test_sprite_runtime89.c src/blank3d_sprite_runtime89.c src/blank3d_image_assets.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89.c $(SPRITEASSET89_ROOT)/src/spriteasset89_renlist89.c \
	 $(SPRITEVERBS89_ROOT)/src/spriteverbs89.c \
	 $(SPRITEVERBS89_ROOT)/adapters/aseprite/spriteverbs89_aseprite.c \
	 $(SPRITEVERBS89_ROOT)/adapters/assetroute89/spriteverbs89_assetroute.c \
	 $(ASSETROUTE89_ROOT)/src/assetroute89.c \
	 $(ASSETROUTE89_ROOT)/providers/posix/assetroute89_posix.c \
	 $(ASSETROUTE89_ROOT)/providers/win32/assetroute89_win32.c $(IMGCC0_LIB) \
	 -o build/tests/test_sprite_runtime89
	./build/tests/test_sprite_runtime89

syntax-check-sprite-runtime89-win32:
	$(CC) -D_WIN32 -U__linux__ -Itests/platform_stubs \
	 -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 -Isrc -I$(IMGCC0_ROOT)/include \
	 -I$(SPRITEASSET89_ROOT)/include -I$(SPRITEASSET89_ROOT)/adapters/imgcc0 \
	 -I$(SPRITEVERBS89_ROOT)/include -I$(SPRITEVERBS89_ROOT)/adapters/aseprite \
	 -I$(SPRITEVERBS89_ROOT)/adapters/assetroute89 \
	 -I$(ASSETROUTE89_ROOT)/include -I$(ASSETROUTE89_ROOT)/providers/posix \
	 -I$(ASSETROUTE89_ROOT)/providers/win32 \
	 -fsyntax-only src/blank3d_sprite_runtime89.c src/blank3d_image_assets.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89_renlist89.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89_softblit.c \
	 $(SPRITEASSET89_ROOT)/adapters/imgcc0/spriteasset89_imgcc0.c \
	 $(SPRITEVERBS89_ROOT)/src/spriteverbs89.c \
	 $(SPRITEVERBS89_ROOT)/adapters/aseprite/spriteverbs89_aseprite.c \
	 $(SPRITEVERBS89_ROOT)/adapters/assetroute89/spriteverbs89_assetroute.c \
	 $(ASSETROUTE89_ROOT)/src/assetroute89.c \
	 $(ASSETROUTE89_ROOT)/providers/win32/assetroute89_win32.c

test-rt-time89-vendor:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror -I$(RT_TIME89_ROOT) \
	 $(RT_TIME89_ROOT)/rt_time.c $(RT_TIME89_ROOT)/tests/test_rt_time.c \
	 -o tests/test_rt_time89
	./tests/test_rt_time89

test-timeclocker89-vendor:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror -I$(TIMECLOCKER89_ROOT)/include \
	 $(TIMECLOCKER89_ROOT)/src/timeclocker.c $(TIMECLOCKER89_ROOT)/tests/test_timeclocker.c \
	 -o tests/test_timeclocker89
	./tests/test_timeclocker89

test-tickoclock89-vendor:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror -I$(TICKOCLOCK89_ROOT)/include \
	 $(TICKOCLOCK89_ROOT)/src/tickoclock89.c $(TICKOCLOCK89_ROOT)/tests/test_tickoclock89.c \
	 -o tests/test_tickoclock89
	./tests/test_tickoclock89

test-timeverbs89-vendor:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror -I$(TIMEVERBS89_ROOT)/include \
	 $(TIMEVERBS89_ROOT)/src/timeverbs89.c $(TIMEVERBS89_ROOT)/tests/test_timeverbs89.c \
	 -o tests/test_timeverbs89
	./tests/test_timeverbs89

test-time-stack89:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 -Isrc -Ivendor/gameverbs89/include -I$(RT_TIME89_ROOT) \
	 -I$(TIMECLOCKER89_ROOT)/include -I$(TICKOCLOCK89_ROOT)/include \
	 -I$(TIMEVERBS89_ROOT)/include \
	 src/blank3d_time89.c src/blank3d_timeverbs89.c \
	 vendor/gameverbs89/src/gameverbs89.c \
	 $(RT_TIME89_ROOT)/rt_time.c $(TIMECLOCKER89_ROOT)/src/timeclocker.c \
	 $(TICKOCLOCK89_ROOT)/src/tickoclock89.c $(TIMEVERBS89_ROOT)/src/timeverbs89.c \
	 tests/test_time_stack89.c -o tests/test_time_stack89
	./tests/test_time_stack89

test-genwinconfigc89-vendor:
	$(MAKE) -C $(GENWINCONFIGC89_ROOT) clean test audit CC="$(CC)"

test-generalvideoconfigc89-vendor:
	$(MAKE) -C $(GENERALVIDEOCONFIGC89_ROOT) clean test audit CC="$(CC)"

test-gameplayscreensizec89-vendor:
	$(MAKE) -C $(GAMEPLAYSCREENSIZEC89_ROOT) clean test audit CC="$(CC)"

test-gwinvrbs89-vendor:
	$(MAKE) -C $(GWINVRBS89_ROOT) clean test audit CC="$(CC)"

test-genvidverbs89-vendor:
	$(MAKE) -C $(GENVIDVERBS89_ROOT) clean test audit CC="$(CC)"

test-gmplyss89-vendor:
	$(MAKE) -C $(GMPLYSS89_ROOT) clean test audit CC="$(CC)"

test-display-sextet89: test-genwinconfigc89-vendor test-generalvideoconfigc89-vendor test-gameplayscreensizec89-vendor test-gwinvrbs89-vendor test-genvidverbs89-vendor test-gmplyss89-vendor test-howm89-genwin-adapter
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror $(CORE_INCLUDES) \
	 tests/test_display_sextet89.c src/blank3d_display_stack89.c \
	 $(GENWINCONFIGC89_ROOT)/src/genwinconfigc89.c \
	 $(GENERALVIDEOCONFIGC89_ROOT)/src/generalvideoconfigc89.c \
	 $(GAMEPLAYSCREENSIZEC89_ROOT)/src/gameplayscreensizec89.c \
	 $(GWINVRBS89_ROOT)/src/gwinvrbs89.c \
	 $(GENVIDVERBS89_ROOT)/src/genvidverbs89.c \
	 $(GMPLYSS89_ROOT)/src/gmplyss89.c \
	 $(GAMEVERBS89_ROOT)/src/gameverbs89.c -o tests/test_display_sextet89
	./tests/test_display_sextet89

# Runtime-capacity regression: Blank3D has 97 GameVerbs before the display
# sextet registers its 47 names. 128 entries silently killed WinMain before
# native window creation; the host capacity is deliberately 256.
test-gameverb-runtime-capacity89:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 -DGVERB89_MAX_ENTRIES=256 $(CORE_INCLUDES) \
	 tests/test_gameverb_runtime_capacity89.c src/blank3d_display_stack89.c \
	 $(GENWINCONFIGC89_ROOT)/src/genwinconfigc89.c \
	 $(GENERALVIDEOCONFIGC89_ROOT)/src/generalvideoconfigc89.c \
	 $(GAMEPLAYSCREENSIZEC89_ROOT)/src/gameplayscreensizec89.c \
	 $(GWINVRBS89_ROOT)/src/gwinvrbs89.c \
	 $(GENVIDVERBS89_ROOT)/src/genvidverbs89.c \
	 $(GMPLYSS89_ROOT)/src/gmplyss89.c \
	 $(GAMEVERBS89_ROOT)/src/gameverbs89.c \
	 -o tests/test_gameverb_runtime_capacity89
	./tests/test_gameverb_runtime_capacity89

audit-display-sextet89:
	@echo "Protocol89 display-sextet source audit:"
	@! grep -RIn -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
	 $(GENWINCONFIGC89_ROOT)/src $(GENWINCONFIGC89_ROOT)/include \
	 $(GENERALVIDEOCONFIGC89_ROOT)/src $(GENERALVIDEOCONFIGC89_ROOT)/include \
	 $(GAMEPLAYSCREENSIZEC89_ROOT)/src $(GAMEPLAYSCREENSIZEC89_ROOT)/include \
	 $(GWINVRBS89_ROOT)/src $(GWINVRBS89_ROOT)/include \
	 $(GENVIDVERBS89_ROOT)/src $(GENVIDVERBS89_ROOT)/include \
	 $(GMPLYSS89_ROOT)/src $(GMPLYSS89_ROOT)/include || \
	 (echo "allocator token found in display vendor" && exit 1)
	@! grep -RIn -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b|stdint\.h' \
	 $(GENWINCONFIGC89_ROOT)/src $(GENWINCONFIGC89_ROOT)/include \
	 $(GENERALVIDEOCONFIGC89_ROOT)/src $(GENERALVIDEOCONFIGC89_ROOT)/include \
	 $(GAMEPLAYSCREENSIZEC89_ROOT)/src $(GAMEPLAYSCREENSIZEC89_ROOT)/include \
	 $(GWINVRBS89_ROOT)/src $(GWINVRBS89_ROOT)/include \
	 $(GENVIDVERBS89_ROOT)/src $(GENVIDVERBS89_ROOT)/include \
	 $(GMPLYSS89_ROOT)/src $(GMPLYSS89_ROOT)/include || \
	 (echo "float/64-bit token found in display vendor" && exit 1)
	@echo "Protocol89 display-sextet vendor audit passed."

audit-time-stack89:
	@echo "Protocol89 time-stack source audit:"
	@! grep -RIn -E '\b(malloc|calloc|realloc|free)[[:space:]]*\(' \
	 $(RT_TIME89_ROOT)/*.c $(RT_TIME89_ROOT)/*.h \
	 $(TIMECLOCKER89_ROOT)/src/*.c $(TIMECLOCKER89_ROOT)/include/*.h \
	 $(TICKOCLOCK89_ROOT)/src/*.c $(TICKOCLOCK89_ROOT)/include/*.h \
	 $(TIMEVERBS89_ROOT)/src/*.c $(TIMEVERBS89_ROOT)/include/*.h || \
	 (echo "allocator token found in time vendor" && exit 1)
	@! grep -RIn -E '\b(float|double|long[[:space:]]+long|int64_t|uint64_t)\b|stdint\.h' \
	 $(RT_TIME89_ROOT)/*.c $(RT_TIME89_ROOT)/*.h \
	 $(TIMECLOCKER89_ROOT)/src/*.c $(TIMECLOCKER89_ROOT)/include/*.h \
	 $(TICKOCLOCK89_ROOT)/src/*.c $(TICKOCLOCK89_ROOT)/include/*.h \
	 $(TIMEVERBS89_ROOT)/src/*.c $(TIMEVERBS89_ROOT)/include/*.h || \
	 (echo "float/64-bit token found in time vendor" && exit 1)
	@echo "Protocol89 time-stack vendor audit passed."

test-howm89-vendor:
	$(MAKE) -C $(HOWM89_ROOT) clean test
	$(MAKE) -C $(HOWM89_ROOT) clean


test-howm89-genwin-adapter:
	mkdir -p build/tests
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic \
	 -I$(HOWM89_ROOT)/include -I$(HOWM89_ROOT)/adapters/genwinconfigc89 \
	 -I$(GENWINCONFIGC89_ROOT)/include \
	 $(HOWM89_ROOT)/src/howm89.c \
	 $(HOWM89_ROOT)/adapters/genwinconfigc89/howm89_genwinconfigc89.c \
	 $(GENWINCONFIGC89_ROOT)/src/genwinconfigc89.c \
	 tests/test_howm89_genwinconfigc89_adapter.c \
	 -o build/tests/test_howm89_genwinconfigc89_adapter
	./build/tests/test_howm89_genwinconfigc89_adapter


test-windowswindow89-backend:
	$(MAKE) -C $(WINDOWSWINDOW89_ROOT) audit
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic -fsyntax-only \
	 -Itests/platform_stubs -I$(HOWM89_ROOT)/include -I$(WINDOWSWINDOW89_ROOT)/include \
	 $(WINDOWSWINDOW89_ROOT)/src/windowswindow89.c


test-primitive2d89-vendor:
	$(MAKE) -C $(PRIMITIVE2D89_ROOT) clean test
	$(MAKE) -C $(PRIMITIVE2D89_ROOT) clean


audit-window-authority89:
	@echo "Window authority audit:"
	@! grep -RInE 'howm89|HOWM89|windowswindow89|WindowsWindow89' $(GENWINCONFIGC89_ROOT)/include $(GENWINCONFIGC89_ROOT)/src --include='*.h' --include='*.c' || (echo "GenWinConfigC89 leaked HOWM89/backend dependency"; exit 1)
	@! grep -nE '\b(CreateWindowA|RegisterClassA|AdjustWindowRect|SetWindowLongA|SetWindowPos)\b' src/blank3d_window_win32.c || (echo "Blank3D bridge still creates native windows directly"; exit 1)
	@$(MAKE) -C $(WINDOWSWINDOW89_ROOT) audit
	@$(MAKE) -C $(HOWM89_ROOT) protocol-audit
	@echo "Window authority audit PASS"


.PHONY: test-visual-feedback-authority89
test-visual-feedback-authority89:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 tests/test_visual_feedback_authority89.c -o tests/test_visual_feedback_authority89
	./tests/test_visual_feedback_authority89

.PHONY: test-hud-world-light-isolation89
test-hud-world-light-isolation89:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror \
	 tests/test_hud_world_light_isolation89.c -o tests/test_hud_world_light_isolation89
	./tests/test_hud_world_light_isolation89

syntax-check:
	$(file >$(SYNTAX_RSP),-Itests/platform_stubs $(CFLAGS_COMMON) -fsyntax-only $(SOURCES))
	$(CC) @$(SYNTAX_RSP)

# Validate the canonical four-head Windows path even when this host has no
# MinGW linker.  The windows.h shim supplies declarations only; this target
# checks that no Win32 type/keycode leaks above the platform provider.
syntax-check-input-win32:
	$(CC) -D_WIN32 -U__linux__ -Itests/platform_stubs $(CFLAGS_COMMON) -fsyntax-only \
	 src/blank3d_input.c src/blank3d_input_platform.c \
	 $(INPUT_KEYS89_ROOT)/src/input_keys89.c \
	 $(POLLS89_ROOT)/key_pc.c $(POLLS89_ROOT)/polls_input_keys89.c \
	 $(POLLS89_ROOT)/by_system_backend/win32/winpckeys_backend.c \
	 $(INPUT_HOOK_ROOT)/input_hook.c $(INPUT_HOOK_ROOT)/input_hook_backend_polls89.c \
	 $(INPUT_HOOK_ROOT)/input_hook_backend_polls89_winpckeys.c \
	 $(INPUT_SCANNER_ROOT)/input_scanner.c $(SCANEMU89_ROOT)/src/scanemu89.c

test-invariants-vendor:
	$(MAKE) -C $(INVARIANTS89_ROOT) clean test audit


test-movementbaseverbs-vendor:
	mkdir -p build/tests
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic \
	 -I$(MOVEMENTBASEVERBS89_ROOT)/include \
	 $(MOVEMENTBASEVERBS89_ROOT)/src/3d_movementbaseverbs89.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/tests/test_movementbaseverbs89.c \
	 -o build/tests/test_movementbaseverbs89
	./build/tests/test_movementbaseverbs89
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic \
	 -I$(MOVEMENTBASEVERBS89_ROOT)/include \
	 -I$(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d \
	 -Ivendor/gamlib3d -Ivendor/gamlib3d/math_helpers \
	 $(MOVEMENTBASEVERBS89_ROOT)/src/3d_movementbaseverbs89.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d/3d_movementbaseverbs89_gamlib3d.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/tests/test_gamlib3d_adapter.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 -o build/tests/test_movementbaseverbs89_gamlib3d
	./build/tests/test_movementbaseverbs89_gamlib3d

test-weapon-runtime-vendors:
	$(MAKE) -C $(GTRIGGER89_ROOT) clean test
	$(MAKE) -C $(GPPA89_ROOT) clean test
	$(MAKE) -C $(GPS89_ROOT) clean test
	$(MAKE) -C $(GCR89_ROOT) clean test
	$(MAKE) -C $(GWS89_ROOT) clean test

test-weapon-system-hosted-vendors: test-weapon-runtime-vendors test-weapon-launch-vendor \
 test-weapon-modules test-bolt-slingshot test-sniper-stack \
 test-player-camera-fire-ray test-player-physical-projectile-aim \
 test-automatic-fire-frame-lock test-aim-convergence test-universal-weapon-aim \
 test-shotgun-runtime test-crosshair-runtime test-weapon-ini \
 test-mechanical-weapon test-actor-equipment test-actor-system-vendor test-equipment-system-vendor test-actor-weapon-isolation \
 test-npc-weapon-inventory syntax-check audit

test: test-display-sextet89 test-gameverb-runtime-capacity89 test-visual-feedback-authority89 test-hud-world-light-isolation89 test-howm89-vendor test-windowswindow89-backend test-primitive2d89-vendor audit-window-authority89 test-rt-time89-vendor test-timeclocker89-vendor test-tickoclock89-vendor test-timeverbs89-vendor test-time-stack89 test-class-manager-vendor test-gloco-vendor test-gloco-provider-stack test-gloco-ground-sweep test-3dkin-gf test-gameverbs89 test-condor-evact89 test-condor-bridge test-3dkin-gameverb-bridge test-var-runtime-vendor test-variables-bridge test-gfo-var-authoring test-class-object-spine test-runtime-object-spine test-runtime-spine test-thing-system-vendor test-actor-system-vendor test-equipment-system-vendor test-weapon-runtime-vendors test-weapon-launch-vendor test-invariants-vendor test-movementbaseverbs-vendor test-katana-melee test-player-camera-fire-ray test-player-physical-projectile-aim test-gfaction-vendor test-faction-bridge test-vphysics-vendor test-vphysics-provider test-casing-physics test-gattach-vendor test-attachment-stack test-nationalmecanicanimal-vendor test-mechanical-weapon test-actor-equipment test-npc-eyes-vendor test-enlightener-vendor test-perception-stack test-perception-ini test-geder-vendor test-truth-gate test-bighud test-numbar test-gfo-entities test-gbar-v04-bighud test-ecg-vitals test-motion-attack-vendors test-motion-q16-win32 test-motion-attacks test-gautomotion-vendor test-automotion test-cycler-stack test-cycler-vendor test-input-keys-vendor test-input-fourhead-vocab test-input-fourhead-chain test-input-stack test-core test-morethanone-vendor test-buster-morethanone test-infinite-weapon-ammo test-special-reload test-actor-weapon-isolation test-npc-weapon-inventory test-vertical-motion test-vertical-vendors test-config test-projectile-meshes test-weapon-modules test-crosshair-runtime test-weapon-ini test-languages test-bolt-slingshot test-sniper-stack test-aoi-trail test-camera-profile-catalog test-cameranaku-provider test-automatic-fire-frame-lock test-aim-convergence test-universal-weapon-aim test-shotgun-runtime test-audio-host test-collision audit-display-sextet89 audit-time-stack89 syntax-check syntax-check-input-win32 audit


audit-link-response89:
	@grep -q '\$$(CC) @\$$(RELEASE_RSP)' Makefile || { echo "release link is not response-file based"; exit 1; }
	@grep -q '\$$(CC) @\$$(DEBUG_RSP)' Makefile || { echo "debug link is not response-file based"; exit 1; }
	@grep -q '\$$(CC) @\$$(VPHYSICS_DEMO_RSP)' Makefile || { echo "vphysics demo link is not response-file based"; exit 1; }
	@grep -q '\$$(CC) @\$$(SYNTAX_RSP)' Makefile || { echo "syntax-check is not response-file based"; exit 1; }
	@echo "Blank3D response-file link audit: PASS"

clean:
	rm -f tests/test_gameverb_runtime_capacity89 tests/test_visual_feedback_authority89 tests/test_hud_world_light_isolation89 $(RELEASE_RSP) $(DEBUG_RSP) $(VPHYSICS_DEMO_RSP) $(SYNTAX_RSP) build/tests/test_howm89_genwinconfigc89_adapter $(TARGET) $(DEBUG_TARGET) $(VPHYSICS_DEMO_TARGET) tests/test_input_fourhead_vocab tests/test_input_fourhead_chain tests/test_systems tests/test_special_reload tests/test_config tests/test_katana_melee \
	 tests/test_audio_host tests/test_collision tests/test_projectile_meshes tests/test_gweaponlaunch89 \
	 tests/test_gatling_projectiles tests/test_weapon_modules tests/test_crosshair_runtime \
	 tests/test_bolt_slingshot tests/test_sniper_stack tests/test_aoi_trail \
	 tests/test_camera_profile_catalog tests/test_cameranaku_provider tests/test_player_camera_fire_ray tests/test_player_physical_projectile_aim tests/test_automatic_fire_frame_lock tests/test_aim_convergence \
	 tests/test_universal_weapon_aim tests/test_shotgun_runtime \
	 tests/test_weapon_ini tests/test_languages \
	 tests/test_actor_weapon_isolation tests/test_npc_weapon_inventory tests/test_infinite_weapon_ammo \
	 tests/test_vertical_motion tests/test_input_stack tests/test_cycler89 \
	 build/tests/test_gloco_provider_stack build/tests/test_gloco_ground_sweep build/tests/test_condor_bridge \
	 build/tests/test_movementbaseverbs89 build/tests/test_movementbaseverbs89_gamlib3d \
	 tests/test_automotion tests/test_motion_attacks \
	 tests/test_motion_q16_win32 tests/test_ecg_vitals tests/test_bighud \
	 tests/test_numbar tests/test_gbar_v04_bighud \
	 tests/test_truth_gate tests/test_perception_stack tests/test_perception_ini \
	 tests/test_mechanical_weapon tests/test_attachment_stack \
	 tests/test_actor_equipment tests/test_actor_system89 tests/test_equipment_system89 build/tests/test_var_runtime89 tests/test_variables_bridge build/tests/test_3dkin_gameverb_bridge tests/test_gfo_var_authoring tests/test_cm89_full tests/test_cm89_bound tests/test_class_object_spine tests/test_runtime_object_spine tests/test_runtime_spine tests/test_thing_system89_ctx tests/test_vphysics_provider_stack tests/test_casing_vphysics \
	 tests/test_faction_bridge tests/gfaction_demo_rampage tests/gfaction_demo_war test_gfo_entities \
	 tests/test_rt_time89 tests/test_timeclocker89 tests/test_tickoclock89 tests/test_timeverbs89 tests/test_time_stack89 tests/test_display_sextet89
	$(MAKE) -C $(SYNTH_ROOT) clean
	@if [ -f $(RPYL_ROOT)/Makefile ]; then $(MAKE) -C $(RPYL_ROOT) clean; fi
	$(MAKE) -C $(FLY89_ROOT) clean
	$(MAKE) -C $(JUMP89_ROOT) clean
	$(MAKE) -C $(INPUT_KEYS89_ROOT) clean
	$(MAKE) -C $(AIRDIVER89_ROOT) clean
	$(MAKE) -C $(MOVEMENTBASEVERBS89_ROOT) clean
	$(MAKE) -C $(GTRIGGER89_ROOT) clean
	$(MAKE) -C $(GPPA89_ROOT) clean
	$(MAKE) -C $(GPS89_ROOT) clean
	$(MAKE) -C $(GCR89_ROOT) clean
	$(MAKE) -C $(GWS89_ROOT) clean
	$(MAKE) -C $(CYCLER89_ROOT) clean
	$(MAKE) -C $(GAUTOMOTION_ROOT) clean
	$(MAKE) -C $(GLOCO89_ROOT) clean
	$(MAKE) -C $(GAMEVERBS89_ROOT) clean
	$(MAKE) -C $(CONDOR_EVACT89_ROOT) clean
	$(MAKE) -C $(GK3D_ROOT) clean
	$(MAKE) -C $(GAIRLUNGE89_ROOT) clean
	$(MAKE) -C $(GGROUNDLANCE89_ROOT) clean
	$(MAKE) -C $(GEDER_ROOT) clean
	$(MAKE) -C $(NPC_EYES_ROOT) clean
	$(MAKE) -C $(ENLIGHTENER_ROOT) clean
	$(MAKE) -C $(NATIONALMECANICANIMAL89_ROOT) clean
	$(MAKE) -C $(GATTACH89_ROOT) clean
	$(MAKE) -C $(VPHYSICS_ROOT) clean
	$(MAKE) -C $(GFACTION89_ROOT) clean
	$(MAKE) -C $(GENWINCONFIGC89_ROOT) clean
	$(MAKE) -C $(GENERALVIDEOCONFIGC89_ROOT) clean
	$(MAKE) -C $(GAMEPLAYSCREENSIZEC89_ROOT) clean
	$(MAKE) -C $(GWINVRBS89_ROOT) clean
	$(MAKE) -C $(GENVIDVERBS89_ROOT) clean
	$(MAKE) -C $(GMPLYSS89_ROOT) clean
	$(MAKE) -C vendor/gbar89 clean

rebuild: clean all

audit:
	@echo "Active integration allocation audit:"
	@! grep -n -E '(^|[^[:alnum:]_])(malloc|realloc|free)[[:space:]]*\(' \
	 src/blank3d_*.c src/monika_blank3d.c vendor/numsys/src/numsys.c \
	 $(VAR_MANAGER89_ROOT)/src/*.c $(VAR_DSL89_ROOT)/src/*.c $(VAR_RUNTIME89_ROOT)/src/*.c \
	 $(GAMEVERBS89_ROOT)/src/*.c $(GK3D_ROOT)/src/*.c $(CONDOR_EVACT89_ROOT)/src/*.c \
	 $(RT_TIME89_ROOT)/*.c $(TIMECLOCKER89_ROOT)/src/*.c $(TICKOCLOCK89_ROOT)/src/*.c $(TIMEVERBS89_ROOT)/src/*.c \
	 $(GENWINCONFIGC89_ROOT)/src/*.c $(GENERALVIDEOCONFIGC89_ROOT)/src/*.c $(GAMEPLAYSCREENSIZEC89_ROOT)/src/*.c \
	 $(GWINVRBS89_ROOT)/src/*.c $(GENVIDVERBS89_ROOT)/src/*.c $(GMPLYSS89_ROOT)/src/*.c \
	 src/blank3d_time89.c src/blank3d_timeverbs89.c src/blank3d_display_stack89.c \
	 $(ECG_SOURCES) $(BIGHUD_SOURCES) $(GEDER_SOURCES) \
	 $(NPC_EYES_SOURCES) $(ENLIGHTENER_SOURCES) \
	 $(NATIONALMECANICANIMAL89_SOURCES) $(GATTACH89_SOURCES) $(VPHYSICS_SOURCES) $(GFACTION89_SOURCES) \
	 src/blank3d_attachment.c src/blank3d_mechanical_weapon.c \
	 src/blank3d_katana_mesh.c src/blank3d_katana_melee.c \
	 $(KATANA89_ROOT)/src/katana89.c $(PDC3D_MELEE_SOURCES) \
	 src/blank3d_truth_gate.c src/blank3d_perception.c \
	 vendor/gbar89/src/gbar89_bighud.c \
	 vendor/fly89/src/*.c vendor/jump89/src/*.c vendor/airdiver89/src/*.c \
	 $(MOVEMENTBASEVERBS89_ROOT)/src/*.c $(MOVEMENTBASEVERBS89_ROOT)/adapters/gamlib3d/*.c \
	 $(INVARIANTS89_ROOT)/src/*.c $(INVARIANTS89_ROOT)/adapters/ddsl2/*.c $(INVARIANTS89_ROOT)/adapters/flags89/*.c \
	 $(GTRIGGER89_ROOT)/src/*.c $(GPPA89_ROOT)/src/*.c $(GPS89_ROOT)/src/*.c \
	 $(EXPANDIBLEFIRE89_ROOT)/src/*.c $(GCR89_ROOT)/src/*.c $(GWS89_ROOT)/src/*.c $(ITEM_CONTACT_SOURCES) \
 $(ACTOR_SYSTEM89_ROOT)/src/*.c $(EQUIPMENT_SYSTEM89_ROOT)/src/*.c \
 src/blank3d_input.c src/blank3d_ddsl_input.c src/blank3d_languages.c \
 vendor/input_hook89/input_hook.c vendor/input_scanner89/input_scanner.c \
 vendor/input_scanner89/input_backend_keyboard.c \
 vendor/input_keys89/src/input_keys89.c vendor/scanemu89/src/scanemu89.c \
 $(INPUT_FOURHEAD_COMMON_SOURCES) $(INPUT_FOURHEAD_PLATFORM_SOURCES) \
 src/blank3d_list_cycle.c vendor/cycler89/src/cycler89.c \
 src/blank3d_automotion.c vendor/gautomotion89/src/*.c \
	 src/blank3d_motion_attack.c vendor/gairlunge89/src/*.c \
	 vendor/ggroundlance89/src/*.c \
	 vendor/flags/*.c vendor/gkinventory/src/*.c $(WEAPON_MANAGER)/src/gweapon89.c || \
	 (echo "allocator token found" && exit 1)
	@echo "OK: no allocator calls in active gameplay/system integration"
	@echo "Simulation float/double audit (renderer/audio API boundaries may contain them):"
	@grep -n -E '\b(float|double)\b' src/blank3d_systems.c src/blank3d_input.c src/blank3d_ddsl_input.c src/monika_blank3d.c vendor/scanemu89/src/scanemu89.c || true

help:
	@echo "make test-display-sextet89 verify window/video/gameplay six-vendor separation"
	@echo "make audit-display-sextet89 verify Protocol89 constraints in six display vendors"
	@echo "make           build blank3d.exe with complete weapon synth library"
	@echo "make debug     build console/debug executable"
	@echo "make test-core run portable NUMSYS/flags/inventory/gweapon89 test"
	@echo "make test-item-contact-weapon verify CT89 -> PBB -> weapon/ammo pickup bridge"
	@echo "make test-item-contact-blank3d verify Blank3D frame/inventory/weapon pickup integration"
	@echo "make test-uzi-pickup verify RPY/INI/GFO Uzi + ammo touch pickups"
	@echo "make test-health-pickup verify generic white-box health pickup via PBB"
	@echo "make test-condor-evact89 verify context Event -> Condition -> Action core"
	@echo "make test-condor-bridge verify GameVerbs/Vars/Input bridge"
	@echo "make test-var-runtime-vendor verify VarDSL -> provider router -> dynamic VarStore"
	@echo "make test-variables-bridge verify Blank3D NumSys/Flags/Thing variable providers"
	@echo "make test-input-keys-vendor build/test standalone input_keys89"
	@echo "make test-vphysics-provider verify transform/collision provider bridge"
	@echo "make test-casing-physics verify shell bounce, damping and sleep"
	@echo "make test-vphysics-vendor run strict Total Solver upstream tests"
	@echo "make test-ecg-vitals verify HP states, threat pulse and damage flash"
	@echo "make test-bighud verify base ECG plus reusable .bhud presets"
	@echo "make test-numbar verify GFO/INI/preset NumBar orchestration"
	@echo "make test-gbar-v04-bighud verify the complete GBar89 v0.4 BVH surface"
	@echo "make test-hud-world-light-isolation89 verify world muzzle lights cannot tint HUD/overlay"
	@echo "make test-truth-gate verify Blank3D GEDER profiles and accumulation"
	@echo "make test-geder-vendor run the standalone GEDER upstream tests"
	@echo "make test-perception-stack verify Socketer/Eyes/Enlightener fallback"
	@echo "make test-katana-melee verify katana89 + Mecanim + frame-gated physical collision"
	@echo "make test-nationalmecanicanimal-vendor run the selected upstream motion tests"
	@echo "make test-gattach-vendor run the standalone GAttach89 tests"
	@echo "make test-attachment-stack verify carrier -> socket -> object -> animator"
	@echo "make test-mechanical-weapon verify data-driven mechanism animation"
	@echo "make test-actor-equipment verify any actor can receive and animate a weapon object"
	@echo "make test-perception-ini   verify entity [perception] loading"
	@echo "make test-npc-eyes-vendor run the 3D_NPC_Eyes upstream tests"
	@echo "make test-enlightener-vendor run the EnlightenerAI upstream tests"
	@echo "make test-config verify conf_total and config/blank3d.toml"
	@echo "make test-audio-host exercise synth events through a WinMM test shim"
	@echo "make test-collision verify CCS + SICOL-DE bullet sweep/provider"
	@echo "make test-projectile-meshes verify gbulletmesh89/rocketmeshes mapping"
	@echo "make test-morethanone-vendor verify standalone charge-stage selector"
	@echo "make test-buster-morethanone verify tap + charged-release Buster path"
	@echo "make test-infinite-weapon-ammo verify infinite ammo + pistol Buster fire DNA"
	@echo "make test-expandiblefire-vendor verify C89 expanding projectile behavior"
	@echo "make test-flamethrower-stack verify real INI -> growth/collision/kill composition"
	@echo "make test-flamethrower verify ID14 growth + 50-frame billboard fire skin"
	@echo "make test-sniper-stack verify zoom/sway/HUD/aim/preset integration"
	@echo "make test-aoi-trail verify native Aoi Trail mesh generation"
	@echo "make test-cameranaku-provider verify receive-provider TRS + weapon camera ABI"
	@echo "make test-automatic-fire-frame-lock verify moving + rotating automatic fire camera coherence"
	@echo "make test-weapon-runtime-vendors verify trigger/player-aim/spawn/casing/snapshot vendors"
	@echo "make test-weapon-launch-vendor verify portable Weapon System launch invariants"
	@echo "make test-aim-convergence verify HUD-ray convergence and gravity zeroing"
	@echo "make test-motion-attacks verify air lunge + ground lance bridge"
	@echo "make test-motion-q16-win32 verify MinGW32-safe Q16.16 attack vectors"
	@echo "make test-motion-attack-vendors build both combat movement vendors"
	@echo "make test-shotgun-runtime verify seven physical pellets and lifetime"
	@echo "make test-weapon-ini verify data-driven weapon profiles and audio recipes"
	@echo "make test-npc-weapon-inventory verify arbitrary INI weapons per NPC"
	@echo "make test-input-stack verify all-key names, edges, scanner, capture and DDSL syntax"
	@echo "make test-invariants-vendor verify invariantSpecialoperations_89 core + DDSL2 + flags adapters"
	@echo "make test-cycler-stack verify active-list next/previous and named list routing"
	@echo "make test-cycler-vendor build standalone libcycler89.a"
	@echo "make test-automotion verify engine bridge and pattern flavor"
	@echo "make test-gautomotion-vendor build and test standalone gautomotion89"
	@echo "make test-languages verify vendored DDSL2, FPIL and RPYL runtimes"
	@echo "make test-time-stack89 verify rt_time89 + TickOClock89 + TimeClocker89 + TimeVerbs89"
	@echo "make audit-time-stack89 reject heap, float/double and explicit 64-bit tokens in time vendors"
	@echo "make test-vertical-motion verify gravity, fly89, jump89, airdiver89 and providers"
	@echo "make test-movementbaseverbs-vendor verify verb vocabulary + Gamlib3D provider adapter"
	@echo "make test-vertical-vendors build all three standalone C89 static libraries"
	@echo "make syntax-check parse the complete active Win32/OpenGL source"
	@echo "make test       run every portable validation and audit"
	@echo "make audit     allocation and gameplay decimal-token scan"
	@echo "make clean     remove executables and synth build artifacts"

.PHONY: test-gfo-entities
test-gfo-var-authoring:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 -I$(CLASS_MANAGER89_ROOT)/include -I$(GFO_ROOT)/include \
	 tests/test_gfo_var_authoring.c src/blank3d_variables.c \
	 $(VAR_MANAGER89_ROOT)/src/var_manager89.c $(VAR_DSL89_ROOT)/src/var_dsl89.c \
	 $(VAR_RUNTIME89_ROOT)/src/var_runtime89.c \
	 src/blank3d_classes.c $(CLASS_MANAGER89_ROOT)/src/cm89.c \
	 src/blank3d_objects.c $(GFO_SOURCES) \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 -o tests/test_gfo_var_authoring
	./tests/test_gfo_var_authoring

test-gfo-entities:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) \
	 -Isrc -I$(CLASS_MANAGER89_ROOT)/include -I$(GFO_ROOT)/include \
	 src/blank3d_classes.c $(CLASS_MANAGER89_ROOT)/src/cm89.c \
	 src/blank3d_objects.c $(GFO_SOURCES) tests/test_gfo_entities.c \
	 -o test_gfo_entities
	./test_gfo_entities

.PHONY: test-weapon-system-hosted-vendors

# Generic item regression: RPY/INI -> CT89/PBB -> host health effect; no weapon grant.
test-health-pickup:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_health_pickup.c src/blank3d_pickups.c src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) \
	 src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/giffany_shapes3d/g3d_shapes.c vendor/soquete3d/soquete3d.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_health_pickup
	./tests/test_health_pickup

# Pickup content regression: catalog -> INI recipe -> CT89/PBB -> weapon + ammo.
test-uzi-pickup:
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_uzi_pickup.c src/blank3d_pickups.c src/blank3d_systems.c $(ITEM_CONTACT_SOURCES) \
	 src/blank3d_weapon_host_io.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c $(GWEAPONPROFILEIO89_ROOT)/src/gweaponprofileio89.c \
	 src/blank3d_weapon_loadout.c $(GWEAPONLOADOUT89_ROOT)/src/gweaponloadout89.c $(GWEAPONIO89_ROOT)/src/gweaponio89.c \
	 src/blank3d_weapon_modules.c $(GWEAPONMODULES89_ROOT)/src/gweaponmodules89.c \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/giffany_shapes3d/g3d_shapes.c vendor/soquete3d/soquete3d.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_uzi_pickup
	./tests/test_uzi_pickup
	$(CC) -std=c89 -Wall -Wextra -Werror -pedantic $(LIMITS) \
	 -Isrc -I$(CLASS_MANAGER89_ROOT)/include -I$(GFO_ROOT)/include \
	 src/blank3d_classes.c $(CLASS_MANAGER89_ROOT)/src/cm89.c \
	 src/blank3d_objects.c $(GFO_SOURCES) tests/test_uzi_pickup_gfo.c \
	 -o tests/test_uzi_pickup_gfo
	./tests/test_uzi_pickup_gfo


test-vehicle-system-split:
	$(MAKE) -C $(GVEHICLE89_ROOT) check CFLAGS="-std=c89 -Wall -Wextra -Werror -pedantic $(addprefix -I,$(CURDIR)/$(GVEHICLE89_ROOT)/include) -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/vehicleprovider89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/vehiclephysics89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/carmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/motorcyclemovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/busmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/tankmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/watermovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/airmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/rotorcraftmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/spacemovement89/include"

.PHONY: test-vehicle-providers
test-vehicle-providers:
	$(MAKE) -C $(GVEHICLE89_ROOT) test-provider-dispatch CFLAGS="-std=c89 -Wall -Wextra -Werror -pedantic $(addprefix -I,$(CURDIR)/$(GVEHICLE89_ROOT)/include) -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/vehicleprovider89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/vehiclephysics89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/carmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/motorcyclemovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/busmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/tankmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/watermovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/airmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/rotorcraftmovement89/include -I$(CURDIR)/$(GVEHICLE89_ROOT)/vendor/spacemovement89/include"
	$(CC) $(CFLAGS_COMMON) src/blank3d_mount_vehicle.c $(MOUNT89_ROOT)/src/mount89.c \
	 $(GVEHICLE89_SOURCES) $(GVEHPOS89_SOURCES) \
	 vendor/gamlib3d/gamlib3d_transform.c vendor/gamlib3d/gamlib3d_scalar.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 tests/test_vehicle_provider_bridge.c -o tests/test_vehicle_provider_bridge
	./tests/test_vehicle_provider_bridge

# End-to-end authoring-mode bridge used by projectile/world billboards.
test-projectile-sprite-modes89: $(IMGCC0_LIB)
	mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_projectile_sprite_modes89.c src/blank3d_projectile_sprite89.c \
	 src/blank3d_image_assets.c $(ASSETROUTE89_ROOT)/src/assetroute89.c \
	 $(STATICSPRITE89_ROOT)/src/staticsprite89.c \
	 $(IMAGESEQUENCER89_ROOT)/src/imagesequencer89.c \
	 $(RENLIST89_ROOT)/src/renlist89.c \
	 $(RENLIST89_ROOT)/adapters/imagesequencer89/renlist89_imagesequencer89.c \
	 $(TILECELL89_ROOT)/src/tilecell89.c \
	 $(GMSPRITESTRIP89_ROOT)/src/gmspritestrip89.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89.c $(IMGCC0_LIB) \
	 -o build/tests/test_projectile_sprite_modes89
	./build/tests/test_projectile_sprite_modes89


.PHONY: test-projectilevisual2d89-vendor test-projectilevisual2d89-bridge

.PHONY: test-gskybox89-vendor test-gskybox89-imgcc0 test-skyboxrecipe89-vendor test-skybox-recipes89

test-skyboxrecipe89-vendor:
	$(MAKE) -C $(SKYBOXRECIPE89_ROOT) clean check CC=$(CC) CFLAGS='-std=c89 -pedantic-errors -Wall -Wextra -Werror -O2'


test-skybox-recipes89: $(IMGCC0_LIB)
	@mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror $(LIMITS) \
	 -Isrc -I$(GSKYBOX89_ROOT)/include -I$(SKYBOXRECIPE89_ROOT)/include \
 -I$(RT_TIME89_ROOT) -I$(TIMECLOCKER89_ROOT)/include \
 -I$(TICKOCLOCK89_ROOT)/include -I$(TIMEVERBS89_ROOT)/include \
	 -I$(IMGCC0_ROOT)/include -I$(ASSETROUTE89_ROOT)/include \
	 -I$(ASSETROUTE89_ROOT)/providers/posix -Ivendor/gamlib3d \
	 tests/test_skybox_recipes89.c src/blank3d_skybox_recipe89.c \
	 src/blank3d_skybox89.c src/blank3d_image_assets.c \
	 $(SKYBOXRECIPE89_ROOT)/src/skyboxrecipe89.c \
	 $(GSKYBOX89_ROOT)/src/gskybox89.c $(GSKYBOX89_ROOT)/src/gskybox89_assets.c \
	 $(ASSETROUTE89_ROOT)/src/assetroute89.c \
	 $(ASSETROUTE89_ROOT)/providers/posix/assetroute89_posix.c \
	 $(IMGCC0_LIB) -o build/tests/test_skybox_recipes89
	./build/tests/test_skybox_recipes89


test-gskybox89-vendor:
	$(MAKE) -C $(GSKYBOX89_ROOT) clean check CC=$(CC) CFLAGS='-std=c89 -pedantic-errors -Wall -Wextra -Werror -O2'


test-gskybox89-imgcc0: $(IMGCC0_LIB)
	@mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror $(LIMITS) \
	 -Isrc -I$(GSKYBOX89_ROOT)/include -I$(IMGCC0_ROOT)/include \
	 -I$(ASSETROUTE89_ROOT)/include -I$(ASSETROUTE89_ROOT)/providers/posix \
	 -Ivendor/gamlib3d \
	 tests/test_gskybox89_imgcc0.c src/blank3d_skybox89.c \
	 src/blank3d_image_assets.c $(GSKYBOX89_ROOT)/src/gskybox89.c \
	 $(GSKYBOX89_ROOT)/src/gskybox89_assets.c \
	 $(ASSETROUTE89_ROOT)/src/assetroute89.c \
	 $(ASSETROUTE89_ROOT)/providers/posix/assetroute89_posix.c \
	 $(IMGCC0_LIB) -o build/tests/test_gskybox89_imgcc0
	./build/tests/test_gskybox89_imgcc0


test-projectilevisual2d89-vendor:
	$(MAKE) -C $(PROJECTILEVISUAL2D89_ROOT) clean test

test-projectilevisual2d89-bridge: $(IMGCC0_LIB)
	@mkdir -p build/tests
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_projectilevisual2d89_bridge.c \
	 src/blank3d_projectilevisual2d89_bridge.c src/blank3d_projectile_sprite89.c \
	 $(PROJECTILEVISUAL2D89_ROOT)/src/projectilevisual2d89.c \
	 $(STATICSPRITE89_ROOT)/src/staticsprite89.c \
	 $(IMAGESEQUENCER89_ROOT)/src/imagesequencer89.c \
	 $(RENLIST89_ROOT)/src/renlist89.c \
	 $(RENLIST89_ROOT)/adapters/imagesequencer89/renlist89_imagesequencer89.c \
	 $(TILECELL89_ROOT)/src/tilecell89.c \
	 $(GMSPRITESTRIP89_ROOT)/src/gmspritestrip89.c \
	 $(SPRITEASSET89_ROOT)/src/spriteasset89.c \
	 src/blank3d_image_assets.c \
	 $(ASSETROUTE89_ROOT)/src/assetroute89.c $(IMGCC0_LIB) \
	 -o build/tests/test_projectilevisual2d89_bridge
	./build/tests/test_projectilevisual2d89_bridge

# Blank3D full-stack integrated runner - MSYS2 MinGW32
# C89 + fixed point + caller-owned/static storage.

# Command-line assignments still override these (`make CC=clang`), while the
# explicit defaults avoid GNU make's built-in `CC=cc` on MSYS2.
CC = gcc
AR = ar
TARGET = blank3d.exe
DEBUG_TARGET = blank3d_debug.exe
VPHYSICS_DEMO_TARGET = blank3d_vphysics_demo.exe

WEAPON_MANAGER = vendor/weapon_system/gweapon89_manager_v2_super_agnostic_provider_bus/gweapon89_manager
BOLT_ROOT = vendor/weapon_system/bolt3d_ballistics_c89/bolt3d_ballistics_c89
AOI_TRAIL_ROOT = vendor/weapon_system/aoi_trail3d89/aoi_trail3d89
ZOOM_ROOT = vendor/weapon_system/gtelescopiczoom89_provider/gtelescopiczoom89
SWAY_ROOT = vendor/weapon_system/gsway89/gsway89
SNIPER_HUD_ROOT = vendor/weapon_system/gsniperhud89/gsniperhud89
AIM_ROOT = vendor/weapon_system/gaimquery89/gaimquery89/gaimquery89
SCOPE_PAINT_ROOT = vendor/weapon_system/gscopepaint89
SCOPE_VECTOR_ROOT = vendor/weapon_system/gscopevector89
SCOPE_PRESETS_ROOT = vendor/weapon_system/gscopepresets89
CAMERANAKU_ROOT = vendor/cameranaku89
SYNTH_ROOT = vendor/weapon_synth_sound_engine89
DDSL2_ROOT = vendor/ddsl2
FPIL_ROOT = vendor/fpil
RPYL_ROOT = vendor/rpyl
GFO_ROOT = vendor/gfo
VERTICAL_COMMON_ROOT = vendor/vertical_motion89_common
FLY89_ROOT = vendor/fly89
JUMP89_ROOT = vendor/jump89
AIRDIVER89_ROOT = vendor/airdiver89
INPUT_HOOK_ROOT = vendor/input_hook89
INPUT_SCANNER_ROOT = vendor/input_scanner89
POLLS89_ROOT = vendor/polls89
SCANEMU89_ROOT = vendor/scanemu89
CYCLER89_ROOT = vendor/cycler89
GAUTOMOTION_ROOT = vendor/gautomotion89
GAIRLUNGE89_ROOT = vendor/gairlunge89
GGROUNDLANCE89_ROOT = vendor/ggroundlance89
ECG_ROOT = vendor/ecg_embedded_c89
BIGHUD_ROOT = vendor/bigvaderhudder
GEDER_ROOT = vendor/geder_truth_gate_pipeline
NPC_EYES_ROOT = vendor/3d_npc_eyes
ENLIGHTENER_ROOT = vendor/enlightenerai
NATIONALMECANICANIMAL89_ROOT = vendor/nationalmecanicanimal89
GATTACH89_ROOT = vendor/gattach89
VPHYSICS_ROOT = vendor/vphysics_total_solver_c89
GFACTION89_ROOT = vendor/gfaction89
SYNTH_LIB = $(SYNTH_ROOT)/build/libweapon_synth_sound_engine89.a
SYNTH_STAMP = $(SYNTH_ROOT)/build/.blank3d_toolchain
# A packaged static library may have been built by another compiler/CPU.
# MinGW cannot link Linux ELF objects even though GNU ar can list them.
SYNTH_TOOLCHAIN_ID := $(strip $(shell $(CC) -dumpmachine 2>/dev/null))-$(strip $(shell $(CC) -dumpfullversion -dumpversion 2>/dev/null))

CORE_INCLUDES = -Isrc \
 -Ivendor/gamlib3d -Ivendor/gamlib3d/math_helpers -Ivendor/g3dweaponzeroing89/include \
 -Ivendor/soquete3d -Ivendor/giffany_shapes3d \
 -Ivendor/numsys/include -Ivendor/flags -Ivendor/gkinventory/include \
 -Ivendor/gbar89/include -Ivendor/conf_total -I$(WEAPON_MANAGER)/src \
 -Ivendor/weapon_system/gbulletmesh89/gbulletmesh89/src \
 -Ivendor/weapon_system/rocketmeshes/rocketmeshes/include \
 -I$(BOLT_ROOT)/include -I$(AOI_TRAIL_ROOT)/include \
 -I$(ZOOM_ROOT)/include -I$(SWAY_ROOT)/include \
 -I$(SNIPER_HUD_ROOT)/include -I$(AIM_ROOT)/include \
 -I$(SCOPE_PAINT_ROOT)/include -I$(SCOPE_VECTOR_ROOT)/include \
 -I$(SCOPE_PRESETS_ROOT)/include \
 -I$(CAMERANAKU_ROOT)/include -I$(GFO_ROOT)/include \
 -I$(VERTICAL_COMMON_ROOT)/include -I$(FLY89_ROOT)/include \
 -I$(JUMP89_ROOT)/include -I$(AIRDIVER89_ROOT)/include \
 -I$(INPUT_HOOK_ROOT) -I$(INPUT_SCANNER_ROOT) -I$(POLLS89_ROOT) \
 -I$(SCANEMU89_ROOT)/include -I$(CYCLER89_ROOT)/include \
 -I$(GAUTOMOTION_ROOT)/include \
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
 -I$(VPHYSICS_ROOT)/include -I$(GFACTION89_ROOT)/include

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

INCLUDES = $(CORE_INCLUDES) $(SYNTH_INCLUDES)

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

COLLISION_SOURCES =  vendor/collision/ccs/ccs_fixed.c  vendor/collision/ccs/ccs_math.c  vendor/collision/ccs/ccs_geom.c  vendor/collision/ccs/ccs_collision.c  vendor/collision/ccs/ccs_shapes.c  vendor/collision/ccs/ccs_narrow.c  vendor/collision/ccs/ccs_sat.c  vendor/collision/ccs/ccs_contact.c  vendor/collision/ccs/ccs_manifold.c  vendor/collision/ccs/ccs_raycast.c  vendor/collision/ccs/ccs_dispatch.c  vendor/collision/ccs/ccs_resolve.c  vendor/collision/ccs/ccs_broad_sweep.c  vendor/collision/ccs/ccs_broad_grid3d.c  vendor/collision/ccs/ccs_world.c  vendor/collision/ccs/ccs_shapecast.c  vendor/collision/ccs/ccs_plane.c  vendor/collision/ccs/ccs_triangle.c  vendor/collision/ccs/ccs_convex.c  vendor/collision/ccs/ccs_compound.c  vendor/collision/ccs/ccs_grid.c  vendor/collision/ccs/ccs_heightfield.c  vendor/collision/ccs/ccs_trimesh.c  vendor/collision/ccs/ccs_aabbtree.c  vendor/collision/ccs/ccs_trace_box.c  vendor/collision/sicol/sicol_math_fx.c  vendor/collision/sicol/sicol_shape.c  vendor/collision/sicol/sicol_mesh.c  vendor/collision/sicol/sicol_convex.c  vendor/collision/sicol/sicol_gjk.c  vendor/collision/sicol/sicol_cast.c  vendor/collision/sicol/sicol_broadphase.c  vendor/collision/sicol/sicol_narrowphase.c  vendor/collision/sicol/sicol_raycast.c  vendor/collision/sicol/sicol_world.c  vendor/collision/sicol/sicol_solver.c

SOURCES = src/monika_blank3d.c \
          src/blank3d_faction.c \
          $(GFACTION89_SOURCES) \
          src/blank3d_input.c \
          src/blank3d_ddsl_input.c \
          src/blank3d_list_cycle.c \
          $(CYCLER89_ROOT)/src/cycler89.c \
          src/blank3d_automotion.c \
          src/blank3d_motion_attack.c \
          src/blank3d_truth_gate.c \
          src/blank3d_perception.c \
          src/blank3d_perception_ini.c \
          src/blank3d_attachment.c \
          src/blank3d_weapon_presentation.c \
          src/blank3d_mechanical_weapon.c \
          src/blank3d_actor_equipment.c \
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
          $(INPUT_HOOK_ROOT)/input_hook_backend_win32_async.c \
          $(INPUT_SCANNER_ROOT)/input_scanner.c \
          $(POLLS89_ROOT)/key_pc.c \
          $(SCANEMU89_ROOT)/src/scanemu89.c \
          src/blank3d_vertical_axis.c \
          $(FLY89_ROOT)/src/fly89.c \
          $(JUMP89_ROOT)/src/jump89.c \
          $(AIRDIVER89_ROOT)/src/airdiver89.c \
          src/engine_bridge.c \
          src/blank3d_systems.c \
          src/blank3d_weapon_ini.c \
          src/blank3d_weapon_loadout.c \
          src/blank3d_npc_inventory.c \
          src/blank3d_languages.c \
          src/blank3d_objects.c \
          src/blank3d_audio.c \
          src/blank3d_config.c \
          src/blank3d_hud.c \
          src/blank3d_numbar.c \
          src/blank3d_bighud.c \
          src/blank3d_ecg_vitals.c \
          src/blank3d_collision.c \
          src/blank3d_projectile_mesh.c \
          src/blank3d_weapon_modules.c \
          src/blank3d_ballistics.c vendor/g3dweaponzeroing89/src/g3dweaponzeroing89.c \
          src/blank3d_universal_aim.c \
          src/blank3d_bolt.c \
          src/blank3d_trails.c \
          src/blank3d_sniper.c \
          src/blank3d_cameranaku.c \
          src/blank3d_fire_frame_sync.c \
          src/blank3d_shotgun.c \
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
          vendor/conf_total/conf_total.c \
          vendor/weapon_system/gbulletmesh89/gbulletmesh89/src/gbulletmesh89.c \
          vendor/weapon_system/rocketmeshes/rocketmeshes/src/rocketmeshes.c \
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
          $(ZOOM_ROOT)/src/gtelescopiczoom89.c \
          $(SWAY_ROOT)/src/gsway89.c \
          $(SNIPER_HUD_ROOT)/src/gsniperhud89.c \
          $(AIM_ROOT)/src/gaimquery89.c \
          $(SCOPE_PAINT_ROOT)/src/gscopepaint89.c \
          $(SCOPE_VECTOR_ROOT)/src/gscopevector89.c \
          $(SCOPE_PRESETS_ROOT)/src/gscopepresets89.c \
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
 -DNS_MAX_TYPES=16 -DNS_MAX_VALUES=32 -DNS_MAX_THRESHOLDS=32 \
 -DNS_MAX_EVENTS=64 -DNS_MAX_TEMPLATES=8 -DNS_MAX_TEMPLATE_ITEMS=32 \
 -DNS_MAX_MODIFIERS=64 -DNS_MAX_DERIVED=16 -DNS_MAX_BINDINGS=16 \
 -DGWP89_MAX_WEAPONS=16 -DGWP89_MAX_USERS=32 -DGWP89_MAX_EVENTS=128 \
 -DGWP89_MAX_AMMO_TYPES=16 -DGWP89_MAX_PROVIDERS=24

CFLAGS_COMMON = -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(INCLUDES)
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g
SYNTH_CFLAGS = -std=c89 -pedantic -Wall -Wextra -O2
LDLIBS = $(SYNTH_LIB) -lopengl32 -lwinmm -lgdi32 -luser32 -lkernel32

.PHONY: all release debug run run-debug clean rebuild audit test test-core \
        test-config test-audio-host test-collision test-projectile-meshes \
        test-weapon-modules test-bolt-slingshot test-sniper-stack test-aoi-trail \
        test-cameranaku-provider test-automatic-fire-frame-lock test-aim-convergence test-shotgun-runtime \
        test-weapon-ini test-languages test-actor-weapon-isolation \
        test-npc-weapon-inventory test-vertical-motion test-vertical-vendors \
        test-input-stack test-perception-ini test-cycler-stack test-cycler-vendor test-automotion test-gautomotion-vendor test-motion-attacks test-motion-q16-win32 test-motion-attack-vendors test-ecg-vitals test-bighud test-numbar test-gbar-v04-bighud test-truth-gate test-geder-vendor test-perception-stack test-npc-eyes-vendor test-enlightener-vendor test-nationalmecanicanimal-vendor test-gattach-vendor test-attachment-stack test-mechanical-weapon test-actor-equipment test-vphysics-vendor test-vphysics-provider test-casing-physics test-gfaction-vendor test-faction-bridge syntax-check help FORCE

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

$(TARGET): $(SOURCES) $(SYNTH_LIB) Makefile
	$(CC) $(CFLAGS_RELEASE) -mwindows $(SOURCES) $(LDLIBS) -o $@

$(DEBUG_TARGET): $(SOURCES) $(SYNTH_LIB) Makefile
	$(CC) $(CFLAGS_DEBUG) $(SOURCES) $(LDLIBS) -o $@

$(VPHYSICS_DEMO_TARGET): $(SOURCES) $(SYNTH_LIB) Makefile
	$(CC) $(CFLAGS_DEBUG) -DB3D_VPHYSICS_DEMO=1 \
		-DB3D_VPHYSICS_DEBUG_DRAW=1 $(SOURCES) $(LDLIBS) -o $@

run: release
	./$(TARGET)

run-debug: debug
	./$(DEBUG_TARGET)

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
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror \
	 -Isrc -I$(GATTACH89_ROOT)/include -I$(NATIONALMECANICANIMAL89_ROOT)/include \
	 -I$(WEAPON_MANAGER)/src \
	 tests/test_mechanical_weapon.c src/blank3d_mechanical_weapon.c \
	 src/blank3d_weapon_presentation.c \
	 $(NATIONALMECANICANIMAL89_SOURCES) $(GATTACH89_ROOT)/src/gattach89.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_mechanical_weapon
	./tests/test_mechanical_weapon

# GWeapon -> presentation -> GAttach -> per-actor NationalMecanicanimal89.
test-actor-equipment:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror \
	 -Isrc -Ivendor/soquete3d -I$(GATTACH89_ROOT)/include \
	 -I$(NATIONALMECANICANIMAL89_ROOT)/include -I$(WEAPON_MANAGER)/src \
	 tests/test_actor_equipment.c src/blank3d_actor_equipment.c \
	 src/blank3d_attachment.c src/blank3d_weapon_presentation.c \
	 src/blank3d_mechanical_weapon.c \
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
	 tests/test_systems.c src/blank3d_systems.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c src/blank3d_weapon_ini.c src/blank3d_weapon_loadout.c \
     src/blank3d_weapon_modules.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_systems
	./tests/test_systems

# Reproduces the exact player/NPC coupling around an empty NPC magazine.
test-actor-weapon-isolation:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_actor_weapon_isolation.c src/blank3d_systems.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c \
	 src/blank3d_weapon_ini.c src/blank3d_weapon_loadout.c \
	 src/blank3d_weapon_modules.c \
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
	 src/blank3d_weapon_ini.c src/blank3d_weapon_modules.c \
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
	 -o tests/test_projectile_meshes
	./tests/test_projectile_meshes

test-weapon-modules:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_weapon_modules.c src/blank3d_weapon_modules.c \
	 -o tests/test_weapon_modules
	./tests/test_weapon_modules

test-bolt-slingshot:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_bolt_slingshot.c src/blank3d_bolt.c \
	 src/blank3d_weapon_modules.c src/blank3d_collision.c \
	 $(BOLT_ROOT)/src/b3d_fixed.c $(BOLT_ROOT)/src/b3d_vec3.c \
	 $(BOLT_ROOT)/src/b3d_transform.c $(BOLT_ROOT)/src/b3d_collision.c \
	 $(BOLT_ROOT)/src/b3d_events.c $(BOLT_ROOT)/src/b3d_projectile.c \
	 $(BOLT_ROOT)/src/b3d_solver.c $(BOLT_ROOT)/src/b3d_world.c \
	 $(COLLISION_SOURCES) $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_bolt_slingshot
	./tests/test_bolt_slingshot


test-sniper-stack:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_sniper_stack.c src/blank3d_sniper.c \
	 $(ZOOM_ROOT)/src/gtelescopiczoom89.c \
	 $(SWAY_ROOT)/src/gsway89.c \
	 $(SNIPER_HUD_ROOT)/src/gsniperhud89.c \
	 $(AIM_ROOT)/src/gaimquery89.c \
	 $(SCOPE_PAINT_ROOT)/src/gscopepaint89.c \
	 $(SCOPE_VECTOR_ROOT)/src/gscopevector89.c \
	 $(SCOPE_PRESETS_ROOT)/src/gscopepresets89.c \
	 -o tests/test_sniper_stack
	./tests/test_sniper_stack


test-aoi-trail:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(CORE_INCLUDES) \
	 tests/test_aoi_trail.c \
	 $(AOI_TRAIL_ROOT)/src/trail3d89.c \
	 $(AOI_TRAIL_ROOT)/src/trail3d89_profiles.c \
	 -o tests/test_aoi_trail
	./tests/test_aoi_trail


test-cameranaku-provider:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_cameranaku_provider.c src/blank3d_cameranaku.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89_profiles.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_cameranaku_provider
	./tests/test_cameranaku_provider


test-automatic-fire-frame-lock:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_automatic_fire_frame_lock.c \
	 src/blank3d_fire_frame_sync.c src/blank3d_cameranaku.c \
	 vendor/gamlib3d/gamlib3d_transform.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89.c \
	 $(CAMERANAKU_ROOT)/src/cameranaku89_profiles.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_automatic_fire_frame_lock
	./tests/test_automatic_fire_frame_lock


test-aim-convergence:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_aim_convergence.c src/blank3d_ballistics.c vendor/g3dweaponzeroing89/src/g3dweaponzeroing89.c \
	 src/blank3d_systems.c src/blank3d_list_cycle.c $(CYCLER89_ROOT)/src/cycler89.c src/blank3d_weapon_modules.c \
	 src/blank3d_weapon_ini.c src/blank3d_weapon_loadout.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/numsys/src/numsys.c vendor/flags/flags_fx.c \
	 vendor/flags/flags_pool.c vendor/flags/flags_util.c \
	 vendor/flags/flags_value.c vendor/flags/flagstore.c \
	 vendor/gkinventory/src/gkinv.c vendor/gkinventory/src/gkinv_fixed.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_aim_convergence
	./tests/test_aim_convergence

test-universal-weapon-aim:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Werror $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_universal_weapon_aim.c src/blank3d_universal_aim.c \
	 src/blank3d_ballistics.c src/blank3d_weapon_modules.c \
	 vendor/g3dweaponzeroing89/src/g3dweaponzeroing89.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_matrix.c \
	 vendor/soquete3d/soquete3d.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c \
	 -o tests/test_universal_weapon_aim
	./tests/test_universal_weapon_aim


test-shotgun-runtime:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_shotgun_runtime.c src/blank3d_shotgun.c \
	 vendor/gamlib3d/math_helpers/gamlib3d_math.c \
	 $(WEAPON_MANAGER)/src/gweapon89.c -o tests/test_shotgun_runtime
	./tests/test_shotgun_runtime


test-weapon-ini:
	$(CC) -std=c89 -Wall -Wextra -pedantic $(LIMITS) $(CORE_INCLUDES) \
	 tests/test_weapon_ini.c src/blank3d_weapon_ini.c \
	 src/blank3d_weapon_modules.c $(WEAPON_MANAGER)/src/gweapon89.c \
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
	 $(POLLS89_ROOT)/key_pc.c $(SCANEMU89_ROOT)/src/scanemu89.c \
	 -o tests/test_input_stack
	./tests/test_input_stack


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
	 tests/test_languages.c src/blank3d_languages.c src/blank3d_ddsl_input.c $(LANGUAGE_SOURCES) $(GFO_SOURCES) \
	 -o tests/test_languages
	./tests/test_languages

# Exercises the complete synth host without requiring a real Windows audio
# device. The WinMM shim is test-only; the release build uses real winmm.dll.
test-audio-host: $(SYNTH_LIB)
	$(CC) -Itests/platform_stubs $(CFLAGS_COMMON) \
	 src/blank3d_audio.c src/blank3d_weapon_modules.c \
	 src/blank3d_weapon_ini.c src/blank3d_weapon_loadout.c tests/platform_stubs/winmm_stub.c \
	 tests/test_audio_host.c $(WEAPON_MANAGER)/src/gweapon89.c \
	 $(SYNTH_LIB) -o tests/test_audio_host
	./tests/test_audio_host

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

# Parses every active source against small Win32/OpenGL declarations. This is
# useful on non-Windows hosts; it does not replace a final MinGW link.
syntax-check:
	$(CC) -Itests/platform_stubs $(CFLAGS_COMMON) -fsyntax-only $(SOURCES)

test: test-gfaction-vendor test-faction-bridge test-vphysics-vendor test-vphysics-provider test-casing-physics test-gattach-vendor test-attachment-stack test-nationalmecanicanimal-vendor test-mechanical-weapon test-actor-equipment test-npc-eyes-vendor test-enlightener-vendor test-perception-stack test-perception-ini test-geder-vendor test-truth-gate test-bighud test-numbar test-gfo-entities test-gbar-v04-bighud test-ecg-vitals test-motion-attack-vendors test-motion-q16-win32 test-motion-attacks test-gautomotion-vendor test-automotion test-cycler-stack test-cycler-vendor test-input-stack test-core test-actor-weapon-isolation test-npc-weapon-inventory test-vertical-motion test-vertical-vendors test-config test-projectile-meshes test-weapon-modules test-weapon-ini test-languages test-bolt-slingshot test-sniper-stack test-aoi-trail test-cameranaku-provider test-automatic-fire-frame-lock test-aim-convergence test-universal-weapon-aim test-shotgun-runtime test-audio-host test-collision syntax-check audit

clean:
	rm -f $(TARGET) $(DEBUG_TARGET) $(VPHYSICS_DEMO_TARGET) tests/test_systems tests/test_config \
	 tests/test_audio_host tests/test_collision tests/test_projectile_meshes \
	 tests/test_gatling_projectiles tests/test_weapon_modules \
	 tests/test_bolt_slingshot tests/test_sniper_stack tests/test_aoi_trail \
	 tests/test_cameranaku_provider tests/test_automatic_fire_frame_lock tests/test_aim_convergence \
	 tests/test_universal_weapon_aim tests/test_shotgun_runtime \
	 tests/test_weapon_ini tests/test_languages \
	 tests/test_actor_weapon_isolation tests/test_npc_weapon_inventory \
	 tests/test_vertical_motion tests/test_input_stack tests/test_cycler89 \
	 tests/test_automotion tests/test_motion_attacks \
	 tests/test_motion_q16_win32 tests/test_ecg_vitals tests/test_bighud \
	 tests/test_numbar tests/test_gbar_v04_bighud \
	 tests/test_truth_gate tests/test_perception_stack tests/test_perception_ini \
	 tests/test_mechanical_weapon tests/test_attachment_stack \
	 tests/test_actor_equipment tests/test_vphysics_provider_stack tests/test_casing_vphysics \
	 tests/test_faction_bridge tests/gfaction_demo_rampage tests/gfaction_demo_war test_gfo_entities
	$(MAKE) -C $(SYNTH_ROOT) clean
	@if [ -f $(RPYL_ROOT)/Makefile ]; then $(MAKE) -C $(RPYL_ROOT) clean; fi
	$(MAKE) -C $(FLY89_ROOT) clean
	$(MAKE) -C $(JUMP89_ROOT) clean
	$(MAKE) -C $(AIRDIVER89_ROOT) clean
	$(MAKE) -C $(CYCLER89_ROOT) clean
	$(MAKE) -C $(GAUTOMOTION_ROOT) clean
	$(MAKE) -C $(GAIRLUNGE89_ROOT) clean
	$(MAKE) -C $(GGROUNDLANCE89_ROOT) clean
	$(MAKE) -C $(GEDER_ROOT) clean
	$(MAKE) -C $(NPC_EYES_ROOT) clean
	$(MAKE) -C $(ENLIGHTENER_ROOT) clean
	$(MAKE) -C $(NATIONALMECANICANIMAL89_ROOT) clean
	$(MAKE) -C $(GATTACH89_ROOT) clean
	$(MAKE) -C $(VPHYSICS_ROOT) clean
	$(MAKE) -C $(GFACTION89_ROOT) clean
	$(MAKE) -C vendor/gbar89 clean

rebuild: clean all

audit:
	@echo "Active integration allocation audit:"
	@! grep -n -E '(^|[^[:alnum:]_])(malloc|realloc|free)[[:space:]]*\(' \
	 src/blank3d_*.c src/monika_blank3d.c vendor/numsys/src/numsys.c \
	 $(ECG_SOURCES) $(BIGHUD_SOURCES) $(GEDER_SOURCES) \
	 $(NPC_EYES_SOURCES) $(ENLIGHTENER_SOURCES) \
	 $(NATIONALMECANICANIMAL89_SOURCES) $(GATTACH89_SOURCES) $(VPHYSICS_SOURCES) $(GFACTION89_SOURCES) \
	 src/blank3d_attachment.c src/blank3d_mechanical_weapon.c \
	 src/blank3d_truth_gate.c src/blank3d_perception.c \
	 vendor/gbar89/src/gbar89_bighud.c \
	 vendor/fly89/src/*.c vendor/jump89/src/*.c vendor/airdiver89/src/*.c \
 src/blank3d_input.c src/blank3d_ddsl_input.c \
 vendor/input_hook89/input_hook.c vendor/input_scanner89/input_scanner.c \
 vendor/polls89/key_pc.c vendor/scanemu89/src/scanemu89.c \
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
	@echo "make           build blank3d.exe with complete weapon synth library"
	@echo "make debug     build console/debug executable"
	@echo "make test-core run portable NUMSYS/flags/inventory/gweapon89 test"
	@echo "make test-vphysics-provider verify transform/collision provider bridge"
	@echo "make test-casing-physics verify shell bounce, damping and sleep"
	@echo "make test-vphysics-vendor run strict Total Solver upstream tests"
	@echo "make test-ecg-vitals verify HP states, threat pulse and damage flash"
	@echo "make test-bighud verify base ECG plus reusable .bhud presets"
	@echo "make test-numbar verify GFO/INI/preset NumBar orchestration"
	@echo "make test-gbar-v04-bighud verify the complete GBar89 v0.4 BVH surface"
	@echo "make test-truth-gate verify Blank3D GEDER profiles and accumulation"
	@echo "make test-geder-vendor run the standalone GEDER upstream tests"
	@echo "make test-perception-stack verify Socketer/Eyes/Enlightener fallback"
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
	@echo "make test-sniper-stack verify zoom/sway/HUD/aim/preset integration"
	@echo "make test-aoi-trail verify native Aoi Trail mesh generation"
	@echo "make test-cameranaku-provider verify receive-provider TRS + weapon camera ABI"
	@echo "make test-automatic-fire-frame-lock verify moving + rotating automatic fire camera coherence"
	@echo "make test-aim-convergence verify HUD-ray convergence and gravity zeroing"
	@echo "make test-motion-attacks verify air lunge + ground lance bridge"
	@echo "make test-motion-q16-win32 verify MinGW32-safe Q16.16 attack vectors"
	@echo "make test-motion-attack-vendors build both combat movement vendors"
	@echo "make test-shotgun-runtime verify seven physical pellets and lifetime"
	@echo "make test-weapon-ini verify data-driven weapon profiles and audio recipes"
	@echo "make test-npc-weapon-inventory verify arbitrary INI weapons per NPC"
	@echo "make test-input-stack verify all-key names, edges, scanner, capture and DDSL syntax"
	@echo "make test-cycler-stack verify active-list next/previous and named list routing"
	@echo "make test-cycler-vendor build standalone libcycler89.a"
	@echo "make test-automotion verify engine bridge and pattern flavor"
	@echo "make test-gautomotion-vendor build and test standalone gautomotion89"
	@echo "make test-languages verify vendored DDSL2, FPIL and RPYL runtimes"
	@echo "make test-vertical-motion verify gravity, fly89, jump89, airdiver89 and providers"
	@echo "make test-vertical-vendors build all three standalone C89 static libraries"
	@echo "make syntax-check parse the complete active Win32/OpenGL source"
	@echo "make test       run every portable validation and audit"
	@echo "make audit     allocation and gameplay decimal-token scan"
	@echo "make clean     remove executables and synth build artifacts"

.PHONY: test-gfo-entities
test-gfo-entities:
	$(CC) -std=c89 -Wall -Wextra -pedantic -Isrc -I$(GFO_ROOT)/include \
	 src/blank3d_objects.c $(GFO_SOURCES) tests/test_gfo_entities.c \
	 -o test_gfo_entities
	./test_gfo_entities

CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -Iinclude -Ivendor/vehicleprovider89/include -Ivendor/vehiclephysics89/include -Ivendor/carmovement89/include -Ivendor/motorcyclemovement89/include -Ivendor/busmovement89/include -Ivendor/tankmovement89/include -Ivendor/watermovement89/include -Ivendor/airmovement89/include -Ivendor/rotorcraftmovement89/include -Ivendor/spacemovement89/include
AR ?= ar
ARFLAGS ?= rcs

SRC = src/gveh.c src/gveh_ai.c src/gveh_assist.c src/gveh_body.c src/gveh_camera.c \
      src/gveh_debug.c src/gveh_drivetrain.c src/gveh_fx.c src/gveh_masspoint.c \
      src/gveh_math.c src/gveh_surface.c src/gveh_susp.c src/gveh_tag.c \
      src/gveh_tire.c src/gveh_watercraft.c src/gveh_wheel.c src/gveh_world.c \
      src/gveh_nitro.c src/gveh_kudos.c src/gveh_skill.c src/gveh_tuning.c \
      src/gveh_driver_profile.c src/gveh_riskboost.c src/gveh_takedown.c \
      src/gveh_aftertouch.c src/gveh_crashbreaker.c src/gveh_airframe.c \
      src/gveh_rotorcraft.c src/gveh_airassist.c src/gveh_airgame.c \
      src/gveh_air_damage.c src/gveh_avionics.c src/gveh_wingman.c \
      src/gveh_air_mission.c src/gveh_spacecraft.c \
      src/gveh_collision_bridge.c src/gveh_entity_bridge.c \
      src/gveh_profile_bank.c src/gveh_engine_adapter.c \
      src/gveh_tank_tracks.c src/gveh_tank_turret.c src/gveh_tank_gun.c \
      src/gveh_tank_armor.c src/gveh_tank_modules.c src/gveh_tank_crew.c \
      src/gveh_tank_firecontrol.c src/gveh_tank_station.c src/gveh_tank_platoon.c \
      src/gveh_tank4.c \
      vendor/vehicleprovider89/src/vehicleprovider89.c \
      vendor/vehiclephysics89/src/vehiclephysics89.c \
      vendor/carmovement89/src/carmovement89.c \
      vendor/motorcyclemovement89/src/motorcyclemovement89.c \
      vendor/busmovement89/src/busmovement89.c \
      vendor/tankmovement89/src/tankmovement89.c \
      vendor/watermovement89/src/watermovement89.c \
      vendor/airmovement89/src/airmovement89.c \
      vendor/rotorcraftmovement89/src/rotorcraftmovement89.c \
      vendor/spacemovement89/src/spacemovement89.c
OBJ = $(SRC:.c=.o)

all: libgvehicle89.a demo_vehicle89 demo_multi_vehicle89

check-subvendors:
	$(MAKE) -C vendor/vehicleprovider89 test
	$(MAKE) -C vendor/vehiclephysics89 test
	$(MAKE) -C vendor/carmovement89 check
	$(MAKE) -C vendor/motorcyclemovement89 check
	$(MAKE) -C vendor/busmovement89 check
	$(MAKE) -C vendor/tankmovement89 check
	$(MAKE) -C vendor/watermovement89 check
	$(MAKE) -C vendor/airmovement89 check
	$(MAKE) -C vendor/rotorcraftmovement89 check
	$(MAKE) -C vendor/spacemovement89 check

check: all check-subvendors test-provider-dispatch
	./demo_vehicle89 >/dev/null
	./demo_multi_vehicle89 >/dev/null

.PHONY: test-provider-dispatch
test-provider-dispatch: tests/test_provider_dispatch
	./tests/test_provider_dispatch

tests/test_provider_dispatch: tests/test_provider_dispatch.c libgvehicle89.a
	$(CC) $(CFLAGS) -o $@ tests/test_provider_dispatch.c libgvehicle89.a

libgvehicle89.a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

demo_vehicle89: demo/demo_vehicle89.c libgvehicle89.a
	$(CC) $(CFLAGS) -o demo_vehicle89 demo/demo_vehicle89.c libgvehicle89.a

demo_multi_vehicle89: demo/demo_multi_vehicle89.c libgvehicle89.a
	$(CC) $(CFLAGS) -o demo_multi_vehicle89 demo/demo_multi_vehicle89.c libgvehicle89.a

clean:
	rm -f $(OBJ) libgvehicle89.a demo_vehicle89 demo_multi_vehicle89 tests/test_provider_dispatch demo_vehicle89.csv demo_vehicle89.ppm demo_multi_vehicle89.csv demo_multi_vehicle89.ppm

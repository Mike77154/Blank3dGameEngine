gvehicle89 v3 air modules
=========================

Goal
----
Add helicopter and airplane gameplay/simulation layers inspired by classic arcade and sim titles before 2008 while keeping the library C89, fixed point and static-storage friendly.

Research translation
--------------------
- Thunder Blade / Air Combat / Ace Combat: arcade readability, fast response, lock-on, cannon/missile loops, afterburner/nitro-like thrust.
- Desert Strike / Nuclear Strike / Army Men Air Attack: helicopters as mission craft, resources, fuel/ammo/armor, rescue/winch/cargo loop.
- Jane's Longbow / Enemy Engaged / Falcon 4.0: mission/campaign logic hooks, pilot profile hooks, sensor/lock-on states, AI-friendly state outputs.
- Microsoft Flight Simulator / X-Plane / JSBSim: forces/moments architecture, lift/drag/thrust, stall state, cockpit-style telemetry, per-aircraft profiles.
- IL-2 style lesson: damage/part failure should eventually be per subsystem, not one global HP bar. This v3 only exposes fuel/armor/ammo state and stall/rotor flags for future damage modules.

New modules
-----------
include/src:
- gveh_airframe.h/.c: fixed-wing lift, drag, thrust, afterburner, stall, pitch/roll/yaw controls.
- gveh_rotorcraft.h/.c: helicopter collective, rotor rpm, translational lift, cyclic force, yaw/tail torque, altitude-hold assist.
- gveh_airassist.h/.c: autolevel, stall guard, bank/rate clamp, beginner arcade smoothing.
- gveh_airgame.h/.c: fuel/ammo/armor, lock-on timer, cannon/missile cooldown, rescue winch/cargo state.

New profiles
------------
- thunder_chopper89: Thunder Blade / arcade helicopter.
- strike_chopper89: Desert/Nuclear Strike style mission helicopter.
- sim_heli89: slower sim-lite helicopter profile.
- ace_fighter89: Air Combat/Ace Combat arcade jet.
- fs_lightplane89: MSFS/X-Plane-inspired light aircraft, stall-aware.

New input fields
----------------
- collective: rotor lift command.
- air_pitch: elevator / helicopter cyclic pitch.
- air_roll: aileron / helicopter cyclic roll.
- air_yaw: rudder / tail rotor yaw.
- fire: cannon/weapon trigger.
- lock_on: lock sensor command.
- winch: rescue/cargo winch command.
- alt_hold: altitude hold assist weight.

Demo modes
----------
./demo_vehicle89 h  -> thunder_chopper89
./demo_vehicle89 s  -> strike_chopper89
./demo_vehicle89 m  -> sim_heli89
./demo_vehicle89 a  -> ace_fighter89
./demo_vehicle89 f  -> fs_lightplane89

CSV columns added
-----------------
airspeed, air_lift, rotor_rpm, rotor_lift, fuel, cannon, missiles, lock, cargo

Notes
-----
This is not a full professional FDM. It is a fixed-point sim-lite/arcade hybrid made to be robust in a small C89 engine. The architecture is intentionally split so a future v4 can add component damage, aircraft stores, terrain-following AI, sensor cones and formation/wingman behavior.

# gvehicle89 v4 air canon expansion

This pass thickens the air layer with lessons from classic helicopter, jet and spacecraft games.

## Canon extracted

- Desert Strike / Jungle Strike / Nuclear Strike: mission resources, fuel, ammo, armor, rescue/cargo, map objectives.
- Gunship 2000: semi-dynamic campaign flow, randomized primary/secondary objectives, wingmen and crew growth.
- LHX Attack Chopper: mixed aircraft roster including Apache, Blackhawk and Osprey-style VTOL logic.
- Comanche: Maximum Overkill: arcade-friendly combat envelope, terrain-skimming, durable helicopter feel.
- Jane's Longbow 2 / Enemy Engaged: simulated attack helicopter, sensors, dynamic campaign, wingman/task force flavor.
- Falcon 4.0 / Falcon BMS: dynamic war, packages, avionics, pilot-to-commander scale.
- IL-2 / MS Combat Flight Simulator: component damage and differentiated aircraft feel.
- Ace Combat / Air Combat: responsive arcade military flight, generous weapons, lock-on, afterburner and mission scoring.
- X-Plane / JSBSim: force/moment architecture and modular FDM style.
- X-Wing / TIE Fighter / Rogue Squadron / Battlefront II space battles: energy management, shields, lasers, engines, wingmen, capital-ship objective thinking.

## Added modules

```txt
gveh_air_damage.h/.c      component damage: engine, rotor, wings, tail, fuel leak, fire, autorotation-lite
gveh_avionics.h/.c        radar/TADS/RWR/space sensor, target id, lock timer, threat warning
gveh_wingman.h/.c         wingman slots, attack/cover/hold/rescue command model
gveh_air_mission.h/.c     destroy/rescue/recon/protect counters, medals, campaign push
gveh_spacecraft.h/.c      X-Wing/TIE style energy, shields, laser charge, heat, boost, inertia damp
```

## New profiles

```txt
comanche_voxel89         arcade combat-envelope helicopter
longbow_campaign89       sim-lite Apache/Longbow-style helicopter
gunship_flight89         campaign-wingman helicopter profile
rogue_xwing89            shields/lasers/engine energy spacecraft
tie_interceptor89        fast unshielded interceptor
battlefront_bomber89     heavier shielded bomber/capital-attack craft
```

## C89 constraints

- Fixed point only.
- No dynamic storage calls.
- No heap ownership.
- No real-number C types.
- Static arrays and profile-driven configuration.

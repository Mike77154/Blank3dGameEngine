# gvehicle89 → Vehicle System split, phase 1

## Goal

Turn `gvehicle89` into a stable vehicle-system facade while subvendorizing movement and common physics.
No public Blank3D API or mount/possession authority changes in this phase.

## Authority map

```text
Actor / input
    |
 gvehpos89               occupancy / driver / passenger rules
    |
 Mount89                 guest -> vehicle mount point relation
    |
 Vehicle Object
    |
 gvehicle89              stable facade / orchestration / gameplay modules
    |
 +-- carmovement89
 +-- tankmovement89
 +-- watermovement89
 +-- airmovement89
 +-- rotorcraftmovement89
 +-- spacemovement89
 |       |
 +-------+--> vehiclephysics89 --> gveh_body / world providers
```

`GAttach` is not part of this chain; it remains for objects attached/equipped to characters.

## Extracted from gveh.c

- common gravity -> `vehiclephysics89`
- mass-point collision response -> `vehiclephysics89`
- linear drag -> `vehiclephysics89`
- body integration -> `vehiclephysics89`
- wheel/suspension/tire/drivetrain force generation -> `carmovement89`
- tank4 movement call + tank ground clamp -> `tankmovement89`
- buoyancy/water drag/thrust/yaw -> `watermovement89`
- airframe force application / shared air assist -> `airmovement89`
- rotor force application -> `rotorcraftmovement89`
- spacecraft movement -> `spacemovement89`

## Specialization seams

`busmovement89` and `motorcyclemovement89` now exist as subvendors, but phase 1 intentionally does not invent new profile tags or fake new physics. They delegate to the deterministic wheeled solver and are not routed by the facade yet.

Future motorcycle work belongs there: lean, balance, counter-steer, wheelie/brake pitch, low-speed stabilization.
Future bus work belongs there: heavy/long-wheelbase policy, body roll, extra axles, and eventually articulation.

## Gameplay deliberately left outside movement vendors

Nitro, kudos, risk boost, takedown, aftertouch, crashbreaker, camera, avionics, wingman, missions, air damage, tank crew/gun/fire-control/platoon remain in the gvehicle package. This phase separates *movement physics* first.

## Determinism/parity QA

Using the immediate pre-split Blank3D baseline and the split build:

```text
demo_vehicle89.csv
SHA-256 49b063ffdb17d70af6345d8b9a93f9e48dfee3880261eaf73b8d7c963ae694fd
old == split byte-for-byte

demo_multi_vehicle89.csv
SHA-256 4bf3e70b482e001b3dec8b8fb8ba5922059fb8f149fd69f5fc700d6204fbb46e
old == split byte-for-byte
```

The split gvehicle tree also builds under strict C89 with `-Wall -Wextra -Werror -pedantic`.

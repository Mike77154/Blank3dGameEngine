# Blank3D v3.15.2 — gairlunge89 + ggroundlance89

Blank3D vendors the two C89 fixed-point controllers from Giffany Motion89 as
**offensive traversal capabilities**. They do not replace ordinary AI movement,
`gautomotion89`, `jump89`, `fly89` or `airdiver89`.

```text
FPIL decision
    |
    +-- ordinary travel ----------> gautomotion89
    +-- takeoff / gravity ---------> jump89 / vertical axis
    +-- perch dive and return -----> airdiver89
    +-- committed mid-air attack --> gairlunge89
    `-- committed ground charge ---> ggroundlance89
```

While either motion attack is active, it temporarily owns the actor transform.
The engine's vertical controller does not overwrite the trajectory. At finish or
cancel, ownership returns to normal AI and gravity.

## Vendored libraries

```text
vendor/gairlunge89/
|-- include/gairlunge89.h
|-- src/gairlunge89.c
|-- Makefile
`-- LICENSE-CC0-1.0.txt

vendor/ggroundlance89/
|-- include/ggroundlance89.h
|-- src/ggroundlance89.c
|-- Makefile
`-- LICENSE-CC0-1.0.txt
```

Both libraries remain engine-agnostic, Q16.16, caller-owned and heap-free. Their
public APIs include optional provider adapters for transform/velocity, target,
ground, contact and impact. Blank3D's bridge converts its Q20.12 values and
frame `dt` into the per-tick units expected by the vendors.

## MinGW32 Q16.16 safety fix (v3.15.1)

MinGW32 uses a 32-bit `signed long`. The previous Q16.16 division evaluated
`remainder * 65536` before dividing. Normalizing world-space differences such
as 14 or 34 units could therefore wrap to zero exactly, erasing X/Z while the
vertical jump pulse remained visible. The actor appeared to bounce in place and
FPIL never reached a useful contact.

Both vendors now build multiplication and division from bounded unsigned
magnitudes, 16-bit fractional products and bitwise fractional division. They
remain C89, fixed-point and heap-free; no `long long` is required. The target
`make test-motion-q16-win32` compiles both vendor scalar types as 32-bit
`signed int` so the regression runs even on hosts where `long` is 64-bit.

## Ground Lancer horizontal charge fix (v3.15.2)

`ground_lancer_enemy` is now a true floor-locked knight charge. Its archetype
default is `lance`, hop height is zero, and STOP contact resolves on the
configured contact-radius shell so the enemy finishes beside the player instead
of overlapping or stepping through them. `pegasus` and `skim_hop` remain
explicit opt-in styles.

The engine-specific bridge is:

```text
src/blank3d_motion_attack.h
src/blank3d_motion_attack.c
```

## Mid-air lunge

The actor must already be airborne. `jump89`, a flying actor, an external
physics system or any other mechanism may produce that state. Once triggered,
`gairlunge89` commits the actor toward the target.

Intents:

```text
push
ram
claw
pierce
```

Target policies:

```text
snapshot   ; attack the position captured at trigger time
track      ; correct toward the target each update
```

FPIL configuration:

```text
:state=0:airlungeintent=claw
:state=0:airlungetarget=track
:state=0:airlungesteering=0.65
:state=0:airlungevelocitykeep=0.15
:state=0:airlungeimpulse=8
:state=0:airlungepiercecontacts=2
:state=0:attackradius=1
```

Triggers:

```text
airlungeplayer=16
midairlungeplayer=16
airramplayer=16
clawairplayer=16
pierceairplayer=16
```

## Ground lance

The actor must be grounded. The controller moves along X/Z while using the
engine's floor value as the terrain surface. A future physics/terrain provider
can feed the vendor's `query_ground` callback without changing FPIL.

Styles:

```text
lance
pegasus
ram
skim_hop
```

Contact policies:

```text
stop
bounce
pierce
```

FPIL configuration:

```text
:state=0:groundlancestyle=pegasus
:state=0:groundlancecontact=stop
:state=0:groundlancetrack=1
:state=0:groundlanceimpulse=10
:state=0:lancehopheight=0.35
:state=0:groundlancepiercecontacts=2
:state=0:attackradius=1
```

Triggers:

```text
groundlanceplayer=12
groundchargeplayer=12
pegasusplayer=12
groundramplayer=12
skimhopplayer=12
```

## Shared state and control

Conditions:

```text
motionattackactive
airlungeactive
groundlanceactive
attackhit
attackdone
```

Long aliases remain available:

```text
motionattackimpact
attackmotionimpact
attackimpact
motionattackfinished
attackmotionfinished
attackfinished
```

Actions:

```text
attackcancel
clearimpact
attackreset
pierceextension=6
attackradius=1
```

`attackhit` is latched until `clearimpact`. `attackdone` remains visible until
`attackreset`, which lets FPIL react deterministically even when the physical
contact and script tick happen on adjacent frames.

Because FPIL is configured with `stop_on_first_match`, impact and finished rules
must appear before ordinary active-motion rules for the same state.

## Demonstration enemies

### Air lunger

```text
air_lunger_enemy 5 pos -14 0 34 hp 34 floor 0
```

Aliases:

```text
airlunge_enemy
midair_lunger_enemy
```

The archetype defaults to CLAW, tracking, 0.65 steering and 0.15 preservation
of previous velocity. Its script uses `jump89` to leave the floor and then
starts the committed lunge.

```text
scripts/air_lunger_enemy.fpi
config/entities/air_lunger_enemy.ini
objects/air_lunger_enemy.gfo
```

### Ground lancer

```text
ground_lancer_enemy 6 pos 14 0 34 hp 42 floor 0
```

Aliases:

```text
groundlance_enemy
pegasus_enemy
```

The archetype defaults to PEGASUS, STOP contact, target tracking and a 0.35
unit surface pulse.

```text
scripts/ground_lancer_enemy.fpi
config/entities/ground_lancer_enemy.ini
objects/ground_lancer_enemy.gfo
```

Archetype defaults are convenience presets only. Every setting remains
modifiable from FPIL before an attack, so any actor can use either capability.

## Time and fixed-point integration

The original controllers are expressed in units per logical tick. Blank3D
accepts speeds in units per second and calculates:

```text
step_distance = requested_speed * engine_dt
```

The bridge then converts Q20.12 to Q16.16. This keeps attack speed independent
of render FPS and preserves the vendors' standalone API.

## Build and tests

```text
make test-motion-attack-vendors
make test-motion-attacks
make test-languages
make syntax-check
make test
```

The vendor targets produce `libgairlunge89.a` and `libggroundlance89.a` when
built independently. Generated archives and objects are not included in the
release ZIP.

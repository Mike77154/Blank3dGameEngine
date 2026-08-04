# Magazine + Rocket Whistle integration v1.8.1

## Runtime graph

```text
gameplay reload event
    -> WSSE89_EVENT_MAGAZINE_ACTION
    -> wmagazine89 fixed voice pool
    -> mechanism-class acoustic path
    -> stereo mixer

rocket fired
    -> REPORT / rocket ignition
    -> WSSE89_EVENT_ROCKET_WHISTLE_START
    -> MOTION updates during flight
    -> RELEASE immediately before collision
    -> ROCKET_BLAST at impact
```

## Magazine provider

`wmagazine89` remains directly accessible and is also exposed through the event facade. Presets cover metal/polymer pistol magazines, SMG steel magazines, rifle magazines, sniper box magazines and heavy drums. Actions cover remove, insert, seat tap, tug check and rattle.

The v1.8.1 demos use:

- pistol: `WMAG89_PRESET_PISTOL_POLYMER`;
- sniper: `WMAG89_PRESET_SNIPER_BOX`;
- machine gun: unchanged `wsoundbeltfeed89` path because the showcase is belt-fed.

## Rocket whistle provider

`grocketwhistle89` supplies a sustained trajectory layer with six-band EQ, vibrato/drift, chorus, optional reverb and radial-velocity Doppler input. The whistle is a separate event with a stable `instance_key`, allowing the game engine to update pan, gain, distance and radial velocity while the projectile moves.

## Memory and ownership

The unified context embeds four magazine voices and four rocket-whistle voices. No allocation occurs at runtime; exhausted pools use deterministic voice stealing. Advanced hosts may call either provider directly when they need a larger external pool.

## Event sequence example

```c
wsse89_event event;

/* Reload */
event.type = WSSE89_EVENT_MAGAZINE_ACTION;
wsse89_magazine_defaults(&event.data.magazine);
event.data.magazine.preset = WMAG89_PRESET_PISTOL_POLYMER;
event.data.magazine.action = WMAG89_ACTION_INSERT;
(void)wsse89_dispatch(&audio, &event, 0);

/* Flight */
event.type = WSSE89_EVENT_ROCKET_WHISTLE_START;
wsse89_rocket_whistle_defaults(&event.data.rocket_whistle_start);
event.data.rocket_whistle_start.instance_key = 77154U;
(void)wsse89_dispatch(&audio, &event, 0);

/* Update from projectile physics */
event.type = WSSE89_EVENT_ROCKET_WHISTLE_MOTION;
event.data.rocket_whistle_motion.instance_key = 77154U;
event.data.rocket_whistle_motion.radial_velocity_mps = -80;
event.data.rocket_whistle_motion.distance_gain_q15 = 21000U;
event.data.rocket_whistle_motion.gain_q15 = 17500;
event.data.rocket_whistle_motion.pan_q15 = 9000;
(void)wsse89_dispatch(&audio, &event, 0);

/* Before impact */
event.type = WSSE89_EVENT_ROCKET_WHISTLE_RELEASE;
event.data.rocket_whistle_control.instance_key = 77154U;
(void)wsse89_dispatch(&audio, &event, 0);
```

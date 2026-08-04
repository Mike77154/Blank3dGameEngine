# Integration order

```text
weapon trigger
  -> wsounddna89          correlated variation
  -> dry weapon synth     gpaah/body/gas/thump or another provider
  -> wsoundreceiver89     barrel/receiver/stock identity
  -> wsoundprop89         arrival delay, distance, directivity, occlusion
  -> voice handler        one logical event per audible source

projectile trajectory
  -> wsoundprojectile89   N-wave, snap, whiz or pellet swarm
  -> voice handler        independent position and priority

weapon action
  -> wsoundaction89       semantic eject/feed/lock/carrier events
  -> chuecka/gklek/Foley  or wsoundimpact89 for a minimal metal click

collision
  -> wsoundimpact89
  -> optional wsoundricochet89

shared room send
  -> wsoundroom89

all buses
  -> wsoundcombatbus89
  -> engine output
```

Every module may be omitted. No module calls another module internally.

## Memory ownership

`wsoundprop89` and `wsoundroom89` take caller-owned signed-16-bit delay memory. This permits arenas, static arrays, per-level pools, or memory supplied by another engine. The other contexts contain all their state directly.

## Voice-handler policy

Treat these as separate voice classes:

- muzzle report: high priority;
- projectile near-miss: critical when close;
- action cycle: high only in first person or at short distance;
- impacts: spatial priority;
- ricochets: medium priority;
- room response: one shared physical voice per acoustic space.

The room module should normally be shared by many shots rather than instantiated per voice.

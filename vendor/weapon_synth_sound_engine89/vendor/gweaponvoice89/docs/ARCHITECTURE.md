# Architecture

```text
Game events
   |
   v
gweaponvoice89 policy
   |-- class limits
   |-- distance priority offset
   |-- distance/occlusion/focus audibility
   |-- class physical reserves
   |-- per-instance limits
   v
gvoice89 logical scheduler (up to 256 recommended)
   |-- logical stealing only when full
   |-- physical ranking at control intervals
   |-- virtual timing and timeout
   |-- promotion/demotion callback
   v
physical voice list (64 recommended)
   |-- gain/pan/bus
   |-- attack/release
   |-- stolen tails
   |-- integer limiter
   v
stereo PCM
```

Logical stealing and physical virtualization are deliberately separate. A quiet report can become virtual without losing its logical event. A casing is actually stolen only when a logical or class limit requires a slot.

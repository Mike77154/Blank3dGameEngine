# HUD bar binding fix — v3.6.2

The HUD now uses the two meter forms in their intended roles:

- **Player health:** continuous scalable vector/linear `GBar89` meter.
- **GunBar ammunition:** discrete segmented `GBar89` units.

The gameplay data sources were not swapped. `player_health` still feeds the
health meter and `clip / clip_capacity` still feed the ammunition meter. The
fix swaps the **visual semantics** that had been assigned backwards.

## Large magazines

A magazine up to 30 rounds gets one visible segment per round. Larger
magazines (for example the 300-round Gatling belt) use 30 proportional blocks,
so the HUD stays readable instead of creating hundreds of sub-pixel lines.
The numeric weapon/inventory count remains exact.

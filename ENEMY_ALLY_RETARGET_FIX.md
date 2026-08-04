# Blank3D v3.25.1 — Enemy/Ally Retarget Fix

## Symptom

Hostile actors legally recognized `allies` as attackable through GFaction89, but
usually kept the player as their target.

## Root causes

1. Blank3D supplied distance to GFaction in the wrong fixed-point direction.
   The engine uses Q20.12 and GFaction uses Q16.16. The old bridge divided by
   16; the correct conversion multiplies by 16. Distance therefore contributed
   almost nothing to target ranking.
2. The player was inserted first in the candidate list. Equal scores kept the
   first candidate.
3. The player has a higher threat value than the armed ally, so without a
   meaningful distance contribution the player won almost every comparison.
4. The generic `pingpong` movement still used `g.player.position` directly,
   even after GFaction had selected another target.

## Corrected policy

Every living actor remains known through Socketer. GFaction rejects illegal
candidates, then the Blank3D stable selector ranks legal attack targets by:

- GFaction relation, role, tags and threat;
- recent damage stimulus;
- actual Q16.16 distance;
- a host distance weight of one score point per world unit;
- a three-point bonus for retaining the current target;
- deterministic tie-breaking by current target, then nearest distance, then ID.

Recent damage retains GFaction's forty-point bonus, so an ally that shoots a
hostile can immediately break target retention.

## Target-first FPIL

`scripts/gunner_enemy.fpi` and `scripts/enemy.fpi` now use target conditions and
actions explicitly. Legacy `player` verbs remain compatible aliases, but the
runtime resolves movement, height comparisons, aiming and damage against the
current GFaction target.

## Expected scene behavior

With the stock scene, the central gunner and actors nearer the blue ally can
select it at startup. Actors for which the player remains the stronger/nearer
candidate may continue attacking the player. Any hostile damaged by the ally
receives an immediate retarget stimulus toward the ally.

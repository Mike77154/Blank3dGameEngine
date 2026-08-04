# Weapon cycle: named active list via cycler89

The weapon cycle is no longer a hardcoded array of weapon IDs.

```text
If key_press M then list_cyclenext=active_weapon
If key_press N then list_cycleprev=active_weapon
```

`active_weapon` is registered in `Blank3DListCycleRegistry`. Its provider reads
weapons in the order they were registered from `config/weapons/weapons.ini`, and
its active predicate checks the player's live GKInventory ownership.

- Weapons with inventory count `0` are skipped.
- One owned weapon remains selected.
- Pickups and removals affect the next cycle operation immediately.
- New INI-only weapons enter the cycle without editing a C array.
- IDs may be sparse or non-consecutive.
- If the current weapon disappeared, next selects the first active entry and
  previous selects the last active entry.
- M/N are owned by DDSL2 rather than direct Win32 key checks.
- The mouse wheel calls the same named-list route.

The same `cycler89` vendor is used by NPC private weapon inventories and can be
reused for menus, targets, skills, inventory items or any other provider-backed
list with active entries.

# Shoot DDSL2 authority fix

The player weapon trigger no longer reads `VK_SPACE` or the raw mouse button in
`update_weapon_system()`.

`shoot` is now a logical action asserted by DDSL2. The host keeps an independent
previous-frame state for that logical action and derives `down`, `pressed`, and
`released` from it.

This matters for spin-up weapons: while a Gatling is spinning up, the weapon
system intentionally reports its accepted trigger as down=0. Using that weapon
state to synthesize the next input edge caused a DDSL2-held `shoot` to look like
a fresh press every frame, resetting the spin timer forever.

Expected behavior with:

    If key_hold MouseLeft then shoot

- MouseLeft press: one logical `pressed` edge.
- MouseLeft hold: stable logical `down` across every spin-up frame.
- Gatling reaches spin-up threshold and begins firing.
- MouseLeft release: one logical `released` edge.
- Space has no firing authority unless the user binds Space to `shoot` in DDSL2.

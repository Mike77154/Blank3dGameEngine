# gweaponio89

Tiny adapter from Weapon-System modules to the existing GWeapon89 provider bus.
It never opens files itself. The host must register `GWP89_SERVICE_IO` and handle
`GWP89_OP_READ_TEXT_FILE`. This keeps profile/loadout/presentation parsing
portable while preserving the no-heap C89 design.

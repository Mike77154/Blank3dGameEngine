# gweaponprofileio89

C89 Weapon-System profile/manifest parser.

It owns weapon profile/module parsing, but **does not own filesystem IO**.
Text is requested through the existing GWeapon89 provider bus using
`GWP89_SERVICE_IO` / `GWP89_OP_READ_TEXT_FILE`, via `gweaponio89`.

Blank3D currently supplies that service from `src/blank3d_weapon_host_io.c`.
Another host may supply ROM assets, package files, virtual files, network data,
or any other text source without changing this library.

Properties:

- C89 / `-pedantic`
- fixed/caller-owned storage
- no malloc/calloc/realloc/free
- no Win32/Linux/macOS dependency
- no `FILE*`, `fopen`, `fgets`, or direct filesystem backend

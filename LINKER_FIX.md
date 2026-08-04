# Blank3D v3.1 — cross-toolchain linker fix

## Symptom

MinGW32 reports undefined references such as:

- `wsse89_dispatch`
- `wsse89_init`
- `wsse89_render_stereo`
- `wsse89_gatling_defaults`

## Cause

The earlier v3 ZIP accidentally contained a Linux x86-64 ELF static archive at:

`vendor/weapon_synth_sound_engine89/build/libweapon_synth_sound_engine89.a`

Because the file already existed, GNU Make considered it current and did not
recompile it with the user's `i686-w64-mingw32-gcc` toolchain.

## Immediate repair for an existing v3 folder

```sh
make clean
make
```

## Permanent repair in v3.1

The package contains no prebuilt synth objects or archives. The root Makefile
also records the compiler target and version in:

`vendor/weapon_synth_sound_engine89/build/.blank3d_toolchain`

When the toolchain changes, only the synth build directory is removed and
rebuilt. This allows the same source tree to move safely between Linux,
MinGW32, and MinGW64.

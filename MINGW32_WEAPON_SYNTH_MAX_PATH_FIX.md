# MinGW32 weapon synth MAX_PATH fix

## Symptom

`ar.exe` failed while creating `build/libweapon_synth_sound_engine89.a` with an error such as:

```
error reading build/libobjs/vendor/wsound_expansion_pack89_v1_2_sendfix/wsoundmuzzledevice89/src/wsoundmuzzledevice89.o: No such file or directory
```

## Root cause

The old Makefile mirrored the complete vendored source path below `build/libobjs/`.
With Blank3D extracted under a long Windows project directory, the full path of
`wsoundmuzzledevice89.o` exceeded the legacy Win32 `MAX_PATH` boundary (about 266
characters in the reported path).  The source itself exists; the failure is the
long object pathname passed to MinGW32 `ar.exe`.

## Fix

`vendor/weapon_synth_sound_engine89/Makefile` now flattens archive objects:

```
build/libobjs/wsoundmuzzledevice89.o
build/libobjs/wsoundparticles89.o
build/libobjs/gsynthworld89.o
...
```

Each flattened object receives an explicit source dependency rule.  The Makefile
also fails at parse time if future sources introduce duplicate basenames.

The package's top-level directory name is shortened as an additional path-length
safety margin.  No source/header names or public APIs were renamed.

## Validation

Validated with a clean rebuild of the same top-level synth target used by Blank3D:

```
make vendor/weapon_synth_sound_engine89/build/libweapon_synth_sound_engine89.a
```

Also validated `make libs` inside `vendor/weapon_synth_sound_engine89`:

- 64 flattened objects built
- `wsoundmuzzledevice89.o` present in the main archive
- all seven sound-engine archives created
- no nested directories under `build/libobjs/`

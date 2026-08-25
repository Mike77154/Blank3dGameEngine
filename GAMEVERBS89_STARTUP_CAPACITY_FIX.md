# GameVerbs89 startup capacity fix

## Symptom

The MinGW32 executable linked successfully but exited immediately with no
window and no visible error because the release is linked with `-mwindows`.

## Root cause

`create_display_window()` is reached only after the shared GameVerbs89
registry is assembled.  The registry defaulted to 128 fixed entries.

Immediately before the display sextet is registered, Blank3D currently owns
97 entries:

- 30 3DKin condition aliases
- 16 movement actions
- 7 vehicle entries (6 actions + 1 condition)
- 44 TimeVerbs89 entries (20 actions + 24 conditions)

The display sextet then contributes 47 entries:

- GWinVrbs89: 15 actions + 8 conditions
- GenVidVerbs89: 8 actions + 8 conditions
- GmplySS89: 5 actions + 3 conditions

Total required during startup: **144**.

With a capacity of 128, `blank3d_display_stack89_register_verbs()` failed,
`WinMain` returned 2, and native window creation was never attempted.

## Fix

Blank3D now compiles its shared GameVerbs89 ABI with:

```text
-DGVERB89_MAX_ENTRIES=256
```

The vendor remains fixed-capacity and heapless; this is its existing supported
compile-time capacity override.  256 leaves 112 free entries at the current
144-entry startup footprint.

## Regression gate

`make test-gameverb-runtime-capacity89` preloads the 97 pre-display entries,
then asks the real display sextet bridge to register all 47 of its names.  It
must finish at 144 entries without capacity exhaustion.

This gate specifically prevents the silent pre-window startup exit from
returning as more DSL vocabulary is added.

# Blank3D MinGW32 GCC response-file link fix

## Symptom

The full Blank3D build reached the final executable step after building imgcc0,
then MSYS2/MinGW32 failed before GCC could compile/link the runner with:

    make: gcc: Argument list too long
    make: *** [Makefile:826: blank3d.exe] Error 127

The final command had grown to tens of kilobytes because the root Makefile sent
all include paths, compile flags, source files, static libraries, and system
libraries through one shell command.

This is a build-transport failure, not a C89/HOWLUND/Primitive2D/imgcc0 source
failure.

## Fix

The root Makefile now uses GCC response files for the large whole-engine
invocations. GNU make writes the response file internally with `$(file ...)`,
so the enormous argument list is never expanded through the shell.

Release:

    gcc @.blank3d_release.rsp

Debug:

    gcc @.blank3d_debug.rsp

VPhysics demo:

    gcc @.blank3d_vphysics_demo.rsp

Whole-engine syntax check:

    gcc @.blank3d_syntax.rsp

The response file still contains the exact normal GCC options/sources/libs. It
only changes how those arguments reach GCC.

## Why not split/remove vendors?

No source/vendor reduction is needed. The project size is legitimate; the
failure was the operating-system/shell command-line transport. Response files
let the project continue growing without making the root command line grow.

## Clean behavior

`make clean` removes all generated `.blank3d_*.rsp` files.

## Regression gate

    make audit-link-response89

The gate verifies that release, debug, VPhysics demo, and whole-engine syntax
check remain response-file based.

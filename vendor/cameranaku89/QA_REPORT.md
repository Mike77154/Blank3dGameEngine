# QA report — Cameranaku89 v3.4.0

## Strict C89 builds

All bundled demos were compiled with:

```bash
gcc -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude src/*.c demo/<demo>.c
```

Passing demos:

```text
demo_cameranaku89
demo_camera_manager
demo_camera_zones
demo_transform_provider
```

## Provider execution

`demo_transform_provider` confirmed non-zero calls for all receive-provider operations:

```text
move
scale
rotate
```

## Fallback behavior

A separate test confirmed that a provider callback returning zero falls back to the internal operation and produces the expected result.

## Manager propagation

A separate test confirmed that a provider assigned before virtual-camera allocation is copied into newly allocated virtual cameras.

## Regression check

The output of the three original demos was compared against the unmodified v3.3.0 package. There were no output differences:

```text
demo_cameranaku89: no diff
demo_camera_manager: no diff
demo_camera_zones: no diff
```

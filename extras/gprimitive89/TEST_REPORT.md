# Validation report

Validation performed for release 1.0.1:

```text
Core compile:
  gcc -std=c89 -pedantic -Wall -Wextra -Werror

Optional OBJ exporter compile:
  gcc -std=c89 -pedantic -Wall -Wextra -Werror

Runtime tests:
  all primitive generators
  all regular/semi-regular polyhedron tables
  index bounds
  capacity bounds
  fixed multiply/divide/square-root
  CORDIC quarter-turn checks
  color/material/UV operations
  transforms and built-in deformations

Sanitizers:
  AddressSanitizer
  UndefinedBehaviorSanitizer

32-bit check:
  dependency-free core compiled successfully as an i386 relocatable object
```

The optional OBJ exporter was validated in the native build. The validation
container did not contain 32-bit C library development headers, so only the core
was compiled under `-m32`. The exporter uses ordinary ISO C `stdio` and is kept
outside the core for that reason.

The generated OBJ bounding boxes were also checked. The capsule configured with
radius `0.5` and straight height `2.0` produced bounds of:

```text
X: -0.5 to 0.5
Y: -1.5 to 1.5
Z: -0.5 to 0.5
```

This confirms total capsule height is `straight height + 2 * radius`.

## 1.0.1 alias validation

The public names `gp89_make_spherocylinder` and
`gp89_make_rounded_cylinder` were compiled and exercised as direct aliases of
`gp89_make_capsule`. They preserve the same allocation-free fixed-point mesh
path and do not duplicate geometry storage.

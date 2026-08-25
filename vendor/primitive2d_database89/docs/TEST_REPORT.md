# Test report — primitive2d_database89 v0.4.0

## Strict C89 build

Both the default `cc` toolchain and Clang were run with:

```text
-std=c89 -pedantic -Wall -Wextra -Werror
```

Both builds passed.

## Full-catalogue traversal

```text
PASS shapes=720 submodules=57 commands=16321
```

The test walks every registered shape, validates lookup/metadata, emits its complete geometry stream, checks the module registry, and exercises selected Spanish aliases.

## Protocol scan

Exact-token scanning of public headers, sources, tests and examples found no use of:

```text
malloc
calloc
realloc
free
float
double
long long
math.h
```

## v0.3 -> v0.4 preservation

The first 336 catalogue IDs were emitted by both versions and serialized as shape name + opcode + six Q14 command fields. The two streams are byte-identical.

```text
bytes: 84461
SHA-256: 7a6160719bade9014d2122f460df36e8ffeab4d1deef7b89e9804a320d9bb119
result: 336 / 336 preserved
```

The v0.4 catalogue appends IDs 336..719; it does not reorder the v0.3 IDs.

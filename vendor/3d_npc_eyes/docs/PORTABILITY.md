# Portability notes

## Language mode

The code is written in C89-compatible style:

- block-start declarations
- no C99 loop declarations
- no compound literals
- no variable-length arrays
- no standard fixed-width integer header dependency

## Numeric model

The public integer type is:

```c
typedef long tdne_i32;
```

and the public unsigned mask type is:

```c
typedef unsigned long tdne_u32;
```

Direction vectors use a fixed scale:

```c
#define TDNE_DIR_SCALE 1024L
```

A forward vector such as:

```c
tdne_vec3_make(0, 0, 1)
```

is normalized internally to:

```txt
0, 0, 1024
```

## Large worlds

The library has saturating integer helpers for distance and products. Still, the cleanest integration pattern is to call the library with coordinates local to the NPC/sensor when your world is huge.

Recommended engine-side pattern:

```txt
world position
  -> subtract sensor origin
  -> optional sector/local transform
  -> call 3D_NPC_Eyes
```

## Forbidden construct check

Run:

```sh
make check
```

It scans public/source/example/test C files for direct use of dynamic allocation calls, floating types, and decimal numeric literals.

## Ownership

The library owns no external memory. The caller owns:

```txt
target arrays
result arrays
world data
user pointers
raycast implementation
```

# class_manager89 v0.2

A small, language-agnostic class semantics manager inspired by the useful
parts of Python's object model. It is not a language, VM, parser, object ABI,
or allocator.

## Owns

- fixed class registry
- single/multiple inheritance
- C3 MRO
- values, methods, static methods, class methods, properties
- `super` dispatch
- `issubclass` and optional `isinstance`
- generational class handles
- registry seal/unseal
- host-provider callable dispatch
- external/bound receiver dispatch for hosts that already own instances

## Two instance modes

Standalone hosts may leave:

```c
#define CM89_ENABLE_INTERNAL_INSTANCES 1
```

and use the fixed `cm89_instance` pool.

Engines with their own identity/lifetime layer should use:

```c
#define CM89_ENABLE_INTERNAL_INSTANCES 0
```

and call:

```c
cm89_call_bound(manager, dynamic_class,
                cm89_value_host_handle(host_instance),
                "method", &call, &result);
```

This prevents ClassManager from creating a second competing instance system.
Blank3D uses its Thing handle as the external receiver.

## Classmethod rule

For an inherited classmethod:

```text
Base defines kind()
Child derives Base
Child.kind()
```

provider invocation receives:

```text
receiver    = Child        (dynamic class)
owner_class = Base         (defining class)
```

## Memory / numeric rules

The core contains no:

- malloc / calloc / realloc / free
- heap-owned containers
- float / double

Every capacity is compile-time fixed.

## Stale-handle protection

`cm89_class_h` is generational. Destroying a class and reusing the same slot
produces a different handle; the stale handle is rejected.

## Build

Full standalone mode:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude \
    src/cm89.c tests/test_cm89.c -o test_cm89
./test_cm89
```

External-instance mode:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Werror \
    -DCM89_ENABLE_INTERNAL_INSTANCES=0 \
    -Iinclude src/cm89.c tests/test_cm89_bound.c -o test_cm89_bound
./test_cm89_bound
```

License: CC0-1.0.

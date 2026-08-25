# class_manager89 v0.2 design

## Boundary

The manager owns **class semantics**. The host owns syntax, execution and,
when desired, runtime instance identity.

```text
host language / object recipe / editor
               |
               v
        class_manager89
      class registry + C3
               |
         provider.invoke
               v
       host execution backend
```

## External receiver mode

For engines that already have Thing/Entity/Object identities:

```text
Class ---- resolves behavior ----+
                                 |
Thing/host handle ---------------+--> provider.invoke
```

No `cm89_instance` is created.

## Dispatch

```text
dynamic class
    |
    v
C3 lookup(name)
    |
    +-- METHOD -------- receiver = external host handle / internal instance
    +-- CLASS_METHOD -- receiver = dynamic class
    `-- STATIC_METHOD - receiver = NONE

owner_class always identifies where the member was defined.
```

## Generational handles

A 16-bit class handle stores slot+1 in the low byte and generation in the high
byte. `CM89_MAX_CLASSES` therefore has an upper bound of 255.

## Registry sealing

Hosts may unseal while authoring/discovering classes and seal before gameplay.
When sealed, class creation/destruction and class-member mutation return
`CM89_ERR_SEALED`.

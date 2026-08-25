# Runtime Variable Model

`var_manager89` is a fixed-capacity **symbol store**, not a predefined variable struct.

Creation path:

```text
runtime text/name
      │
      ▼
  vm89_set()
      │
      ├── lookup same name in requested scope
      │        └── found -> replace value
      │
      └── not found
               │
               ▼
          claim free slot
               │
               ├── copy runtime name
               └── copy runtime value
```

Deletion:

```text
vm89_unset()
   ↓
clear slot
   ↓
slot has no identity again
```

Therefore variable identities can be authored by:

- a DSL
- an editor
- an event sheet
- loaded data
- generated gameplay
- scripts
- console commands
- network/provider input

without rebuilding this library.

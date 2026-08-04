# RPYL entity spawn aliases

RPYL scene scripts can now name the entity archetype directly instead of
using the generic `enemy` command.

```rpy
gunner_enemy 0 pos 0 0 18 hp 30
zombie 1 pos -8 0 24 hp 30
zombie 2 pos 8 0 24 hp 30
```

The command name is stored as the entity archetype and resolves by convention
to:

```text
config/entities/<archetype>.ini
```

The INI then selects its GFO, logic script, mesh recipe and initial values.
The old `enemy` command remains as a compatibility alias for `zombie`.

To expose another archetype as a direct RPYL word, register its wrapper in
`src/blank3d_languages.c`; the engine-side dispatcher already accepts an
archetype string and resolves the matching INI.

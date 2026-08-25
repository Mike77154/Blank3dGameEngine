# Legacy semantic vector stubs

During provider integration validation, 44 presets from the original `preset_pack192_vector_only` source were confirmed to be semantic stubs in the original ABI3 data: they are marked as vector, but their low-level geometry is `shape=cross`, `arm_mask=0`, `dot_enabled=0`, so the old core emits no primitive for their normal state.

This predates the INI migration and is intentionally not silently changed here, because changing their geometry would alter the original preset data.

Affected IDs:

```text
39 40 41
44 45 46 47
51 53 55 56 57 58
60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76
80 81 82 83 84 85 86 87 88 89 90
93 94 95
```

Provider ABI 2 solves the integration side without corrupting compatibility: high-level vector providers receive `semantic_id`, `semantic_name` and `semantic_category`, so a host vector/HUD system can implement these named shapes even though their old low-level fields are identical.

Native ABI3 shapes (for example the newer geometric/vector presets from ID 144 onward) still render entirely through the bundled core and software primitives.

A future recipe-only cleanup can replace these legacy stubs with explicit composed vector recipes without changing provider89 or runtime89.

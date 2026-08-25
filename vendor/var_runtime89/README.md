# var_runtime89

C89, fixed-capacity, no-heap variable resolver/router for `var_dsl89`.

It deliberately does **not** replace specialized state systems. Providers may
claim known names (for example NumSys values or FlagStore keys), while unknown
assignments fall through to `var_manager89` and are created dynamically.

Resolution rules:

- `var x = ...` -> local frame only.
- `x = ...` / `self.x = ...` -> provider first, then dynamic instance variable.
- `global.x = ...` -> provider first, then dynamic global variable.
- unqualified RHS lookup -> local, provider-backed self, dynamic self.
- globals are explicit, matching GameMaker-style `global.` access.

No provider owns Thing/Entity identity; the host passes an opaque numeric owner
handle. Blank3D uses its Thing runtime key as that owner.

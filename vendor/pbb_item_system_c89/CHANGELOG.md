# Changelog

## 0.2.0

- Split public headers into focused modules plus umbrella `pbb_item_system.h`.
- Split source into world, actor, definition registry, item lifecycle, interaction, drop, rules, effects, events, drop tables, iterators, save/load, and internal helpers.
- Added item categories via `default_category_mask` and `category_mask`.
- Added actor touch/interact masks with `pbb_item_actor_set_masks()`.
- Added opt-in strict manual-droppable mode with `pbb_item_world_set_strict_droppable()`.
- Added consumed-item cleanup with `pbb_item_gc_consumed()`.
- Added global and per-item recycle-on-consume behavior.
- Added item count helpers: `pbb_item_count_used()`, `pbb_item_count_active()`, `pbb_item_count_visible()`.
- Added `PBB_ItemIter` and iterator helpers for rendering/query loops.
- Added runtime save/load helpers with `PBB_ItemSaveState`.
- Expanded tests to cover masks, strict drops, iterators, GC, recycle, and save/load.

## 0.1.0

- Initial C89 fixed-array item interaction core.

# Migration from v1

The original basic provider and `gv89_init`, `gv89_start`, `gv89_reserve`, `gv89_commit`, render and stop calls remain available.

Changes:

- `GV89_STEAL_PRIORITY_QUIETEST` remains as an alias of the new priority+audibility policy.
- `gv89_voice_params` adds `audibility_q15`; call `gv89_voice_params_default` before overriding fields.
- `gv89_request` adds virtualization, bus, instance and protection fields; call `gv89_request_default` before overriding fields.
- `gv89_provider_ex` enables cheap virtual advancement and physical-state callbacks.
- `gv89_stats.peak_active` remains as a compatibility mirror of `peak_logical`.

The ABI changed because the structures grew, so rebuild every object that includes the header.

# Changelog

## v3.4.0

- Added optional `CNK_TRANSFORM_MODE_RECEIVE_PROVIDER` for host-supplied 3D transform math.
- Added independent `move`, component-scale and Euler-rotate callbacks through `cnk_transform_provider`.
- Added safe per-operation fallback to the built-in fixed-point implementation for missing or failed callbacks.
- Added `cnk_transform_apply_trs` and provider-aware basis construction.
- Added per-camera provider storage plus set, clear and query APIs.
- Added `cnk_adapter_receive_transform_provider` for engine/external-library binding.
- Propagated provider math through core camera solving, shake placement, world-to-view, freelook, spring arm, extended collision, virtual cameras and manager blends.
- Added manager-level provider propagation for existing and future virtual cameras.
- Added `demo_transform_provider.c` and `RECEIVE_PROVIDER.md`.
- Preserved C89, fixed-point, no heap, no malloc/realloc/free, no float/double.
- ABI note: `cnk_camera` and `cnk_camera_manager` changed size; hosts embedding them must recompile.

## v3.3.0

- Added `cnk_composer` for rule-of-thirds, center/dead-zone shot scoring and optional viewport offset correction.
- Added `cnk_camera_zone_bank` for trigger volumes, room volumes, fixed-camera routing and cutscene/follow handoffs.
- Added zone flags for force camera, enable camera, blend, confiner application and sticky-style routing.
- Added manager force/hold by virtual-camera id.
- Added manager hysteresis via `switch_quality_margin` and per-vcam `min_live_ticks` support to avoid camera flip-flop.
- Added temporary `priority_bias` on virtual cameras, useful for zones/cues without mutating base priority.
- Upgraded collision strategies: `SLIDE`, `SHOULDER_SWAP` and `CUT_TO_BACKUP` now attempt concrete fallback positions through the world probe.
- Added `cnk_collision_set_strategy` and `cnk_collision_set_fallback`.
- Added `demo_camera_zones.c`.
- Updated umbrella include with composer and zones.
- Kept C89, fixed-point, no heap, no malloc/realloc/free, no float/double.

## v3.2.0

- Added static `cnk_camera_manager` for final camera arbitration.
- Added `cnk_virtual_camera` with priority, standby/live state and shot-quality score.
- Added collision helper module with decollider/deoccluder/confiner-box behavior.
- Added collision-aware `cnk_spring_arm` with independent collision and return lag.
- Added `cnk_target_group` for weighted multi-target framing.
- Added lens extension helpers for fov axis, aspect policy, frustum offset, viewport offset and render/effect masks.
- Added `CNK_PROJ_FRUSTUM`, `CNK_FOV_VERTICAL`, `CNK_FOV_HORIZONTAL`, `CNK_KEEP_HEIGHT`, `CNK_KEEP_WIDTH`.
- Added three-rig `cnk_freelook` helper.
- Extended camera shake with play-space support via `cnk_camera_add_shake_ex`.
- Added declarative `cnk_shake_request` presets.
- Added `cameranaku89_all.h` umbrella include.
- Added `demo_camera_manager.c`.
- Kept C89, fixed-point, no heap, no malloc/realloc/free, no float/double.

## v3.1.0

- Removed old wrapper include paths.
- Renamed franchise-specific shoulder style to generic `OTS`.
- Split futureable profile styles into `cameranaku89_profiles.*`.
- Kept the original seed functionality as native general camera behavior: perspective, orthographic, orbit, viewport, clip planes, FOV, view/projection matrix and world-to-screen.

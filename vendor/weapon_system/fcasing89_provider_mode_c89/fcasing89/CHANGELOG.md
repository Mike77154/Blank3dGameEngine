# Changelog

## 1.1.0

Added optional external provider mode while retaining the complete 1.0
standalone path.

- independent gravity callback with Q8 vector acceleration
- independent move, rotate, and scale callbacks
- external collision query/result contract
- per-callback return-zero fallback to the original implementation
- provider-resolved velocity and forced-sleep collision options
- Q8 scale stored in nodes and render items
- provider binding/removal/query API
- external provider example and integration documentation

No original profile, LOD, pool, event, render, fake-casing, or internal floor
behavior was removed.

# thing_system89

C89, no-heap, context-based runtime instance identity/lifecycle registry derived from the supplied robust ThingSystem design.

It keeps generational handles, reserve->publish, stale-handle rejection, quarantine, locking, flags, namespaces, userdata and integrity checks, but removes singleton globals. Multiple `TS89_System` contexts can coexist.

This library is identity/lifecycle only. It does not own transforms, rendering, physics, ECS components, actors, factions, inventory, equipment, or weapons.

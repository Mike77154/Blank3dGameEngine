# equipment_system89

Provider-driven actor equipment lifecycle for strict C89/no-heap hosts.

The core knows only `actor_id`, `item_id`, opaque presentation descriptors and
attach/runtime callbacks. It does not know weapons, inventory, rendering,
physics, Gamlib3D, GAttach, Blank3D, or an operating system.

A host may bind weapons, clothing, tools, lanterns, shields, quest props or any
other equipable object through providers.

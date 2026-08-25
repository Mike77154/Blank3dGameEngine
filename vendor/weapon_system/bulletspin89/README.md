# bulletspin89

Tiny C89/no-heap barrel-slot sequencer. The host owns one `bs89_state` per independent shooter/weapon pair. Each accepted shot returns the current slot and advances by a configurable step/direction.

It knows nothing about guns, actors, meshes, transforms or projectiles. A six-barrel Gatling is only one possible consumer.

# vehicleprovider89

Optional provider contracts for `gvehicle89` movement and body-physics phases.

The provider is deliberately host-agnostic. It does not include Gamlib3D,
VPhysics, Blank3D, Actor, ECS, rendering, or OS APIs. A host adapter may use
those systems and return `VEHICLEPROVIDER89_HANDLED` for the phases/modules it
owns. Returning `VEHICLEPROVIDER89_DECLINED` preserves the internal fallback.

Movement is capability-scoped per module (`CAR`, `TANK`, `WATER`, `AIR`,
`SPACE`). Physics is phase-scoped (`GRAVITY`, `MASSPOINTS`, `LINEAR_DRAG`,
`INTEGRATE`). This permits partial delegation such as internal car movement +
external integration, or external car movement + internal vehiclephysics89.

# Total Solver changes

## Added

- optional `vpTotalSolver` orchestration layer;
- external transform provider;
- external transform-math provider with per-operation capabilities;
- internal fixed-point fallback for every provider-backed operation;
- public provider-backed math service;
- collision provider for contact generation and sweep TOI;
- transform nodes, hierarchy and external parent mapping;
- manual, external, physics and blended authorities;
- pre/post transform constraints with priorities and iterations;
- parent, copy position/rotation/scale, look-at, distance, position-limit and follow constraints;
- automatic chaining and restoration of legacy world callbacks;
- strict C89 examples and self-tests;
- arena alignment guarantees.

## Preserved

- manual `vpContactsAdd*()` contact flow;
- old `vpCallbacks` behavior;
- direct `vpWorldStep()` usage;
- all existing body, contact, joint, event, water, debug, snapshot and CCD APIs;
- optional islands and split impulse builds.

## Corrected

- arena segment alignment;
- duplicate C89 forward typedefs;
- unsafe fixed-point magnitude handling around the minimum signed value;
- 32x32 multiplication carry handling;
- pair-cache replacement after the table has seen more unique pairs than slots;
- stale example code and contact-normal ordering;
- CCD fallback behavior while Total Solver is attached.

# vertical_motion89_common

Header-only provider ABI shared by `fly89`, `jump89`, and `airdiver89`.

The vendors never include an engine vector, transform, collision, or physics
header. The host supplies:

- `vm89_math_provider`: scalar arithmetic and vector normalization;
- `vm89_transform_provider`: read/write an opaque actor position;
- optional `vm89_physics_provider`: dynamic floor query, grounded query, and
  resolved movement/collision callback.

`vm89_scalar` is a signed `long`. Its numeric format belongs to the host. In
Blank3D it carries Gamlib3D Q20.12 values, but another host can use a different
fixed-point format by implementing the math callbacks consistently.

No allocation is performed by the ABI or by the three consumers.

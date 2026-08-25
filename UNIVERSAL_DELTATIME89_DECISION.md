# UniversalDeltaTime89 decision

Decision: do **not** add a second UniversalDeltaTime89 core.

The supplied `rt_time` already owned the correct concepts but violated the
project protocol through floating point and 64-bit integer types.  Its sanitized
`vendor/rt_time89` port is now the single universal time/delta authority.

Creating another library would duplicate pause/scale/clamp/fixed-step state and
make subsystem ownership ambiguous.  Future movement/physics code should consume
`rt_time89` directly or the host-provided `g.dt`/fixed-step samples rather than
creating private delta clocks.

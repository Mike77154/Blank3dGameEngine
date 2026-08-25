# Protocol89 audit

The supplied timeclocker was nearly compliant and its behavior/tests were sound.
One portability issue remained: `tc_i32`/`tc_u32` used `long`, which becomes 64-bit on LP64 hosts.
The vendored protocol89 copy uses 32-bit `int`/`unsigned int`, replaces LONG limits with INT limits,
and keeps the original fixed-point/no-heap design.

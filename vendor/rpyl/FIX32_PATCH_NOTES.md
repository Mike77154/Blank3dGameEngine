# RPYL 0.4.0 FIX32 patch notes

Patch date: 2026-07-12

This patch hardens the recovered modular RPYL tree for the intended C89/MinGW32 profile.

## Corrected

- `rpyl_u32`, `rpyl_i32`, `rpyl_fixed`, and `rpyl_fx` now select exact 32-bit C89 integer types through `<limits.h>` instead of assuming that `long` is 32 bits.
- Signed 16.16 multiplication and division use manual two-word arithmetic. They do not require `long long`, floating point, heap storage, or a 64-bit `long`.
- Fixed-point conversion, addition, subtraction, multiplication, division, and decimal parsing avoid signed-overflow undefined behaviour and saturate at the signed 32-bit limits.
- `rpyl_compile()` now uses the semantic compiler facade and validates its resulting bytecode before publishing it.
- Buffer, file, and external bytecode installation paths now call `rpyl_vm_validate()` automatically and clear rejected bytecode.
- VM validation now checks table capacities, string/define references, instruction operands, SET flags, skip targets, and duplicate label names.
- Arena alignment now works for any positive alignment value, not only powers of two, and detects `size_t` overflow.
- Added the missing `tests/Makefile`, so `make -C tests run` now works.

## Regression coverage added

- exact 4-byte `rpyl_u32`, `rpyl_i32`, and `rpyl_fx` checks;
- the former MinGW32 failure case `10.0 * 0.5 == 5.0`;
- fixed-point division, decimal parsing, signed limits, and saturation;
- non-power-of-two arena alignment;
- rejection of unresolved static jump targets;
- rejection of bytecode with invalid string operands;
- rejection of serialized bytecode with an invalid opcode.

## Validation performed for this archive

- GCC strict C89 build, CLI build, core check, strict no-stdio core, and tests;
- Clang strict C89 build and tests;
- AddressSanitizer plus UndefinedBehaviorSanitizer tests;
- 200,169 fixed-point arithmetic comparison pairs against a wide-integer reference;
- Clang cross-target compilation of the exact-width types and fixed-point module for `i686-w64-windows-gnu`;
- generated/transpiled C89 source compilation.

## Intentionally unchanged architecture

The default pools remain global and bounded. Therefore the library still has a relatively large static-memory profile and is not inherently thread-safe or reentrant across unsynchronised callers. Those are architectural choices rather than corruption in this recovered copy.

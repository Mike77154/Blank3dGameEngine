# dsl_minimum

SPDX-License-Identifier: CC0-1.0

Strict modular RPYL layout.

This tree keeps the RenPy-ish orchestration surface while staying suitable for deterministic C89 builds: caller-owned or bounded storage, fixed-point helpers, bytecode, VM/runtime, extlang `init cualquierdsl:`, polysym `$ symbol(...)`, configurable block keyword, and configurable start block.

## Hardened ExtLang behavior

`init <language>:` still runs registered external-language callbacks before the
remaining RPYL script is parsed. The sanitized release rejects overlong lines,
out-of-range priorities, ambiguous tab indentation, undersized init indentation,
missing plugins, and exhausted bounded buffers with diagnostics available from
`rpyl_get_last_error()`. Plugins can report their own details with
`rpyl_set_error()`.

See `SANITIZED_RELEASE_NOTES.md` for the complete behavior and regression-test
list.

## Build

```sh
make check
make test
make strict-core
make -C tests run
```

## Important APIs

- `rpyl_set_label_keyword(ctx, "task")` lets scripts use `task boot:` instead of `label start:`.
- `rpyl_set_start_block(ctx, "boot")` changes the default entry block.
- `rpyl_load_buffer_extlang(...)` and `rpyl_load_buffer_extlang_symbols(...)` preserve `init cualquierdsl:` and `$ symbol(...)` orchestration.
- `rpyl_compiler_compile_ast_ex(...)` runs semantics, optionally emits IR, and then emits bytecode.
- `rpyl_vm_validate(...)` and `rpyl_vm_step_opcode(...)` expose bytecode introspection without forcing execution.

## Fixed-point portability

`rpyl_fx` is an exact signed 32-bit 16.16 value on both ILP32 and LP64 hosts. Its multiply/divide implementation is strict C89 and does not depend on `long long`, floating point, or heap allocation.

## Module test

`tests/test_modules.c` covers collections, IO, fixed-point edge cases, warpers, parser, semantics, IR, compiler, VM, invalid-bytecode rejection, and public API execution. See `FIX32_PATCH_NOTES.md` for the recovered-copy corrections and validation record.

## License

This library is dedicated under **CC0 1.0 Universal** (`SPDX-License-Identifier: CC0-1.0`). The complete legal code is included in `LICENSE`.

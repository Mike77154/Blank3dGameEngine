# RPYL strict modular implementation manifest

This tree is no longer a placeholder map. Every runtime/toolchain folder has a compiling C89 surface, and the previously thin modules now carry usable implementation:

| Module | Status |
|---|---|
| `common` | bounded copy/append, hashes, parsing helpers, identifier checks |
| `span` | span construction, join, containment, line checks |
| `error` | fixed diagnostic log and single-error helpers |
| `io` | buffer/reader adapters, stream adapters, line reader |
| `symtab` | owned-name symbol table with typed symbols |
| `registry` | owned-name callback registry with dispatch/removal |
| `store` | owned key/value store with copy/removal |
| `IR` | bounded IR with string interning, arg table, labels, AST lowering |
| `opcodes` | opcode metadata, sizing, validation, formatting |
| `VM` | bytecode validator, label lookup, cursor stepping, runtime bridge |
| `builtins` | builtin catalog, arity rules, RenPy-ish/orchestration flags |
| `semantics` | label/define collection, duplicate checks, static call/jump validation |
| `compiler` | semantics + optional IR + bytecode compilation facade |
| `warper` | fixed-point warper/easing helpers |

Validation commands:

```sh
make -C dsl_minimum check
make -C dsl_minimum test
make -C dsl_minimum strict-core
make -C tests run
```

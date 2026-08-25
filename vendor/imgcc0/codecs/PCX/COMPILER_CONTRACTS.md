# PCX compiler contracts (1.16.0)

This round strengthens compiler-aware contracts in three ways:

- feature-detection helpers in the installed export header
- semantic attributes for pure/const-style APIs
- reproducible smoke targets for header compatibility and static analysis

The intent is to keep the public headers conservative for consumers while still exposing stronger hints to GCC/Clang/MSVC analyzers when supported.

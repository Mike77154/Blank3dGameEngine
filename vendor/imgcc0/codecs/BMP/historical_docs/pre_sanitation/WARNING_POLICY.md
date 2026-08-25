# Warning policy

This round tightens warning policy by compiler family.

- **GCC**: keep the baseline `-Wall -Wextra -Wpedantic -Werror` and add contract-oriented checks such as `-Wstrict-prototypes`, `-Wmissing-prototypes`, `-Wshadow`, `-Wpointer-arith`, `-Wformat=2`, `-Wundef`, `-Walloc-zero`, and `-Wmismatched-dealloc`.
- **Clang/AppleClang**: keep the same baseline and add the portable subset that is supported broadly on Clang.
- **MSVC**: use `/W4 /WX`, opt into stricter conformance with `/permissive-`, and reduce third-party noise with `/external:anglebrackets /external:W0`.

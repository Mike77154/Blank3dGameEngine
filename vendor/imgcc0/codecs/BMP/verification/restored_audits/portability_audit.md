# Portability audit

Overall: PASS

| Check | Result | Details |
|---|---|---|
| configure preset linux-gcc-release | PASS | available configure presets: ['base', 'linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| build preset linux-gcc-release | PASS | available build presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| test preset linux-gcc-release | PASS | available test presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| configure preset linux-clang-release | PASS | available configure presets: ['base', 'linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| build preset linux-clang-release | PASS | available build presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| test preset linux-clang-release | PASS | available test presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| configure preset macos-clang-release | PASS | available configure presets: ['base', 'linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| build preset macos-clang-release | PASS | available build presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| test preset macos-clang-release | PASS | available test presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| configure preset windows-msvc-release | PASS | available configure presets: ['base', 'linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| build preset windows-msvc-release | PASS | available build presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| test preset windows-msvc-release | PASS | available test presets: ['linux-gcc-release', 'linux-clang-release', 'macos-clang-release', 'windows-msvc-release', 'windows-msvc-analyze'] |
| ci workflow portability job | PASS | expected portability-matrix job in .github/workflows/ci.yml |
| ci matrix contains ubuntu-latest | PASS | runner token search |
| ci matrix contains macos-latest | PASS | runner token search |
| ci matrix contains windows-latest | PASS | runner token search |
| ci references linux-gcc-release | PASS | preset token search |
| ci references linux-clang-release | PASS | preset token search |
| ci references macos-clang-release | PASS | preset token search |
| ci references windows-msvc-release | PASS | preset token search |
| CMake uses CTest | PASS | expected include(CTest) |
| CMake enables Windows export-all-symbols | PASS | expected WINDOWS_EXPORT_ALL_SYMBOLS property or variable |
| CMake sets MSVC runtime library | PASS | expected MSVC runtime property |
| CMake sets MSVC /W4 /WX | PASS | expected /W4 and /WX in compile options |

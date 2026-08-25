# Release 1.9.0

## Theme

Ecosystem packaging and consumer validation.

## Highlights

- root-level Conan 2 recipe with `test_package/`
- local vcpkg overlay port
- `pkg-config` consumer smoke example
- CPack binary/source archive generation and audit
- packaging audit automation and CI coverage

## Suggested smoke matrix

```bash
make test-smoke
make abi-check
make cmake-smoke
make ecosystem-audit
make pkgconfig-smoke
make cpack-smoke
make release-package
make verify-release
make verify-provenance
```

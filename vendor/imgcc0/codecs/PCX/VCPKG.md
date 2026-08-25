# vcpkg overlay port for PCX

A local overlay port is available at `packaging/vcpkg/ports/pcx`.

## Typical flow

```bash
vcpkg install pcx --overlay-ports=$PWD/packaging/vcpkg/ports
```

## Port behavior

The port builds directly from the checked-out repository root. This is intentional for internal validation and pre-publication testing.

## Publication follow-up

Before publishing to a public registry, replace the placeholder `copyright` file with the upstream license text and, if desired, switch the port to `vcpkg_from_github()` or a release archive URL.

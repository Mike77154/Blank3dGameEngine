# Backend contract

HOWM89 owns only portable window state and fixed-capacity handles.
A backend supplies native operations through `HOWM89_BackendVTable`.

Required callbacks:

- `create_window`
- `destroy_window`

Everything else is optional. Missing operations return `HOWM89_ERROR_NOT_SUPPORTED`.

The provider may be:

- a native OS API adapter
- a window toolkit adapter
- a virtual-machine or emulator facade
- a test/mock implementation
- a remote/embedded host shim

HOWM89 never asks the provider which operating system is underneath it.

## Native-handle escape hatch

`get_native_handle` is optional and intentionally opaque. It exists for render/context adapters that genuinely need the underlying native object. Portable game code should not depend on it.

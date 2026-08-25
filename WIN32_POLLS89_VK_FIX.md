# Win32 polls89 virtual-key portability fix

The Win32 `GetAsyncKeyState` backend previously referenced `VK_STOP`,
`VK_UNDO`, `VK_CUT`, `VK_COPY`, `VK_PASTE`, and `VK_FIND`. Those names are
not part of the standard Win32 virtual-key set used by MinGW headers.

The affected USB HID Keyboard/Keypad usages remain valid canonical
`input_keys89` identities, but the polling backend now returns unsupported
(`0`) for them instead of inventing a Win32 VK mapping. This preserves the
four-head input model: identity can exist even when one acquisition backend
cannot observe it through `GetAsyncKeyState`.

The local Win32 test shim was also corrected so it no longer defines these
nonexistent VK names and therefore cannot mask this regression again.

`blank3d_list_cycle.c` also replaced the warning-prone bounded `strncpy` with
an exact `memcpy(strlen + 1)` after normalization, whose output is already
bounded by the destination capacity.

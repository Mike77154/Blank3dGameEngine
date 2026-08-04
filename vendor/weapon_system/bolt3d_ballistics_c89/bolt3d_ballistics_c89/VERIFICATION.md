# Verification

The 0.3.0 source package was checked with:

```txt
GCC C89 strict build: passed
GCC test suite: passed
Clang C89 strict build: passed
Clang test suite: passed
Forbidden-token audit: passed
32-bit core object compilation: passed
```

Test executables cover fixed-point math, collision, legacy projectiles, hitscan, transform round trips, solver blocking, contract sliding, external transform synchronization, event pointer separation, host-provided vector gravity, and declined-hook fallback to built-in gravity.

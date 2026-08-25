# Fit with Display Sextet89

HOWM89 is intended to sit below GenWinConfigC89, not beside GeneralVideoConfigC89.

```
GWinVrbs89
    |
GenWinConfigC89
    |
provider adapter
    |
HOWM89
    |
OS/toolkit/custom backend
```

HOWM89 must not receive:

- render resolution
- ScreenScale
- camera draw size
- scene/room size

Those remain responsibilities of GeneralVideoConfigC89 and GameplayScreenSizeC89.

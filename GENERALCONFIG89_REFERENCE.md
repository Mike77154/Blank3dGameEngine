# GeneralConfig.cfg reference

`GeneralConfig.cfg` is deliberately not `blank3d.toml`. It describes host/window/render presentation policy that a port may replace without changing game data.

## [Window]

- `WindowSize = WIDTHxHEIGHT`: requested native client area.
- `PlayMode = windowed | fullscreen | borderless | borderless_fullscreen`.
- `Resizable = 0|1`.
- `Decorated = 0|1`.
- `Center = 0|1`.
- `AlwaysOnTop = 0|1`.
- `Visible = 0|1`.
- `Title = text`.
- `Monitor = integer index`.
- Optional `Position = X,Y`; explicit position disables automatic centering.

These values are consumed by GenWinConfigC89 and executed only by its bound provider.

## [Video]

- `Resolution = WIDTHxHEIGHT`: internal render resolution.
- `ScreenScale = native | fit | stretch | integer | overscan | Nx` where N is 1..16.
- `KeepAspect = 0|1`.
- `CenterOutput = 0|1`.
- `Filter = nearest | linear`.
- `VSync = 0|1`.

GeneralVideoConfigC89 calculates an integer presentation rectangle; it never creates a native window.

## Example: retro internal resolution on a large window

```ini
[Window]
WindowSize = 1920x1080
PlayMode = borderless_fullscreen

[Video]
Resolution = 320x180
ScreenScale = integer
Filter = nearest
KeepAspect = 1
CenterOutput = 1
```

The renderer remains 320x180. Presentation scales it to the largest integer multiple that fits the output.

## Gameplay dimensions are elsewhere

Game-space values stay in `blank3d.toml`:

```toml
[camera]
camerasizedraw = "640x360"

[scene]
scenescreensize = "8000x4500"
```

Changing native WindowSize or renderer Resolution does not implicitly mutate those values.

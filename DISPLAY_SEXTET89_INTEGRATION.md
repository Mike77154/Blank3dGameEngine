# Display Sextet89 integration

Blank3D splits display behavior into three independent spaces and gives each space a state/config vendor plus a verb vendor.

## Responsibility graph

```text
Native window / OS shell
    GenWinConfigC89
        |
    GWinVrbs89
        |
    platform provider (Win32 today, Howlund/SDL/X11/etc later)

Renderer/output presentation
    GeneralVideoConfigC89
        |
    GenVidVerbs89
        |
    renderer provider (OpenGL today, any renderer later)

Gameplay logical screen space
    GameplayScreenSizeC89
        |
    GmplySS89
        |
    engine/camera provider (Blank3D/Cameranaku today, any engine later)
```

The six vendors never include Win32, OpenGL, Blank3D, SDL, GameMaker, or Howlund headers.

## Five sizes that are intentionally different

1. SceneScreenSize: logical room/scene extent.
2. CameraSizeDraw: logical rectangle observed by the gameplay camera.
3. Resolution: internal renderer resolution.
4. Presentation rectangle: Resolution mapped into the current output by ScreenScale.
5. WindowSize: native client-area size owned by the platform/window provider.

Example:

```text
SceneScreenSize  = 8000x4500
CameraSizeDraw   = 640x360
Resolution       = 320x180
ScreenScale      = integer
WindowSize       = 1920x1080
```

A window resize changes only WindowSize/output presentation. It does not silently change Resolution, CameraSizeDraw, or SceneScreenSize.

## Configuration ownership

`config/GeneralConfig.cfg` is host/port presentation configuration, not game configuration:

```ini
[Window]
WindowSize = 960x540
PlayMode = windowed
Resizable = 1
Decorated = 1
Center = 1
AlwaysOnTop = 0
Visible = 1
Title = Blank3D
Monitor = 0

[Video]
Resolution = 960x540
ScreenScale = fit
KeepAspect = 1
CenterOutput = 1
Filter = nearest
VSync = 1
```

`config/blank3d.toml` retains gameplay-oriented logical dimensions:

```toml
[camera]
camerasizedraw = "960x540"

[scene]
scenescreensize = "1920x1080"
```

The old `window 960 540` line was removed from active `scripts/startup.rpy` and preserved as `scripts/legacy/window_from_startup_rpy.rpy`.

## Scaling modes

GeneralVideoConfigC89 supports:

- native / 1:1
- fit / letterbox
- stretch
- integer
- overscan / crop
- fixed 1x..16x

Filter mode is independently `nearest` or `linear`.

## GameVerbs / DSL bridge

Blank3D registers all three verb catalogs in the shared GameVerbs89 registry. RPYL, FPIL and DDSL2 therefore see the same named operations through their existing GameVerbs bridge.

Window actions include `window_size`, `window_mode`, `window_position`, `window_title`, `window_resizable`, `window_decorated`, `window_visible`, `window_topmost`, `window_monitor`, `window_apply`, `window_create`, `window_destroy`, and `window_refresh`. `window W H` remains as a legacy alias for `window_size W H`.

Video actions include `video_resolution`, `video_output_size`, `screen_scale`, `video_vsync`, `video_filter`, `video_keep_aspect`, `video_center_output`, and `video_apply`.

Gameplay actions include `camera_size_draw`, `scene_screen_size`, `camera_screen_pos`, `camera_screen_clamp`, and `gameplay_screen_apply`.

## Blank3D runtime adapters

- `src/blank3d_display_stack89.*`: orchestration and GameVerbs registration.
- `src/blank3d_window_win32.*`: current native-window provider only.
- `src/blank3d_video_gl89.*`: current OpenGL render/presentation provider only.

`WM_SIZE` updates GenWinConfigC89 observed client size and GeneralVideoConfigC89 output size. It does not rewrite gameplay camera dimensions.

## Why this is ready for Howlund Window Maker

A future Howlund adapter only has to implement `gwc89_provider`. GenWinConfigC89 does not need to know whether the provider internally uses Win32, X11, Cocoa, SDL, an emulator, or a fake test host. GeneralVideoConfigC89 remains separate, so Howlund's historical `internal_size` concept should not become the renderer authority in Blank3D.

## External design references consulted

- Microsoft Win32 `AdjustWindowRectEx`: distinguishes requested client area from the outer native window rectangle.
  https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectex
- SDL3 `SDL_SetRenderLogicalPresentation`: logical render resolution is mapped to the actual output independently of window resize/high DPI.
  https://wiki.libsdl.org/SDL3/SDL_SetRenderLogicalPresentation
- SDL3 logical presentation modes: disabled, stretch, letterbox, overscan, integer scale.
  https://wiki.libsdl.org/SDL3/SDL_RendererLogicalPresentation
- GameMaker room/camera/viewport documentation: camera view and physical viewport are separate rectangles.
  https://manual.gamemaker.io/lts/en/The_Asset_Editors/Room_Properties/Room_Properties.htm

## Native window provider after HOWM89 integration

The sextet's native-window side now resolves through:

```text
GWinVrbs89 -> GenWinConfigC89 -> HOWM89 adapter -> HOWM89 -> WindowsWindow89
```

This does not add a Howlund dependency to GenWinConfigC89. The dependency
exists only in the optional adapter. Blank3D's Win32 host file is now a thin
native-handle/HDC bridge rather than the native-window implementation.

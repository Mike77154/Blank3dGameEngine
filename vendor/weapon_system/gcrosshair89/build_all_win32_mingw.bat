@echo off
setlocal
for %%D in (gcrosshair_core89 gcrosshair_params89 gcrosshair_base89 gcrosshair_provider89 gcrosshair_anim89 gcrosshair_runtime89) do (
  echo == %%D ==
  pushd %%D
  call build_win32_mingw.bat
  if errorlevel 1 exit /b 1
  popd
)
endlocal

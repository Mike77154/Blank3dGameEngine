@echo off
for %%L in (gweaponbody89 gmuzzlegas89 gballisticcrack89 glatetail89 gcinemathump89) do (
  pushd %%L
  call build_mingw32.bat || exit /b 1
  popd
)

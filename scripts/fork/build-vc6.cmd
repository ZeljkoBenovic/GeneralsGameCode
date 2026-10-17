@echo off
rem Builds the retail compatible "Online" Zero Hour executable with VC6 SP6, like upstream CI does.
rem Usage: build-vc6.cmd [preset] [clean]   (default preset: vc6)
rem Needs VC6SP6 in %VC6_ROOT% (default D:\Dev\VC6\VC6SP6) and CMake + Ninja in %CMAKE_BIN% / %NINJA_BIN%.
rem
rem IMPORTANT: VC6 cannot report header dependencies to Ninja, so a changed header does not rebuild the files
rem that include it. Pass "clean" after any header change and for every release build, otherwise objects
rem compiled against different class layouts get linked together and the game crashes.
setlocal
set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=vc6"
set "CLEAN=%~2"
if "%VC6_ROOT%"=="" set "VC6_ROOT=D:\Dev\VC6\VC6SP6"
if "%CMAKE_BIN%"=="" set "CMAKE_BIN=D:\Dev\VS2022BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
if "%NINJA_BIN%"=="" set "NINJA_BIN=D:\Dev\VS2022BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

set "VSCommonDir=%VC6_ROOT%\Common"
set "MSDevDir=%VC6_ROOT%\Common\msdev98"
set "MSVCDir=%VC6_ROOT%\VC98"
set "VcOsDir=WINNT"
set "PATH=%MSDevDir%\BIN;%MSVCDir%\BIN;%VSCommonDir%\TOOLS\%VcOsDir%;%VSCommonDir%\TOOLS;%CMAKE_BIN%;%NINJA_BIN%;%SystemRoot%\System32;%SystemRoot%"
set "INCLUDE=%MSVCDir%\ATL\INCLUDE;%MSVCDir%\INCLUDE;%MSVCDir%\MFC\INCLUDE"
set "LIB=%MSVCDir%\LIB;%MSVCDir%\MFC\LIB"

cd /d "%~dp0\..\.." || exit /b 1
if not exist build\%PRESET%\build.ninja (
  cmake --preset %PRESET% || exit /b 1
)
if /i "%CLEAN%"=="clean" (
  cmake --build build/%PRESET% --target clean || exit /b 1
)
cmake --build build/%PRESET% --target z_generals || exit /b 1

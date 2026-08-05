@echo off
setlocal

set ROOT=%~dp0
set SRC=%ROOT%src
set OUT=%ROOT%build
set ART=%ROOT%artifacts
set LIB=%ROOT%simple_ui

if not exist "%LIB%\simple_ui.dll" (
  echo Library not found: "%LIB%\simple_ui.dll"
  exit /b 1
)

if not exist "%OUT%" mkdir "%OUT%"
if not exist "%ART%" mkdir "%ART%"

echo Compiling resources...
windres "%SRC%\app.rc" -O coff -o "%ART%\app_res.o" --include-dir "%SRC%"
if errorlevel 1 (
  echo Resource compile failed.
  exit /b 1
)

echo Building Nuclear Fusion...
g++ -std=c++17 -O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
  -o "%OUT%\nuclear_fusion.exe" ^
  "%SRC%\main.cpp" ^
  "%SRC%\scenes\game.cpp" ^
  "%SRC%\game\star_canvas_renderer.cpp" ^
  "%ART%\app_res.o" ^
  -I "%LIB%" -I "%SRC%" ^
  -L "%LIB%" -lsimple_ui ^
  -ld3d11 -ldxgi -ld3dcompiler -lgdi32 -luser32 -lole32 -lwindowscodecs ^
  -static-libgcc -static-libstdc++ ^
  -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive,-Bdynamic ^
  -mwindows -municode

if errorlevel 1 (
  echo Build failed.
  exit /b 1
)

copy /Y "%LIB%\simple_ui.dll" "%OUT%\simple_ui.dll" >nul
copy /Y "%SRC%\icon.ico" "%OUT%\icon.ico" >nul

if exist "%OUT%\assets" rmdir /S /Q "%OUT%\assets"
xcopy /E /I /Y "%SRC%\assets" "%OUT%\assets" >nul

echo Built %OUT%\nuclear_fusion.exe
endlocal

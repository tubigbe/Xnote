@echo off
setlocal

echo === Xnote C++ Build ===

:: Check for Visual Studio
where cl >nul 2>&1
if errorlevel 1 (
    echo Error: cl.exe not found. Run from Visual Studio Developer Command Prompt.
    exit /b 1
)

:: Create output directories
if not exist "build" mkdir build
if not exist "build\plugins" mkdir build\plugins

echo Compiling Xnote.exe...
cl /EHsc /utf-8 /I. Xnote.cpp /Fe:build\Xnote.exe /link user32.lib shell32.lib ole32.lib
if errorlevel 1 (
    echo Failed to compile Xnote.exe
    exit /b 1
)

echo Compiling plugins...

echo   - change_archive_path.dll
cl /EHsc /utf-8 /I. /DPLUGIN_EXPORTS plugins\change_archive_path.cpp /Fe:build\plugins\change_archive_path.dll /LD /link user32.lib shell32.lib ole32.lib
if errorlevel 1 (
    echo Failed to compile change_archive_path.dll
    exit /b 1
)

echo   - inject_timestamp.dll
cl /EHsc /utf-8 /I. /DPLUGIN_EXPORTS plugins\inject_timestamp.cpp /Fe:build\plugins\inject_timestamp.dll /LD /link user32.lib
if errorlevel 1 (
    echo Failed to compile inject_timestamp.dll
    exit /b 1
)

echo   - open_plugins_folder.dll
cl /EHsc /utf-8 /I. /DPLUGIN_EXPORTS plugins\open_plugins_folder.cpp /Fe:build\plugins\open_plugins_folder.dll /LD /link user32.lib shell32.lib
if errorlevel 1 (
    echo Failed to compile open_plugins_folder.dll
    exit /b 1
)

echo   - open_system_folder.dll
cl /EHsc /utf-8 /I. /DPLUGIN_EXPORTS plugins\open_system_folder.cpp /Fe:build\plugins\open_system_folder.dll /LD /link user32.lib shell32.lib
if errorlevel 1 (
    echo Failed to compile open_system_folder.dll
    exit /b 1
)

echo.
echo === Build Complete ===
echo Output: build\Xnote.exe
echo Plugins: build\plugins\*.dll
echo.
echo To run: build\Xnote.exe

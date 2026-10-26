@echo off
rem Opcional: apenas para desenvolver/recompilar o compilador.
setlocal
cd /d "%~dp0"
python tools\generate_vocabulary.py
if errorlevel 1 exit /b %errorlevel%
if not exist bin\windows-x64 mkdir bin\windows-x64
if exist C:\msys64\ucrt64\bin\g++.exe set "PATH=C:\msys64\ucrt64\bin;%PATH%"
where g++ >nul 2>nul
if not errorlevel 1 goto mingw
where cl >nul 2>nul
if not errorlevel 1 goto msvc
echo Compilador C++17 ausente. Para USAR UmaScript, execute Instalar_Windows.exe.
exit /b 1
:mingw
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -static -static-libgcc -static-libstdc++ -municode -Wl,--stack,268435456 -Wl,--no-insert-timestamp src\uma.cpp -o bin\windows-x64\uma.exe -lshell32 -lole32 -ladvapi32 -luser32 -luuid
if errorlevel 1 exit /b %errorlevel%
python -c "import tomllib,pathlib;pathlib.Path('bin/windows-x64/version.txt').write_text(tomllib.loads(pathlib.Path('palavras.toml').read_text())['version'])"
exit /b %errorlevel%
:msvc
cl /nologo /std:c++17 /EHsc /O2 /W4 /MT /utf-8 /Fe:bin\windows-x64\uma.exe src\uma.cpp /link /STACK:268435456 Shell32.lib Ole32.lib Advapi32.lib User32.lib Uuid.lib
if errorlevel 1 exit /b %errorlevel%
python -c "import tomllib,pathlib;pathlib.Path('bin/windows-x64/version.txt').write_text(tomllib.loads(pathlib.Path('palavras.toml').read_text())['version'])"
exit /b %errorlevel%

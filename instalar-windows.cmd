@echo off
setlocal
"%~dp0bin\windows-x64\uma.exe" install --package-root "%~dp0."
if errorlevel 1 (
  echo Falha na instalacao. Leia a mensagem acima.
  pause
  exit /b 1
)
echo.
echo Feche e reabra o terminal para usar o comando uma.
pause

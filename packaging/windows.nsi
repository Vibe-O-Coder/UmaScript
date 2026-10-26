Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
Name "UmaScript 3.0.0"
OutFile "Instalar_Windows.exe"
InstallDir "$LOCALAPPDATA\Programs\UmaScript"
RequestExecutionLevel user
SetCompressor /SOLID lzma
BrandingText "UmaScript | Compilador independente"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "PortugueseBR"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK "Este pacote inclui uma.exe para Windows de 64 bits."
    Abort
  ${EndIf}
FunctionEnd

Section "Instalar"
  InitPluginsDir
  SetOutPath "$PLUGINSDIR\package\bin\windows-x64"
  File "bin/windows-x64/uma.exe"
  File "bin/windows-x64/version.txt"
  SetOutPath "$PLUGINSDIR\package\src"
  File /r "src/*"
  SetOutPath "$PLUGINSDIR\package\stdlib"
  File /r "stdlib/*"
  SetOutPath "$PLUGINSDIR\package\examples"
  File /r "examples/*"
  SetOutPath "$PLUGINSDIR\package\docs"
  File /r "docs/*"
  SetOutPath "$PLUGINSDIR\package"
  File "README.md"
  File "LICENSE"
  nsExec::ExecToLog '"$PLUGINSDIR\package\bin\windows-x64\uma.exe" install --prefix "$INSTDIR" --package-root "$PLUGINSDIR\package"'
  Pop $0
  StrCmp $0 "0" success
    MessageBox MB_OK "A instalacao falhou. Consulte os detalhes acima."
    Abort
  success:
  WriteUninstaller "$INSTDIR\Desinstalar.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript" "DisplayName" "UmaScript 3.0.0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript" "DisplayVersion" "3.0.0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript" "UninstallString" '"$INSTDIR\Desinstalar.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript" "NoRepair" 1
SectionEnd

Section "Uninstall"
  nsExec::ExecToLog '"$INSTDIR\uma.exe" uninstall --prefix "$INSTDIR"'
  Pop $0
  StrCmp $0 "0" remove
    MessageBox MB_OK "Nao foi possivel concluir a desinstalacao. Consulte os detalhes."
    Abort
  remove:
  Delete "$INSTDIR\uma.exe"
  Delete "$INSTDIR\Desinstalar.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\UmaScript"
SectionEnd

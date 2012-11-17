; kate: encoding iso8859-15

;
; use this file with InnoSetup http://www.jrsoftware.org/isinfo.php
;

#define MyAppName "pickAndPlaceMachine"
#define MyAppExeName "pickAndPlaceMachine.exe"
#define MyURL "https://github.com/sibbi77/pickAndPlaceMachine"
#include "..\version.h"

#define MinGW "c:\dev\QtSDK\mingw"
#define QTDIR "c:\dev\QtSDK\Desktop\Qt\4.8.1\mingw"
#define VTKDIR "c:\dev\projects\openems-w32\vtk"

[Setup]
AppId=pickAndPlaceMachine
AppName={#MyAppName}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher=Held
AppPublisherURL={#MyURL}
AppSupportURL={#MyURL}
AppUpdatesURL={#MyURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputBaseFilename=setup
Compression=lzma2/max
SolidCompression=true
ShowLanguageDialog=yes
InternalCompressLevel=normal
AppVersion={#MyAppVersion}
AppCopyright=© Sebastian Held 2012
AppContact=Sebastian Held <sebastian.held@gmx.de>
;InfoBeforeFile=infoBefore.txt
LicenseFile=..\COPYING
ChangesAssociations=yes

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}

[Files]
Source: ..\release\pickAndPlaceMachine.exe; DestDir: {app}; Flags: ignoreversion

; Qt
Source: {#QTDIR}\bin\QtCore4.dll; DestDir: {app}; Flags: ignoreversion
Source: {#QTDIR}\bin\QtGui4.dll; DestDir: {app}; Flags: ignoreversion
Source: {#QTDIR}\bin\QtNetwork4.dll; DestDir: {app}; Flags: ignoreversion
Source: {#QTDIR}\bin\QtSql4.dll; DestDir: {app}; Flags: ignoreversion
Source: {#QTDIR}\bin\QtWebKit4.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#QTDIR}\bin\QtHelp4.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#QTDIR}\bin\QtCLucene4.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#QTDIR}\bin\phonon4.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#QTDIR}\bin\QtXmlPatterns4.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#QTDIR}\bin\QtXml4.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#QTDIR}\bin\QtSvg4.dll; DestDir: {app}; Flags: ignoreversion

Source: {#MinGW}\bin\mingwm10.dll; DestDir: {app}
Source: {#MinGW}\bin\libgcc_s_dw2-1.dll; DestDir: {app}
;Source: {#MinGW}\bin\libstdc++-6.dll; DestDir: {app}

Source: ..\COPYING; DestDir: {app}
Source: ..\AUTHOR; DestDir: {app}
Source: ..\CHANGELOG; DestDir: {app}
Source: ..\README.md; DestDir: {app}

; Qt-Plugins
Source: {#QTDIR}\plugins\imageformats\qgif4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
Source: {#QTDIR}\plugins\imageformats\qico4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
Source: {#QTDIR}\plugins\imageformats\qjpeg4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
Source: {#QTDIR}\plugins\imageformats\qmng4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
Source: {#QTDIR}\plugins\imageformats\qsvg4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
Source: {#QTDIR}\plugins\imageformats\qtga4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
Source: {#QTDIR}\plugins\imageformats\qtiff4.dll; DestDir: {app}\imageformats; Flags: ignoreversion
;Source: {#QTDIR}\plugins\sqldrivers\qsqlodbc4.dll; DestDir: {app}\sqldrivers; Flags: ignoreversion
;Source: {#QTDIR}\plugins\sqldrivers\qsqlite4.dll; DestDir: {app}\sqldrivers; Flags: ignoreversion
;Source: {#QTDIR}\plugins\sqldrivers\qsqlmysql4.dll; DestDir: {app}\sqldrivers; Flags: ignoreversion
;Source: {#QTDIR}\plugins\sqldrivers\qsqlpsql4.dll; DestDir: {app}\sqldrivers; Flags: ignoreversion

; Qxt
;Source: {#LIBQXT}\lib\QxtCore.dll; DestDir: {app}; Flags: ignoreversion
;Source: {#LIBQXT}\lib\QxtGui.dll; DestDir: {app}; Flags: ignoreversion

; VTK
Source: {#VTKDIR}\bin\*.dll; DestDir: {app}; Flags: ignoreversion

Source: {#QTDIR}\translations\qt_de.qm; DestDir: {app}\translations; Languages: 

[Icons]
Name: {commondesktop}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; Tasks: desktopicon; IconFilename: {app}\{#MyAppExeName}
Name: {commondesktop}\{#MyAppName}-USB; Filename: {app}\{#MyAppExeName}; Tasks: desktopicon; IconFilename: {app}\{#MyAppExeName}
Name: {group}\{#MyAppName}; Filename: {app}\{#MyAppExeName}; IconFilename: {app}\{#MyAppExeName}
Name: {group}\Homepage; Filename: {#MyURL}

[Run]
Filename: {app}\{#MyAppExeName}; Description: {cm:LaunchProgram,{#MyAppName}}; Flags: nowait postinstall skipifsilent

[Languages]
Name: en; MessagesFile: compiler:Default.isl
Name: de; MessagesFile: compiler:Languages\German.isl

[Registry]
; Root: HKCR; Subkey: .PaPM; ValueType: string; ValueName: ; ValueData: PickAndPlaceMachine_Project; Flags: uninsdeletevalue
;Root: HKCR; Subkey: PickAndPlaceMachine_Project; ValueType: string; ValueName: ; ValueData: pickAndPlaceMachine project; Flags: uninsdeletekey
;Root: HKCR; Subkey: PickAndPlaceMachine_Project\DefaultIcon; ValueType: string; ValueName: ; ValueData: {app}\{#MyAppExeName},0; Flags: uninsdeletekey
;Root: HKCR; Subkey: PickAndPlaceMachine_Project\shell\open\command; ValueType: string; ValueName: ; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey

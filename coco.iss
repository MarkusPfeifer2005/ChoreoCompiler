; coco.iss

#define MyAppName "COCO"
#define MyAppVersion "1.0.0-alpha"
#define MyAppPublisher "ChoreoCompiler"
#define MyAppExeName "coco.exe"

[Setup]
AppId={{8F6E7D4A-3C91-4F5A-A6C2-COCO00000001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}

DefaultDirName={autopf}\ChoreoCompiler
DefaultGroupName=ChoreoCompiler

OutputDir=.
OutputBaseFilename=coco-setup

ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

Compression=lzma
SolidCompression=yes

Uninstallable=yes

[Files]
Source: "build\Release\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\COCO"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\COCO"; Filename: "{app}\{#MyAppExeName}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch COCO"; Flags: nowait postinstall skipifsilent

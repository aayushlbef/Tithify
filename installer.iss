; Tithify - Inno Setup Installer Script
; Compiles to a single Setup.exe that installs the widget

#define MyAppName "Tithify"
#define MyAppVersion "3.6.4"
#define MyAppPublisher "Aayush"
#define MyAppURL "https://github.com/aayushlbef/Tithify"
#define MyAppExeName "Tithify.exe"
#define MyAppWebsite "https://aayushlbef.github.io/Tithify/"

[Setup]
; Unique App ID - DO NOT change this after first release
AppId={{8A2F4C6E-3D1B-4E7A-9F5C-2B8D6E1A4C3F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={localappdata}\{#MyAppName}
DefaultGroupName={#MyAppName}
UsePreviousAppDir=no
UsePreviousGroup=no
DirExistsWarning=no
DisableProgramGroupPage=yes
LicenseFile=LICENSE
OutputDir=installer_output
OutputBaseFilename=Tithify_Setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\icon.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Visual settings removed to use defaults

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startuprun"; Description: "Launch at Windows startup"; GroupDescription: "Other options:"

[Files]
Source: "Tithify.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\icon.ico"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\icon.ico"; Tasks: desktopicon

[Registry]
; Add to startup if user checked the option
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Tithify"; ValueData: """{app}\{#MyAppExeName}"""; Flags: uninsdeletevalue; Tasks: startuprun

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
Filename: "{#MyAppWebsite}"; Description: "Visit the Tithify website"; Flags: shellexec nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\tithify.cfg"
Type: files; Name: "{app}\widget.cfg"

[Code]
// Kill running instance before install/uninstall and migrate old settings
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  OldDir, OldCfg, NewCfg: String;
begin
  if CurStep = ssInstall then
  begin
    Exec('taskkill', '/F /IM Tithify.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Exec('taskkill', '/F /IM NepaliDateWidget.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Sleep(500);
  end
  else if CurStep = ssPostInstall then
  begin
    // Migrate legacy config if present
    OldDir := ExpandConstant('{localappdata}\Nepali Date Widget');
    OldCfg := OldDir + '\widget.cfg';
    NewCfg := ExpandConstant('{app}\tithify.cfg');
    if FileExists(OldCfg) and not FileExists(NewCfg) then
    begin
      CopyFile(OldCfg, NewCfg, False);
    end;
    // Clean up old legacy directory if it exists
    if DirExists(OldDir) then
    begin
      DelTree(OldDir, True, True, True);
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usUninstall then
  begin
    Exec('taskkill', '/F /IM Tithify.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Exec('taskkill', '/F /IM NepaliDateWidget.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Sleep(500);
  end;
end;

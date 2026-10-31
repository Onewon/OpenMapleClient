param(
    [Parameter(Mandatory = $true)][string]$ResourcePath,
    [string]$Device = 'emulator-5556',
    [string]$Sdk = "$env:LOCALAPPDATA\Android\Sdk"
)
$ErrorActionPreference = 'Stop'
$adb = Join-Path $Sdk 'platform-tools\adb.exe'
$names = @('Base', 'Character', 'Effect', 'Etc', 'Item', 'Map', 'MapPretty',
    'MapLatest', 'Map001', 'Mob', 'Morph', 'Npc', 'Quest', 'Reactor', 'Skill',
    'Sound', 'String', 'TamingMob', 'UI')
foreach ($name in $names) {
    if (-not (Test-Path -LiteralPath (Join-Path $ResourcePath "$name.nx") -PathType Leaf)) {
        throw "Missing resource: $name.nx"
    }
}
$destination = '/sdcard/Android/media/com.OpenMapleClient/NX'
& $adb -s $Device shell mkdir -p $destination
if ($LASTEXITCODE -ne 0) { throw 'Cannot create the resource directory. Install the APK first.' }
foreach ($name in $names) {
    & $adb -s $Device push (Join-Path $ResourcePath "$name.nx") "$destination/$name.nx"
    if ($LASTEXITCODE -ne 0) { throw "Copy failed: $name.nx" }
}

param(
    [string]$Avd = 'BeiDou083_API34',
    [string]$Device = 'emulator-5556',
    [string]$Sdk = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$Apk = "$PSScriptRoot\..\app\build\outputs\apk\debug\app-debug.apk"
)
$ErrorActionPreference = 'Stop'
$adb = Join-Path $Sdk 'platform-tools\adb.exe'
$emulator = Join-Path $Sdk 'emulator\emulator.exe'
if (-not ((& $adb devices) -match "^$Device\s+device")) {
    Start-Process -FilePath $emulator -ArgumentList "-avd $Avd -port 5556 -no-snapshot -no-audio -gpu swiftshader_indirect" -WindowStyle Hidden
    & $adb -s $Device wait-for-device
    do {
        Start-Sleep -Seconds 2
        $booted = & $adb -s $Device shell getprop sys.boot_completed
    } until ($booted.Trim() -eq '1')
}
foreach ($port in @(18484, 17575, 17576, 17577)) {
    & $adb -s $Device reverse "tcp:$port" "tcp:$port"
    if ($LASTEXITCODE -ne 0) { throw "Port mapping failed: $port" }
}
& $adb -s $Device install -r $Apk
if ($LASTEXITCODE -ne 0) { throw 'APK installation failed.' }
& $adb -s $Device shell am start -n com.OpenMapleClient/openMapleClient.MainActivity
Write-Host 'Use 127.0.0.1:18484 and enable the shared channel host option for adb reverse.'

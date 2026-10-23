# Portable Windows release build via MSYS2 (clang64).
# Usage: .\release-build.ps1   ->  dist\BlockStart\BlockStart.exe and dist\BlockStart-<version>-windows-portable.zip

# Not "Stop": PowerShell 5.1 turns native stderr (e.g. CMake warnings) into errors; we check $LASTEXITCODE instead
$ErrorActionPreference = "Continue"
$msys = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path $msys)) { throw "MSYS2 not found at C:\msys64" }

$env:MSYSTEM = "CLANG64"
$env:CHERE_INVOKING = "1"

# Qt's moc (MinGW) breaks on non-ASCII paths, so build through an ASCII junction
$asciiRoot = "C:\dev\freesm"
if (-not (Test-Path $asciiRoot)) {
    New-Item -ItemType Directory -Force (Split-Path $asciiRoot) | Out-Null
    cmd /c mklink /J $asciiRoot $PSScriptRoot | Out-Null
}
Set-Location $asciiRoot

& $msys -l ./release-build.sh
if ($LASTEXITCODE -ne 0) { throw "Release build failed" }

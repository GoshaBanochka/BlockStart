# Local Windows dev build via MSYS2 (clang64).
# Usage:
#   .\dev-build.ps1            # incremental build + run
#   .\dev-build.ps1 -NoRun     # build only
#   .\dev-build.ps1 -Clean     # reconfigure from scratch, then build + run
param(
    [switch]$NoRun,
    [switch]$Clean
)

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

$buildArgs = @("./dev-build.sh")
if ($Clean) { $buildArgs += "--clean" }
& $msys -l @buildArgs
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

if (-not $NoRun) {
    # Qt DLLs live in clang64\bin; portable.txt keeps launcher data inside install-dev
    $env:PATH = "C:\msys64\clang64\bin;$env:PATH"
    if (-not (Test-Path "install-dev\portable.txt")) { New-Item "install-dev\portable.txt" | Out-Null }
    Start-Process "install-dev\freesmlauncher.exe"
}

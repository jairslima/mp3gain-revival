param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("windows", "linux")]
    [string]$Platform,

    [string]$Version = "dev"
)

$ErrorActionPreference = "Stop"

$repo = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $repo "dist"
$stage = Join-Path $dist ("mp3gain-" + $Platform + "-" + $Version)

if (Test-Path $stage) {
    Remove-Item -Recurse -Force $stage
}

New-Item -ItemType Directory -Path $stage | Out-Null

switch ($Platform) {
    "windows" {
        $exe = Join-Path $repo "build\\Release\\mp3gain.exe"
        $dll = Join-Path $repo "build\\Release\\mpg123.dll"

        if (!(Test-Path $exe)) {
            throw "Missing Windows executable: $exe"
        }
        if (!(Test-Path $dll)) {
            throw "Missing Windows runtime DLL: $dll"
        }

        Copy-Item $exe $stage
        Copy-Item $dll $stage
    }
    "linux" {
        $exe = Join-Path $repo "build\\mp3gain"
        if (!(Test-Path $exe)) {
            throw "Missing Linux executable: $exe"
        }

        Copy-Item $exe $stage
    }
}

Copy-Item (Join-Path $repo "LICENSE") $stage
if (Test-Path (Join-Path $repo "CHANGELOG.md")) {
    Copy-Item (Join-Path $repo "CHANGELOG.md") $stage
}

$archive = Join-Path $dist ("mp3gain-" + $Platform + "-" + $Version + ".zip")
if (Test-Path $archive) {
    Remove-Item -Force $archive
}

Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $archive

Write-Host ("Created package: " + $archive)

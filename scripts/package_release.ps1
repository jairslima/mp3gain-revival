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
        $guiDirCandidates = @(
            (Join-Path $repo "gui\\bin\\Release\\net8.0"),
            (Join-Path $repo "gui\\bin\\Debug\\net8.0")
        )
        $guiDir = $null

        if (!(Test-Path $exe)) {
            throw "Missing Windows executable: $exe"
        }
        if (!(Test-Path $dll)) {
            throw "Missing Windows runtime DLL: $dll"
        }

        foreach ($candidate in $guiDirCandidates) {
            if (Test-Path (Join-Path $candidate "MP3GainUI.exe")) {
                $guiDir = $candidate
                break
            }
        }

        if ($null -ne $guiDir) {
            Copy-Item (Join-Path $guiDir "*") $stage -Recurse
            Copy-Item $exe $stage -Force
            Copy-Item $dll $stage -Force
        }
        else {
            Copy-Item $exe $stage
            Copy-Item $dll $stage
        }
    }
    "linux" {
        $linuxCandidates = @(
            (Join-Path $repo "build\\mp3gain"),
            (Join-Path $repo "build-linux\\mp3gain"),
            (Join-Path $repo "build-wsl\\mp3gain")
        )
        $exe = $null

        foreach ($candidate in $linuxCandidates) {
            if (Test-Path $candidate) {
                $exe = $candidate
                break
            }
        }

        if ($null -eq $exe) {
            throw "Missing Linux executable. Expected one of: $($linuxCandidates -join ', ')"
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

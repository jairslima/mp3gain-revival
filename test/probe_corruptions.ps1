$ErrorActionPreference = "Stop"

$repo = Split-Path -Parent $PSScriptRoot
$fixture = Join-Path $repo "test\fixtures\test1.mp3"
$tmp = Join-Path $repo "test\_tmp_corrupt_probe"
$resultsPath = Join-Path $repo "test\probe_corruptions.results.json"
$linuxExe = "/mnt/c/Users/jairs/Claude/MP3Gain/build/mp3gain"

function Get-FrameSize([byte[]]$b, [int]$i) {
    if ($i + 4 -gt $b.Length) { return 0 }
    if ($b[$i] -ne 0xFF) { return 0 }
    if (($b[$i + 1] -band 0xE0) -ne 0xE0) { return 0 }

    $versionBits = ($b[$i + 1] -shr 3) -band 0x03
    $layerBits = ($b[$i + 1] -shr 1) -band 0x03
    if ($versionBits -eq 1 -or $layerBits -ne 1) { return 0 }

    $bitrateIdx = ($b[$i + 2] -shr 4) -band 0x0F
    $freqIdx = ($b[$i + 2] -shr 2) -band 0x03
    $padding = ($b[$i + 2] -shr 1) -band 0x01
    if ($bitrateIdx -eq 0 -or $bitrateIdx -eq 15 -or $freqIdx -eq 3) { return 0 }

    $bitratesMpeg1 = @(0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0)
    $bitratesMpeg2 = @(0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0)
    $freqsMpeg1 = @(44100, 48000, 32000, 0)
    $freqsMpeg2 = @(22050, 24000, 16000, 0)
    $freqsMpeg25 = @(11025, 12000, 8000, 0)

    switch ($versionBits) {
        3 { $mult = 144; $freq = $freqsMpeg1[$freqIdx]; $br = $bitratesMpeg1[$bitrateIdx] * 1000 }
        2 { $mult = 72;  $freq = $freqsMpeg2[$freqIdx]; $br = $bitratesMpeg2[$bitrateIdx] * 1000 }
        0 { $mult = 72;  $freq = $freqsMpeg25[$freqIdx]; $br = $bitratesMpeg2[$bitrateIdx] * 1000 }
        default { return 0 }
    }

    if ($freq -eq 0 -or $br -eq 0) { return 0 }
    return [int]([math]::Floor(($mult * $br) / $freq) + $padding)
}

function Get-FirstFrameOffset([byte[]]$bytes) {
    for ($i = 0; $i -lt 65536 -and $i -lt ($bytes.Length - 4); $i++) {
        $size = Get-FrameSize $bytes $i
        if ($size -gt 0) { return $i }
    }
    return -1
}

function Invoke-Candidate([string]$name, [scriptblock]$mutator) {
    $path = Join-Path $tmp $name
    Copy-Item $fixture $path
    & $mutator $path

    $wslPath = "/mnt/c/Users/jairs/Claude/MP3Gain/test/_tmp_corrupt_probe/$name"
    $out = & wsl.exe $linuxExe $wslPath 2>&1
    $code = $LASTEXITCODE
    $ffmpegOut = & wsl.exe ffmpeg -v error -i $wslPath -f null - 2>&1
    $ffmpegText = ($ffmpegOut | Select-Object -First 12) -join "`n"

    [pscustomobject]@{
        name = $name
        exit_code = $code
        output = (($out | Select-Object -First 12) -join "`n")
        ffmpeg_decode_error = [bool]($ffmpegText.Trim())
        ffmpeg_output = $ffmpegText
    }
}

if (Test-Path $tmp) {
    Remove-Item -Recurse -Force $tmp
}
New-Item -ItemType Directory -Path $tmp | Out-Null

$results = @()

$results += Invoke-Candidate "corrupt-truncated-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    [System.IO.File]::WriteAllBytes($path, $bytes[0..99])
}

$results += Invoke-Candidate "payload-corrupt-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 128) -and $hits -lt 400) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }
        $payloadStart = $pos + 24
        $payloadLen = [Math]::Min(96, $size - 32)
        if ($payloadLen -gt 0) {
            for ($j = 0; $j -lt $payloadLen; $j++) {
                $bytes[$payloadStart + $j] = 0xFF
            }
        }
        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "scattered-zero-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    foreach ($off in @(262144, 524288, 786432, 1048576, 1310720)) {
        if ($off + 256 -lt $bytes.Length) {
            [Array]::Clear($bytes, $off, 256)
        }
    }
    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "header-window-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 128) -and $hits -lt 120) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 4) {
            $windowStart = $pos + 2
            $windowLen = [Math]::Min(10, $size - 2)
            for ($j = 0; $j -lt $windowLen; $j++) {
                $bytes[$windowStart + $j] = 0x00
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "sync-byte-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 8) -and $hits -lt 80) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 8) {
            $bytes[$pos] = 0x7F
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "crc-sideinfo-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 32) -and $hits -lt 120) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 6) {
            $windowStart = $pos + 4
            $windowLen = [Math]::Min(8, $size - 4)
            for ($j = 0; $j -lt $windowLen; $j++) {
                $bytes[$windowStart + $j] = 0xFF
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "transition-gap-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 64) -and $hits -lt 100) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 10) {
            $gapStart = $pos + $size - 6
            $gapLen = [Math]::Min(12, $bytes.Length - $gapStart)
            for ($j = 0; $j -lt $gapLen; $j++) {
                $bytes[$gapStart + $j] = 0x00
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "main-data-begin-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 32) -and $hits -lt 120) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 5) {
            $sideInfoStart = $pos + 4
            $sideInfoLen = [Math]::Min(3, $bytes.Length - $sideInfoStart)
            for ($j = 0; $j -lt $sideInfoLen; $j++) {
                $bytes[$sideInfoStart + $j] = 0x00
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "reservoir-burst-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 96) -and $hits -lt 120) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 8) {
            $burstStart = $pos + 12
            $burstLen = [Math]::Min(24, $size - 16)
            for ($j = 0; $j -lt $burstLen; $j++) {
                $bytes[$burstStart + $j] = if (($j % 2) -eq 0) { 0xFF } else { 0x00 }
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "cross-frame-stitch-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 96) -and $hits -lt 100) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 10) {
            $bridgeStart = $pos + $size - 8
            $bridgeLen = [Math]::Min(20, $bytes.Length - $bridgeStart)
            for ($j = 0; $j -lt $bridgeLen; $j++) {
                $bytes[$bridgeStart + $j] = 0xA5
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "sideinfo-shear-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 64) -and $hits -lt 120) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 7) {
            $windowStart = $pos + 5
            $windowLen = [Math]::Min(12, $size - 8)
            for ($j = 0; $j -lt $windowLen; $j++) {
                $bytes[$windowStart + $j] = 0x55
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "late-payload-scramble-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 128) -and $hits -lt 160) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -gt 12) {
            $payloadStart = $pos + [Math]::Min(28, $size - 24)
            $payloadLen = [Math]::Min(20, $bytes.Length - $payloadStart)
            for ($j = 0; $j -lt $payloadLen; $j++) {
                $bytes[$payloadStart + $j] = $bytes[$payloadStart + $j] -bxor 0x5A
            }
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "late-header-poison-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 64) -and $hits -lt 140) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -ge 16 -and $hits -lt 28) {
            $bytes[$pos] = 0x00
            $bytes[$pos + 1] = 0x00
            $bytes[$pos + 2] = 0x00
            $bytes[$pos + 3] = 0x00
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results += Invoke-Candidate "frame-cluster-poison-probe.mp3" {
    param($path)
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $start = Get-FirstFrameOffset $bytes
    if ($start -lt 0) { return }

    $pos = $start
    $hits = 0
    while ($pos -lt ($bytes.Length - 128) -and $hits -lt 160) {
        $size = Get-FrameSize $bytes $pos
        if ($size -le 0) { break }

        if ($hits -eq 20) {
            $clusterPos = $pos
            for ($k = 0; $k -lt 5; $k++) {
                $clusterSize = Get-FrameSize $bytes $clusterPos
                if ($clusterSize -le 0) { break }
                $windowStart = $clusterPos + 4
                $windowLen = [Math]::Min($clusterSize - 4, 48)
                for ($j = 0; $j -lt $windowLen; $j++) {
                    $bytes[$windowStart + $j] = 0x00
                }
                $clusterPos += $clusterSize
            }
            break
        }

        $pos += $size
        $hits++
    }

    [System.IO.File]::WriteAllBytes($path, $bytes)
}

$results | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 $resultsPath
$results | Format-List

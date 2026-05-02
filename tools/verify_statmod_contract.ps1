param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$constantsPath = Join-Path $RepoRoot "StatModification_Extension/src/Constants.h"
$fcsDefPath = Join-Path $RepoRoot "StatModification_Extension/StatModification_Extension/fcs.def"

$constants = Get-Content -Raw -Path $constantsPath
$fcsDef = Get-Content -Raw -Path $fcsDefPath

$expectedActionKeys = @(
    "train squad skill levels",
    "untrain squad skill levels",
    "train squad skill levels until",
    "untrain squad skill levels until",
    "train other squad skill levels",
    "untrain other squad skill levels",
    "train other squad skill levels until",
    "untrain other squad skill levels until"
)

$failed = $false

foreach ($key in $expectedActionKeys) {
    if ($constants -notmatch [regex]::Escape("`"$key`"")) {
        Write-Host "Missing StatModification action key in Constants.h: $key" -ForegroundColor Red
        $failed = $true
    }

    if ($fcsDef -notmatch [regex]::Escape($key + ":")) {
        Write-Host "Missing StatModification action key in fcs.def: $key" -ForegroundColor Red
        $failed = $true
    }
}

if ($failed) {
    exit 1
}

Write-Host "StatModification contract verification passed."

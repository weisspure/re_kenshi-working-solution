param(
	[string]$Configuration = "Release",
	[string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$compileDbScript = Join-Path $PSScriptRoot "generate_compile_commands.ps1"

if (!(Test-Path $compileDbScript)) {
	throw "Missing compile database generator: $compileDbScript"
}

Write-Host "[refresh_serena] Regenerating compile_commands.json..."
& $compileDbScript -Configuration $Configuration -Platform $Platform

if (!$?) {
	throw "generate_compile_commands.ps1 failed."
}

$serenaCommand = Get-Command serena -ErrorAction SilentlyContinue
if ($null -eq $serenaCommand) {
	throw "The 'serena' command was not found on PATH. Compile commands were regenerated, but Serena was not re-indexed."
}

Write-Host "[refresh_serena] Re-indexing Serena project..."
Push-Location $repoRoot
try {
	& $serenaCommand.Source project index
	if (!$?) {
		throw "serena project index failed."
	}
}
finally {
	Pop-Location
}

Write-Host "[refresh_serena] Done."
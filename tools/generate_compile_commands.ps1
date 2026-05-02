param(
	[string]$Configuration = "Release",
	[string]$Platform = "x64",
	[string]$Output = "compile_commands.json"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$msbuildNs = New-Object System.Xml.XmlNamespaceManager((New-Object System.Xml.NameTable))
$msbuildNs.AddNamespace("msb", "http://schemas.microsoft.com/developer/msbuild/2003")
$inferredMacros = @{}

function Add-InferredMacro([string]$name, [string]$path) {
	if ([string]::IsNullOrWhiteSpace($path) -or !(Test-Path $path)) {
		return
	}

	$inferredMacros[$name] = [System.IO.Path]::GetFullPath($path)
}

function Initialize-InferredMacros {
	$parentDir = Split-Path -Parent $repoRoot
	$repoName = Split-Path -Leaf $repoRoot
	$depsCandidates = @(
		(Join-Path $parentDir "$($repoName)_deps"),
		(Join-Path $repoRoot "deps")
	)

	$depsDir = $depsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
	if (!$depsDir) {
		return
	}

	Add-InferredMacro "KENSHILIB_DEPS_DIR" $depsDir

	$kenshiLibRoot = Get-ChildItem -Path $depsDir -Directory -Recurse -ErrorAction SilentlyContinue |
		Where-Object {
			(Test-Path (Join-Path $_.FullName "Include")) -and
			(Test-Path (Join-Path $_.FullName "Libraries"))
		} |
		Sort-Object {
			if ($_.Parent.FullName -eq [System.IO.Path]::GetFullPath($depsDir)) { 0 } else { 1 }
		}, FullName |
		Select-Object -First 1
	if ($kenshiLibRoot) {
		Add-InferredMacro "KENSHILIB_DIR" $kenshiLibRoot.FullName
	}

	$boostRoot = Get-ChildItem -Path $depsDir -Directory -Recurse -Filter "boost_*" -ErrorAction SilentlyContinue |
		Where-Object {
			(Test-Path (Join-Path $_.FullName "boost")) -and
			(Test-Path (Join-Path $_.FullName "stage\lib"))
		} |
		Sort-Object {
			if ($_.Parent.FullName -eq [System.IO.Path]::GetFullPath($depsDir)) { 0 } else { 1 }
		}, FullName |
		Select-Object -First 1
	if ($boostRoot) {
		Add-InferredMacro "BOOST_INCLUDE_PATH" $boostRoot.FullName
		Add-InferredMacro "BOOST_ROOT" $boostRoot.FullName
	}
}

Initialize-InferredMacros
Add-InferredMacro "SolutionDir" $repoRoot

function Split-MsbuildList([string]$value) {
	if ([string]::IsNullOrWhiteSpace($value)) {
		return @()
	}

	return $value -split ";" |
		ForEach-Object { $_.Trim() } |
		Where-Object {
			$_ -and
			$_ -notmatch "^%\(" -and
			$_ -notmatch "^\$\((IncludePath|LibraryPath)\)$" -and
			$_ -notmatch "^\$\((PreprocessorDefinitions|AdditionalIncludeDirectories)\)$"
		}
}

function Expand-MsbuildMacros([string]$value, [string]$projectDir) {
	if ([string]::IsNullOrWhiteSpace($value)) {
		return $value
	}

	$expanded = $value
	$expanded = $expanded -replace "\$\(ProjectDir\)", [Regex]::Escape($projectDir).Replace("\\", "\")
	$expanded = $expanded -replace "\$\(Configuration\)", $Configuration
	$expanded = $expanded -replace "\$\(Platform\)", $Platform

	$expanded = [Regex]::Replace($expanded, "\$\(([^)]+)\)", {
		param($match)
		$name = $match.Groups[1].Value
		$envValue = [Environment]::GetEnvironmentVariable($name)
		if (![string]::IsNullOrWhiteSpace($envValue)) {
			return $envValue
		}
		if ($inferredMacros.ContainsKey($name)) {
			return $inferredMacros[$name]
		}
		return $match.Value
	})

	return $expanded
}

function Normalize-PathForCompileDb([string]$path, [string]$baseDir) {
	if ([string]::IsNullOrWhiteSpace($path)) {
		return $null
	}

	$expanded = [Environment]::ExpandEnvironmentVariables($path.Trim())
	if ([string]::IsNullOrWhiteSpace($expanded) -or $expanded.Contains('$(')) {
		return $null
	}

	if (![System.IO.Path]::IsPathRooted($expanded)) {
		$expanded = Join-Path $baseDir $expanded
	}

	return [System.IO.Path]::GetFullPath($expanded)
}

$projects = Get-ChildItem -Path $repoRoot -Recurse -Filter "*.vcxproj" |
	Where-Object { $_.FullName -notmatch "\\(x64|Debug|Release|\.vs)\\" } |
	Sort-Object FullName

$entries = New-Object System.Collections.Generic.List[object]

foreach ($project in $projects) {
	[xml]$xml = Get-Content -Path $project.FullName
	$projectDir = Split-Path -Parent $project.FullName
	$configNeedle = "$Configuration|$Platform"

	$includeValues = New-Object System.Collections.Generic.List[string]
	$defineValues = New-Object System.Collections.Generic.List[string]

	$includeValues.Add($projectDir)
	$srcDir = Join-Path $projectDir "src"
	if (Test-Path $srcDir) {
		$includeValues.Add($srcDir)
	}

	$propertyGroups = $xml.SelectNodes("//msb:PropertyGroup", $msbuildNs) |
		Where-Object {
			$condition = $_.GetAttribute("Condition")
			!$condition -or $condition.Contains($configNeedle)
		}
	foreach ($group in $propertyGroups) {
		if ($group.IncludePath) {
			foreach ($item in Split-MsbuildList (Expand-MsbuildMacros ([string]$group.IncludePath) $projectDir)) {
				$includeValues.Add($item)
			}
		}
	}

	$itemDefinitions = $xml.SelectNodes("//msb:ItemDefinitionGroup", $msbuildNs) |
		Where-Object {
			$condition = $_.GetAttribute("Condition")
			!$condition -or $condition.Contains($configNeedle)
		}
	foreach ($definition in $itemDefinitions) {
		$compile = $definition.SelectSingleNode("msb:ClCompile", $msbuildNs)
		if (!$compile) {
			continue
		}
		if ($compile.AdditionalIncludeDirectories) {
			foreach ($item in Split-MsbuildList (Expand-MsbuildMacros ([string]$compile.AdditionalIncludeDirectories) $projectDir)) {
				$includeValues.Add($item)
			}
		}
		if ($compile.PreprocessorDefinitions) {
			foreach ($item in Split-MsbuildList (Expand-MsbuildMacros ([string]$compile.PreprocessorDefinitions) $projectDir)) {
				$defineValues.Add($item)
			}
		}
	}

	foreach ($fallbackDefine in @("WIN32", "_WINDOWS", "_USRDLL", "UNICODE", "_UNICODE")) {
		$defineValues.Add($fallbackDefine)
	}

	$includeArgs = $includeValues |
		ForEach-Object { Normalize-PathForCompileDb $_ $projectDir } |
		Where-Object { $_ } |
		Sort-Object -Unique |
		ForEach-Object { @("/I", $_) }

	$defineArgs = $defineValues |
		Where-Object { $_ } |
		Sort-Object -Unique |
		ForEach-Object { "/D$_" }

	$sources = $xml.SelectNodes("//msb:ClCompile[@Include]", $msbuildNs)
	foreach ($source in $sources) {
		$sourcePath = Normalize-PathForCompileDb $source.Include $projectDir
		if (!$sourcePath -or !(Test-Path $sourcePath)) {
			continue
		}

		$args = @(
			"clang-cl.exe",
			"/TP",
			"/nologo",
			"/EHsc",
			"/GR",
			"/W3",
			"/std:c++14",
			"-fms-compatibility-version=16.00"
		) + $defineArgs + $includeArgs + @("/c", $sourcePath)

		$entries.Add([ordered]@{
			directory = $projectDir
			file = $sourcePath
			arguments = $args
		})
	}
}

$outputPath = Normalize-PathForCompileDb $Output $repoRoot
$json = $entries | ConvertTo-Json -Depth 8
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($outputPath, $json, $utf8NoBom)

Write-Host "Wrote $($entries.Count) entries to $outputPath"

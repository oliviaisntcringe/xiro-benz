[CmdletBinding()]
param(
	[Parameter(Mandatory = $true, Position = 0)]
	[string]$Reference,
	[string]$Source = "velocity-cs2/project/protection/patterns.cpp",
	[switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$sourceRegex = [regex]::new(
	'&\s*(?<Name>[A-Za-z_]\w*)\s*=\s*ADDRESS_IMPL\s*\(\s*::protection::addresses::hash\("(?<Pattern>[^"]+)"\)',
	[Text.RegularExpressions.RegexOptions]::Singleline
)
$sectionRegex = [regex]::new('^\s*\[\s*(?<Module>[^\]]+?)\s*\]')
$entryRegex = [regex]::new('^\s*(?<Name>[A-Za-z_]\w*)\s+(?<Pattern>.+?)\s*$')
$tokenRegex = [regex]::new('\?\?|\?|[0-9A-F]{2}', [Text.RegularExpressions.RegexOptions]::IgnoreCase)
$offsetRegex = [regex]::new('[+-][0-9A-F]+~?$', [Text.RegularExpressions.RegexOptions]::IgnoreCase)

function Convert-Pattern {
	param(
		[Parameter(Mandatory = $true)] [string]$Raw,
		[switch]$Compact
	)

	$value = $Raw.ToUpperInvariant().Trim()
	if ($Compact) {
		$value = $offsetRegex.Replace($value, "")
		$value = $value.Replace(">", "").Replace("*", "").Replace("~", "")
		$value = [regex]::Replace($value, '\s+', "")
	}

	$tokens = [System.Collections.Generic.List[string]]::new()
	$cursor = 0
	foreach ($match in $tokenRegex.Matches($value)) {
		$between = $value.Substring($cursor, $match.Index - $cursor)
		if ($between.Trim().Length -gt 0) {
			throw "unrecognized pattern text near '$between'"
		}
		$tokens.Add($(if ($match.Value.Contains("?")) { "?" } else { $match.Value }))
		$cursor = $match.Index + $match.Length
	}
	if ($cursor -lt $value.Length -and $value.Substring($cursor).Trim().Length -gt 0) {
		throw "unrecognized pattern text near '$($value.Substring($cursor))'"
	}
	if ($tokens.Count -eq 0) {
		throw "empty pattern"
	}
	return ($tokens -join " ")
}

function New-PatternRecord {
	param(
		[string]$Name,
		[string]$Module,
		[string]$Raw,
		[string]$Normalized,
		[int]$Line
	)
	return [pscustomobject]@{
		Name = $Name
		Module = $Module.ToLowerInvariant()
		Raw = $Raw
		Normalized = $Normalized
		Line = $Line
	}
}

$sourceText = Get-Content -LiteralPath $Source -Raw
$sourcePatterns = [System.Collections.Generic.List[object]]::new()
foreach ($match in $sourceRegex.Matches($sourceText)) {
	$full = $match.Groups["Pattern"].Value
	$parts = $full.Split(":", 2)
	if ($parts.Count -ne 2) {
		throw "${Source}: invalid module prefix near '$full'"
	}
	try {
		$normalized = Convert-Pattern -Raw $parts[1] -Compact
	} catch {
		$line = ($sourceText.Substring(0, $match.Index) -split "`n").Count
		throw "${Source}:${line}: $($_.Exception.Message)"
	}
	$line = ($sourceText.Substring(0, $match.Index) -split "`n").Count
	$sourcePatterns.Add((New-PatternRecord $match.Groups["Name"].Value $parts[0] $parts[1] $normalized $line))
}

$referencePatterns = [System.Collections.Generic.List[object]]::new()
$referenceParseErrors = [System.Collections.Generic.List[object]]::new()
$module = $null
$lineNumber = 0
foreach ($line in (Get-Content -LiteralPath $Reference)) {
	$lineNumber++
	$section = $sectionRegex.Match($line)
	if ($section.Success) {
		$module = $section.Groups["Module"].Value.Trim().ToLowerInvariant()
		continue
	}
	if ([string]::IsNullOrWhiteSpace($module)) {
		continue
	}
	$entry = $entryRegex.Match($line)
	if (!$entry.Success) {
		continue
	}
	try {
		$normalized = Convert-Pattern -Raw $entry.Groups["Pattern"].Value
	} catch {
		$referenceParseErrors.Add([pscustomobject]@{
			Name = $entry.Groups["Name"].Value
			Module = $module
			Line = $lineNumber
			Reason = $_.Exception.Message
		})
		continue
	}
	$referencePatterns.Add((New-PatternRecord $entry.Groups["Name"].Value $module $entry.Groups["Pattern"].Value $normalized $lineNumber))
}

$sourceByKey = @{}
$referenceByKey = @{}
foreach ($pattern in $sourcePatterns) {
	$key = "$($pattern.Module)|$($pattern.Normalized)"
	if ($sourceByKey.ContainsKey($key)) { $sourceByKey[$key] += $pattern } else { $sourceByKey[$key] = @($pattern) }
}
foreach ($pattern in $referencePatterns) {
	$key = "$($pattern.Module)|$($pattern.Normalized)"
	if ($referenceByKey.ContainsKey($key)) { $referenceByKey[$key] += $pattern } else { $referenceByKey[$key] = @($pattern) }
}

$exactMatches = @($sourcePatterns | Where-Object { $referenceByKey.ContainsKey("$($_.Module)|$($_.Normalized)") })
$missingSource = @($sourcePatterns | Where-Object { !$referenceByKey.ContainsKey("$($_.Module)|$($_.Normalized)") })
$sourceModules = @{}
$referenceModules = @{}
foreach ($pattern in $sourcePatterns) {
	if ($sourceModules.ContainsKey($pattern.Module)) { $sourceModules[$pattern.Module]++ } else { $sourceModules[$pattern.Module] = 1 }
}
foreach ($pattern in $referencePatterns) {
	if ($referenceModules.ContainsKey($pattern.Module)) { $referenceModules[$pattern.Module]++ } else { $referenceModules[$pattern.Module] = 1 }
}

Write-Output ("source patterns:    {0}" -f $sourcePatterns.Count)
Write-Output ("reference patterns: {0}" -f $referencePatterns.Count)
Write-Output ("exact byte matches: {0}" -f $exactMatches.Count)
Write-Output ("source unmatched:   {0}" -f $missingSource.Count)
Write-Output ""

if ($exactMatches.Count -gt 0) {
	Write-Output "exact matches (project alias -> reference name):"
	foreach ($pattern in $exactMatches) {
		$key = "$($pattern.Module)|$($pattern.Normalized)"
		$referenceNames = @($referenceByKey[$key] | ForEach-Object { $_.Name } | Sort-Object -Unique) -join ", "
		Write-Output ("  {0,-40} -> {1,-28} {2}" -f $pattern.Name, $referenceNames, $pattern.Module)
	}
	Write-Output ""
}

Write-Output "module coverage:"
foreach ($name in @($sourceModules.Keys + $referenceModules.Keys | Sort-Object -Unique)) {
	$sourceCount = if ($sourceModules.ContainsKey($name)) { $sourceModules[$name] } else { 0 }
	$referenceCount = if ($referenceModules.ContainsKey($name)) { $referenceModules[$name] } else { 0 }
	Write-Output ("  {0,-24} source={1,3} reference={2,3}" -f $name, $sourceCount, $referenceCount)
}

$duplicateNames = @($referencePatterns | Group-Object Name | Where-Object Count -gt 1)
if ($duplicateNames.Count -gt 0) {
	Write-Output ""
	Write-Output "duplicate reference names:"
	foreach ($group in $duplicateNames | Sort-Object Name) {
		Write-Output ("  {0} ({1} entries)" -f $group.Name, $group.Count)
	}
}

if ($missingSource.Count -gt 0) {
	Write-Output ""
	Write-Output "source signatures without an exact reference match:"
	foreach ($pattern in $missingSource) {
		Write-Output ("  {0,-40} {1,-22} patterns.cpp:{2}" -f $pattern.Name, $pattern.Module, $pattern.Line)
	}
}

$referenceOnlyCount = 0
foreach ($key in $referenceByKey.Keys) {
	if (!$sourceByKey.ContainsKey($key)) { $referenceOnlyCount += @($referenceByKey[$key]).Count }
}
Write-Output ""
Write-Output ("reference signatures not represented by source: {0}" -f $referenceOnlyCount)

if ($referenceParseErrors.Count -gt 0) {
	Write-Output ""
	Write-Output "reference entries not parsed:"
	foreach ($errorEntry in $referenceParseErrors) {
		Write-Output ("  {0} {1} line {2}: {3}" -f $errorEntry.Module, $errorEntry.Name, $errorEntry.Line, $errorEntry.Reason)
	}
}

if ($Strict -and $missingSource.Count -gt 0) {
	exit 1
}
exit 0

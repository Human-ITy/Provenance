$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../../..'))
$manifest = Get-Content (Join-Path $PSScriptRoot 'resources.json') -Raw | ConvertFrom-Json
$compiler = Join-Path $project 'Build/x64_Debug/EsotericaResourceCompiler.exe'
$results = @()
foreach ($resource in $manifest.resources) {
    $lines = @(& $compiler -compile $resource -force 2>&1)
    $code = $LASTEXITCODE
    $lines | Out-File (Join-Path $PSScriptRoot 'resource_compile.log') -Append -Encoding utf8
    $ok = ($code -eq 1) -and ($lines -match 'Compiled successfully:') -and -not ($lines -match '\[(Error|Warning)\]')
    $results += [pscustomobject]@{resource=$resource;exitCode=$code;pass=[bool]$ok}
    if (-not $ok) { $lines | Write-Output; throw "Hammer resource failed: $resource (exit $code)" }
}
$results | ConvertTo-Json | Out-File (Join-Path $PSScriptRoot 'resource_results.json') -Encoding utf8
Write-Output ("PASS: {0} native resources compiled without warnings" -f $results.Count)

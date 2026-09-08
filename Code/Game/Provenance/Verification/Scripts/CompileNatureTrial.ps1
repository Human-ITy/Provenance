$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '../../../../..')).Path
$manifest = Get-Content (Join-Path $PSScriptRoot '../Fixtures/NatureTrialResources.json') -Raw | ConvertFrom-Json
$compiler = Join-Path $project 'Build/x64_Debug/EsotericaResourceCompiler.exe'
$log = Join-Path $project 'Build/Verification/MovementBaseline/nature-resources.log'
$results = @()
foreach ($resource in $manifest.resources) {
    $lines = @(& $compiler -compile $resource -force 2>&1)
    $code = $LASTEXITCODE
    $lines | Out-File -LiteralPath $log -Append -Encoding utf8
    # Resource compiler success is 1 (not the ordinary shell convention).
    $ok = ($code -eq 1) -and ($lines -match 'Compiled successfully:') -and -not ($lines -match '\[(Error|Warning)\]')
    $results += [pscustomobject]@{resource=$resource;exitCode=$code;pass=[bool]$ok}
    if (-not $ok) { $lines | Write-Output; throw "Trial resource did not compile cleanly: $resource (exit $code)" }
}
$results | ConvertTo-Json | Out-File (Join-Path $project 'Build/Verification/MovementBaseline/nature-resource-results.json') -Encoding utf8
Write-Output ("PASS: {0} native resources compiled without warnings" -f $results.Count)

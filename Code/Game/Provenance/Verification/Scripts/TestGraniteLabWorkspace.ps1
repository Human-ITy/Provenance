$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
[xml]$solution = Get-Content -LiteralPath (Join-Path $repo 'GraniteLab.slnx') -Raw
$projects = @($solution.Solution.Folder.Project | Where-Object { $_ })
if ($projects.Count -ne 10) { throw 'Expected one launcher and nine production projects.' }
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($project in $projects) {
    $path = [IO.Path]::GetFullPath((Join-Path $repo $project.Path))
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing project $path" }
    if (-not $seen.Add($path)) { throw "Duplicate project $path" }
    if ($project.Path -notlike '*/GraniteLab/GraniteLab.vcxproj' -and $project.Build.Project -ne 'false') {
        throw 'Shared projects must not run their broad pre-build event from solution Build.'
    }
}
foreach ($path in $seen) {
    [xml]$project = Get-Content -LiteralPath $path -Raw
    foreach ($reference in $project.SelectNodes("//*[local-name()='ProjectReference']")) {
        $dependency = [IO.Path]::GetFullPath((Join-Path (Split-Path $path) $reference.Include))
        if (-not $seen.Contains($dependency)) { throw "Missing dependency $dependency" }
    }
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found.' }
& $msbuild (Join-Path $repo 'GraniteLab.slnx') /t:ValidateSolutionConfiguration /p:Configuration=Debug /p:Platform=x64 /nologo /verbosity:minimal
if ($LASTEXITCODE -ne 0) { throw 'Solution configuration validation failed.' }
& (Join-Path $repo 'RunGraniteLab.cmd') check
if ($LASTEXITCODE -ne 0) { throw 'Launcher path validation failed.' }
& (Join-Path $PSScriptRoot 'BuildGraniteLab.ps1') -CheckOnly
Write-Output 'PASS: solution configuration, ten unique projects, closed dependency graph, shared-build exclusion and launch/build preflight.'
Write-Output 'No executable was launched, compiled, stopped, or deleted. This is not an F5/live-play test.'

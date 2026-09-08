param([switch]$CheckOnly)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer vswhere.exe not found.' }
$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found.' }
$projects = @(
    'Code/Applications/ResourceCompiler/Esoterica.Applications.ResourceCompiler.vcxproj',
    'Code/Applications/ResourceServer/Esoterica.Applications.ResourceServer.vcxproj',
    'Code/Applications/Editor/Esoterica.Applications.Editor.vcxproj',
    'Code/Applications/ProvenanceVerifier/Esoterica.Applications.ProvenanceVerifier.vcxproj'
)
foreach ($project in $projects) {
    if (-not (Test-Path -LiteralPath (Join-Path $repo $project))) { throw "Missing project: $project" }
}
# Match only executables under this checkout's Build directory. No process is killed.
$buildPrefix = (Join-Path $repo 'Build') + '\'
$running = @(Get-Process -Name 'Esoterica*' -ErrorAction SilentlyContinue | Where-Object {
    # An unreadable executable path is conservatively treated as a possible lock.
    -not $_.Path -or $_.Path.StartsWith($buildPrefix, [StringComparison]::OrdinalIgnoreCase)
})
if ($CheckOnly) {
    Write-Output "MSBuild: $msbuild"
    Write-Output 'Four entry projects found; dependencies built by project references. Debug x64 only.'
    Write-Output 'PreBuildEventUseInBuild=false; BuildProjectReferences=true; no clean and no process termination.'
    Write-Output "Currently running checkout executables: $($running.Count)"
    return
}
if ($running.Count -gt 0) {
    $running | Select-Object ProcessName, Id, Path | Format-Table | Out-Host
    throw 'Close this checkout editor/resource server/compiler helpers before building. Nothing was stopped.'
}
foreach ($project in $projects) {
    & $msbuild (Join-Path $repo $project) /t:Build /m:2 /nologo /verbosity:minimal `
        /p:Configuration=Debug /p:Platform=x64 "/p:SolutionDir=$repo\" `
        /p:BuildProjectReferences=true /p:PreBuildEventUseInBuild=false
    if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE): $project" }
}
Write-Output 'Granite Lab shared Debug build complete. Verification executables were built, not run.'

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string] $Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$clientRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$source = Join-Path $clientRoot 'Code\Applications\ProvenanceClient\Main.cpp'
$projectionCertSource = Join-Path $clientRoot 'Tools\Integration\Ei3DetailedProjectionClientCert.cpp'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Provenance client source is missing: $source"
}
if (-not (Test-Path -LiteralPath $projectionCertSource -PathType Leaf)) {
    throw "Cut 0 projection certificate source is missing: $projectionCertSource"
}
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
    throw "Visual Studio discovery tool is missing: $vswhere"
}

$visualStudio = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($visualStudio)) {
    throw 'A Visual Studio installation with the x64 C++ toolchain is required.'
}

$toolsetRoot = Join-Path $visualStudio 'VC\Tools\MSVC'
$toolset = Get-ChildItem -LiteralPath $toolsetRoot -Directory |
    Sort-Object -Property Name -Descending |
    Select-Object -First 1
if ($null -eq $toolset) {
    throw "No C++ toolset was found under: $toolsetRoot"
}

$compiler = Join-Path $toolset.FullName 'bin\Hostx64\x64\cl.exe'
$linker = Join-Path $toolset.FullName 'bin\Hostx64\x64\link.exe'
$kitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
$sdk = Get-ChildItem -LiteralPath (Join-Path $kitsRoot 'Include') -Directory |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt') } |
    Sort-Object -Property Name -Descending |
    Select-Object -First 1
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf) -or
        -not (Test-Path -LiteralPath $linker -PathType Leaf) -or
        $null -eq $sdk) {
    throw 'The x64 compiler, linker, or Windows SDK could not be located.'
}

$intermediate = Join-Path $clientRoot "Build\_Direct\x64_$Configuration"
$destination = Join-Path $clientRoot "Build\x64_$Configuration"
New-Item -ItemType Directory -Force -Path $intermediate, $destination | Out-Null

$object = Join-Path $intermediate 'ProvenanceClient.obj'
$projectionCertObject = Join-Path $intermediate 'Ei3DetailedProjectionClientCert.obj'
$candidate = Join-Path $intermediate 'ProvenanceClient.exe'
$projectionCertCandidate = Join-Path $intermediate 'Ei3DetailedProjectionClientCert.exe'
$candidatePdb = Join-Path $intermediate 'ProvenanceClient.pdb'
$sdkVersion = $sdk.Name

$compileArguments = @(
    '/nologo', '/c', '/bigobj', '/W3', '/EHsc', '/std:c++20', '/utf-8',
    '/DWIN32', '/D_WINDOWS',
    "/I$clientRoot\Code",
    "/I$clientRoot\Code\Base\ThirdParty\stb",
    "/I$($toolset.FullName)\include",
    "/I$kitsRoot\Include\$sdkVersion\ucrt",
    "/I$kitsRoot\Include\$sdkVersion\um",
    "/I$kitsRoot\Include\$sdkVersion\shared",
    "/I$kitsRoot\Include\$sdkVersion\winrt",
    "/Fo$object",
    $source
)
if ($Configuration -eq 'Release') {
    $compileArguments += @('/O2', '/MD', '/DNDEBUG')
} else {
    $compileArguments += @('/Od', '/Zi', '/MDd', '/D_DEBUG')
}

& $compiler @compileArguments
if ($LASTEXITCODE -ne 0) {
    throw "Direct Provenance client compilation failed with exit code $LASTEXITCODE."
}

$projectionCertCompileArguments = @(
    '/nologo', '/c', '/W3', '/EHsc', '/std:c++20', '/utf-8',
    "/I$clientRoot\Code",
    "/I$($toolset.FullName)\include",
    "/I$kitsRoot\Include\$sdkVersion\ucrt",
    "/I$kitsRoot\Include\$sdkVersion\um",
    "/I$kitsRoot\Include\$sdkVersion\shared",
    "/Fo$projectionCertObject",
    $projectionCertSource
)
if ($Configuration -eq 'Release') {
    $projectionCertCompileArguments += @('/O2', '/MD', '/DNDEBUG')
} else {
    $projectionCertCompileArguments += @('/Od', '/Zi', '/MDd', '/D_DEBUG')
}
& $compiler @projectionCertCompileArguments
if ($LASTEXITCODE -ne 0) {
    throw "Cut 0 projection certificate compilation failed with exit code $LASTEXITCODE."
}

$linkArguments = @(
    '/nologo', '/SUBSYSTEM:WINDOWS', '/MACHINE:X64',
    "/OUT:$candidate", "/PDB:$candidatePdb",
    "/LIBPATH:$($toolset.FullName)\lib\x64",
    "/LIBPATH:$kitsRoot\Lib\$sdkVersion\ucrt\x64",
    "/LIBPATH:$kitsRoot\Lib\$sdkVersion\um\x64",
    $object
)
if ($Configuration -eq 'Release') {
    $linkArguments += @('/OPT:REF', '/OPT:ICF')
} else {
    $linkArguments += '/DEBUG'
}

& $linker @linkArguments
if ($LASTEXITCODE -ne 0) {
    throw "Direct Provenance client link failed with exit code $LASTEXITCODE."
}

$projectionCertLinkArguments = @(
    '/nologo', '/SUBSYSTEM:CONSOLE', '/MACHINE:X64',
    "/OUT:$projectionCertCandidate",
    "/LIBPATH:$($toolset.FullName)\lib\x64",
    "/LIBPATH:$kitsRoot\Lib\$sdkVersion\ucrt\x64",
    "/LIBPATH:$kitsRoot\Lib\$sdkVersion\um\x64",
    $projectionCertObject
)
if ($Configuration -eq 'Release') {
    $projectionCertLinkArguments += @('/OPT:REF', '/OPT:ICF')
} else {
    $projectionCertLinkArguments += '/DEBUG'
}
& $linker @projectionCertLinkArguments
if ($LASTEXITCODE -ne 0) {
    throw "Cut 0 projection certificate link failed with exit code $LASTEXITCODE."
}

$executable = Join-Path $destination 'ProvenanceClient.exe'
$projectionCertExecutable = Join-Path $destination 'Ei3DetailedProjectionClientCert.exe'
Copy-Item -LiteralPath $candidate -Destination $executable -Force
Copy-Item -LiteralPath $projectionCertCandidate -Destination $projectionCertExecutable -Force
if (Test-Path -LiteralPath $candidatePdb -PathType Leaf) {
    Copy-Item -LiteralPath $candidatePdb -Destination (Join-Path $destination 'ProvenanceClient.pdb') -Force
}

$hash = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
Write-Host "BUILT: $executable" -ForegroundColor Green
Write-Host "SHA-256: $hash"
Write-Host "CUT 0 CERT: $projectionCertExecutable" -ForegroundColor Green

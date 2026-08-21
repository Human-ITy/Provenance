[CmdletBinding()]
param(
    [switch] $ProbeOnly,
    [switch] $RichLandforms,
    [string[]] $ClientArgument = @('--ei3-authority'),
    [string] $CanonicalWorkspaceRoot = '',
    [int] $ControlPort = 8765,
    [int] $BulkPort = 8766
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Find-Ei3Python {
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Python\bin\python.exe'),
        (Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe')
    )
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    $command = Get-Command python.exe -ErrorAction SilentlyContinue
    if ($command -and $command.Source) { return $command.Source }
    throw 'A Python 3.12+ runtime is required to start FableScript authority.'
}

function Assert-Ei3PortsFree {
    foreach ($port in $ControlPort, $BulkPort) {
        $listener = Get-NetTCPConnection -State Listen -LocalPort $port -ErrorAction SilentlyContinue
        if ($listener) {
            throw "Port $port is already occupied. No process was terminated."
        }
    }
}

$clientRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$workspaceRoot = if ($CanonicalWorkspaceRoot) {
    (Resolve-Path -LiteralPath $CanonicalWorkspaceRoot).Path
} else {
    (Resolve-Path -LiteralPath (Join-Path $clientRoot '..\..')).Path
}
$engineRoot = Join-Path $workspaceRoot 'Engine\FableScript'
$enginePackage = Join-Path $engineRoot 'fablescript'
$clientExe = Join-Path $clientRoot 'Build\x64_Release\ProvenanceClient.exe'
$macroCache = Join-Path $clientRoot 'Data\Worldgen\MacroAuthority'
$worldStateRoot = Join-Path $workspaceRoot 'State\canonical-playable'
$worldInstanceName = if ($RichLandforms) {
    'world-instance-ei3qb-rich-landforms.json'
} else {
    'world-instance.json'
}
$worldInstance = Join-Path $worldStateRoot $worldInstanceName
$compiledContexts = Join-Path $workspaceRoot 'Cache\Worldgen\compiled_context\drainage'
$evidenceRoot = Join-Path $workspaceRoot 'Evidence\Playable\launcher-runs'
$workspaceManifest = Join-Path $workspaceRoot 'CANONICAL_WORKSPACE.json'
$probeModule = Join-Path $enginePackage 'tools\probe_playable_authority.py'

foreach ($required in $engineRoot, $enginePackage, $clientRoot, $macroCache,
        $workspaceManifest, $probeModule) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Canonical deployment component is missing: $required"
    }
}
if (-not (Test-Path -LiteralPath $clientExe -PathType Leaf)) {
    throw "Canonical client executable is missing: $clientExe"
}

Assert-Ei3PortsFree
$python = Find-Ei3Python
$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([guid]::NewGuid().ToString('N'))
$runRoot = Join-Path $evidenceRoot $runId
$shutdownFile = Join-Path $runRoot 'owned-authority.shutdown'
$authorityStdout = Join-Path $runRoot 'authority.stdout.log'
$authorityStderr = Join-Path $runRoot 'authority.stderr.log'
$probeReceipt = Join-Path $runRoot 'authority-session.json'
$probeStderr = Join-Path $runRoot 'probe.stderr.log'
$launcherReceipt = Join-Path $runRoot 'launcher-receipt.json'
New-Item -ItemType Directory -Force -Path $worldStateRoot, $compiledContexts, $runRoot | Out-Null

$authority = $null
$identity = $null
$clientExitCode = $null
$cleanShutdown = $false
$launcherStatus = 'FAIL'
try {
    $authorityArgs = @(
        '-m', 'tools.serve_worldgen_projection',
        '--cache-root', $macroCache,
        '--compiled-context-root', $compiledContexts,
        '--world-instance', $worldInstance,
        '--control-port', "$ControlPort",
        '--bulk-port', "$BulkPort",
        '--shutdown-file', $shutdownFile
    )
    if ($RichLandforms) { $authorityArgs += '--rich-landforms' }
    $authority = Start-Process -FilePath $python -ArgumentList $authorityArgs `
        -WorkingDirectory $enginePackage -WindowStyle Hidden `
        -RedirectStandardOutput $authorityStdout -RedirectStandardError $authorityStderr `
        -PassThru
    @{
        owner_pid = $PID
        authority_pid = $authority.Id
        shutdown_file = $shutdownFile
        started_utc = [DateTime]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $worldStateRoot 'last-owned-authority.json') -Encoding utf8

    $deadline = [DateTime]::UtcNow.AddSeconds(180)
    do {
        if ($authority.HasExited) {
            $detail = if (Test-Path -LiteralPath $authorityStderr) {
                (Get-Content -LiteralPath $authorityStderr -Raw)
            } else { 'no authority log' }
            throw "FableScript authority exited during startup: $detail"
        }
        $probeArgs = @(
            $probeModule,
            '--control-port', "$ControlPort", '--bulk-port', "$BulkPort",
            '--timeout', '120', '--receipt', $probeReceipt
        )
        & $python @probeArgs 2> $probeStderr | Out-Null
        if ($LASTEXITCODE -eq 0) { break }
        Start-Sleep -Milliseconds 500
    } while ([DateTime]::UtcNow -lt $deadline)

    if (-not (Test-Path -LiteralPath $probeReceipt -PathType Leaf)) {
        $detail = if (Test-Path -LiteralPath $probeStderr) {
            (Get-Content -LiteralPath $probeStderr -Raw)
        } else { 'probe produced no diagnostic' }
        throw "Canonical control/bulk handshake did not become valid: $detail"
    }
    $identity = Get-Content -LiteralPath $probeReceipt -Raw | ConvertFrom-Json
    if ($identity.status -ne 'PASS' -or $identity.authority_session -ne 'VALID' -or
            $identity.control_bulk_same_session -ne $true -or
            $identity.macro_pages_valid -ne 25 -or
            $identity.detailed_projection -ne 'ACTIVE') {
        throw 'Canonical authority readiness receipt is incomplete or incompatible.'
    }

    Write-Host 'AUTHORITY SESSION: VALID' -ForegroundColor Green
    Write-Host ("WORLD UUID: {0}" -f $identity.world_uuid)
    Write-Host ("MACRO GENESIS: {0}" -f $identity.macro_genesis_digest)
    Write-Host ("WORLD BASELINE: {0}" -f $identity.world_baseline_digest)
    Write-Host ("MACRO PAGES: {0}/25 VALID" -f $identity.macro_pages_valid)
    Write-Host 'DETAIL PROJECTION: ACTIVE'
    Write-Host 'PRESENTATION PATH: EI3 AUTHORITATIVE'
    Write-Host ("LANDFORM STAGE: {0}" -f $(if ($RichLandforms) { 'EI3.Q.B RICH' } else { 'EI3.Q.A CERTIFIED' }))

    if (-not $ProbeOnly) {
        $client = Start-Process -FilePath $clientExe -ArgumentList $ClientArgument `
            -WorkingDirectory $clientRoot -PassThru -Wait
        $clientExitCode = $client.ExitCode
        if ($clientExitCode -ne 0) {
            throw "ProvenanceClient exited with code $clientExitCode"
        }
    }
    $launcherStatus = 'PASS'
}
finally {
    if ($authority) {
        @{
            owner_pid = $PID
            authority_pid = $authority.Id
            requested_utc = [DateTime]::UtcNow.ToString('o')
        } | ConvertTo-Json | Set-Content -LiteralPath $shutdownFile -Encoding utf8
        $authority.Refresh()
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        while (-not $authority.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 100
            $authority.Refresh()
        }
        if (-not $authority.HasExited) {
            Stop-Process -Id $authority.Id -ErrorAction Stop
            $authority.WaitForExit(5000) | Out-Null
        }
        $authority.Refresh()
        $cleanShutdown = $authority.HasExited
    }
    @{
        certificate = 'EI3_CANONICAL_PLAYABLE_LAUNCHER/1'
        status = $launcherStatus
        probe_only = [bool]$ProbeOnly
        rich_landforms = [bool]$RichLandforms
        authority_pid = if ($authority) { $authority.Id } else { $null }
        owned_authority_shutdown = $cleanShutdown
        client_exit_code = $clientExitCode
        client_arguments = $ClientArgument
        control_port = $ControlPort
        bulk_port = $BulkPort
        world_uuid = if ($identity) { $identity.world_uuid } else { $null }
        macro_genesis_digest = if ($identity) { $identity.macro_genesis_digest } else { $null }
        world_baseline_digest = if ($identity) { $identity.world_baseline_digest } else { $null }
        macro_pages_valid = if ($identity) { $identity.macro_pages_valid } else { 0 }
        detailed_projection = if ($identity) { $identity.detailed_projection } else { 'UNKNOWN' }
        world_instance_path = $worldInstance
        client_executable = $clientExe
        completed_utc = [DateTime]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath $launcherReceipt -Encoding utf8
}

[CmdletBinding()]
param(
    [switch] $ProbeOnly,
    [switch] $RichLandforms,
    [switch] $ColdCertification,
    [string[]] $ClientArgument = @('--ei3-authority'),
    [string] $ValidatedMacroCacheRoot = '',
    [string] $CanonicalWorkspaceRoot = '',
    [int] $ControlPort = 8765,
    [int] $BulkPort = 8766
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$launchClock = [Diagnostics.Stopwatch]::StartNew()

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

function Stop-Ei3PreviouslyOwnedAuthority {
    param([Parameter(Mandatory = $true)][string] $ReceiptPath)

    if (-not (Test-Path -LiteralPath $ReceiptPath -PathType Leaf)) { return }
    try {
        $receipt = Get-Content -LiteralPath $ReceiptPath -Raw | ConvertFrom-Json
        $authorityPid = [int]$receipt.authority_pid
        $ownerPid = [int]$receipt.owner_pid
        $shutdownPath = [string]$receipt.shutdown_file
    } catch {
        throw "The previous authority ownership receipt is malformed: $ReceiptPath"
    }

    # A live launcher still owns its authority. Never interfere with it.
    if ($ownerPid -ne $PID -and (Get-Process -Id $ownerPid -ErrorAction SilentlyContinue)) {
        return
    }
    $process = Get-Process -Id $authorityPid -ErrorAction SilentlyContinue
    if (-not $process) {
        Remove-Item -LiteralPath $ReceiptPath -Force
        return
    }

    # Fail closed: only stop the exact Python authority recorded by our previous
    # launcher when it owns both configured listeners and its command line proves
    # the canonical projection module and ports. An unrelated listener is never
    # terminated merely because it occupies 8765/8766.
    $listeners = @()
    foreach ($port in $ControlPort, $BulkPort) {
        $listener = @(Get-NetTCPConnection -State Listen -LocalPort $port `
            -ErrorAction SilentlyContinue)
        if ($listener.Count -ne 1 -or [int]$listener[0].OwningProcess -ne $authorityPid) {
            return
        }
        $listeners += $listener
    }
    $processInfo = Get-CimInstance Win32_Process -Filter "ProcessId = $authorityPid" `
        -ErrorAction SilentlyContinue
    $commandLine = if ($processInfo) { [string]$processInfo.CommandLine } else { '' }
    if ($commandLine -notmatch 'tools\.serve_worldgen_projection' -or
            $commandLine -notmatch "--control-port\s+$ControlPort" -or
            $commandLine -notmatch "--bulk-port\s+$BulkPort" -or
            [string]::IsNullOrWhiteSpace($shutdownPath)) {
        return
    }

    @{
        owner_pid = $PID
        authority_pid = $authorityPid
        requested_utc = [DateTime]::UtcNow.ToString('o')
        reason = 'STALE_OWNED_AUTHORITY_RELAUNCH'
    } | ConvertTo-Json | Set-Content -LiteralPath $shutdownPath -Encoding utf8
    $deadline = [DateTime]::UtcNow.AddSeconds(10)
    while ((Get-Process -Id $authorityPid -ErrorAction SilentlyContinue) -and
            [DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 100
    }
    if (Get-Process -Id $authorityPid -ErrorAction SilentlyContinue) {
        Stop-Process -Id $authorityPid -ErrorAction Stop
    }
    Remove-Item -LiteralPath $ReceiptPath -Force
}
$worldInstance = Join-Path $worldStateRoot $worldInstanceName
$compiledContexts = Join-Path $workspaceRoot 'Cache\Worldgen\compiled_context\drainage'
$evidenceRoot = Join-Path $workspaceRoot 'Evidence\Playable\launcher-runs'
$workspaceManifest = Join-Path $workspaceRoot 'CANONICAL_WORKSPACE.json'
$probeModule = Join-Path $enginePackage 'tools\probe_playable_authority.py'
$ownershipReceipt = Join-Path $worldStateRoot 'last-owned-authority.json'

foreach ($required in $engineRoot, $enginePackage, $clientRoot, $macroCache,
        $workspaceManifest, $probeModule) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Canonical deployment component is missing: $required"
    }
}
if (-not (Test-Path -LiteralPath $clientExe -PathType Leaf)) {
    throw "Canonical client executable is missing: $clientExe"
}

New-Item -ItemType Directory -Force -Path $worldStateRoot | Out-Null
Stop-Ei3PreviouslyOwnedAuthority -ReceiptPath $ownershipReceipt
Assert-Ei3PortsFree
$python = Find-Ei3Python
$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([guid]::NewGuid().ToString('N'))
$runRoot = Join-Path $evidenceRoot $runId
$validatedMacroCacheSource = $null
$macroCacheAccessAlias = $null
$effectiveClientArgument = @($ClientArgument)
if ($RichLandforms -and
        -not ($effectiveClientArgument -contains '--ei3-rich-landforms-stage')) {
    $effectiveClientArgument += '--ei3-rich-landforms-stage'
}
$isQbVisualCertificate = $effectiveClientArgument -contains '--cert-ei3qb-visual-qa' -or
    $effectiveClientArgument -contains '--cert-ei3qb-visual-qb'
if ($isQbVisualCertificate -and
        -not ($effectiveClientArgument -contains '--cert-out-dir')) {
    $effectiveClientArgument += @('--cert-out-dir', $runRoot)
}
$effectiveMacroCache = $macroCache
$expectedReadinessPages = 25
$projectedCachePages = 25
if ($ColdCertification) {
    # Detailed context is always isolated so this run cannot borrow readiness
    # from an earlier authority process.
    $compiledContexts = Join-Path $runRoot 'cold-cache\compiled_context\drainage'
    if ($isQbVisualCertificate) {
        # Build a disposable engine-owned projection cache around every matched
        # Q.A/Q.B station.  This is a representation prewarm from the pinned
        # WorldGenesis, not a client generator and not a world-law change.
        $effectiveMacroCache = if ($ValidatedMacroCacheRoot) {
            $validatedMacroCacheSource = (Resolve-Path -LiteralPath $ValidatedMacroCacheRoot).Path
            # Page V2 manifests address immutable artifacts below sha256/<digest>.mcp.
            # A deeply nested evidence root can push those valid files beyond the
            # legacy Win32 path limit.  A temporary directory junction supplies a
            # short spelling for the exact same cache; it does not copy, regenerate,
            # or alter any authority artifact or manifest.
            $macroCacheAccessAlias = Join-Path ([IO.Path]::GetTempPath()) `
                ('provenance-macro-cache-{0}' -f ([guid]::NewGuid().ToString('N')))
            New-Item -ItemType Junction -Path $macroCacheAccessAlias `
                -Target $validatedMacroCacheSource | Out-Null
            $macroCacheAccessAlias
        } else {
            Join-Path $runRoot 'cold-cache\macro-authority'
        }
        $projectedCachePages = 34
    }
}
$effectiveClientArgument += @('--macro-authority-root', $effectiveMacroCache)
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
$authorityStartedMs = $null
$prewarmStartedMs = $null
$prewarmCompletedMs = $null
$firstDetailedPublicationMs = $null
$timeUntilPlayableMs = $null
$clientStartedMs = $null
try {
    $prewarmStartedMs = $launchClock.ElapsedMilliseconds
    if ($ColdCertification -and $isQbVisualCertificate -and -not $ValidatedMacroCacheRoot) {
        $projectionCoords = @()
        foreach ($ri in ((-9)..(-7))) {
            foreach ($rj in ((-9)..(-7))) { $projectionCoords += ("{0},{1}" -f $ri, $rj) }
        }
        $compileTool = Join-Path $enginePackage 'tools\ei1_worldgenesis.py'
        $compileArgs = @($compileTool, 'compile-cache', $effectiveMacroCache,
            '--source-cache', $macroCache, '--jobs', '9', '--coords') + $projectionCoords
        $compileOutput = & $python @compileArgs
        if ($LASTEXITCODE -ne 0) {
            throw 'Cold WorldGenesis macro projection prewarm failed.'
        }
        $compileOutput | Set-Content -LiteralPath (Join-Path $runRoot 'macro-cache-compile.json') -Encoding utf8
    }
    $authorityArgs = @(
        '-m', 'tools.serve_worldgen_projection',
        '--cache-root', $effectiveMacroCache,
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
    $authorityStartedMs = $launchClock.ElapsedMilliseconds
    @{
        owner_pid = $PID
        authority_pid = $authority.Id
        shutdown_file = $shutdownFile
        started_utc = [DateTime]::UtcNow.ToString('o')
        control_port = $ControlPort
        bulk_port = $BulkPort
        engine_package = $enginePackage
    } | ConvertTo-Json | Set-Content -LiteralPath $ownershipReceipt -Encoding utf8

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
            $identity.macro_pages_valid -ne $expectedReadinessPages -or
            $identity.detailed_projection -ne 'ACTIVE') {
        throw 'Canonical authority readiness receipt is incomplete or incompatible.'
    }
    $prewarmCompletedMs = $launchClock.ElapsedMilliseconds
    # The readiness probe validates a published detailed snapshot; this is the
    # first detailed publication intentionally exposed by the normal launcher.
    $firstDetailedPublicationMs = $prewarmCompletedMs

    Write-Host 'AUTHORITY SESSION: VALID' -ForegroundColor Green
    Write-Host ("WORLD UUID: {0}" -f $identity.world_uuid)
    Write-Host ("MACRO GENESIS: {0}" -f $identity.macro_genesis_digest)
    Write-Host ("WORLD BASELINE: {0}" -f $identity.world_baseline_digest)
    Write-Host ("READINESS PAGES: {0}/{1} VALID" -f $identity.macro_pages_valid, $expectedReadinessPages)
    Write-Host ("PROJECTED CACHE PAGES: {0}" -f $projectedCachePages)
    Write-Host 'DETAIL PROJECTION: ACTIVE'
    Write-Host 'PRESENTATION PATH: EI3 AUTHORITATIVE'
    Write-Host ("PLAYABLE STAGE: {0}" -f $(if ($RichLandforms) { 'STAGE 0 CUT 0 CONTINUOUS WORLD PREVIEW' } else { 'EI3.Q.A CERTIFIED' }))

    if (-not $ProbeOnly) {
        $timeUntilPlayableMs = $launchClock.ElapsedMilliseconds
        $clientStartedMs = $timeUntilPlayableMs
        $client = Start-Process -FilePath $clientExe -ArgumentList $effectiveClientArgument `
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
        if ($cleanShutdown -and (Test-Path -LiteralPath $ownershipReceipt -PathType Leaf)) {
            $owned = Get-Content -LiteralPath $ownershipReceipt -Raw | ConvertFrom-Json
            if ([int]$owned.authority_pid -eq $authority.Id) {
                Remove-Item -LiteralPath $ownershipReceipt -Force
            }
        }
    }
    if ($macroCacheAccessAlias -and [IO.Directory]::Exists($macroCacheAccessAlias)) {
        # Delete only the temporary junction.  The persistent cache target and all
        # 34 validated authority pages remain untouched.
        [IO.Directory]::Delete($macroCacheAccessAlias)
    }
    @{
        certificate = 'EI3_CANONICAL_PLAYABLE_LAUNCHER/1'
        status = $launcherStatus
        probe_only = [bool]$ProbeOnly
        rich_landforms = [bool]$RichLandforms
        cold_certification = [bool]$ColdCertification
        authority_pid = if ($authority) { $authority.Id } else { $null }
        owned_authority_shutdown = $cleanShutdown
        client_exit_code = $clientExitCode
        client_arguments = $effectiveClientArgument
        cold_startup_ms = $authorityStartedMs
        prewarm_ms = if ($prewarmCompletedMs -ne $null) {
            $prewarmCompletedMs - $prewarmStartedMs
        } else { $null }
        time_until_playable_ms = $timeUntilPlayableMs
        first_detailed_publication_ms = $firstDetailedPublicationMs
        client_started_ms = $clientStartedMs
        control_port = $ControlPort
        bulk_port = $BulkPort
        world_uuid = if ($identity) { $identity.world_uuid } else { $null }
        macro_genesis_digest = if ($identity) { $identity.macro_genesis_digest } else { $null }
        world_baseline_digest = if ($identity) { $identity.world_baseline_digest } else { $null }
        macro_pages_valid = if ($identity) { $identity.macro_pages_valid } else { 0 }
        expected_readiness_pages = $expectedReadinessPages
        projected_cache_pages = $projectedCachePages
        macro_cache_root = $effectiveMacroCache
        persistent_macro_cache_root = if ($validatedMacroCacheSource) {
            $validatedMacroCacheSource
        } else { $effectiveMacroCache }
        macro_cache_access_strategy = if ($macroCacheAccessAlias) {
            'TEMPORARY_DIRECTORY_JUNCTION'
        } else { 'DIRECT' }
        detailed_projection = if ($identity) { $identity.detailed_projection } else { 'UNKNOWN' }
        world_instance_path = $worldInstance
        client_executable = $clientExe
        completed_utc = [DateTime]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath $launcherReceipt -Encoding utf8
}

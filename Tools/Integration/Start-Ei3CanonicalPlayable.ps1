[CmdletBinding()]
param(
    [switch] $ProbeOnly,
    [switch] $RichLandforms,
    [switch] $PlayerFacing,
    [switch] $ColdCertification,
    [string[]] $ClientArgument = @('--ei3-authority'),
    [string] $ValidatedMacroCacheRoot = '',
    [string] $CanonicalWorkspaceRoot = '',
    [string] $WorldSeed = '',
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
$pendingClientExe = Join-Path $clientRoot 'Build\x64_Release\ProvenanceClient.pending.exe'
$macroCache = Join-Path $clientRoot 'Data\Worldgen\MacroAuthority-v11'
$worldStateRoot = Join-Path $workspaceRoot 'State\canonical-playable'
$worldInstanceName = if ($WorldSeed) {
    # The player-facing .cmd deliberately invokes inbox Windows PowerShell 5.1.
    # SHA256.HashData and Convert.ToHexString are newer .NET APIs, so use the
    # compatible instance API and explicit byte formatting here.
    $seedBytes = [Text.Encoding]::UTF8.GetBytes($WorldSeed)
    $seedHasher = [Security.Cryptography.SHA256]::Create()
    try {
        $seedDigest = $seedHasher.ComputeHash($seedBytes)
    } finally {
        $seedHasher.Dispose()
    }
    $seedHash = (-join ($seedDigest | ForEach-Object { $_.ToString('x2') })).Substring(0, 16)
    $isOrographicPhase17 = ($WorldSeed -eq '20260827' -or $WorldSeed -eq 'orographic-phase17-canonical')
    if ($isOrographicPhase17) {
        # Distinct instance file from world-instance-stage0-genesis-v11-*.
        # Existing v11 origins on the v11 path are not rewritten.
        "world-instance-stage0-orographic-phase17-$seedHash.json"
    } elseif ($RichLandforms) {
        "world-instance-stage0-genesis-v11-$seedHash.json"
    } else {
        "world-instance-v9-$seedHash.json"
    }
} elseif ($RichLandforms) {
    # Preserve older receipts for audit/recovery. Generator v11 has a new
    # canonical baseline identity and therefore owns a distinct persisted world.
    'world-instance-stage0-genesis-v11.json'
} else {
    'world-instance-v9.json'
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
$openingReceipt = "$worldInstance.opening.json"
$isOrographicPhase17 = $worldInstanceName -like 'world-instance-stage0-orographic-phase17-*'
$compiledContexts = Join-Path $workspaceRoot 'Cache\Worldgen\compiled_context\drainage'
$evidenceRoot = Join-Path $workspaceRoot 'Evidence\Playable\launcher-runs'
$workspaceManifest = Join-Path $workspaceRoot 'CANONICAL_WORKSPACE.json'
$probeModule = Join-Path $enginePackage 'tools\probe_playable_authority.py'
$openingTool = Join-Path $enginePackage 'tools\select_stage0_opening.py'
$ownershipReceipt = Join-Path $worldStateRoot 'last-owned-authority.json'

if (-not $ProbeOnly -and (Test-Path -LiteralPath $pendingClientExe -PathType Leaf)) {
    try {
        Copy-Item -LiteralPath $pendingClientExe -Destination $clientExe -Force
        Remove-Item -LiteralPath $pendingClientExe -Force
        $pendingPdb = Join-Path $clientRoot 'Build\x64_Release\ProvenanceClient.pending.pdb'
        if (Test-Path -LiteralPath $pendingPdb -PathType Leaf) {
            Copy-Item -LiteralPath $pendingPdb -Destination `
                (Join-Path $clientRoot 'Build\x64_Release\ProvenanceClient.pdb') -Force
            Remove-Item -LiteralPath $pendingPdb -Force
        }
        Write-Host 'PROMOTED PENDING CLIENT BUILD' -ForegroundColor Green
    } catch [System.IO.IOException] {
        throw 'A newer client build is ready, but ProvenanceClient.exe is still open. Close the existing playtest window and launch again.'
    }
}

New-Item -ItemType Directory -Force -Path $macroCache | Out-Null
foreach ($required in $engineRoot, $enginePackage, $clientRoot, $macroCache,
        $workspaceManifest, $probeModule, $openingTool) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Canonical deployment component is missing: $required"
    }
}
if (-not (Test-Path -LiteralPath $clientExe -PathType Leaf)) {
    throw "Canonical client executable is missing: $clientExe"
}
$clientHash = (Get-FileHash -LiteralPath $clientExe -Algorithm SHA256).Hash
if ($PlayerFacing) {
    Write-Host 'PROVENANCE STAGE 0' -ForegroundColor Cyan
    Write-Host 'Preparing the nearest authoritative terrain first...'
} else {
    Write-Host ("CLIENT BUILD: {0}" -f $clientExe) -ForegroundColor Cyan
    Write-Host ("CLIENT SHA-256: {0}" -f $clientHash) -ForegroundColor Cyan
}

New-Item -ItemType Directory -Force -Path $worldStateRoot | Out-Null
Stop-Ei3PreviouslyOwnedAuthority -ReceiptPath $ownershipReceipt
Assert-Ei3PortsFree
$python = Find-Ei3Python
$runId = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([guid]::NewGuid().ToString('N'))
$runRoot = Join-Path $evidenceRoot $runId
$validatedMacroCacheSource = $null
$macroCacheAccessAlias = $null
$clientMacroCacheAlias = $null
$effectiveClientArgument = @($ClientArgument)
$clientEndpointArguments = @(
    '127.0.0.1',
    "$ControlPort",
    "--bulk-port=$BulkPort"
)
$effectiveClientArgument = $clientEndpointArguments + $effectiveClientArgument
if ($RichLandforms -and
        -not ($effectiveClientArgument -contains '--ei3-rich-landforms-stage')) {
    $effectiveClientArgument += '--ei3-rich-landforms-stage'
}
if ($PlayerFacing -and
        -not ($effectiveClientArgument -contains '--ei3-player-facing')) {
    $effectiveClientArgument += '--ei3-player-facing'
}
$isQbVisualCertificate = $effectiveClientArgument -contains '--cert-ei3qb-visual-qa' -or
    $effectiveClientArgument -contains '--cert-ei3qb-visual-qb'
if ($isQbVisualCertificate -and
        -not ($effectiveClientArgument -contains '--cert-out-dir')) {
    $effectiveClientArgument += @('--cert-out-dir', $runRoot)
}
$effectiveMacroCache = $macroCache
$clientMacroCache = $effectiveMacroCache
$expectedReadinessPages = if ($WorldSeed -and $RichLandforms) { 1 } else { 25 }
$projectedCachePages = $expectedReadinessPages
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
$clientMacroCache = $effectiveMacroCache
$shutdownFile = Join-Path $runRoot 'owned-authority.shutdown'
$authorityStdout = Join-Path $runRoot 'authority.stdout.log'
$authorityStderr = Join-Path $runRoot 'authority.stderr.log'
$probeReceipt = Join-Path $runRoot 'authority-session.json'
$probeStdout = Join-Path $runRoot 'probe.stdout.log'
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
    if ($isOrographicPhase17 -and $RichLandforms) {
        # Canonical orographic.phase17 lands on certified page (1,1). Do not
        # reuse the v11 caused-opening for provenance-stage0-genesis-010.
        $spawnX = '1536'
        $spawnY = '1536'
        $spawnZ = '0'
        $spawnYaw = '0'
        $effectiveClientArgument += @(
            '--ei3-spawn-x', $spawnX,
            '--ei3-spawn-y', $spawnY,
            '--ei3-spawn-z', $spawnZ,
            '--ei3-spawn-yaw', $spawnYaw
        )
        if ($PlayerFacing) {
            Write-Host 'World ready: orographic.phase17 canonical openworld' -ForegroundColor Green
        }
    } elseif ($WorldSeed -and $RichLandforms) {
        # Select the opening from caused terrain/biome/drainage fields before page
        # projection begins. The receipt is deterministic world metadata, so returning
        # worlds reuse it. The shipped first world also carries its already-verified
        # receipt; only a genuinely new seed performs the one-time opening search.
        if (-not (Test-Path -LiteralPath $openingReceipt -PathType Leaf)) {
            $bundledOpening = Join-Path $clientRoot `
                'Data\Worldgen\Stage0Openings\provenance-stage0-genesis-010-v11.json'
            if ($WorldSeed -eq 'provenance-stage0-genesis-010' -and
                    (Test-Path -LiteralPath $bundledOpening -PathType Leaf)) {
                Copy-Item -LiteralPath $bundledOpening -Destination $openingReceipt
            } else {
                $openingOutput = & $python $openingTool '--world-seed' $WorldSeed `
                    '--compiled-context-root' $compiledContexts '--output' $openingReceipt
                if ($LASTEXITCODE -ne 0 -or
                        -not (Test-Path -LiteralPath $openingReceipt -PathType Leaf)) {
                    throw 'Stage 0 caused-opening selection failed.'
                }
            }
        }
        $opening = Get-Content -LiteralPath $openingReceipt -Raw | ConvertFrom-Json
        if ($opening.status -ne 'PASS' -or -not $opening.start_position -or
                [string]$opening.world_seed -ne $WorldSeed) {
            throw 'Stage 0 caused-opening receipt is incomplete.'
        }
        $spawnX = [string]::Format([Globalization.CultureInfo]::InvariantCulture,
            '{0:R}', [double]$opening.start_position.x_m)
        $spawnY = [string]::Format([Globalization.CultureInfo]::InvariantCulture,
            '{0:R}', [double]$opening.start_position.y_m)
        $spawnZ = [string]::Format([Globalization.CultureInfo]::InvariantCulture,
            '{0:R}', [double]$opening.start_position.ground_elevation_m)
        $spawnYaw = [string]::Format([Globalization.CultureInfo]::InvariantCulture,
            '{0:R}', [double]$opening.start_position.facing_yaw_rad)
        $effectiveClientArgument += @(
            '--ei3-spawn-x', $spawnX,
            '--ei3-spawn-y', $spawnY,
            '--ei3-spawn-z', $spawnZ,
            '--ei3-spawn-yaw', $spawnYaw
        )
        if ($PlayerFacing) {
            Write-Host ("World ready: {0}" -f $WorldSeed) -ForegroundColor Green
        }
    }
    $effectiveManifest = Join-Path $effectiveMacroCache 'macro_manifest.mcm'
    $visualColdPrewarmOwnsCache = $ColdCertification -and $isQbVisualCertificate `
        -and -not $ValidatedMacroCacheRoot
    if ($WorldSeed -and -not $isOrographicPhase17) {
        # A non-default GenesisIdentity owns a separate immutable cache below
        # its digest. Build the page under the player's feet before the client
        # appears; farther pages remain deterministic cache misses and are
        # published by the authority as exploration approaches them.
        # Orographic.phase17 skips this v11 prewarm: live emit is
        # orographic_production_page, not a relabeled WorldGenesis page.
        $centerRi = 0
        $centerRj = 0
        if ($RichLandforms) {
            $centerRi = [int][Math]::Floor(([double]$spawnX + 32000.0) / 64000.0)
            $centerRj = [int][Math]::Floor(([double]$spawnY + 32000.0) / 64000.0)
        }
        $projectionCoords = @(("{0},{1}" -f $centerRi, $centerRj))
        $compileTool = Join-Path $enginePackage 'tools\ei1_worldgenesis.py'
        $compileArgs = @($compileTool, 'compile-cache', $effectiveMacroCache,
            '--world-seed', $WorldSeed, '--per-genesis-root',
            '--compiled-context-root', $compiledContexts,
            '--jobs', '1')
        foreach ($coord in $projectionCoords) { $compileArgs += ("--coord={0}" -f $coord) }
        $compileOutput = & $python @compileArgs
        if ($LASTEXITCODE -ne 0) {
            throw 'Seeded WorldGenesis v11 macro readiness prewarm failed.'
        }
        $compileOutput | Set-Content -LiteralPath `
            (Join-Path $runRoot 'seeded-macro-cache-v8-compile.json') -Encoding utf8
    }
    if ((-not $WorldSeed) -and
            (-not (Test-Path -LiteralPath $effectiveManifest -PathType Leaf)) -and
            (-not $visualColdPrewarmOwnsCache)) {
        # A generator version owns its own immutable macro cache. Build the
        # canonical readiness ring once; returning worlds load it directly.
        $projectionCoords = @()
        foreach ($ri in ((-2)..2)) {
            foreach ($rj in ((-2)..2)) { $projectionCoords += ("{0},{1}" -f $ri, $rj) }
        }
        $compileTool = Join-Path $enginePackage 'tools\ei1_worldgenesis.py'
        $compileArgs = @($compileTool, 'compile-cache', $effectiveMacroCache,
            '--compiled-context-root', $compiledContexts,
            '--jobs', '9')
        foreach ($coord in $projectionCoords) { $compileArgs += ("--coord={0}" -f $coord) }
        $compileOutput = & $python @compileArgs
        if ($LASTEXITCODE -ne 0) {
            throw 'WorldGenesis v11 macro readiness prewarm failed.'
        }
        $compileOutput | Set-Content -LiteralPath (Join-Path $runRoot 'macro-cache-v8-compile.json') -Encoding utf8
    }
    if ($ColdCertification -and $isQbVisualCertificate -and -not $ValidatedMacroCacheRoot) {
        $projectionCoords = @()
        foreach ($ri in ((-9)..(-7))) {
            foreach ($rj in ((-9)..(-7))) { $projectionCoords += ("{0},{1}" -f $ri, $rj) }
        }
        $compileTool = Join-Path $enginePackage 'tools\ei1_worldgenesis.py'
        $compileArgs = @($compileTool, 'compile-cache', $effectiveMacroCache,
            '--source-cache', $macroCache,
            '--compiled-context-root', $compiledContexts,
            '--jobs', '9')
        foreach ($coord in $projectionCoords) { $compileArgs += ("--coord={0}" -f $coord) }
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
    if ($WorldSeed) { $authorityArgs += @('--world-seed', $WorldSeed) }
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

    # A brand-new seed may need to compile its first 5x5 authority ring before
    # the listeners open. Returning worlds reuse that content-addressed cache
    # and retain the original tighter readiness budget.
    $readinessSeconds = if ($WorldSeed) { 600 } else { 180 }
    $deadline = [DateTime]::UtcNow.AddSeconds($readinessSeconds)
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
        if ($WorldSeed -and $RichLandforms) {
            # Normal play becomes ready when the authoritative page containing
            # the caused spawn is present.  The server remains responsible for
            # every outward page requested during exploration.  The probe's
            # default 5x5 sweep is retained for explicit certification launches.
            $probeArgs += ("--page-coord={0},{1}" -f $centerRi, $centerRj)
        }
        # A cold authority normally refuses the first connection while Python is
        # still importing/compiling. Invoke the probe as a child process so its
        # expected non-zero retry result is data, not a PowerShell NativeCommandError
        # promoted to a terminating exception by ErrorActionPreference=Stop.
        $probe = Start-Process -FilePath $python -ArgumentList $probeArgs `
            -WindowStyle Hidden -Wait -PassThru `
            -RedirectStandardOutput $probeStdout `
            -RedirectStandardError $probeStderr
        if ($probe.ExitCode -eq 0) { break }
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
            $identity.macro_pages_valid -lt $expectedReadinessPages -or
            $identity.detailed_projection -ne 'ACTIVE') {
        throw 'Canonical authority readiness receipt is incomplete or incompatible.'
    }
    $prewarmCompletedMs = $launchClock.ElapsedMilliseconds
    # The readiness probe validates a published detailed snapshot; this is the
    # first detailed publication intentionally exposed by the normal launcher.
    $firstDetailedPublicationMs = $prewarmCompletedMs

    if ($PlayerFacing) {
        Write-Host 'Entering world...' -ForegroundColor Green
    } else {
        Write-Host 'AUTHORITY SESSION: VALID' -ForegroundColor Green
        Write-Host ("WORLD UUID: {0}" -f $identity.world_uuid)
        Write-Host ("MACRO GENESIS: {0}" -f $identity.macro_genesis_digest)
        Write-Host ("WORLD BASELINE: {0}" -f $identity.world_baseline_digest)
        Write-Host ("READINESS PAGES: {0}/{1} VALID" -f $identity.macro_pages_valid, $expectedReadinessPages)
        Write-Host ("PROJECTED CACHE PAGES: {0}" -f $projectedCachePages)
        Write-Host 'DETAIL PROJECTION: ACTIVE'
        Write-Host 'PRESENTATION PATH: EI3 AUTHORITATIVE'
        Write-Host ("PLAYABLE STAGE: {0}" -f $(if ($RichLandforms) { 'STAGE 0 GENESIS WORLD' } else { 'EI3.Q.A CERTIFIED' }))
    }

    # Non-default worlds are stored below their GenesisIdentity so pages from
    # different seeds can never alias. Give the dumb client the exact cache root
    # admitted by the server after the handshake establishes that identity.
    $seededMacroCache = Join-Path $effectiveMacroCache $identity.macro_genesis_digest
    if ($WorldSeed -and
            (Test-Path -LiteralPath (Join-Path $seededMacroCache 'macro_manifest.mcm') -PathType Leaf)) {
        # Content-addressed filenames below a full worktree + genesis path can
        # exceed the legacy Win32 stream limit used by the C++ client. A short
        # junction is another spelling for the same immutable cache, not a copy.
        $clientMacroCacheAlias = Join-Path ([IO.Path]::GetTempPath()) `
            ('provenance-seeded-cache-{0}' -f ([guid]::NewGuid().ToString('N')))
        New-Item -ItemType Junction -Path $clientMacroCacheAlias `
            -Target $seededMacroCache | Out-Null
        $clientMacroCache = $clientMacroCacheAlias
    }
    $effectiveClientArgument += @('--macro-authority-root', $clientMacroCache)

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
    if ($clientMacroCacheAlias -and [IO.Directory]::Exists($clientMacroCacheAlias)) {
        [IO.Directory]::Delete($clientMacroCacheAlias)
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
        macro_cache_root = $clientMacroCache
        persistent_macro_cache_root = if ($validatedMacroCacheSource) {
            $validatedMacroCacheSource
        } else { $effectiveMacroCache }
        macro_cache_access_strategy = if ($macroCacheAccessAlias) {
            'TEMPORARY_DIRECTORY_JUNCTION'
        } else { 'DIRECT' }
        detailed_projection = if ($identity) { $identity.detailed_projection } else { 'UNKNOWN' }
        world_instance_path = $worldInstance
        opening_receipt_path = if (Test-Path -LiteralPath $openingReceipt -PathType Leaf) {
            $openingReceipt
        } else { $null }
        client_executable = $clientExe
        completed_utc = [DateTime]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath $launcherReceipt -Encoding utf8
}

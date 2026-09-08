param(
    [string]$DestinationParent,
    [switch]$Create
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
if (-not (Test-Path -LiteralPath (Join-Path $repo 'Esoterica.slnx'))) { throw 'Repository root not found.' }
if (-not $DestinationParent) { $DestinationParent = Join-Path (Split-Path (Split-Path $repo)) 'Checkpoints' }
$DestinationParent = [IO.Path]::GetFullPath($DestinationParent)
if ($DestinationParent.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase) -or $DestinationParent -eq $repo) {
    throw 'Checkpoint destination must be outside the checkout.'
}

# Preserve all editable trees (including output/models originals), local dependencies,
# generated headers, and evidence. This is byte preservation, NOT authorship attribution.
$files = [Collections.Generic.List[IO.FileInfo]]::new()
foreach ($tree in @('Code', 'Data', 'Assets', 'Docs', 'Tools', 'External')) {
    $path = Join-Path $repo $tree
    if (Test-Path -LiteralPath $path) {
        foreach ($file in Get-ChildItem -LiteralPath $path -File -Recurse -Force) { $files.Add($file) }
    }
}
foreach ($file in Get-ChildItem -LiteralPath $repo -File -Force) {
    if ($file.Extension -notin @('.zip', '.7z', '.rar')) { $files.Add($file) }
}
# Keep runnable binaries, editable imports, Wavefront authoring files, recipes and
# reports. Omit only the named compiler/cache directories and binary intermediates.
$build = Join-Path $repo 'Build'
if (Test-Path -LiteralPath $build) {
    foreach ($dir in Get-ChildItem -LiteralPath $build -Directory) {
        if ($dir.Name -eq '_Temp') { continue }
        foreach ($file in Get-ChildItem -LiteralPath $dir.FullName -File -Recurse -Force) {
            $relative = $file.FullName.Substring($build.Length + 1)
            if ($relative -match '(^|\\)CompiledData(\\|$)') { continue }
            if ($file.Extension -in @('.pdb', '.ilk', '.lib', '.exp', '.pch', '.idb', '.ipdb', '.iobj', '.tlog', '.zip', '.7z')) { continue }
            if ($file.Extension -eq '.obj') {
                # COFF objects have binary headers; preserve text Wavefront OBJ inputs.
                $stream = $file.OpenRead()
                try {
                    $bytes = New-Object byte[] 256
                    $count = $stream.Read($bytes, 0, $bytes.Length)
                    $header = [Text.Encoding]::UTF8.GetString($bytes, 0, $count)
                    if ($header -notmatch '(?m)^(#|v |o |mtllib |g )') { continue }
                } finally { $stream.Dispose() }
            }
            $files.Add($file)
        }
    }
    foreach ($file in Get-ChildItem -LiteralPath $build -File -Force) {
        if ($file.Extension -notin @('.zip', '.7z', '.pdb', '.ilk', '.lib', '.obj', '.exp')) { $files.Add($file) }
    }
}
$ordered = @($files | Sort-Object FullName -Unique)
$total = ($ordered | Measure-Object Length -Sum).Sum
Write-Output ('Candidate: {0} files, {1:N2} GiB; destination parent: {2}' -f $ordered.Count, ($total / 1GB), $DestinationParent)
if (-not $Create) { return }

Add-Type -AssemblyName System.IO.Compression
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$destination = Join-Path $DestinationParent ('GraniteLab-' + $stamp)
if (Test-Path -LiteralPath $destination) { throw 'Destination already exists; no overwrite permitted.' }
[void][IO.Directory]::CreateDirectory($destination)
$archivePath = Join-Path $destination 'checkout.zip'
$manifest = [Collections.Generic.List[object]]::new()
$archiveStream = [IO.File]::Open($archivePath, [IO.FileMode]::CreateNew)
$zip = [IO.Compression.ZipArchive]::new($archiveStream, [IO.Compression.ZipArchiveMode]::Create, $false)
$hash = [Security.Cryptography.SHA256]::Create()
try {
    $index = 0
    foreach ($file in $ordered) {
        $name = $file.FullName.Substring($repo.Length + 1).Replace('\', '/')
        # Exclusive against writers while hashing and copying this file.
        $input = [IO.File]::Open($file.FullName, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
        try {
            $digest = [BitConverter]::ToString($hash.ComputeHash($input)).Replace('-', '')
            $length = $input.Length
            $input.Position = 0
            $entry = $zip.CreateEntry($name, [IO.Compression.CompressionLevel]::Fastest)
            $output = $entry.Open()
            try { $input.CopyTo($output) } finally { $output.Dispose() }
            $manifest.Add([pscustomobject]@{Path=$name; Bytes=$length; SHA256=$digest})
        } finally { $input.Dispose() }
        $index++
        if ($index % 1000 -eq 0) { Write-Output ('Archived {0}/{1}' -f $index, $ordered.Count) }
    }
} finally { $zip.Dispose(); $archiveStream.Dispose(); $hash.Dispose() }
$manifest | Export-Csv -LiteralPath (Join-Path $destination 'manifest.csv') -NoTypeInformation -Encoding UTF8
$head = (& git -C $repo rev-parse HEAD | Out-String).Trim()
$status = & git -C $repo status --porcelain=v1 --untracked-files=all 2>&1 | Out-String
[IO.File]::WriteAllText((Join-Path $destination 'git-status.txt'), "HEAD $head`r`n$status")

Write-Output 'Verifying every archived entry against its source-byte SHA256...'
$zip = [IO.Compression.ZipArchive]::new([IO.File]::OpenRead($archivePath), [IO.Compression.ZipArchiveMode]::Read, $false)
$hash = [Security.Cryptography.SHA256]::Create()
try {
    if ($zip.Entries.Count -ne $manifest.Count) { throw 'Archive entry count mismatch.' }
    $index = 0
    foreach ($item in $manifest) {
        $entry = $zip.GetEntry($item.Path)
        if (-not $entry -or $entry.Length -ne $item.Bytes) { throw "Missing or wrong length: $($item.Path)" }
        $input = $entry.Open()
        try { $digest = [BitConverter]::ToString($hash.ComputeHash($input)).Replace('-', '') } finally { $input.Dispose() }
        if ($digest -ne $item.SHA256) { throw "Hash mismatch: $($item.Path)" }
        $index++
        if ($index % 1000 -eq 0) { Write-Output ('Verified {0}/{1}' -f $index, $manifest.Count) }
    }
} finally { $zip.Dispose(); $hash.Dispose() }
$archiveHash = (Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash
$receipt = @"
# Granite Lab local recovery checkpoint

Created: $(Get-Date -Format o)
Source checkout: $repo
Git HEAD: $head
Files: $($manifest.Count)
Uncompressed bytes: $total
Archive SHA256: $archiveHash
Verification: every ZIP entry matched the recorded source-byte SHA256 and length.

This is a local mixed-worktree recovery snapshot, not a commit, push, or authorship
claim. Includes all Code, Data, Assets, Docs, Tools, External; root files except
old archives; Build runnable binaries, editable imports/OBJ and evidence.
Excluded: .git, .vs, .cursor; Build/_Temp, Build/**/CompiledData, compiler/link
intermediates and nested Build archives. Original output/models files are kept.
Installed Visual Studio/SDKs and inputs outside this checkout are NOT included.
See the archived NewGraniteLabCheckpoint.ps1 for the exact selection rule.

Restore into a separate checkout of HEAD above; overlay checkout.zip, preserving
paths. Inspect git-status.txt. Do not overlay a newer working checkout blindly.
Rebuild/regenerate omitted caches using the retained recipes. Runtime evidence is
preserved, but archive integrity is not an independent clean-machine rebuild test.
"@
[IO.File]::WriteAllText((Join-Path $destination 'CHECKPOINT.md'), $receipt)
Write-Output "VERIFIED CHECKPOINT: $destination"

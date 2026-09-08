# Build cleanup pass 1 — playable preview preservation

Preservation target: the user-designated V5.9 playable `ProvenanceSandbox.map` preview (granite, soil, grass, movement, excavation and debris), its dependencies, editable inputs and validation evidence. The editor workbench is not a substitute acceptance target. Historical fixtures may still protect contracts; none were removed.

## Completed

- Moved 39 historical-named test executables and compiler objects (49,518,471 bytes / 47.22 MiB) from `Build/x64_Debug` to `Build/Archive/LegacyTests-2026-09-06`.
- Exact file list, original/archive directories and SHA-256 values: [archive manifest](BUILD_LEGACY_ARCHIVE_2026-09-06.json).
- Candidate names were limited to `_new`, `_v35`, `_v36`, `V49*`, and `V410*` `.exe`/`.obj` products. No exact filename references were found in scanned client `.cmd`, `.bat`, `.ps1`, `.py`, `.vcxproj`, `.props`, or `.json` files. This is not proof against every possible dynamically constructed reference; the archive preserves recovery.
- Preflight checked every source size/hash, confined paths to the explicit directories, rejected destination collisions/reparse-point source files, and checked that no target executable was running.
- Verified all 39 archived files against their original SHA-256 hashes after relocation.
- Rechecked the earlier 126-file protected inventory manifest: zero hash mismatches. The editor was running at initial inspection but was no longer listed at the final process check. No command in this pass stopped or restarted it; the reason for its exit was not investigated.

## Unchanged

No deletion, source/asset edits, Git staging, commit, push, build or test execution. The current editor/runtime, current test products, scripts, authored mesh/header, backup shapes, imported material backups, diagnostic evidence, PDBs, compiled resources and `_Temp` were left in place. The editor was not restarted or operated, so this is file-integrity validation, not a fresh visual/playthrough certification.

This is recoverable consolidation, not disk-space reclamation. The archive remains under ignored Build and is not a substitute for a reviewed external backup or Git checkpoint.

## Recovery

For any entry in the manifest, its former path is `sourceDirectory/Name` and its archived path is `archiveDirectory/Name`. Restore only after checking the original destination is absent and the archived hash matches. Never overwrite a newer file during restoration.

## Still pending

- Preserve/version the ignored authoring and test scripts in a maintained location, updating and verifying their relative paths.
- Decide which shape/material history and crash evidence needs long-term retention.
- Capture a reproducible build and behavior baseline before clearing intermediates or retiring current binaries.
- Review the source-root test artifacts separately; they were not moved in this pass.
- Retain unrelated client/EI/Phase 18 work. The user's playable-target instruction is not evidence that unrelated work is disposable.

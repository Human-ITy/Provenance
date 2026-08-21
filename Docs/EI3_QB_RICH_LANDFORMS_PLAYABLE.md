# EI3.Q.B Rich Causal Landforms — Play and Edit Guide

## Launch the playable stage

Double-click the plainly named desktop launcher:

`C:\Users\D-Day\Desktop\Provenance EI3 QB\PLAY EI3 QB - RICH LANDFORMS.cmd`

The source launcher it calls is:

`C:\Users\D-Day\.codex\.chatgpt-projects\g-p-6a3709c376088191918eed8315fa9751\.ei3q-provenance\PLAY_EI3_QB_RICH_LANDFORMS.cmd`

The same launcher works in both the certified integration checkout and the
consolidated workspace. It discovers the appropriate authority workspace and
does not silently fall back to a non-authoritative client-only mode.

The launcher starts the local authoritative FableScript process in the Q.B
physical-baseline mode, waits for the canonical authority and projection lanes,
then starts ProvenanceClient directly in the selectable **EI3.Q.B — Rich Causal
Landforms** stage.

Inside the client:

1. Press `M` to open the stage board.
2. The Q.B launcher starts directly in **EI3.Q.B — Rich Causal Landforms**, so
   the board opens centered on its current row.
3. If you opened the generic client instead, Q.B is board row **25**. Scroll
   upward when the footer says it is showing rows 26–41.
4. Press `Enter` to re-enter the highlighted stage if needed.

The stage deliberately fails closed in a normal Q.A authority session. Use the
Q.B launcher so the visible terrain, grounding, and detailed matter projection
all refer to the same Q.B `WorldBaselineIdentity`.

## Code ownership and edit locations

FableScript owns the physical world truth. The certified integration source is:

`C:\Users\D-Day\.codex\.chatgpt-projects\g-p-6a3709c376088191918eed8315fa9751\.ei3q-fablescript\fablescript\worldgen\world_substrate.py`

ProvenanceClient owns stage selection and presentation. Edit these certified
integration files:

- `C:\Users\D-Day\.codex\.chatgpt-projects\g-p-6a3709c376088191918eed8315fa9751\.ei3q-provenance\Code\Applications\ProvenanceClient\Main.cpp`
- `C:\Users\D-Day\.codex\.chatgpt-projects\g-p-6a3709c376088191918eed8315fa9751\.ei3q-provenance\Code\Applications\ProvenanceClient\Ei3DetailedProjection.h`

The one-click local-authority launch path is here:

- `Tools/Integration/Start-Ei3CanonicalPlayable.ps1`
- `PLAY_EI3_QB_RICH_LANDFORMS.cmd`

## Open and edit the project

The currently certified Q.B code is in the two integration folders above. It
has not yet been promoted into `C:\Users\D-Day\ProvenanceWorkspace`, so do not
edit that older copy expecting this Q.B stage to change.

For C++ client work, open this exact project in Visual Studio:

- `C:\Users\D-Day\.codex\.chatgpt-projects\g-p-6a3709c376088191918eed8315fa9751\.ei3q-provenance\Code\Applications\ProvenanceClient\Esoterica.Applications.ProvenanceClient.vcxproj`

For engine Python, launcher scripts, documentation, or mixed engine/client
editing, open both `.ei3q-fablescript` and `.ei3q-provenance` in Visual Studio
Code. The desktop folder contains an `OPEN EI3 QB CODE FOLDERS.cmd` helper.

- `.ei3q-fablescript\fablescript\worldgen\world_substrate.py`
- `.ei3q-provenance\Code\Applications\ProvenanceClient\Main.cpp`
- `.ei3q-provenance\Code\Applications\ProvenanceClient\Ei3DetailedProjection.h`
- `.ei3q-provenance\Tools\Integration\Start-Ei3CanonicalPlayable.ps1`

Build the standalone ProvenanceClient project in `Release | x64`. Do not build
the entire Esoterica solution just to edit or run this stage; optional upstream
profiling integrations are unrelated to ProvenanceClient.

Do not add physical outcrops, ledges, talus, banks, or other world objects only
in the client. New physical structure belongs in FableScript authority; the
client reconstructs and presents the accepted projection.

## Certification boundary

EI3.Q.B is certified for the seven emitted causal structure classes and the
matched player-view continuity gates. The certificate does not claim caves,
detached boulders, waterfalls, flora, snow, or weather; those remain future
engine-authority stages.

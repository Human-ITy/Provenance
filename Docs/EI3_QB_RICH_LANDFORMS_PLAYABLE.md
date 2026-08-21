# EI3.Q.B Rich Causal Landforms — Play and Edit Guide

## Launch the playable stage

Double-click `PLAY_EI3_QB_RICH_LANDFORMS.cmd` in the Provenance client repository.

The launcher starts the local authoritative FableScript process in the Q.B
physical-baseline mode, waits for the canonical authority and projection lanes,
then starts ProvenanceClient directly in the selectable **EI3.Q.B — Rich Causal
Landforms** stage.

Inside the client:

1. Press `M` to open the stage board.
2. Select **EI3.Q.B — Rich Causal Landforms** with the arrow keys or mouse wheel.
3. Press `Enter` to enter it.

The stage deliberately fails closed in a normal Q.A authority session. Use the
Q.B launcher so the visible terrain, grounding, and detailed matter projection
all refer to the same Q.B `WorldBaselineIdentity`.

## Code ownership and edit locations

FableScript owns the physical world truth. Edit the deterministic Q.B landform
laws here:

- `fablescript/worldgen/world_substrate.py` in the FableScript integration repo

ProvenanceClient owns stage selection and presentation. Edit those pieces here:

- `Code/Applications/ProvenanceClient/Main.cpp`
- `Code/Applications/ProvenanceClient/Ei3DetailedProjection.h`

The one-click local-authority launch path is here:

- `Tools/Integration/Start-Ei3CanonicalPlayable.ps1`
- `PLAY_EI3_QB_RICH_LANDFORMS.cmd`

## Open and edit the project

The consolidated workspace is `C:\Users\D-Day\ProvenanceWorkspace`.

For C++ client work, open this project in Visual Studio:

- `Client\ProvenanceClient\Code\Applications\ProvenanceClient\Esoterica.Applications.ProvenanceClient.vcxproj`

For engine Python, launcher scripts, documentation, or mixed engine/client
editing, open the entire `C:\Users\D-Day\ProvenanceWorkspace` folder in Visual
Studio Code. The main files for this stage are:

- `Engine\FableScript\fablescript\worldgen\world_substrate.py`
- `Client\ProvenanceClient\Code\Applications\ProvenanceClient\Main.cpp`
- `Client\ProvenanceClient\Code\Applications\ProvenanceClient\Ei3DetailedProjection.h`
- `Client\ProvenanceClient\Tools\Integration\Start-Ei3CanonicalPlayable.ps1`

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

# Hammer native integration

Source asset: `output/models/tools/hammer/v001/README.md`.

Installed resources: `Data/Provenance/Tools/Hammer/v001` under the client project root. The installation adds one entity named `ToolPickup_GuildHammer` to `Data/Provenance/ProvenanceSandbox.map`. `sandbox_before.map.txt` records the pre-install map. Existing entities were preserved.

`Geometry/ToolPickup.h` contains the independent selection/inventory rules. `Systems/GraniteLabToolPickup.h` adapts them to the game world and loaded static mesh. `Systems/GraniteOutcropLab.cpp` supplies input, terrain occlusion and prompts. This integration is restricted to the development lab and does not change F harvesting.

Reproduction order: build model and textures; prepare native report; run `PackageHammer.cjs` to emit `native_plan.json`; install with `install_resources.py`; run `compile_resources.ps1`; build the Debug Game Runtime project. Native installation and compilation write outside the asset workspace and must be performed with authorized project write access. Preserve unrelated map edits. The installer refuses conflicting resources and avoids adding a duplicate entity.

`resource_results.json`, `runtime_build.log`, `delivery_validation.json` and the model folder's `validation.json` record verification. The running editor may need the sandbox map reloaded and a new Play session. Interactive pickup validation remains to be performed in the editor.

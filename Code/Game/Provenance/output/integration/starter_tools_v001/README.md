# Starter tool integration

Models and placement instructions: `output/models/tools/starter_stone/v001/README.md`.

The corrected shovel, axe and pickaxe resources are installed under `Data/Provenance/Tools/StarterStone/v001`. Three named pickups were added to `Data/Provenance/ProvenanceSandbox.map`, preserving existing entities. `sandbox_before.map.txt` records the preceding map. `resource_results.json` records 56 successful native resource compiles.

`Geometry/ToolPickup.h` now tracks four independent tool kinds. Selection uses shaped stone-head prisms and shaft/grip capsules. `Systems/GraniteLabToolPickup.h` selects the nearest eligible tool and hides only the collected item. `Systems/GraniteOutcropLab.cpp` supplies E input, terrain obstruction and named HUD messages. Existing F harvesting is unchanged. Ownership remains Play-session only.

The corrected source compiled, but the installed runtime DLL was locked by the project's resource server during linking (`LNK1168`). A complete replacement was linked separately in `staged/Esoterica.Game.Runtime.dll` using `StageRuntime.targets`; its log has the expected output-location warning because this is a temporary delivery destination. `delivery_validation.json` records whether the corrected runtime has subsequently been installed. No running application was stopped by this task without authorization.

After releasing this checkout's resource-server/compiler locks, rebuild the Game Runtime project normally (without the staging targets) to publish to `Build/x64_Debug`. This is preferred over copying a stale staged file if other source changes occurred meanwhile. Preserve unrelated work in the shared checkout. Then reload the sandbox and start Play for the interactive check.

Reproduction: run the model builder and validator; run `ProbeStarterTools.cmd`; run `prepare_native.py`; emit `native_plan.json` with `PackageStarterTools.cjs`; run `install_resources.py`; compile resources; build the game runtime. Native installation writes outside the model workspace and needs authorized project access. The installer refuses conflicting resource files and does not create duplicate entities.

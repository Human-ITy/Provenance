# Living-World Load Harness

Base: Stage 12 commit `2c51bfe9`.

This is a performance-load ladder, not permission to add game authority. Every
level reuses the certified Stage-12 terrain and adds only one bounded workload
family. A level must pass before the next level is admitted.

| Level | Added workload | Authority boundary |
|---|---|---|
| L0 | Terrain only | Immutable Stage-12 control |
| L1 | Textured crossed grass cards | Synthetic presentation/admission only |
| L2 | Trees | Static/instanced only |
| L3 | Rocks and props | Static presentation/collision proxies only |
| L4 | Animals | Locomotion and animation only |
| L5 | NPCs | Movement, animation, and cheap perception only |
| L6 | Combined | L1-L5 together, with no added behavior |

Each level runs north, east, south, and west in four movement modes: walk,
sprint, 240 m/s free flight, and 480 m/s free flight. Every case must retain a
complete 192 m live-terrain radius with no holes, fallback, authority mismatch,
collision mismatch, or movement frame above 16.667 ms.

Receipts report per-system CPU time, active/culled/LOD or impostor counts,
worker backlog, resident packages, completeness, and player-facing frame-time
statistics. The L1/L6 grass receipt additionally records candidate cells/sites,
host admission and each rejection channel, active clumps, two/three-card LODs,
submitted cards/triangles, the required texture dimensions/digest, and a saved
player-scale visual. Cold initialization and final image readback are excluded
from the movement-frame gate but remain visible as separate operations.

Explicitly closed in this harness: water coupling, mutation, ecological
succession, full animal or NPC behavior, macro geography, Stage 13, and Stage
14. Synthetic workloads may measure capacity; they may not mint world truth.

Run the control with `CERT_LIVING_WORLD_LOAD_0.cmd`. Its permanent receipt is
`Docs/provenance_living_world_load_0_cert.txt`.

Run presentation-only grass with `CERT_LIVING_WORLD_LOAD_1.cmd`. Five
deterministic candidate sites per one-metre cell are admitted only on the
fixture's sandstone/shale caps below the declared slope threshold. Survivors
are short, surface-snapped, alpha-tested crossed cards: two cards generally and
an optional third card inside the near band. The cells are submitted through
one linear client-array draw. This admission is a representative load mask,
not evidence of soil, moisture, growth, succession, or plant authority. Grass
has no collision, occupancy, support, persistence, or gameplay state.

L1 and L6 require `Assets/Vegetation/grass_tuft.png` at 256 x 256. A missing or
different-size workload texture fails the certificate instead of silently
substituting a cheaper render path. Runtime captures are buffered outside the
measured route and written only when the complete certificate ends.

Run static trees with `CERT_LIVING_WORLD_LOAD_2.cmd`. L2 uses deterministic,
surface-snapped near/mid/impostor tree proxies. They are presentation instances,
not plant actors, material bodies, collision, support, or growth authority.

Levels L3-L6 use the matching `CERT_LIVING_WORLD_LOAD_<level>.cmd` launcher.
L3 is static rocks/props; L4 is bounded animal locomotion/animation; L5 is
bounded NPC movement/animation plus fixed-cost cheap perception; L6 combines
the already measured L1-L5 workloads. These remain synthetic capacity probes,
not new world, ecology, body, or gameplay authority.

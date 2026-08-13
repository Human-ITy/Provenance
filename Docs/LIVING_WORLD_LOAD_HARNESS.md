# Living-World Load Harness

Base: Stage 12 commit `2c51bfe9`.

This is a performance-load ladder, not permission to add game authority. Every
level reuses the certified Stage-12 terrain and adds only one bounded workload
family. A level must pass before the next level is admitted.

| Level | Added workload | Authority boundary |
|---|---|---|
| L0 | Terrain only | Immutable Stage-12 control |
| L1 | Grass | Presentation only |
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
statistics. Cold initialization and final image readback are excluded from the
movement-frame gate but remain visible as separate operations.

Explicitly closed in this harness: water coupling, mutation, ecological
succession, full animal or NPC behavior, macro geography, Stage 13, and Stage
14. Synthetic workloads may measure capacity; they may not mint world truth.

Run the control with `CERT_LIVING_WORLD_LOAD_0.cmd`. Its permanent receipt is
`Docs/provenance_living_world_load_0_cert.txt`.

Run presentation-only grass with `CERT_LIVING_WORLD_LOAD_1.cmd`. Its grass is
an absolute, deterministic, surface-snapped render grid. It has no collision,
occupancy, support, persistence, growth, moisture, or ecological authority.

Run static trees with `CERT_LIVING_WORLD_LOAD_2.cmd`. L2 uses deterministic,
surface-snapped near/mid/impostor tree proxies. They are presentation instances,
not plant actors, material bodies, collision, support, or growth authority.

Levels L3-L6 use the matching `CERT_LIVING_WORLD_LOAD_<level>.cmd` launcher.
L3 is static rocks/props; L4 is bounded animal locomotion/animation; L5 is
bounded NPC movement/animation plus fixed-cost cheap perception; L6 combines
the already measured L1-L5 workloads. These remain synthetic capacity probes,
not new world, ecology, body, or gameplay authority.

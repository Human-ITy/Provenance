# Living-World Load Ladder Handoff

Date: 13 August 2026  
Stage-12 base: `2c51bfe9`  
Status: L0-L6 exact-tree certification PASS; harness is a separate cut above
the frozen Stage-12 base.

## Ruling

The certified 192 m live-terrain streamer has demonstrated representative
non-terrain game-system headroom. The combined synthetic load remained below
the requested 12-14 ms safety target without weakening terrain completeness,
authority, collision, or presentation continuity.

| Level | Isolated workload | Representative active count | Worst movement frame |
|---|---|---:|---:|
| L0 | terrain only | 2,601 terrain packages | 9.165 ms |
| L1 | presentation grass | 5,518 blades | 10.123 ms |
| L2 | static trees | 230 trees | 6.366 ms |
| L3 | static rocks/props | 498 props | 8.183 ms |
| L4 | animal locomotion/animation | 20 animals | 8.187 ms |
| L5 | NPC movement/animation/cheap perception | 10 NPCs | 7.868 ms |
| L6 | L1-L5 combined | all counts above | 10.771 ms |

Every level ran 16 cases: walk, sprint, 240 m/s free-flight, and 480 m/s
free-flight toward north, east, south, and west. Across the final exact-tree
ladder:

- all 112 cases passed;
- the 192 m live-terrain radius stayed complete;
- resident terrain remained bounded at 2,601 packages;
- there were zero terrain holes or fallback pixels;
- there were zero authority or collision mismatches;
- there were zero movement frames above 16.667 ms;
- per-system time, active/culled/LOD counts, worker backlog, and minimum complete
  terrain radius were recorded in each permanent receipt.

## Authority boundary

This harness does not implement game features. Grass, trees, rocks, animals,
and NPCs are deterministic workload probes made from existing presentation
primitives. They create no ecology, growth, felling, body, history, persistence,
water, mutation, macro-geography, Stage-13, or Stage-14 authority.

## Continue here

1. Preserve `2c51bfe9` as the read-only Stage-12 base.
2. Preserve this living-world harness as its own cut.
3. Keep the L0-L6 launchers and receipts as permanent performance controls.
4. Because every isolated level and L6 passed with safety margin, there is no
   isolated workload blocker to optimize now.
5. The next architecture cut may be selected between Stage 13/14 and the causal
   macro-geography floor; neither was opened by this harness.

Receipts: `Docs/provenance_living_world_load_0_cert.txt` through
`Docs/provenance_living_world_load_6_cert.txt`.

# Living-World Load Ladder Handoff

Date: 13 August 2026  
Stage-12 base: `2c51bfe9`  
Status: L0-L6 certification PASS; the L1 textured grass-card cut is a separate
cut above the frozen Stage-12 base and the original load-ladder commit.

## Ruling

The certified 192 m live-terrain streamer has demonstrated representative
non-terrain game-system headroom. The textured grass-card prototype preserves
the 60 FPS hard floor without weakening terrain completeness, authority,
collision, or presentation continuity. Its combined L6 worst-of-three is
15.724 ms, so the hard gate is closed but the former 12-14 ms comfort target is
not. That slim margin is recorded rather than rounded into extra headroom.

| Level | Isolated workload | Representative active count | Worst movement frame |
|---|---|---:|---:|
| L0 | terrain only | 2,601 terrain packages | 9.165 ms |
| L1 | textured crossed grass cards | 18,912 clumps / 77,554 triangles | 12.233 ms (3 runs) |
| L2 | static trees | 230 trees | 6.366 ms |
| L3 | static rocks/props | 498 props | 8.183 ms |
| L4 | animal locomotion/animation | 20 animals | 8.187 ms |
| L5 | NPC movement/animation/cheap perception | 10 NPCs | 7.868 ms |
| L6 | L1-L5 combined | grass above + prior L2-L5 counts | 15.724 ms (3 runs) |

Every level runs 16 cases: walk, sprint, 240 m/s free-flight, and 480 m/s
free-flight toward north, east, south, and west. Across the final exact-tree
ladder:

- the grass-card cut repeated L1 and L6 three times each: all 96 cases passed;
- the 192 m live-terrain radius stayed complete;
- resident terrain remained bounded at 2,601 packages;
- there were zero terrain holes or fallback pixels;
- there were zero authority or collision mismatches;
- there were zero movement frames above 16.667 ms;
- per-system time, active/culled/LOD counts, worker backlog, and minimum complete
  terrain radius were recorded in each permanent receipt;
- grass admission, rejection channels, clumps, cards, triangles, texture digest,
  and saved L1/L6 visual receipts were recorded explicitly.

## Authority boundary

This harness does not implement game features. Grass, trees, rocks, animals,
and NPCs are deterministic workload probes made from existing presentation
primitives. They create no ecology, growth, felling, body, history, persistence,
water, mutation, macro-geography, Stage-13, or Stage-14 authority. The grass
host gate is deliberately named `synthetic_sedimentary_low_slope`; it is not a
biological claim that bare sandstone or shale should grow grass.

## Continue here

1. Preserve `2c51bfe9` as the read-only Stage-12 base.
2. Preserve this living-world harness as its own cut.
3. Keep the L0-L6 launchers and receipts as permanent performance controls.
4. The immediate-mode driver path exposed periodic 60-95 ms GPU-completion
   stalls during repeats. The accepted cut retains the same card geometry but
   emits one linear client-array batch. No such stall appears in the final six
   runs.
5. L6's 0.943 ms worst-run margin to 16.667 ms is sufficient for this prototype,
   not a claim of final ecology headroom. If grass graduates beyond this fixture,
   move it to persistent GPU buffers/instancing before increasing density.
6. The next architecture cut may be selected between Stage 13/14 and the causal
   macro-geography floor; neither was opened by this harness.

Receipts: `Docs/provenance_living_world_load_0_cert.txt` through
`Docs/provenance_living_world_load_6_cert.txt`.

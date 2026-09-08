# Structural support reuse and sustained action checks

Date: 2026-09-07. Headless assessment; no playable executable rebuilt or published.

## Status

Experimental boundary completion is implemented but OFF by default. Ordinary
`GraniteStructuralSupport::Apply` and `PrepareStrike` callers still use the
existing full-boundary rebuild. The differential tests explicitly opt into
reuse with the final boolean argument set to false.

The objective is faster repeated accepted actions, not a claim based on capped
FPS. Do not count an empty soil column or a refused cut as a fast removal.

## Avoidable work found

A compact support graph already calculates connectivity, component volume and
cross-cell contact intersections. When a remnant releases, the existing path
builds the full graph again to obtain boundary ports. `CompleteBoundary` adds
the missing internal ports and cached bonds, reusing the previously calculated
cross-cell polygons. It preserves the full rebuild's port and portal ordering.

The first experiment represented the same contacts but changed their ordering;
the broader exact tests caught that difference. The corrected version also
reproduces bucket traversal order. Do not cite the initial experiment as final
validation. Expanded graphs are boundary-use-only: their edges retain the
already-solved compact representation and must not be solved again.

No support thresholds, anchors, grade rules or material density were weakened.
No persistent cache was introduced. There are additional transient port maps;
peak allocation and live-runtime latency remain unmeasured.

## Results

| Check | Result |
| --- | --- |
| Corrected 32 sequential strikes | 464,964 checks pass; 18 remnant-release events; no refusals |
| 32-strike total support CPU | Reference 2,214.590 ms; candidate 1,772.518 ms: 19.96% lower |
| Broader support fixtures | 194 checks pass in each of /O2 and /Od, including actual large detached bodies and exact full-boundary expansion |
| 64 sequential attempts | 1,347,891 checks pass before final geometric-volume assertion fails; reference/candidate transactions match throughout |
| Extended soil sequence | 29 accepted digs, 3 empty-column attempts, exact cumulative milligrams and unchanged granite ledger |
| Existing soil regression | 203,562 checks pass in each of /O2 and /Od |

The 32/64 differential route releases tiny remnants, not large falling islands;
large-body coverage comes from the broader support fixture suite. Timings
alternate reference/candidate order, with transaction copies, cutting and
diagnostic volume measurements outside support timers. Exact comparisons cover
decisions, identities, mass, volumes, ordered retained/chip geometry and render
patches. These comparisons do not independently prove every runtime subsystem.

An initial 64-run measured reference 7,121.140 ms versus candidate 5,790.867 ms
(18.68% lower total support CPU). Later diagnostic runs reproduced the failure
and similar totals. This is not an end-to-end strike speedup. Work still grows:
the sequence's support node count rises from 299 to 17,736. Eliminating the
second build helps release events but does not remove this growth.

## Failing long-run certificate: retain, do not relax

Zero-based attempt 36 (37th attempt) is rejected by the cut phase with
`REFUSED: cavity/cast volume mismatch`, before support runs. Both paths match.

Zero-based attempt 46 (47th attempt) is the first to cross the existing final
volume tolerance of 1e-7 m3. Residual after cutting is 7.25208018081958e-8 m3;
after support, it is 1.74775346195588e-7 m3. Two tiny remnants are credited on
this action. The measured boundary/accounting discrepancy is therefore enlarged
by the release stage; its exact geometric cause remains unresolved.

Final 64-attempt residual: 1.74796049634551e-7 m3, about 0.175 cm3. The integer
milligram ledger matches exactly, but that does NOT override the geometric
failure. Exact transaction parity from the same initial state means this is
also present on the reference trajectory, not unique to reuse. A separate
reference-only 64-body run was not performed.

The test intentionally returns failure. Do not widen the tolerance or present
the long-run certificate as passing. Production stays on the prior path while
this is investigated.

## Digging measurements

Four sites receive eight downward attempts each, with edits retained between
sites. Attempts 23, 30 and 31 miss; independent material-field samples at 1 mm
spacing along those columns find no positive material. This sampling is a
diagnostic, not an exhaustive proof for arbitrary geometry. Those attempts are
explicitly logged and excluded from successful-removal timings.

Latest accepted dig preparation: 10.214–48.774 ms; support portion:
0.023–10.178 ms. Cumulative recovered soil: 128,355,852 mg. These figures are
baseline measurements, not an optimization claim. The existing horizontal
22-cut bore regression also passes: 1.593 m advancement, maximum 8,912 surface
triangles, optimized worst preparation 16.62 ms on this run. That fixture differs
from the downward sequence and must not be compared as an A/B speedup.

Eight profiled reference picks still spend roughly 42–44 ms rebuilding walking
collision, mostly in the whole collision tree. Headless commit timings exclude
the input queue, worker scheduling, GPU publication and frame presentation.

## Next work, in order

1. Inspect the 47th attempt's remnant boundary reconstruction: compare retained
   solid volume, published boundary and credited remnant volume before/after
   release. Separately preserve the 37th-attempt cavity mismatch reproduction.
2. Once geometric checks pass, repeat support comparisons across more strike
   routes and large releases, including allocation measurements. Then consider
   enabling reuse; never skip support merely to accelerate repeated hits.
3. Assess truly incremental support connectivity over affected components.
   Invalidation must follow severed contacts through any newly disconnected
   region, including connections to buried anchors; a fixed-radius shortcut
   alone is not sufficient. Measure retained memory and worst-case expansion.
4. Prototype local walking-collision updates against the full reference. Verify
   all support-release replacements and movement behavior before publication.
5. Measure accepted-input-to-visible-result and sustainable action rate in the
   playable runtime, including queueing and main-thread publication.

## Reproduction and evidence

- `Scripts/RunGraniteSupportReuseTest.cmd 32`: passing differential baseline.
- `Scripts/RunGraniteSupportReuseTest.cmd 64`: expected failing long-run repro.
- `Scripts/RunGraniteStructuralSupportTest.cmd`: broad support fixtures.
- `Scripts/RunTerrainResponseBaseline.cmd`: phases plus sustained digging.
- `Scripts/RunSoilScoopTest.cmd`: existing soil regressions.
- `Artifacts/SupportReuse/support-reuse-32-2026-09-07.txt`
- `Artifacts/SupportReuse/support-reuse-64-2026-09-07.txt`
- `Artifacts/SupportReuse/support-reuse-64-drift-2026-09-07.txt`
- `Artifacts/SupportReuse/support-reuse-64-phases-2026-09-07.txt`
- `Artifacts/Landscape/repeated-dig-profile-2026-09-07.txt`

This record supersedes the previous profile's immediate-next ordering: support
reuse has now been tried, but the extended geometric check precedes publication.

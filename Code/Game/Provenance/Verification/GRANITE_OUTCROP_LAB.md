# Walk-up granite outcrop prototype — V5.9

2026-09-07 landscape integration update: see
`MEADOW_SEAM_TREE_TRIAL_V1.md` for exterior grass streaming, intact edge shading,
the 14-tree native visual trial, GPU texture-mip correction, and live stationary
frame-time evidence. This does not extend the editable soil ledger or implement
tree harvesting.

The visible V5.9 surface is now also available through the explicit OBJ
round-trip documented in `GRANITE_OUTCROP_AUTHORING.md`. Artist-authored crest
and face heights are baked into `GraniteOutcropAuthoredShape.h`; exact mass is
reconciled only through the buried root, never by rescaling the visible mesh.

## V5.9: fractured mass, buried saddles and contact staining

The supplied paint-over now drives the authoritative silhouette rather than
serving only as a shading reference. The main outcrop is composed from a steep
left crown, a lower right crown, a broken central saddle and an angular front
apron. Rotated high-order plane fields produce sharper ridge changes and broad
faceted sectors without adding a separate decorative shell. The resulting
crest is 1.465 m (4.81 ft) above datum. Its complete connected body remains
exactly 9.000 m3 / 24,300 kg and the closed exterior agrees with the tetrahedral
matter to floating-point precision across every tested seed.

Two structural joints now cut approximately 25–28 mm into the real surface.
Their material centerlines are about 12 mm wide and nearly black-brown, so the
recess itself carries the contrast while the adjacent granite retains its
ordinary mineral response. The overall granite range is darker and more varied
between cool gray and warm feldspar. Orange-brown earth staining is stronger
where the deterministic soil heightfield physically meets the base, then fades
over the first 30 cm rather than forming a constant world-height stripe.

The three granite saddle roofs remain geologically connected below grade, but
broader soil mantles again cover every visible neck between the master mass and
its satellite crowns. The general perimeter wash is narrower and more patchy.
Only the front-left joint receives a small tapered wash fan, avoiding the prior
three uniformly raised wedges while preserving a localized drape into the
crease. Granite and soil are still authoritative, continuous heightfield-driven
matter rather than overlapping presentation meshes.

The steeper silhouette exposed a traversal edge case in which one axis could
advance by a few millimeters while the intended wall tangent remained stalled.
The bounded tangent search now also handles these nearly-stalled frames. Its
step-clearance bisections use five iterations, which resolves the full 22 cm
step range to under 7 mm while reducing repeated capsule probes. The elevated
excavated-lip route advances 1.815 m with zero stalled frames and 39 probes at
worst.

Verification on 2026-09-05: optimized and Debug outcrop certificates pass
8,684 checks, grass passes 32,839 checks, movement passes 34 checks, contact
casting passes 20 checks, structural support passes 145 checks, local updates
pass 2,712 checks, soil passes 203,562 checks, and debris passes 303 checks.
The granite material compiles successfully and the complete Debug editor build
succeeds.

## V5.8: mineral granite, alluvial fans and fractured crowns

Granite no longer inherits the source albedo's chalk-white value uniformly.
Two stable world-space mineral fields preserve its authored grain while
shifting broad regions between cool quartz gray and warmer feldspar. The
initial seeded soil heightfield is mirrored by the granite material, so earth
staining now follows actual bank contact rather than a generic elevation band.
It is strongest on low side faces touched by soil and fades within a few
inches above the physical bank.

The two authored structural recesses are geometrically deeper and slightly
narrower. Their material response is correspondingly confined to a darker,
rougher centerline; adjacent lowered granite retains its ordinary mineral
value. Satellite crowns now use three more distinct facet rotations and two
different exposure levels while remaining connected through authoritative
granite below the soil.

Perimeter wash is more intermittent. Most of the boundary receives a thin,
irregular mantle, while three tapered alluvial fans climb toward the buried
satellite saddles. The nearest fan reaches 0.388 m above datum compared with
0.305 and 0.315 m on its cross-fan flanks. This is real contiguous heightfield
relief, not a decal or a separate mound mesh, and it remains smoothly walkable
at the fixed 12 cm terrain sampling.

The pile certificate still settles 24 simultaneous granite chips under its
three-millisecond average-frame budget and sleeping piles remain effectively
free. Structural graph construction now avoids the reverse duplicate of every
unchanged shared-face union. More aggressive component caching and flood-pass
experiments were rejected because they either increased retained memory or
measured slower; strike preparation remains the next performance target.

Verification on 2026-09-05: optimized and Debug outcrop certificates pass
8,689 checks, grass passes 32,839 checks, movement passes 34 checks, contact
casting passes 522 checks, structural support passes 144 checks, local updates
pass 2,603 checks, and debris passes 303 checks. The granite material compiles
successfully and the complete Debug editor build succeeds.

## V5.7: angular upper crowns with traversal-safe skirts

The upper and rear portions of the authoritative main outcrop now transition
into rotated six-sided macro facets. Satellite crowns use the same angular
construction more sparingly over broader buried skirts. The result introduces
planar-looking sectors and ridge changes at geological scale without adding
chip-sized surface noise or separating any crown from the common granite
body. The front skirt deliberately retains its gentler grade so walking along
the face and across excavated cavity lips does not inherit an artificial
collision wedge.

The angular field uses a smooth high-order plane norm rather than a hard
maximum. That distinction keeps the visual ridge breaks while preventing
inverted tetrahedra at the fixed authoritative sample spacing. Stored matter,
the closed exterior, collision, excavation and recovered chips therefore
continue to agree exactly at 9.000 m3 / 24,300 kg. The crest is 1.327 m
(4.35 ft) above datum and the buried root reaches 0.474 m below datum.

The cavity-lip movement certificate now names the important failure mode:
isolated support-plane transitions are permitted, but the capsule must keep
advancing without a direction change and may never enter a persistent catch.
The V5.7 route advances 1.735 m with three isolated contact frames and eight
probes at worst.

Verification on 2026-09-05: optimized and Debug outcrop certificates pass
8,689 checks, grass passes 32,838 checks, movement passes 34 checks, contact
casting passes 522 checks, structural support passes 144 checks, and local
updates pass 2,578 checks. The complete Debug editor build succeeds.

## V5.6: settled soil banks and buried satellite necks

Washed soil is now geometric heightfield relief, not only a grass-cover
clearing. A continuous low-frequency field raises and lowers the bank around
the granite perimeter, while three broad deposits cross the shallow saddles
between the main outcrop and its satellite crowns. The crowns remain visibly
separate above ground, but their authoritative granite connections remain
hidden below the single contiguous soil surface.

The central body remains exactly 9.000 m3 / 24,300 kg and reaches 1.317 m
(4.32 ft) above datum. Satellite exposure is controlled independently from
the central mass scaling, preventing an increase in central granite from
pulling their connecting necks back through the soil. The body remains closed
and conserves exactly through excavation.

Blade cards now reject fragments outside their true 0–1 card coordinates
before atlas sampling. This prevents the card's 4 mm ground offset from
clamping its upper edge onto neighboring atlas texels and stretching those
texels into floating rectangular strips. Existing atlas alpha and blade
silhouettes remain unchanged.

Verification on 2026-09-05: optimized and Debug outcrop certificates pass
8,689 checks, grass passes 32,838 checks, movement passes 34 checks, contact
casting passes 522 checks, structural support passes 145 checks, and local
updates pass 2,627 checks. The grass material compiles successfully and the
complete Debug editor build succeeds.

## V5.5: four-foot crown, patchy mineral soil and narrow creases

The seeded body now owns exactly 9.000 m3 of authoritative granite, or
24,300 kg at 2700 kg/m3. Its visible crest is 1.295 m (4.25 ft) above datum
and its structural root reaches 0.449 m below datum. The same tetrahedra still
drive appearance, contact, excavation, recovered chips and both volume and
integer-mass conservation. The larger form does not add a finer partition or
extra collision grid: normalization grows the crown and buried root around
local grade, preserving the surface saddles that allow soil to pass over the
three buried satellite connections.

Soil remains a single deterministic, contiguous heightfield. Exposed mineral
ground is an ecology/cover decision rather than a second terrain mesh. Its
clearance from granite now varies continuously from close-growing grass to
broader settled pockets, avoiding the former constant-width moat. Grass roots
still cannot cross the original granite envelope, while blades rooted in valid
soil may lean across it.

The granite material now uses a stronger, warmer irregular earth stain at the
base. Its two authored crease responses are confined to narrow recess centers;
the broader granite surrounding an indent keeps the ordinary granite response.

Verification on 2026-09-05: optimized and Debug outcrop certificates pass
8,689 checks. The initial boundary is exactly 9.000 m3, and the excavated
boundary plus recovered chips returns exactly 9.000 m3. Hidden acceleration
partitions remain bounded without changing the independent pick-scale contact
cutter. Four-bin render ownership holds the taller solid to 122 procedural
groups after 128 strikes. Grass passes 32,837 checks with 0.706 m of smooth,
bounded terrain relief. Contact-cast, movement, cavity-lip, local-update and
structural-support certificates also pass. The granite material compiles and
the complete Debug editor build succeeds.

## V5.4: buried granite ridges, settled soil and isolated atlas tiles

The deterministic test seed now declares exactly 6.000 m3 of granite, or
16,200 kg at 2700 kg/m3. The added matter is not decorative: it enters the
same excavation and conservation ledger as the former 4.000 m3 body. The
visible surface is a lower, broader asymmetric outcrop with broad angular
plane breaks and two erosion creases. Three narrower satellite ridges remain
connected to the master body below grade, while explicit low granite saddles
allow the continuous soil heightfield to pass over the connections. They read
as separate crowns at the surface without becoming independent rocks.

The local soil mantle now has deterministic edge wobble and small settled
banks. It rises irregularly against portions of the granite instead of tracing
a level perimeter. Initial grass cover is softly reduced through a seeded
0.05–0.205 m (roughly two-to-eight-inch) mineral margin beside exposed rock.
That band is habitat initialization only: it is not counted as excavation,
wear, or lost soil. Tuft roots still cannot start inside granite, although a
blade whose base is in valid soil may lean across the rock above it.

Granite shading is slightly darker overall, with a restrained earth stain near
the first exposed foot, darker crease response, and more legible sheltered
lichen. The stain and lichen are irregular world-space fields rather than
uniform height rings. Grass vigor—not terrain elevation—controls both the
existing 12–18 inch blade-height variation and its warmer-to-richer-green
grading; terrain height only anchors the tuft base to the continuous field.

Both supplied 1536x1024 blade atlases already contain genuine RGBA
transparency and remain unchanged. Their silhouettes cross some 4x2 atlas-cell
boundaries, which caused detached opaque strips when a single variant was
sampled. The grass shader now applies a half-texel inset and per-variant spill
masks so those detached boundary fragments are discarded without baking or
replacing either source PNG.

Verification on 2026-09-05: optimized and Debug outcrop certificates pass
8,683 checks with exact 6.000 m3 conservation across excavation and all tested
seeds. The maximum hidden partition is 9.26 inches across the additional seed
set. Grass passes 32,837 checks, including deterministic localized mineral
margin coverage and 0.706 m of bounded terrain relief. Both revised materials
compile successfully, and the complete Debug editor build succeeds.

## V5.3: embedded satellite crowns and habitat variation

Three small granite crowns now emerge around the main apron. They are connected
through its buried substrate and participate in the same excavation, collision,
support, and conservation geometry; they are not decorative meshes. Final
height normalization redistributes the silhouette while preserving the seeded
body at exactly 4.000 m3 / 10,800 kg. A regression verifies all three crowns
remain visibly exposed above the continuous soil heightfield.

Grass vigor now combines deterministic low-frequency moisture, local terrain
slope, and non-lattice tuft variation. It controls blade height within the
existing 12–18 inch contract and carries into the material as a sub-cell code:
short exposed tufts are subtly warmer, while sheltered vigorous growth is a
deeper green. Tuft positions, density, root exclusion, and cover-cell ownership
remain unchanged. Granite receives sparse, low-elevation sheltered lichen from
stable world-space fields, with a restrained color and roughness response.

Optimized and Debug certificates pass 16,451 grass checks, 8,689 outcrop checks,
522 contact-cast checks, 3,626 local-update checks, and 143 structural-support
checks. Both updated materials compile successfully.

## V5.2: granite-clear grass roots and rolling seeded field

The actual two crossed base segments of every deterministically jittered tuft
are tested against the original granite envelope with a 6 mm clearance. A tuft
is omitted only when its root footprint crosses or lies inside granite; cards
rooted in valid soil remain free to lean across the rock above their base. The
original envelope is intentional: excavating a cavity does not reveal dormant
grass that was never rooted in soil.

Grass grading preserves more atlas color, adds restrained saturation, and bends
the result toward a deeper moss green while retaining the full blade detail and
alpha. The surrounding heightfield now has 0.723 m (2.37 ft) of deterministic
broad relief. Relief fades out through the outcrop's immediate soil mantle, so
the continuous buried seam and exact 4.0 m3 granite ledger do not change. Its
largest sampled rise is 0.078 m per 0.12 m heightfield interval.

The optimized and Debug grass certificates pass 16,449 checks. Soil excavation,
movement, contact, outcrop geometry, and structural-support regressions also
pass; the Debug game runtime and grass material compile successfully.

## V5.1: buried structural root and safe soil publication

The generated granite's buried basal elevation is now the structural anchor.
Anchoring is no longer hard-coded to world Z=0, so an ordinary first strike
cannot release and lift the complete outcrop. A first-strike regression requires
a pick-scale cavity and chip with the rooted host retained. The continuous soil
mantle rises farther around the geological perimeter, hiding the rectangular
substrate edge. Edited soil and loose-clod meshes use compact optimized meshlets
to avoid the renderer's 16-bit cluster-center boundary case.

## V5.0: seeded natural outcrop

- Replaces the upright wall silhouette with a deterministic, connected granite
  deposit modeled after the supplied reference: a broad left crown, shallow
  center saddle, lower right shoulder, and ground-level front apron.
- The generated body is 3.60 m wide, 2.00 m deep, and about 1.06 m exposed at
  its crown. Its substrate continues 0.22 m below the local terrain datum, so
  the continuous soil heightfield buries the perimeter instead of exposing a
  rectangular pedestal or terrain seam.
- This test seed now owns exactly 4.0 m3 (10.8 tonnes at 2700 kg/m3), up from
  the former wall's measured 3.303840347385 m3. All added granite enters the
  initial ledger and remains subject to the same exact excavation conservation.
- The buried acceleration partition was coarsened without changing the 1 cm
  constructive excavation cells. This keeps the broader shape from inflating
  local collision/support ownership.
- A rock-support wake now clears stale rest/watchdog timers, preventing a body
  whose support disappeared from waking and immediately sleeping in one frame.

## V4.10: quiet terrain piles and loose-chip aim

- A soil revision no longer wakes every settled granite chip resting anywhere
  over previously edited terrain. Settled piles remain immutable visual debris
  until explicitly targeted; active and newly released chips still collide with
  the current soil surface normally.
- Loose-chip targeting has a 0.5-inch allowance around the chip's exact
  triangle geometry. A slight near miss while aiming at a small ground chip is
  therefore much less likely to become an unintended shovel scoop.
- These rules remove the captured 33-body terrain wake (`debris` 1,893 ms plus
  `poses` 401 ms) without deleting pieces or changing their accounted mass.

## V4.9: quiet settled piles and optimized support worker

- Ordinary player walking no longer wakes settled granite chips. Settled piles
  remain non-blocking visual debris, so crossing a pile cannot tear one chip out
  of an immutable sleeping batch and trigger a one-frame mesh republication.
  Looking at a loose chip and pressing F remains the explicit way to wake it.
- The structural-support preparation header is compiled as an optimized,
  worker-only hot path in Debug editor builds. This retains the same contact,
  gravity-load, remnant and conservation decisions while reducing the growing
  delay between strikes caused by evaluating tens of thousands of support
  nodes under the solution-wide `/Od` setting.
- One strike input made while the worker or surface publication is busy is now
  retained and starts automatically as soon as the current revision commits.
  Slow accumulated support analysis no longer silently eats the next click.
- Process memory shown by Visual Studio is the high-water allocation of the
  whole editor process. Modified granite storage is reported separately on the
  HUD; allocator/resource retention across preview runs need not return the
  process graph to its initial baseline until the editor exits.

## V4.8: exact chip support and cascade waking

- A settled chip resting in a granite cavity now records the precise supporting
  contact point and normal. After another strike changes nearby rock, that
  contact is re-raycast against the new surface. Chips whose actual support
  remains no longer wake merely because they share a coarse edited cell.
- If a strike genuinely removes support from several sleeping chips, at most
  one enters motion per frame. The pile responds as a short physical cascade
  instead of jumping simultaneously and rebuilding several sleeping batches in
  one frame.
- The traversal HUD reports the number of rock-revision wakes beside debris
  physics, allowing this path to be distinguished from player brushing.

Verification on 2026-09-04: the full Debug solution builds successfully.
Debris passes 301 checks in optimized and unoptimized builds, including exact
retained support and deferred unsupported wake coverage. Movement remains at 34
passing checks.

## V4.7: cavity-lip walking and bounded chip wakes

- Every committed excavation revision now owns one closed, current-surface
  walking BVH, prepared on the fracture worker. A capsule crossing holes no
  longer switches back to separate edited-cell queries and untouched
  tetrahedron reconstruction.
- The raised-feet regression places the capsule bottom at the chipped-hole
  elevation and slides it across the cavity lips. It traverses 1.835 m with no
  stalls, at most six probes per frame, and a 0.442 ms worst unoptimized Debug
  frame.
- Granite debris remains excluded from player capsule and step-height
  authority. Walking contact only brushes it. Each chip now has an immutable
  local collision BVH, so exact five-micron wake contact does not scan and
  transform its entire fracture mesh.
- Walking wakes at most one contacted granite chip per frame. This bounds both
  newly active debris physics and sleeping-batch mesh publication while the
  player continues naturally pushing through a pile.
- The HUD now retains traversal component peaks for walking, chip brushing,
  debris physics, and chip poses, making any remaining transient attributable.

Verification on 2026-09-04: the full Debug solution builds successfully.
Movement passes 34 checks in optimized and unoptimized builds; exact contact
passes 18, local updates pass 3,150, and debris passes 297 in both
configurations.

## V4.6: bounded front-face wall sliding

- An equal-height terrain floor is no longer mistaken for a tread above a
  blocked granite wall. Pressing against the outcrop therefore skips the
  pointless lift and grounding searches that multiplied capsule probes.
- Horizontal subdivision is based on actual travel distance. Ordinary 60 FPS
  movement uses one collision pass; a delayed 100 ms running frame remains
  safely bounded to two.
- If faceted or diagonal granite blocks both world-axis components, a bounded
  direction fan selects the closest clear tangent. This prevents the capsule
  catching until the player reverses direction, without changing the granite
  collision boundary or allowing straight-on movement through a wall.
- The fresh front-face regression traverses 2.36 m without a stalled frame,
  averages 5.1 collision probes, and peaks at 36 only at the sloped outer edge.
  The worst unoptimized Debug frame takes 1.77 ms.

Verification on 2026-09-04: the full Debug solution builds successfully.
Movement passes 33 checks in optimized and unoptimized builds. The shared soil
and terrain movement suite passes 364,076 checks in each build; combined soil
plus granite walking averages 0.067 ms/frame optimized and 0.495 ms/frame in
unoptimized Debug.

## V4.5: immutable original-granite walking BVH

- The untouched outcrop now owns one immutable, closed-surface BVH. A player
  capsule near original granite queries that surface once rather than gathering
  and reconstructing thousands of small source tetrahedra on every probe.
- Edited neighborhoods still use their exact local cavity BVHs. Original
  tetrahedra in mixed edited/unedited neighborhoods are tested allocation-free
  against their immutable vertices.
- A fresh-rock 100 ms movement regression reproduced the live uncut-map cost:
  25 probes took 35.5 ms in an unoptimized build. The immutable query reduces
  the same test to about 2.0 ms; the 16-chip tunnel test takes about 4.5 ms.

Verification on 2026-09-04: the full Debug solution builds successfully.
Movement and untouched/edited granite collision pass 31 checks; exact contact
passes 18, local updates pass 3,150, and debris performance passes 297. The
optimized fresh-rock benchmark takes 0.431 ms for 25 probes, and the 16-chip
tunnel benchmark takes 0.745 ms for 12 probes.

## V4.4: bounded sleeping-chip publication

- Sleeping granite is divided into immutable batches of at most eight chips.
  Waking or settling one chip now republishes only its small batch instead of
  every chip ever removed from the outcrop.
- Pick-scale sleeping chips use a 20-triangle collision-independent render
  proxy. Active chips retain the existing 80-triangle render detail, while all
  gameplay collision and conserved mass continue to use the exact chip body.
- The live V4.3 capture identified the publication spike directly: 29 sleeping
  chips produced 2,320 triangles and a 147.04 ms pose/mesh update. The V4.4
  equivalent is four bounded render batches totaling 580 sleeping triangles;
  an individual wake changes at most one eight-chip batch.

## V4.3: bounded cavity walking

- A slow frame can no longer create a collision-query feedback loop. Player
  movement performs at most two horizontal catch-up substeps; even at the
  maximum accepted frame delay, the fastest increment remains below the
  capsule radius. Vertical falling keeps its distance-based subdivision.
- Cavity BVH overlap traversal now stops immediately after proving a capsule
  collision. Containment rays visit nearer nodes first so a nearby excavated
  boundary prunes the rest of a detailed cell.
- The movement regression now cuts a 16-chip straight tunnel and tests a
  100 ms blocked/sliding frame beside it. It performs 12 collision probes and
  takes about 1.3 ms optimized or 8.9 ms unoptimized on the verification host;
  the former live capture reported 95 probes and 181.9 ms.

Verification on 2026-09-04: movement passes 30 checks, exact contact passes 18,
local updates pass 3,150, and debris passes 297. The full Debug editor rebuild
succeeds.

## V4.2: exact debris brushing and mass-only sub-pick remnants

- Loose granite now uses its bounding sphere only as a broad rejection test.
  Player brushing must also intersect the transformed chip triangles. Long,
  thin fragments can no longer wake through the hill or from several feet away
  and repeatedly force sleeping-debris render-batch rebuilds. The exact test
  uses only a 5-micron numerical skin around the player capsule.
- A 4,096-shape sample places the smallest 400 J pick chip at 48.39 cm3. The
  remnant cutoff is conservatively 45 cm3. Fracture-created dangling branches
  below that limit are deleted from retained geometry and credited only as
  volume/mass on the struck chip; no remnant triangles are added to its render
  or collision mesh.
- Strand cleanup contracts contacts above the pick-scale cross-section and
  peels only sub-pick leaf branches. This preserves connections between valid
  larger bodies and prevents cleanup-driven bulk collapse.

Verification on 2026-09-04: the 96-strike render soak culled 56 remnants while
creating zero extra falling bodies. Exact loose-chip brushing rejects a player
one meter above a meter-long thin shard, rejects a 10-micron air gap, accepts a
4-micron gap inside the explicit numerical skin, and accepts real contact. The
structural, local-collision, debris, closed-volume and milligram-ledger suites
pass, and the Debug editor rebuild succeeds.

## V3.2: bounded cavity collision and deterministic debris sleep

- Player collision against an edited granite cell now queries its immutable
  boundary BVH. It no longer scans every convex clipping remnant accumulated by
  every strike. The HUD reports the number of collision probes beside walk time
  so a costly probe can be distinguished from excessive movement retries.
- Walking brushes a detached soil island only after capsule-to-AABB rejection
  and an exact surface/containment check. A large clod below an intact cave roof
  can no longer be awakened by its broad bounding sphere reaching the player
  above it. Detached soil only accepts upward-facing support, preventing cavity
  sides and roofs from putting visibly floating clods to sleep.
- Granite chips use the ordinary velocity sleep first. A contacted chip that
  makes less than 10 cm of net progress for three consecutive one-second
  windows is frozen in its existing conserved pose, ending permanent wedged
  oscillation without breaking it into new pieces.
- Soil zero samples are treated as the material boundary during tetrahedral
  meshing. An original surface exactly coincident with an edited chunk plane is
  therefore republished instead of becoming a rectangular top/corner hole.

Verification on 2026-09-04: an 80-chip cavity completed 522 checks in optimized
and Debug builds. 1,024 dense near/above-cavity capsule probes took 118.78 ms
optimized and 532.08 ms Debug (0.116/0.520 ms per probe). Soil passed 364,076
checks, grass 16,446, debris 291, contact routing 18, and movement 28. The v145
Debug game runtime rebuilt successfully.

## V3.1: local collision, brushable debris, surface-faithful grass

- Soil ray, capsule, step, grass-refresh, clod-support, and granite-support
  queries now visit only coarse patches and sparse volume chunks overlapping the
  local request. Excavation elsewhere no longer makes every walking probe scan
  the entire accumulated terrain query set.
- Loose soil is still visible, shovelable, conserved, and simulated, but it no
  longer behaves like a rigid doorway wall or step under the player. Walking or
  landing contact gently wakes and moves both soil clods and granite pieces;
  standing still does not repeatedly reactivate them.
- Grass refresh distinguishes real removal of the original top skin from the
  tiny interpolation/normal change caused when an intact coarse patch is
  republished as fine volume geometry. Side and underside work can therefore
  make real round breakthroughs without painting square or stepped bare areas
  across untouched grass.

## V3.0: grass covering, traffic wear, gradual recovery

- Imported the supplied v006 grass albedo and normal PNGs byte-for-byte at 2K.
  Their texture groups provide sRGB albedo, linear normal data and mipmaps.
  A dedicated grass/soil material leaves the existing bare-soil material intact.
- Grass is a surface covering, not excavatable matter or a collision layer.
  Grounded movement gradually reduces cover in a soft footprint; idle time,
  airborne movement and teleports do not wear paths. Digging clears cover on
  newly exposed surfaces without changing shovel volume or material accounting.
- Stable upward-facing soil can recover after traffic stops. Height-qualified
  coverage belongs to the topmost host surface, so hidden tunnel floors and
  ceilings do not inherit the grass overhead. Granite, steep vertical walls and
  falling/settled loose clods retain their separate bare materials. Regrowth
  does not fill excavated geometry. Open-air pit floors may eventually recover.
- Defaults in WorldSettings / Provenance / Grass: wear 0.20 per metre of weighted
  traffic (roughly 20 repeated center crossings), 120 simulation seconds before
  recovery, 1200 seconds for full recovery, and a 1x recovery clock. These are
  tunable gameplay calibrations, not botanical estimates. Increasing the clock
  is useful for a quick playtest. R resets terrain and grass state together.
- A world-owned 128x128 RGBA32F coverage/height texture is 256 KiB. Dirty uploads
  are capped at 10 Hz, and terrain-height refresh is capped at 48 cells/frame.
  Wear and regrowth do not rebuild terrain meshes. Albedo and normal use the
  same randomized sample offsets, with a soft noisy grass/soil transition.

Verification on 2026-09-04:

- Grass: 16,404 checks passed in optimized and debug standalone builds, covering
  wear, idle/air/teleport exclusions, subdivision tolerance, recovery delay and
  reset, dig clearing, topmost-roof selection, and unchanged excavation volume.
  All 16,384 cells simultaneously recovering cost 0.052 / 0.279 ms per frame
  respectively. This CPU stress test excludes rendering and texture uploads.
- Soil: 347,718 checks passed in each build; movement: 28 and contact: 15 in
  each build. Shader reflection/compilation and the Debug x64 editor succeeded.
  Grass albedo, normal and material resources compiled successfully.
- Startup initially exposed an engine allocator bug: summing reflected field
  strides omitted alignment gaps created by an odd number of packed 16-bit
  texture handles (62 bytes versus the generated 64-byte layout). Shader data
  sizing now uses the maximum reflected offset plus stride. The editor rebuilt
  and remained alive through a controlled eight-second startup smoke test.

Limits: this is a bounded lab covering, reset on world reload; there is not yet
save/load persistence, seasons, moisture, seed propagation or a general terrain
vegetation system. The shader map bounds match this lab's existing world offset.
The grass is textured ground cover, not upright grass-blade geometry. A live
Play check is still needed for blend appearance, normal orientation, slope
coverage and GPU/frame-time cost; no live FPS improvement is claimed.

## V2.9: falling soil, whole small-clod collection, boundary normals

User repro: a detached soil clod stayed floating, and side-wall scoops produced
rectangular dark bands beyond the excavation. User clarified that a targeted
loose clod smaller than a normal shovel scoop must be collected whole, not
shaved into additional fragments.

- Cached positive-node tetrahedral connectivity separates unsupported soil
  components, including components crossing chunk seams. A lazy initial-field
  connectivity cache follows untouched cores to actual floor support rather
  than assuming every untouched neighbor is an anchor. Only detached cores
  become new rendered/edited chunks. Zero-only tetrahedra have zero volume and
  do not generate phantom faces.
- Detached clods keep their finite volumetric fields, exact volume, collision,
  ray contact, and worker-prepared meshes. Gravity translates them down to the
  actual soil/rock/clod surface or the lab floor. Settled clods sleep, then wake
  after soil or granite support changes. Motion updates transforms, not meshes.
- Falling does not credit the harvest ledger. Targeting a loose clod no larger
  than the nominal spherical-cap scoop (1.523710 L) collects that clod whole.
  Larger clods accept ordinary scoops and their disconnected remainders become
  independently falling bodies. Retained clod volume plus harvested volume is
  conserved. An obsolete contact in newly empty space cannot collect again.
- Coarse and edited vertical lab skirts now use the same outward axis normal;
  top/side corner normals have separate cache identities. This fixes the
  demonstrated normal mismatch capable of shading whole wall patches darker.
  No light, shadow-map, or supplied texture settings were changed.
- Worker-prepared indexed meshes reuse identical position/normal vertices.
  The regression terrain uploads 39,485 vertices instead of 148,611 (73.4%
  fewer), without removing any triangles or changing their normals. Normal and
  vertex caches use hash lookup. Boundary-cap generation skips irrelevant
  sides; granite distance checks stop at the soil field's existing clamp band.
- HUD now identifies V2.9, displays soil vertex publication, retained slough
  volume, active clods, and support/motion timing. Whole-clod collection has an
  explicit status message. Soil motion pauses during a soil worker transaction
  to keep its captured local contact and immutable geometry snapshot consistent.

Verification on 2026-09-04:

- Soil suite: 347,718 checks passed in both optimized and unoptimized builds.
  Includes all four side-wall normals, connected-soil retention, detached
  cross-chunk clods, an untouched interior core, falling/settling/reawakening,
  repeat scoops, whole small-clod collection without new fragments, exact mass,
  indexed geometry equivalence, tunnels, capsule collision and slope motion.
- Final soil run: 22-cut bore advanced 1.593 m; maximum preparation 23.20 ms
  optimized / 93.97 ms unoptimized, maximum publication 7,578 triangles. The
  2.502 L clod fixture's gravity peak was 0.046 / 0.358 ms respectively. These
  are standalone CPU measurements, not live-editor FPS or publication times.
- Contact: 15 checks each build; movement: 28 checks each build; debris:
  290 checks in the optimized build. Debug x64 editor rebuilt successfully.

Limits and next playtest: this is a clod-level vertical falling model, not
granular spreading, angle-of-repose erosion, or a cohesive-soil load solver.
The 2 cm field still sets the smallest representable shape. Cached support
searches and indexed preparation add worker work; mining latency is not claimed
solved. Verify the formerly dark side patches and publication/action-peak ms in
the editor. No live visual shadow/FPS comparison has been performed here.

## Soil source replacement — v003, 2026-09-04

Replaced the active soil source PNGs with the user's
`T_Soil_A_2K_MASTER_v003.png` and `T_Soil_N_2K_v003.png`, both confirmed
2048x2048. Imported copies match source SHA-256 hashes. Both textures and the
soil material compiled successfully. The V2.8 shader, normal strength, tiling
scale, mipmap/compression settings and gameplay are unchanged. The claimed
3x3 visual tiling pass was not independently verified during this replacement.
Previous PNGs are retained under
`Build/x64_Debug/SoilMaterialImport/before-v003-7e880e13d08149d6846c94493f2a9d68/`.

## V2.8: soil boundaries, shading, continuous slopes and loose-chip taps

User repro: repeating soil, rigid scoop teeth, inside/outside disagreement at
the terrain perimeter, ramp stepping, slow mining, and loose chips blocking work.

- Coarse skirt clipping now includes coplanar chunk faces instead of retaining
  the old exterior wall over an excavated cavity. Domain-boundary tetrahedral
  faces are explicitly capped, field-clipped and outward-wound; degenerate caps
  are rejected. Four exterior-side regression fixtures verify recessed contact,
  retained wall visibility and collision air after excavation.
- Removed the scoop's planar entrance cutoff. The spherical bowl overlaps air
  and neighboring cavities, avoiding thin retained wafers between cuts. The
  first flat scoop is now 1.523503 L against the ideal 1.523710 L. Repeated
  overlapping scoops may remove more sidewall than V2.7; actual represented
  volume, not the nominal capacity, supplies soil mass. The 2 cm discretization
  remains; this is not general soil support/collapse or a guarantee that no
  arbitrarily thin remnant can survive a complex excavation.
- Soil shading normals come from the local field and are prepared/cached on
  the worker. Original terrain normals use its smooth height function, removing
  the coarse triangle shading pattern. Collision remains on the actual mesh.
- New soil-only ProvenanceSoilTriplanarPBR shader blends three hashed translated
  texture samples with identical color/normal offsets and explicit derivatives.
  Negligible projection axes are skipped. Flat normal maps preserve the geometry
  normal rather than introducing projection-axis bias on slopes. Source images,
  2K resolution, 1 m scale and granite material remain unchanged. Shader/type
  reflection generation and shader compilation succeeded. GPU cost and visible
  repetition must still be checked in-editor, especially oblique tunnel walls.
- Step-up searches for minimum capsule clearance above a genuine tread instead
  of raising feet to the leading-edge height. Small descending slope gaps are
  followed only while moving/grounded; ledges still fall and jump/roof checks
  remain. Identical collision tests share a per-update cache. A triangle ramp
  test measures maximum per-frame rises/descents of 5.6/8.3 mm at walk speed.
- F on loose granite now wakes and nudges that existing piece sideways/upward,
  with bounded spin and mass-scaled motion. No geometry or recovery accounting
  changes, and that action does not also mine the host behind the chip. Larger
  collapsed bodies move less. Full rigid-body stacking/CCD remains future work.
- Support solve uses flat adjacency and reusable traversal scratch instead of
  per-node adjacency allocations. Granite publication reserves vertex/index
  capacity. A 32-strike Debug standalone comparison reduced final support from
  269 to 212 ms, mean worker+commit from 254 to 241 ms, and max from 694 to 656 ms.
  These are noisy local timings, not an FPS guarantee; substantial action latency
  remains, especially support graph construction. No queued stale tool hits.

Validation: soil suite passes 47,727 checks in optimized and Debug builds;
the tunnel sequence advances 1.593 m and publishes at most 7,578 triangles/action.
Worker preparation peaked around 14/83 ms optimized/Debug, near-rock exposure
about 99/622 ms. Full soil+rock approach averaged 0.73/6.88 ms per simulated
frame. Contact (15), movement (28), debris (290), and structural support (103,
32-strike conservation plus full-vs-cached graph comparison) regressions pass.
Editor Debug/x64 rebuild, generated soil shader, and soil/granite material
resource compilation succeeded. Live screenshot/FPS validation is still
required. All timings above are standalone.

## Soil material import — 2026-09-04

The current soil material now uses the user's `T_Soil_A_2K_MASTER.png` and
`T_Soil_N_2K.png`, copied unchanged into Data/Provenance/materials/soil.
Both sources are 2048 square. Opposite-edge RGB pixel differences are zero
on both axes for both images; this is an edge check, not a certification of
visual repetition, normal/albedo correspondence, or compressed mip seams.

Reuses the existing `ProvenanceGraniteTriplanarPBR` shader (despite its name,
its inputs are material-independent), with 1.0 metre tiling, normal strength
0.35, roughness 0.95 and zero metalness. World-space projection covers original
ground and excavated floors, walls and ceilings. Granite settings are unchanged.
Albedo uses sRGB BC7; normals use linear BC5 with no green inversion, matching
the shader's positive-V tangent direction and the supplied +Y normal convention.
Both use generated mipmaps. Source PNGs are preserved without regeneration.

Both texture resources and Soil.material compiled successfully with the existing
Debug resource compiler. No C++/shader changes or editor rebuild were required.
The previous color-only material is backed up in
Build/x64_Debug/SoilMaterialImport/Soil.material.before-2k.bak.
Live appearance, lighting/normal orientation, repeat visibility and FPS remain
an in-editor check; no live rendering measurements were taken during import.

### Remaining follow-up priorities

- Play-test V2.7 horizontal/upward tunneling, collision and the soil material.
- Profile action peak frame time, publication/upload and support-worker latency
  during long mixed soil/granite digging sessions; compare settled and active
  debris, and track actual process memory separately from the HUD cache count.
- Stress-test near-surface granite contact, soil/granite step transitions,
  large detached bodies and debris wedged between soil and granite. Existing
  fixes and standalone tests are not a substitute for these live cases.
- Soil roof/support collapse is not implemented. Loose soil clods/inventory,
  deeper terrain beyond the finite lab floor, production rigid-body stacking
  and re-chipping recovered granite remain future features, not delivered fixes.
- Updated granite texture assets still need a separate confirmed import;
  this change installs only the approved soil pair.

## V2.7: volumetric, reticle-directed soil excavation

The HUD reads `GRANITE OUTCROP V2.7 - VOLUMETRIC SOIL / AIMED SCOOPS`.
`F` still automatically selects soil/shovel or granite/pick from nearest contact.
Soil now removes a 3D spherical-cap scoop oriented along the captured reticle
ray, rather than lowering a heightfield. Horizontal, downward, angled and
upward cuts can leave roofs and separate tunnel floors. Nominal dimensions
remain 8.5 inches across and 2.85 inches deep; the test's discretized first
scoop is 1.414224 L versus the ideal cap's 1.523710 L (7.2% difference).

`SoilScoop.h` uses sparse immutable 24 cm chunks with 2 cm field samples and a
consistent six-tetrahedra-per-cell surface/volume definition. Original coarse
ground is clipped around activated chunks, not refined into long vertical
heightfield strips. Clamped narrow-band samples and global lattice positions
keep neighboring chunk faces consistent. `SoilHeightfieldV26.h` retains the
superseded implementation for reference; the runtime no longer uses it.

Unchanged chunks and surface BVHs are shared across worker snapshots. Only
changed chunks and newly affected coarse base patches are rebuilt/uploaded.
Revision-checked publication happens on the main thread; reset and teardown
join outstanding workers. HUD diagnostics now separate worker preparation,
soil publication time/patches/triangles, and the last action's peak frame delta
(including 12 frames after publication). Frame delta is not a CPU-only timer.

Full soil surface collision replaces the highest-soil-height walking shortcut.
Capsules collide with tunnel walls and ceilings; step support queries start
below the step ceiling rather than above the entire terrain. Crouch cannot be
released into a low roof. Debris floor support excludes surfaces above the
piece's center; motion tests soil walls as well as granite, and resting chips
wake after nearby excavation. Debris is still the lab approximation, not a
production rigid-body stacking/continuous-collision solver.

The original granite surface defines soil's material exclusion at initialization,
so removing granite cannot invent soil in its former volume. A granite hit on
the scoop centerline bounds shovel depth; once exposed, granite receives the
normal pick action through nearest-material contact. Soil/rock interfaces are
sampled at the field resolution, not exact analytic granite Boolean surfaces.
Soil recovery remains a separate bulk-volume/mass ledger at 1590 kg/m3.

Still intentionally limited: finite test floor at local Z=-0.12, numerical soil
recovery rather than loose soil clods/inventory, and no soil structural collapse.
A single shovel scoop is too small for a player: widen and heighten tunnels to
fit the 46 cm body width and 1.15 m crouched / 1.80 m standing height.

Validation: `RunSoilScoopTest.cmd` passes 55,143 checks in /O2 and /Od, including
22 horizontal scoops advancing 1.593 m under a retained roof, ceiling digging,
debris settling inside the tunnel, shared-node and normal validation, represented
volume accounting, granite exposure/tool routing, real-ground approach, full-size
crouched corridor movement/ceiling/jump checks, shared worker snapshots, stale
publication rejection, and reset. The tunnel sequence uploaded at most 8,332
triangles per action; worker preparation peaked around 12 ms /O2 and 75 ms /Od.
Near-granite initialization can take longer (~0.6 s in /Od in the exposure test),
but stays on the worker. The combined rock/soil approach averaged ~0.6 ms /O2
and ~5.7 ms /Od per simulated frame. These are standalone timings, not live GPU
or editor FPS measurements. Contact (15), movement (28) and debris (290) existing
regressions also pass. Editor Debug/x64 build succeeded; live visual/FPS review
is still required.

## V2.6: automatic soil scoop / granite pick (historical)

The HUD reads `GRANITE OUTCROP V2.6 - AUTO SOIL SCOOP / GRANITE PICK`.
`F` routes the nearest actual contact to a soil scoop or the existing 400 J,
grade-2 granite pick. No hotbar or tool selection is required; the old `T`
pick/hand toggle is no longer active in this lab. Loose granite still blocks
strikes; re-chipping recovered bodies is not part of this change.

`SoilScoop.h` owns shared 15 mm heightfield vertices, locally refined from the
original 120 mm ground cells. A spherical cutter produces a nominal 8.5-inch
diameter / 2.85-inch-deep open scoop on level ground. Measured first scoop:
1.517924 L versus the analytic 1.523710 L cap (0.38% discretization difference).
Only exposed soil columns are lowered; this is not underground tunneling,
overhang generation, or a soil structural-collapse simulation. A conservative
column envelope prevents cutting granite, leaving a small protected rim at
soil/rock intersections. Granite geometry and its mass ledger are unchanged.
The finite soil patch stops at local Z=-0.12 (the editor floor); flat approach
soil is only 4.72 inches deep. Perimeter nodes remain fixed to the closed skirt.

Soil is currently recovered into a separate numerical ledger, not spawned as
loose clods or placed in an inventory. Provisional settled bulk density is
2650 kg/m3 mineral density times 0.60 solid fraction = 1590 kg/m3. The standard
scoop accounts for about 2.413499 kg; this is bulk soil volume, not solid mineral
volume. Exact integration of the triangulated cavity supplies volume; cumulative
milligram rounding supplies each receipt without drift or duplicate credits.

Contact rays, walking/step height, rendering, and granite-debris support read
the same edited terrain. Soil edits wake settled soil-supported granite chips
in edited regions. Reset restores original ground and clears the soil ledger.
Shared boundary vertices dirty neighboring patches to avoid coarse/fine cracks.

Soil preparation runs on a worker snapshot; the old terrain remains usable
until publication. Main-thread publication reuses prepared triangles and updates
only dirty render patches. Reset/release joins both the granite and soil workers.
The HUD's soil worker time is preparation latency, not frame time. A 100-scoop
distributed test reached roughly 80 ms worst-case preparation in /Od, which is
why this work is off-thread; no claim of measured live editor FPS is made.

Verification: `RunSoilScoopTest.cmd` passes 702 checks in both /O2 and /Od,
including volume integration, repeat cuts, floor limits, rock protection, ray
and step agreement, coarse/fine and patch-corner continuity, waking/settling
debris, 100 distributed scoops, immutable worker preparation and publication,
and reset. Contact and movement regressions pass 15 and 28 checks respectively
per configuration. Debris performance regression passes 290 checks.

## V2.5: contact occlusion and small-step clearance (historical)

The HUD reads `GRANITE OUTCROP V2.5 - CONTACT + STEP CLEARANCE`.
C now toggles crouch on the key-down edge; holding/releasing it does not change
the latched posture. A stand request under a low ceiling is refused; press
again once clear. Shift, Space, ledge falling, and body dimensions are unchanged.

Rock step-up requires an upward-facing tread (normal Z >= 0.65) within 22 cm
of the feet, sampled at the capsule's leading edge/footprint. The vertical path
and destination require head/body clearance. Tall walls are not climbed by
repeated step attempts. Capsule plane-inflation remains a broad rejection,
followed by exact capsule-axis-to-triangle distance testing; this removes
oversized collision envelopes around acute internal partition corners.

`GraniteLabContact.h` chooses the nearest host/soil/loose-chip intersection for
the reticle and strike eligibility. Soil triangles/skirt share a cached BVH;
debris uses conservative bounds and the actual rigidly transformed chip mesh.
Green means host; yellow means an intervening soil/loose surface. The latter
receives a zero-removal contact receipt; soil excavation and loose re-chipping
are still not implemented. They no longer permit mining the host behind them.
A yellow world-space marker shows the captured contact while the worker runs.
Aim may move during this work; the original contact remains authoritative.
Host cutter depth stops at the first air gap along its inward normal, preventing
a thin lip from permitting removal of a separate rear slab in the same cut.
Internal contiguous partition intervals are merged, not treated as gaps.

Support port buckets now sweep spatial bounds before exact polygon comparison.
The broad phase is verified against an independent all-pairs overlap search;
support strength/calibration and accounting are unchanged. HUD cache/graph/load
timings distinguish initial support phases (release processing remains included
in total support time). No claim of production-ready strike latency is made.

Tests: `RunGraniteLabMovementTest.cmd` (28 checks per configuration),
`RunGraniteLabContactTest.cmd` (15 checks per configuration), contact-cast
regressions, and structural-support regressions. Real soil-shelf-to-granite
crossing and wall-climbing rejection are both exercised. Live input/visual
feel remains a manual preview check.

## V2.4 movement quality-of-life update (superseded controls)

The Play HUD now reads `GRANITE OUTCROP V2.4 - GROUNDED MOVEMENT`.
WASD walks at 1.6 m/s, either Shift runs at 3.4 m/s, Space jumps on press,
and holding C crouches at 0.8 m/s. Diagonals are normalized; looking vertically
does not slow horizontal movement. RMB retains the existing look behavior.
F/T/R/H/V keep their existing meanings. R also resets vertical velocity and
jump state. The HUD displays GROUNDED or AIRBORNE.

`Geometry/GraniteLabMovement.h` separates body motion from editor-camera
translation. The 0.23 m capsule radius, 1.80 m standing height / 1.65 m eye,
and 1.15 m crouching height / 1.05 m eye are unchanged. Standing is refused
under granite without clearance. Gravity is 9.81 m/s2 and jump takeoff is
4.2 m/s (approximately 0.88 m rise). Holding jump does not auto-repeat and
airborne jumps are not allowed. Movement uses bounded temporal/spatial
substeps; capsule overlaps stop walls and ceilings and allow vertical landing
on retained granite. Soil follows the rendered triangles. No XY fence or
downward ledge teleport remains; leaving support starts a fall. Outside the
finite soil patch, the lab uses the editor floor datum at world Z=0.

This remains a conservative lab-local controller, not general scene physics:
loose chips and arbitrary editor objects are not player colliders. Granite
collision is against remaining host matter; small rock ledges require jumping,
not automatic step-up. Soil rises up to 22 cm per movement substep can be
followed with overhead checks. Soil is still a heightfield (no soil overhangs).
Frame simulation is capped at 100 ms after stalls to avoid runaway catch-up.

`Build/x64_Debug/RunGraniteLabMovementTest.cmd` runs optimized/unoptimized
standalone tests for movement speeds, diagonal normalization, jump/re-jump,
air-jump rejection, ledge falling/landing, wall sliding, head clearance,
ceiling impacts, fast falls, support removal, slopes, actual granite collision
and landing, and leaving the soil patch. Live camera/input validation remains
a manual preview check.

## V2.4: automatic support release and performance accounting

The previous Play HUD read `GRANITE OUTCROP V2.4 - LOAD-BEARING SUPPORT`.
F performs the contact strike and its support update. **V is only inspection**;
it neither teaches the engine nor triggers cleanup. R resets all owned matter.
The approved rounded contact cutter and weathered exterior are unchanged.

### Matter and support

`Geometry/GraniteStructuralSupport.h` evaluates the retained convex volumes.
Original shared faces and subsequent cutting planes have persistent interface
identities. Opposing polygons must overlap by positive area (tolerance 1e-12 m2);
mere vertex/edge contact is not attachment. Untouched original regions are
compacted for traversal, not replaced with a voxel support proxy. Bottom faces
at the fixture foundation are anchors.

Disconnected connected-components become separate falling bodies, with no
pick-chip size ceiling. The same geometry supplies their closed cast and the
new host boundary. A worker prepares the complete change; geometry and its
ledger publish together on the world thread. Existing bridge/cube certificates
remain separate regressions.

Support strength is an explicitly **approximate lab load network**, not a
validated granite continuum solver. Gravity load is shared over rootward
interfaces by area, with iterative redistribution after overloaded links fail
(up to 16 passes). The local test calibration is 3 MPa tension/bending,
8 MPa shear and 80 MPa compression, using equivalent circular section modulus
for bending. Weight, remaining contact area and load lever arm matter. Tests
distinguish a roughly 15 lb cantilever on a 1/4-inch-wide, 2-inch-long neck
from an adequately supported body and from axial compression. Multiple weak
parallel paths are tested. Only interfaces separating final bodies are
published as fractures; this does not implement persistent internal elastic
cracking or a general finite-element solver.

Disconnected components <=0.5 cm3 and <=2 cm along each axis are explicit
non-rendered fines. Their volume and milligram mass go to the nearest larger
body released by that event, or to its direct pick chip if no larger body falls.
The receipt exposes the credited fines separately:

`accounted volume = visible cast volume + credited fines volume`.

Each recovered item has its own monotonically assigned identity, independent
of strike/revision number. Both direct chips and structural bodies enter the
same conservation ledger. No supported remnant is deleted merely for being
small. Malformed support boundaries are refused, with a visible receipt,
without publishing a partial structural release.

Large releases start with gravity and a small outward separation velocity,
not the pick chip's ejection impulse. Motion still uses the lab's center-sweep
rock collision and exact footprint/soil support. It is **not** full convex-body
rock collision, debris stacking or a production rigid-body solver; large-body
rock contact remains a limitation to validate in Play.

### Performance and memory

- Unchanged modified-cell contact data is immutable and cached. Ordinary
  support checks use cached bond sums; full internal boundary polygons are
  expanded only for a release. This stays on the fracture worker. The HUD
  separates total preparation, support, mesh registration and debris time.
- Strike preparation no longer deep-copies an already modified cell and its
  collision tree. Launch transfers recovered geometry instead of keeping an
  additional complete copy in the latest receipt. Zeroed damage entries are
  erased rather than retained indefinitely.
- This lab opts into linear meshlet clustering after vertex-cache optimization.
  Ordinary asset/fixture rendering keeps the existing spatial builder. No
  triangles or attributes are simplified away. Existing renderer packing and
  bounds checks remain in force.
- The main RAM counter formerly used rpmalloc `mapped_total`, a **lifetime**
  mapping counter. It now uses current `mapped` plus committed engine arenas.
  The tooltip explicitly distinguishes this from total process RAM: standard
  library/system allocations are not included. A separate lab figure estimates
  modified matter and support-cache capacities (not total process memory).

The clustering benchmark on a 32,709-triangle excavated region measured
32.402 ms spatial versus 0.404 ms linear with unoptimized meshoptimizer code.
Both reconstruct exactly the same indexed input triangles/attributes; cluster
counts were 1,583 and 1,558. These are **CPU-stage measurements, not live FPS**.
The allocator test maps/frees 32 MiB: current mappings return from 33 to 1 MiB,
while the old cumulative counter remains 33 MiB.

Support preparation still grows with cavity complexity and can cause visible
action latency even though it is off the rendering thread. This pass does not
claim a guaranteed 60 FPS floor or production-scale support throughput.

### Verification and next Play check

`RunGraniteStructuralSupportTest.cmd` covers disconnected bodies, point contact,
fine credit to either recipient, weak/strong necks, multiple weak paths, a
large isolated region of the real outcrop, gravity onset, exact released/host
volume, multiple item IDs per strike and reset ownership. Its optional argument
sets the soak length (720 for the long run). Timings/private-memory readings
are standalone CPU/process measurements without editor or GPU resources.

Other regressions: `RunGraniteContactCastTest.cmd` (521 checks, optimized and
unoptimized), `RunGraniteLocalUpdateTest.cmd` (3,598 each),
`RunGraniteDebrisPerformanceTest.cmd` (290), `RunGraniteClusterBuildTest.cmd`,
`RunAllocatorMemoryCounterTest.cmd`, and the procedural-root upload test.
The existing authority verifier retains its frozen fingerprints.

The Debug editor has been rebuilt. A live walkthrough has not been performed
by the agent. Next check: excavate around a larger island/neck, watch automatic
release and fine credits, then compare worker/mesh/debris timing and the current
RAM display during strikes and after settling/reset. No manual floater key is
required for release.

Final V2.4 results (2026-09-03): the optimized 720-strike run passes **1,528
checks with zero failures**, releases 50 additional structural bodies and
retains all 770 recovered bodies. The lightweight support graph matches the
full geometric graph. Modified matter/cache capacities finish at 184.62 MiB;
standalone process private memory is 783.12 MiB before reset and 89.05 MiB
after reset (56.01 MiB initial baseline). Allocator caches may remain after
reset; no claim of identical working-set return is made. Unoptimized 32-strike
support coverage passes 102 checks, including the large-body gravity fixture.

The optimized soak's worker+commit sample averages 344.83 ms with a 1,258.94 ms
maximum; the last support check is 292.01 ms. Some other verification/build
work ran concurrently, so these are indicative samples, not a controlled
performance guarantee. They clearly establish remaining complex-action latency
despite the improved renderer path. Contact-cast, local-update, debris,
clustering, allocator-counter and root-upload regressions all pass; the
authority verifier reports zero failures and unchanged fingerprints.

## Historical V2.3: localized display updates and accelerated contact queries

The current HUD reads `GRANITE OUTCROP V2.3 - LOCAL MESH UPDATES`.
Approved V2.1 fracture/cast geometry, exterior normals and accounting are
unchanged. The wall's original material cells are assigned to stable 26 cm
rendering groups. These are ownership groups, NOT new clipping planes or
piece templates: their union is exactly the original visible boundary.

Fracture preparation also triangulates the affected groups on the existing
background worker. On commit, only those groups replace their render meshes;
unaffected groups and all existing loose-chip geometry stay cached. The HUD
reports the number of groups replaced by the last strike. Initial scene load
still prepares all groups. Renderer registration/upload remains on the world
thread; this is not a claim of zero-cost GPU uploads.

Ray queries now walk the spatial bins actually crossed by the ray. Modified
cells carry immutable triangle bounding-volume trees, prepared with the cut.
Unmodified triangles are read directly instead of constructing face arrays on
each query. Collision continues to use the same exact retained surfaces.

Sloped-soil support transforms each distinct chip vertex once, reuses cached
soil triangles, and clips convex faces directly instead of their triangle
fans. It preserves the full rotated footprint/terrain-plane support result;
no spherical approximation, mesh simplification or chip deletion is used.

Press **V** to toggle optional reticle inspection of a suspected floater within
2 m. It identifies either HOST granite (structural connectivity not evaluated)
or a numbered LOOSE chip, with moving/sleep-on-rock/sleep-on-soil state and
vertical soil clearance. Inspection adds queries only while enabled and should
be disabled when measuring normal gameplay performance. This pass does not
claim to resolve an unidentified floater or add structural detachment.

`RunGraniteLocalUpdateTest.cmd` runs 128 corner-region strikes, verifies exact
cached-patch positions and normals against current matter, checks conservation,
compares accelerated rays with the previous surface query, and exercises dense
chips on the sloped seam. In this fixture each strike replaces at most 4 of 311
groups. Three released chips settle above the soil within ten simulated
seconds. Timing output is CPU-only; live FPS and main-thread mesh registration
cost still require preview validation.

Final V2.3 verification: 3,598 local-update checks pass in optimized and
unoptimized builds; 521 contact-cast checks pass in both; 290 debris checks
pass; the existing authority verifier reports PASS with zero failures. The
Debug editor rebuild succeeds. With the build finished, the standalone
unoptimized fixture measured old/new average ray times of 17.666/0.067 ms and
old/new dense sloped-support times of 4.699/1.647 ms. The three-chip seam run
averaged 0.290 ms with an 11.398 ms peak over 600 frames; all three settled.
These samples establish reduced CPU work, not a guaranteed frame-rate floor.

## Historical V2.2 performance pass

### V2.2 details: cached rounded contact casts and tumbling debris

Play uses `GraniteContactCast.h` and `GraniteContactDebris.h`. The HUD reads
`GRANITE OUTCROP V2.2 - CACHED CHIP GEOMETRY`. Controls and player/outcrop
dimensions are unchanged. The V1 and V2 headers/certificates remain regression
fixtures; V2 tetrahedra now serve only as an invisible volume decomposition.
They are NOT harvested piece templates.

Each real contact defines a seeded, anisotropic fracture volume using 12–16
varied macro-plane directions. Adjacent planes receive sampled Minkowski-offset
roundovers approximately 1–2.7 mm wide at the basic-pick calibration. These
strips are real geometry, not smoothed normals on a sharp tetrahedron. Original
weathered surface coordinates and interpolated normals survive clipping. Fresh
faces retain their own identity. Intersection with earlier cavities can produce
concave recovered shapes. A conservative size bound keeps the basic tool below
five inches; repeatable seeds do not select a small reusable mesh library.

The SAME constructed boundary produces the retained cavity and the recovered
cast. Closed solid intersections independently measure removed volume; a
mismatching transaction is refused before changing the world. Integer milligram
accounting uses cumulative rounding, preventing per-chip rounding drift. Damage
accumulates in local 12 mm contact bins, not a whole-wall health counter.

Fracture preparation runs on one background worker, reading immutable world
state. The world thread atomically commits a completed transaction; camera/ray
queries can continue meanwhile. Additional F presses while preparing are not
queued. Reset and teardown join the worker before replacing its inputs. No
renderer, input, or physics state is modified from that worker.

Debris now integrates at fixed 120 Hz with gravity 9.81 m/s², varied outward
release velocity, quaternion tumbling, damped impacts and terrain friction.
The actual rotated mesh footprint is supported against the rendered soil.
Each fragment uploads its immutable local mesh once. Subsequent motion updates
only its rigid transform; settled fragments require no transform upload. The
renderer queues transform writes before the device update-pool snapshot. Reset
and teardown unregister all fragment instances. All recovered pieces remain
accounted and rendered, with their V2.1 shapes unchanged.

Soil support uses an exact vertex/plane shortcut only when every terrain vertex
under the conservative rotated-chip bounds is coplanar. Otherwise the original
full footprint clipping is retained. Wall impacts preserve the remaining
tangential travel instead of discarding the downward step. Rest detection
tolerates the small contact skin, and chips resting on rock wake after the host
geometry changes. Genuine supporting ledges can still retain chips.

The standalone unoptimized 24-chip reproduction measured average debris-only
CPU time of approximately 11.0 ms before this pass and 0.28 ms afterward over
420 frames. Previously one chip remained active; afterward all 24 slept and
the final idle interval rounded to 0.000 ms. These are not total frame times
or live editor FPS. `RunGraniteDebrisPerformanceTest.cmd` also checks settling,
nonpenetration and fast support against full clipping at varied poses/terrain.
The V2.2 debris regression passes 290 checks with zero failures. The unchanged
521-check contact-cast certificate passes in optimized and unoptimized modes,
and the Debug editor rebuild succeeds. Live frame-rate recovery remains to be
measured in the preview.

### V2.2 initial-upload alignment correction

Live startup exposed a debug assertion in `Memory::CopyToWriteCombined`, called
by `UploadProceduralMeshRootAndInitialize`: the captured 128-byte root array
was only ordinarily aligned, while the SIMD copy requires a 32-byte-aligned
source. The callback now copies the payload into explicitly 32-byte-aligned
local storage at its point of use. Aligning only the original captured variable
would not guarantee alignment after callback copies. The global memory-copy
assertion remains unchanged. No fracture geometry or debris behavior changes.

`RunProceduralRootUploadTest.cmd` exercises the corrected staging pattern with
the real engine copy routine, checking all eight 4-byte capture alignments
modulo 32, exact payload bytes and surrounding guards. Optimized and
unoptimized runs both pass. This is a CPU upload-contract test, not a live GPU
or editor-startup validation.

Known limitations: release energy, fracture planes and friction are test
calibrations, not a measured constitutive granite model or a direct invocation
of the frozen geological generator. Rock collision still sweeps fragment
centers rather than full convex bodies, and there is no fragment stacking,
chip-to-chip collision, disconnected-component splitting, soil excavation,
inventory, or structural collapse. A compound recovered cast currently moves
as one debris object. Settling is damped lab motion, not the full rigid-body
solver. Arbitrarily long excavation sessions still grow the retained convex
decomposition and require further profiling.

`RunGraniteContactCastTest.cmd` runs optimized and unoptimized certificates for
matching volumes, mass ledger, size bounds, six contact directions, local weak
strikes, immutable background preparation, stale transaction refusal, seeded
shape variation, rotated soil support, and 30/144 FPS settling agreement.
Offline triangle renders are geometry checks, NOT editor screenshots. Live
input, appearance, shadows and frame rate still require preview validation.

Verification on 2026-09-03: final contact-cast certificate **521 checks, zero
failures** in both optimized and unoptimized builds. The extended 256-strike
run passed **984 checks, zero failures** before adding the final 65 ray/exterior
checks. The existing authority verifier also passed (1,156 gates, 268 tool
strike checks, 88 local-excavation checks; frozen fingerprints unchanged).
The Debug editor and game runtime were rebuilt successfully. No live-editor
performance or visual pass is claimed.

## Historical V2 correction pass (superseded harvesting method)


Play now uses `GraniteOutcropV2.h`, algorithm version 2. The original
`GraniteOutcrop.h` and its certificate remain intact as the V1 regression.
The frozen granite, surface, bridge and 1 cm cube certificates are unchanged.

V2 replaces the six-centimetre square parcel boundary with a conforming,
jittered five-tetrahedron partition. Alternating cell topology avoids a repeated
single-direction diagonal pattern. A central tetrahedron may join one or two
corners to produce connected wedges; other corners are individual fragments.
The shared triangular interfaces use identical vertex IDs, so no cosmetic
fragment generation or independent cavity scaling is involved. Overlarge joined
wedges use a smaller connected group instead of shrinking their visual mesh.

The outer envelope now has broad asymmetrical lobes, a shallow ledge, upper
shoulder narrowing and a varying crest, with smooth exterior normals and hard
fresh-fracture normals. This is still a deliberately bounded, shaped test wall,
not a finished geological outcrop generator. Nominal dimensions are 2.904 m wide,
0.748 m deep and 1.584 m high before the shape mapping; the crest reaches about
1.70 m. The HUD reports the actual maximum height.

The soil is a continuous triangulated heightfield blending through the granite
footprint, so its interface is visually occluded by the solid rock rather than
cut open by skipped terrain faces. The finite patch has perimeter skirts.
Walking and loose-chip support use those same triangles. Soil is still not an
excavatable, accounted soil solid in this fixture.

Fragment settling clips the full projected fragment footprint against the soil
triangles and solves the required vertical support. No zero-height fallback is
used. Loose-rock motion uses bounded **center** sweeps with tangential damping;
this is simplified translational debris, not full convex-body collision,
rotation, fragment stacking, or inventory. Pieces may rest in the cavity or on
rock ledges as well as on soil. All recovered pieces remain rendered: the old
128-piece display cutoff is removed.

Controls remain WASD/RMB/F/T/R/H. **Hold C to crouch.** Standing eye height is
1.65 m with a 1.80 m capsule; crouched eye height is 1.05 m with a 1.15 m capsule.
Standing up is refused when the remaining granite blocks headroom. The HUD is
lowered clear of the preview toolbar and identifies this version as
`GRANITE OUTCROP V2 - IRREGULAR CAST CHIPPING`.

`RunGraniteOutcropV2Test.cmd` tests optimized and unoptimized builds, including
every fragment's geometry/size on the active seed, shared-face pairing,
conservation, visible-boundary ray agreement, repeated local excavation,
continuous soil coverage, exact footprint support, and released-chip settling.
Four additional seeds exercise the five-inch bound and closed host volume.
Active-seed fragment diameters measured approximately 1.75–4.92 inches.

`render_outcrop_shape.py` renders exported certificate triangles for offline
visual QA. It was used to catch the first draft's repeated diagonal pattern.
These images are explicitly **not editor screenshots**. Live preview validation
of the final build remains a user check; full structural collapse is still out
of scope.

## Historical V1 notes (superseded for the default Play scene)

The default Play view of `ProvenanceSandbox.map` now starts at a soil approach
and inclined granite face with a soil-covered upper shelf. It uses the existing
granite material, including its triplanar texture. Soil is a rough brown material.

## Controls

- WASD: walk; RMB: look using the existing preview camera controls.
- F: one strike per press, at the center reticle; two metre reach.
- T: switch between calibrated basic pick and an ineffective bare hand.
- R: reset the outcrop, recovered matter and walking position.
- H: return to the old certificate/cube/bridge fixtures; H again returns here.

The camera is constrained to the rendered soil triangles with a conservative
vertical capsule against the remaining granite. This is a lab-local walking
adapter, not a new engine player controller or a PhysX integration. Existing
mouse-release fixes remain in place; the lab does not capture the OS cursor.

## Geometry and matter contract

`Geometry/GraniteOutcrop.h` owns a separate, bounded test body. It does not change
the frozen granite, tool-strike or 1 cm excavation algorithms or goldens.

The face is 2.88 m wide and rises 1.56 m, receding 0.624 m as it rises. This is an
inclined test wall with soil blending at its edges, not production terrain
generation or a finished natural-outcrop sculpt.

The body contains 6 cm parcels, each split by a deterministic seeded oblique
plane into two complementary convex fragments. The maximum possible fragment
diameter is the enclosing sheared parcel diagonal: approximately 4.70 inches.
The pieces vary in taper, cut orientation and face count. They are not the old
large B2/spall examples. Grid-aligned parcel boundaries can still show in cavities.

Each completed strike removes only the contacted convex fragment. The very same
polyhedron supplies the recovered piece and the cavity subtraction. Neighboring
damage is independent. Incoming direction and angle determine transferred work;
grazing or weak strikes may yield only a contact/damage receipt, not a fragment.
400 J pick input and the fracture-work threshold are test calibrations, not
measurements or a general-purpose tool/material model.

Volume is geometric; mass uses granite density 2700 kg/m3. Complementary masses
sum to exactly 583,200 mg per parcel, with at most 1 mg rounding discrepancy
against an individual fragment's geometric volume. Removed-piece identities
cannot be counted twice. All recovered mass remains in the ledger.

Released casts have a lightweight gravity/ground-settling animation. The newest
128 are displayed; older pieces remain accounted but are no longer rendered.
This is not full rigid-body collision, chip stacking, pickup or inventory.
Soil is a rendered walking surface here, not an excavatable soil matter field.
Support recomputation and structural collapse are intentionally not implemented.

## Verification

`Verification/GraniteOutcropTest.cpp` is a standalone C++17 certificate; run
`Build/x64_Debug/RunGraniteOutcropTest.cmd` for optimized and unoptimized builds.
Both pass 1,696 checks, covering:

- complementary casts, mass and closed host volume;
- size bounds and shape diversity;
- weak contacts and independent local accumulation;
- repeated same-direction excavation and all six approach directions;
- collision against remaining matter and deterministic replay;
- independent ray/triangle intersection versus the visible excavated boundary.

The existing Provenance verifier also passes: 1,156 gates, 268 tool-strike checks,
88 cube-excavation checks, and unchanged granite/surface fingerprints.

The Debug editor, soil resource and sandbox map compile successfully. A live
editor walkthrough has not been performed by the agent. The next user check is
walking to the face, striking its front and cavity surfaces, then walking around
a side to the upper soil shelf while checking input release and frame time.

#!/usr/bin/env python3
"""Apply P4.2 matter-identity fields to a local fablescript terrain_mutate.py (no Mygame git commit)."""
from __future__ import annotations

from pathlib import Path
import sys

HELPER = '''
# P4.2: Fablescript-issued matter identity (clients must not invent body/aggregate ids).
_MATTER_ID_FLOOR = 18472


def _issue_matter_identity(eng, *, kind, material, grams, materials, source, parent_id=0):
    """Allocate an authoritative body_id or aggregate_id and attach provenance/seeds."""
    nid = int(getattr(eng, "_next_matter_id", _MATTER_ID_FLOOR))
    eng._next_matter_id = nid + 1
    rev = int(getattr(eng.matter, "terrain_rev", 0) or 0)
    seed = (hash((nid, kind, material, int(grams), rev)) & 0xFFFFFFFF)
    src = source or {}
    prov = "%s@%s#%s" % (
        src.get("origin_action", kind),
        src.get("source", {}),
        src.get("origin_tick", getattr(eng, "tick_count", 0)),
    )
    body_id = int(nid) if kind == "body" else 0
    aggregate_id = int(nid) if kind == "aggregate" else 0
    return {
        "body_id": body_id,
        "aggregate_id": aggregate_id,
        "material": material,
        "grams": int(grams),
        "materials": dict(materials or {}),
        "source": src,
        "provenance": prov,
        "form_seed": int(seed),
        "fracture_seed": int(seed ^ 0xA5A5A5A5),
        "parent_id": int(parent_id or 0),
        "revision": rev,
    }

'''

CARVE_OLD = '''    settled = eng.settle_cascade([(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)])
    # TERRAIN REVISION: the client marks the dug chunk dirty with THIS rev and re-meshes it only once its
    # cache data reaches it (a voxel_region fetch stamped >= rev) -- revision-gated, no timer.
    return {"ok": True, "removed": removed, "settled": settled, "rev": eng.matter.terrain_rev}
'''

CARVE_NEW = '''    settled = eng.settle_cascade([(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1)])
    # TERRAIN REVISION: the client marks the dug chunk dirty with THIS rev and re-meshes it only once its
    # cache data reaches it (a voxel_region fetch stamped >= rev) -- revision-gated, no timer.
    # P4.2: authoritative detached/scoop identity — clients present this id, never invent from local H2H.
    total_g = int(sum(int(a) for a in (removed or {}).values() if int(a) > 0))
    dominant = max(removed, key=removed.get) if removed else "dirt"
    ident = _issue_matter_identity(
        eng, kind="body", material=dominant, grams=total_g, materials=removed,
        source={"source": {"x": x, "y": y}, "actor": pid, "origin_action": "dig",
                "origin_tick": eng.tick_count, "origin_id": _sid},
        parent_id=int(_sid) if isinstance(_sid, int) else 0)
    out = {"ok": True, "removed": removed, "settled": settled, "rev": eng.matter.terrain_rev}
    out.update(ident)
    return out
'''

PLACE_OLD = '''    return {"ok": True, "placed": placed, "placed_by": placed_by,      # total + the per-material truth
            "radius": radius_out or radius, "material": dominant,      # radius -> the client's reticle
            "radius_requested": radius_requested,           # the ask, so logs never mistake it for the truth
            "amount": sum(g for _, g in asks), "pickup_g": pickup_g,   # the sized ask + the implement's default
            "rheology": eng.matter.place_rheology(dominant),
            "settled": [], "rev": eng.matter.terrain_rev}
'''

PLACE_NEW = '''    # P4.2: authoritative aggregate identity for the placed mound / refill.
    ident = _issue_matter_identity(
        eng, kind="aggregate", material=dominant, grams=int(placed), materials=placed_by,
        source={"source": {"x": x, "y": y}, "actor": pid, "origin_action": "place",
                "origin_tick": eng.tick_count},
        parent_id=int(params.get("parent_id", 0) or 0))
    out = {"ok": True, "placed": placed, "placed_by": placed_by,      # total + the per-material truth
            "radius": radius_out or radius, "material": dominant,      # radius -> the client's reticle
            "radius_requested": radius_requested,           # the ask, so logs never mistake it for the truth
            "amount": sum(g for _, g in asks), "pickup_g": pickup_g,   # the sized ask + the implement's default
            "rheology": eng.matter.place_rheology(dominant),
            "settled": [], "rev": eng.matter.terrain_rev}
    out.update(ident)
    return out
'''


def main() -> int:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else
                r"C:\Users\D-Day\OneDrive\Desktop\Mygame\_engine_truth_lane3\fablescript\engine\terrain_mutate.py")
    text = path.read_text(encoding="utf-8")
    if "_issue_matter_identity" in text:
        print("already patched:", path)
        return 0
    anchor = "_grade_at = tectonic.grade_at\n\n\ndef sculpt_column_rich"
    if anchor not in text:
        print("helper anchor missing", file=sys.stderr)
        return 2
    text = text.replace(anchor, "_grade_at = tectonic.grade_at\n" + HELPER + "\ndef sculpt_column_rich", 1)
    if CARVE_OLD not in text:
        print("carve anchor missing", file=sys.stderr)
        return 3
    text = text.replace(CARVE_OLD, CARVE_NEW, 1)
    if PLACE_OLD not in text:
        print("place anchor missing", file=sys.stderr)
        return 4
    text = text.replace(PLACE_OLD, PLACE_NEW, 1)
    path.write_text(text, encoding="utf-8")
    print("patched", path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

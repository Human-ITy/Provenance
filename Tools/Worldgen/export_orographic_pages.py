"""Export canonical Phase 17 production pages for Esoterica adopt_page.

The banked golden `canonical_orographic_openworld_v1.json` is a digest/summary.
This writes the actual production_page bytes (carrier v2) plus a tectonic-v3
mismatch page so the client can adopt/refuse without inventing geography.

  python Tools/Worldgen/export_orographic_pages.py
"""
from __future__ import annotations

import hashlib
import json
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Data" / "Worldgen" / "orographic_phase17"

# Phase 17 producer (fablescript). Prefer the landed worktree, then Mygame.
_ENGINE_CANDIDATES = [
    Path(r"c:\Users\D-Day\OneDrive\Desktop\Mygame\_phase17_orographic\fablescript"),
    Path(r"c:\Users\D-Day\OneDrive\Desktop\Mygame\fablescript"),
]


def _engine_root() -> Path:
    env = os.environ.get("FABLESCRIPT_ROOT")
    if env:
        p = Path(env)
        if (p / "worldgen" / "orographic_carrier.py").exists():
            return p
    for p in _ENGINE_CANDIDATES:
        if (p / "worldgen" / "orographic_carrier.py").exists():
            return p
    raise SystemExit("Phase 17 fablescript not found (set FABLESCRIPT_ROOT)")


def _dump(path: Path, obj) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    blob = json.dumps(obj, sort_keys=True, separators=(",", ":"))
    path.write_text(blob, encoding="utf-8")
    return hashlib.sha256(blob.encode("utf-8")).hexdigest()


def main() -> int:
    engine = _engine_root()
    sys.path.insert(0, str(engine))
    from worldgen import orographic as oro
    from worldgen import orographic_carrier as ocar
    from worldgen.core import make_worldgen_identity
    import worldgen.provenance_tectonic as tectonic
    import worldgen.core as wgcore

    ident = tectonic.make_canonical_genesis_identity()
    tectonic.install(ident)
    oro.clear_caches()
    stamp = ocar.genesis_stamp(ident)
    page11 = ocar.production_page(ident, 1, 1, step=500)
    page21 = ocar.production_page(ident, 2, 1, step=500)
    ctx11 = ocar.ecological_context_page(ident, 1, 1)
    ctx21 = ocar.ecological_context_page(ident, 2, 1)

    in_page = ((1100.0, 1100.0), (1500.0, 1720.0), (1024.0, 1024.0),
               (1536.0, 1536.0), (1800.0, 1400.0), (1600.0, 1900.0),
               (2048.0, 1024.0))
    samples = [{"x": x, "y": y,
                "grade": round(tectonic.grade_at(x, y), 9),
                "carrier": round(ocar.carrier_elevation(page11, x, y), 9)}
               for x, y in in_page]

    hist = make_worldgen_identity(seed=7, preset="grassland")
    tectonic.install(hist)
    oro.clear_caches()
    hist_page = ocar.build_carrier(hist, 1, 1, step=500, with_drainage=True)
    hist_page["feature_definitions"] = ocar.feature_definitions(hist_page)

    tectonic.install(ident)
    oro.clear_caches()

    OUT.mkdir(parents=True, exist_ok=True)
    d11 = _dump(OUT / "canonical_orographic_page_1_1.json", page11)
    d21 = _dump(OUT / "canonical_orographic_page_2_1.json", page21)
    dc11 = _dump(OUT / "canonical_orographic_context_1_1.json", ctx11)
    dc21 = _dump(OUT / "canonical_orographic_context_2_1.json", ctx21)
    dh = _dump(OUT / "tectonic_v3_page_1_1.json", hist_page)
    _dump(OUT / "canonical_genesis.json", stamp)
    meta = {
        "engine_root": str(engine),
        "world_identity_hash": tectonic.world_identity_hash(ident),
        "orographic_hash": oro.world_identity_hash(ident),
        "terrain_law": tectonic.terrain_law_of(ident),
        "page_1_1_digest": d11,
        "page_2_1_digest": d21,
        "context_1_1_digest": dc11,
        "context_2_1_digest": dc21,
        "influence_radius_m": ctx11["influence_radius_m"],
        "page_size_m": ctx11["page_size_m"],
        "context_system_ids": [s["id"] for s in ctx11["systems"]],
        "context_range_ids": [r["id"] for r in ctx11["ranges"]],
        "context_massif_ids": [m["id"] for m in ctx11["massifs"]],
        "tectonic_v3_digest": dh,
        "expected_page_digest": "03f579fed39e4685240aaf4d51efae5a2347bf414277242ee068a215a3b8fd69",
        "cache_key": page11["cache_key"],
        "samples": samples,
        "feature_ids": {k: [f["id"] if isinstance(f, dict) and "id" in f else f
                            for f in page11["feature_definitions"][k]]
                        for k in ocar.REQUIRED_FEATURE_KINDS},
        "shared_ridge_ids": sorted(
            {f["id"] for f in page11["feature_definitions"]["ridges"]}
            & {f["id"] for f in page21["feature_definitions"]["ridges"]}),
    }
    _dump(OUT / "export_meta.json", meta)
    ok = d11 == meta["expected_page_digest"]
    print("EXPORT %s digest=%s expected=%s engine=%s" % (
        "OK" if ok else "DIGEST_MISMATCH", d11, meta["expected_page_digest"], engine))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

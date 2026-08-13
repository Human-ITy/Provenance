"""Compile the canonical Stage-1 CAUSAL_WORLD geology-kernel floor.

This script stands on the Python/Fablescript authority side of the boundary.
Esoterica must load and validate its product; it must never recreate this
history from a missing or rejected descriptor.
"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "Data" / "Worldgen" / "causal_world_geology_kernel_floor.cwg"
EXPOSURE_OUTPUT = ROOT / "Data" / "Worldgen" / "causal_world_geologic_exposure_floor.cwe"
OFFSET = 14695981039346656037
PRIME = 1099511628211


def fnv1a64(text: str) -> int:
    value = OFFSET
    for byte in text.encode("utf-8"):
        value ^= byte
        value = (value * PRIME) & 0xFFFFFFFFFFFFFFFF
    return value


def stable_id(name: str) -> str:
    return f"{fnv1a64(name):016x}"


def main() -> None:
    world_identity = "provenance:causal-world:kernel-floor:seed-7"
    events = [
        f"deposit_sandstone,{stable_id('event:deposit_sandstone')},10,deposition",
        f"deposit_shale,{stable_id('event:deposit_shale')},10,deposition",
        f"lithify_01,{stable_id('event:lithify_01')},20,lithification",
        f"fold_03,{stable_id('event:fold_03')},30,folding",
    ]
    formations = [
        f"F15,{stable_id('feature:F15')},sandstone,-18,-12,deposit_sandstone|lithify_01|fold_03",
        f"F16,{stable_id('feature:F16')},shale,-12,-6,deposit_shale|lithify_01|fold_03",
        f"F17,{stable_id('feature:F17')},sandstone,-6,0,deposit_sandstone|lithify_01|fold_03",
        f"F18,{stable_id('feature:F18')},shale,0,6,deposit_shale|lithify_01|fold_03",
        f"F19,{stable_id('feature:F19')},sandstone,6,12,deposit_sandstone|lithify_01|fold_03",
        f"F20,{stable_id('feature:F20')},shale,12,18,deposit_shale|lithify_01|fold_03",
    ]
    event_catalog = "".join(f"{event}\n" for event in events)
    body_catalog = "".join(f"{formation}\n" for formation in formations)
    lines = [
        "PROVENANCE_CAUSAL_WORLD_DESCRIPTOR_V1",
        "# Canonical authority product. Regenerate with Tools/Worldgen/compile_causal_world_floor.py",
        f"world_identity={world_identity}",
        f"world_identity_hash={fnv1a64(world_identity):016x}",
        "worldgen_id=provenance_causal_world",
        "worldgen_version=1",
        "schema_version=1",
        "region_key=causal_world_geology_kernel_floor",
        f"event_program_digest={fnv1a64(event_catalog):016x}",
        f"body_catalog_digest={fnv1a64(body_catalog):016x}",
        "authority_revision=1",
        "fold_amplitude_m=12",
        "fold_wavelength_m=96",
        "fold_axis_deg=34",
        "datum_m=0",
        *(f"event={event}" for event in events),
        *(f"formation={formation}" for formation in formations),
        "",
    ]
    geology_text = "\n".join(lines)
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(geology_text, encoding="utf-8", newline="\n")

    surfaces = [
        "present_erosion,erosion,1.5,0,0,2.5,144,71,0",
        "flat_control,flat,3,0,0,0,1,0,0",
        "terraced_control,terraced,1.5,0,0,2.5,144,71,2",
    ]
    surface_catalog = "".join(f"{surface}\n" for surface in surfaces)
    exposure_lines = [
        "PROVENANCE_CAUSAL_WORLD_EXPOSURE_V1",
        "# Canonical present-day surface authority; no runtime erosion simulation.",
        f"world_identity_hash={fnv1a64(world_identity):016x}",
        "worldgen_id=provenance_causal_world",
        "worldgen_version=1",
        "schema_version=1",
        "region_key=causal_world_geologic_exposure_floor",
        f"geology_descriptor_digest={fnv1a64(geology_text):016x}",
        f"surface_program_digest={fnv1a64(surface_catalog):016x}",
        "authority_revision=1",
        *(f"surface={surface}" for surface in surfaces),
        "",
    ]
    EXPOSURE_OUTPUT.write_text("\n".join(exposure_lines), encoding="utf-8", newline="\n")
    print(OUTPUT)
    print(EXPOSURE_OUTPUT)


if __name__ == "__main__":
    main()

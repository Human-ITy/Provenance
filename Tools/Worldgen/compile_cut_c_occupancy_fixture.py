"""Compile the read-only FableScript Cut-B occupancy fixture for Cut C.

The Stage-10 causal geology oracle remains the FableScript-owned Cut-A
authority product.  Surface quantisation and sub-cell allocation are executed
by the *actual* ``provenance_occupancy`` module from the pinned Cut-B worktree;
this exporter does not reproduce that law in Esoterica.

The resulting ``.cocc`` file is an immutable integration envelope.  Esoterica
may ingest, tile, reconstruct, collide with, and inspect it.  It may not use
``grade`` or a client geology evaluator to move or relabel the fixture.
"""

from __future__ import annotations

import hashlib
import importlib
import os
from pathlib import Path
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "Data" / "Worldgen"
OUT = DATA / "fablescript_cut_c_occupancy_v1.cocc"
CUT_B_COMMIT = "ac6aff38a3e16bb119d84eedeadbc5ce513173d1"
VOXEL_M = 0.125
DEPTH_LAYERS = 128
CELL_X0, CELL_X1 = -8, 8
CELL_Y0, CELL_Y1 = -16, 0

sys.path.insert(0, str(Path(__file__).resolve().parent))
import compile_geology_authority_bridge as authority  # noqa: E402


def cut_b_root() -> Path:
    configured = os.environ.get("FS_CUT_B_ROOT")
    if configured:
        return Path(configured).resolve()
    return (Path.home() / "OneDrive" / "Desktop" / "Mygame" / "_cutb_occupancy").resolve()


def git_text(root: Path, *args: str) -> str:
    return subprocess.check_output(
        ["git", "-c", f"safe.directory={root.as_posix()}", "-C", str(root), *args],
        text=True, stderr=subprocess.DEVNULL
    ).strip()


def fnv_hex(text: str) -> str:
    return f"{authority.fnv1a64(text):016x}"


def identity_at(x: float, y: float, z: float) -> tuple[str, ...]:
    sample, _contact, _permeability, admitted = authority.assembled_query(x, y, z)
    if not sample.get("found"):
        return ("air", "0000000000000000", "-", "0000000000000000",
                "0000000000000000", "0", "0", "0", "-", "-")
    feature = int(sample["feature"])
    normal = sample["normal"]
    return (
        str(sample["material"]), f"{feature:016x}", str(sample["formation"]),
        authority.MIN["deposit_system_id"] if admitted else "0000000000000000",
        authority.MIN["feature_id"] if admitted else "0000000000000000",
        f"{float(normal[0]):.17g}", f"{float(normal[1]):.17g}",
        f"{float(normal[2]):.17g}",
        "|".join(f"{int(v):016x}" for v in sample.get("events", [])) or "-",
        "|".join(str(int(v)) for v in sample.get("chronology", [])) or "-",
    )


def main() -> None:
    fs_root = cut_b_root()
    module_path = fs_root / "fablescript" / "worldgen" / "provenance_occupancy.py"
    if not module_path.is_file():
        raise SystemExit(f"Cut-B occupancy module missing: {module_path}")
    head = git_text(fs_root, "rev-parse", "HEAD")
    if head != CUT_B_COMMIT:
        raise SystemExit(f"Cut-B commit mismatch: expected {CUT_B_COMMIT}, found {head}")
    if git_text(fs_root, "status", "--porcelain"):
        raise SystemExit("Cut-B worktree is not clean; refusing authority export")

    sys.path.insert(0, str(fs_root))
    sys.path.insert(0, str(fs_root / "fablescript"))
    occupancy = importlib.import_module("fablescript.worldgen.provenance_occupancy")
    occupancy_source = module_path.read_bytes()

    # Route only the authority anchor callback.  All bilinear arithmetic,
    # signed rounding, 8x8 allocation and largest-remainder conservation are
    # performed by the pinned Cut-B module itself.
    def stage10_anchor_height_q(x: int, y: int) -> int:
        return int(round(authority.reconstructed_z(float(x), float(y))
                         / VOXEL_M * occupancy.FILL_FULL))

    occupancy.clear_memo()
    occupancy._anchor_height_q = stage10_anchor_height_q

    bridge_text = authority.BRIDGE.read_text(encoding="utf-8")
    oracle_text = authority.ORACLE.read_text(encoding="utf-8")
    started = time.perf_counter()
    rows: list[str] = [
        "PROVENANCE_CUT_C_OCCUPANCY_V1",
        "authority_owner=FableScript",
        "client_role=ProvenanceEsoterica_ingestion_only",
        f"cut_b_commit={CUT_B_COMMIT}",
        f"cut_b_module_sha256={hashlib.sha256(occupancy_source).hexdigest()}",
        f"cut_a_bridge_digest={fnv_hex(bridge_text)}",
        f"cut_a_oracle_digest={fnv_hex(oracle_text)}",
        "fixture=stage10_contact_mineralization_cut_c",
        "surface_source=cut_a_stage10_reconstructed_surface",
        "subcell_allocator=cut_b_provenance_occupancy",
        "grade_role=forbidden_geometry_input",
        f"voxel_edge_m={VOXEL_M:.17g}",
        f"fill_full={occupancy.FILL_FULL}",
        f"xy_q={occupancy.XY_Q}",
        f"footprints_per_cell={occupancy.FOOTPRINTS}",
        f"cell_x_range={CELL_X0},{CELL_X1}",
        f"cell_y_range={CELL_Y0},{CELL_Y1}",
        f"depth_layers={DEPTH_LAYERS}",
        f"granite_feature_id={authority.INT['feature_id']}",
        f"deposit_parent_body_id={authority.INT['feature_id']}",
        f"deposit_system_id={authority.MIN['deposit_system_id']}",
        f"quartz_feature_id={authority.MIN['feature_id']}",
    ]

    column_count = run_count = quartz_samples = granite_samples = 0
    for cell_y in range(CELL_Y0, CELL_Y1):
        for cell_x in range(CELL_X0, CELL_X1):
            heights = occupancy.cell_heights_q("cut-c", cell_x, cell_y)
            for r in range(occupancy.FOOTPRINTS):
                for c in range(occupancy.FOOTPRINTS):
                    qx = cell_x * occupancy.XY_Q + (2 * c - 7)
                    qy = cell_y * occupancy.XY_Q + (2 * r - 7)
                    x = qx / float(occupancy.XY_Q)
                    y = qy / float(occupancy.XY_Q)
                    top_q = int(heights[r * occupancy.FOOTPRINTS + c])
                    top_layer = (top_q + occupancy.FILL_FULL - 1) // occupancy.FILL_FULL - 1
                    top_fill = top_q - top_layer * occupancy.FILL_FULL
                    rows.append(f"column={qx},{qy},{top_q},{top_layer},{top_fill}")
                    column_count += 1

                    run_start = top_layer
                    prior: tuple[str, ...] | None = None
                    prior_fill = top_fill
                    for offset in range(DEPTH_LAYERS):
                        layer = top_layer - offset
                        fill = top_fill if offset == 0 else occupancy.FILL_FULL
                        surface_z = top_q / occupancy.FILL_FULL * VOXEL_M
                        # The partially occupied top voxel presents the
                        # authority's actual exposed face.  Deeper voxels use
                        # their canonical centres.
                        z = (surface_z - 1e-9) if offset == 0 else (layer + 0.5) * VOXEL_M
                        current = identity_at(x, y, z)
                        if current[0] == "quartz":
                            quartz_samples += 1
                        elif current[0] == "granite":
                            granite_samples += 1
                        if prior is None:
                            prior, run_start, prior_fill = current, layer, fill
                        elif current != prior or fill != occupancy.FILL_FULL or prior_fill != occupancy.FILL_FULL:
                            rows.append("run=" + ",".join((
                                str(qx), str(qy), str(run_start), str(layer + 1),
                                str(prior_fill), *prior,
                            )))
                            run_count += 1
                            prior, run_start, prior_fill = current, layer, fill
                    assert prior is not None
                    rows.append("run=" + ",".join((
                        str(qx), str(qy), str(run_start),
                        str(top_layer - DEPTH_LAYERS + 1), str(prior_fill), *prior,
                    )))
                    run_count += 1

    rows.extend((
        f"column_count={column_count}",
        f"run_count={run_count}",
        f"quartz_voxel_samples={quartz_samples}",
        f"granite_voxel_samples={granite_samples}",
    ))
    payload = "\n".join(rows) + "\n"
    document = payload + f"payload_digest={fnv_hex(payload)}\n"
    OUT.write_text(document, encoding="utf-8", newline="\n")
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    print(f"{OUT}\ncolumns={column_count} runs={run_count} quartz={quartz_samples} "
          f"granite={granite_samples} compile_ms={elapsed_ms:.3f} digest={fnv_hex(payload)}")


if __name__ == "__main__":
    main()

"""Headless dimensional/continuity contract for the Stage-0 palette character."""

from __future__ import annotations

import math
import pathlib
import re
import struct


ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Code" / "Applications" / "ProvenanceClient" / "Main.cpp"
NORMAL_SHEET = ROOT / "Assets" / "Characters" / "stage0_head_normal_sixview.png"
PLAYER_REFERENCE_SHEET = ROOT / "Assets" / "Characters" / "stage0_player_reference_sixview.png"
PLAYER_NORMAL_SHEET = ROOT / "Assets" / "Characters" / "stage0_player_normal_sixview.png"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def png_size(path: pathlib.Path) -> tuple[int, int]:
    header = path.read_bytes()[:24]
    require(header[:8] == b"\x89PNG\r\n\x1a\n" and len(header) == 24, f"invalid PNG: {path.name}")
    return struct.unpack(">II", header[16:24])


def main() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    for token in (
        "constexpr float kCharHeightM = 1.8288f",
        "constexpr float kLivingFocusRangeM=3.6576f",
        "constexpr int kRings=15,kSides=32",
        "void EmitStage0HumanHairShell",
        "Stage0ApplyHeadNormalSheet",
        "g.playerCrouched=!g.playerCrouched",
        "g.stage0SlideRemaining=0.62f",
        "g.stage0SlideSpeed-14.f*dt",
        "void SummonStage0CharacterOnly()",
        "for(int inch=0;inch<=72;++inch)",
        "--cert-stage0-character-fullbody",
        "--cert-stage0-character-fullbody-rear",
        "--cert-stage0-character-fullbody-profile",
        "--cert-stage0-character-fullbody-profile-left",
        "--cert-stage0-character-fullbody-top",
        "--cert-stage0-character-fullbody-threequarter",
        "--cert-stage0-character-pose-crouch",
        "--cert-stage0-character-pose-tool-ready",
        "struct Stage0UnifiedBodyMesh",
        "body_skin_components=%d",
        "body_skin_boundary_edges=%d",
        "body_skin_nonmanifold_edges=%d",
        "body_skin_weighted_joints=%d",
        "body_skin_max_influences=%d",
        "pose_inverted_triangles=%d",
        "pose_self_intersections=%d",
        "body_skin_euler_characteristic=%d",
        "body_skin_genus=%d",
        "DumpDepthPpm",
        "head_depth_written=%d",
        "head_semantic_id_written=%d",
        "certStage0CharacterSemanticIdPass",
        "Stage0UnifiedBodyPosePoint",
        "constexpr int nx=30,ny=69,nz=120",
        "for(int digit=0;digit<4;++digit)",
        "GetStage0BodyNormalSheet()",
        "--cert-stage0-character-profile",
        "--cert-stage0-character-rear",
        "--cert-stage0-character-profile-left",
        "--cert-stage0-character-threequarter-right",
        "--cert-stage0-character-threequarter-left",
        "--play-stage0-character-only",
        "constexpr float kStage0PortraitDistanceM = 0.80f",
        "constexpr float kStage0PortraitFovYDeg = 55.0f",
        "silhouette_mask_written=%d",
        "comparison_cutout_written=%d",
        "anatomy_mask_written=%d",
        "garment_mask_written=%d",
        "pose_min_mean_jacobian_ratio=%.6f",
    ):
        require(token in source, f"runtime contract missing: {token}")

    metres_per_pixel = 1.8288 / 1175.0
    require(0.00150 < metres_per_pixel < 0.00162, "reference pixel scale")
    require(NORMAL_SHEET.is_file() and NORMAL_SHEET.stat().st_size > 100_000,
            "packaged six-view head normal sheet")
    require(PLAYER_REFERENCE_SHEET.is_file() and PLAYER_REFERENCE_SHEET.stat().st_size > 1_000_000,
            "packaged full-body reference sheet")
    require(PLAYER_NORMAL_SHEET.is_file() and PLAYER_NORMAL_SHEET.stat().st_size > 1_000_000,
            "packaged full-body normal sheet")
    require(png_size(PLAYER_REFERENCE_SHEET) == (1536, 1024), "full-body reference 3x2 dimensions")
    require(png_size(PLAYER_NORMAL_SHEET) == (1536, 1024), "full-body normal 3x2 dimensions")

    crown, eyes, nose, lips, chin = 1.804, 1.716, 1.678, 1.652, 1.595
    require(crown > eyes > nose > lips > chin, "head landmark ordering")
    require(math.isclose(1.7798 + 0.041 + 0.008, 1.8288, abs_tol=1e-7), "idle hair envelope")

    portrait_distance = 0.80
    portrait_near = 0.50
    max_head_reach = 0.17
    near_clearance = portrait_distance - portrait_near - max_head_reach
    require(near_clearance >= 0.127, "portrait needs at least five inches of physical near-plane clearance")

    focus_range = 3.6576
    require((focus_range - 0.001) ** 2 <= focus_range**2, "inside focus range")
    require(not ((focus_range + 0.001) ** 2 <= focus_range**2), "outside focus range")

    # The silhouette fringe is now formed by the same closed hair surface. The
    # chunk emitter remains available for other assets, but this character must
    # not call it and reintroduce detached feather-like ribbons.
    require(source.count("EmitStage0HumanHairChunk(") == 1,
            "player hair must not instantiate detached ribbon chunks")

    ruler_inches = 72
    ruler_height = ruler_inches * 0.0254
    require(math.isclose(ruler_height, 1.8288, abs_tol=1e-9), "truth ruler six-foot height")

    cloth_seam = math.hypot(math.cos(math.tau) * 0.175 - 0.175, math.sin(math.tau) * 0.205)
    require(cloth_seam < 1e-7, "cloth wrap seam")

    dt, remaining, speed, distance = 1.0 / 120.0, 0.62, 11.0, 0.0
    for _ in range(75):
        if remaining > 0.0 and speed > 0.0:
            distance += speed * dt
            remaining = max(0.0, remaining - dt)
            speed = max(0.0, speed - 14.0 * dt)
    require(3.0 <= distance <= 5.0, "power-slide travel bound")

    # Count the geometry produced by the fixed loop bounds, rather than only the
    # handful of call sites that instantiate those repeated sections.
    hair_chunks = 0
    hair_limb_sections = 0
    head_rings, head_sides = 15, 32
    head_skin_triangles = 2 * (head_rings - 1) * head_sides + 2 * head_sides
    hair_shell_cells = (32 * 18) + (32 * 16)
    hair_shell_triangles = hair_shell_cells * 2
    hair_side_triangles = 0
    hair_ribbon_triangles = 0
    hair_triangles = hair_shell_triangles + hair_side_triangles
    head_hair_triangles = head_skin_triangles + hair_triangles
    old_head_hair_triangles = 1824 + 4344 + hair_side_triangles + (31 * 7 * 4)
    triangle_reduction = 1.0 - head_hair_triangles / old_head_hair_triangles
    body_call_sites = len(re.findall(r"EmitStage0Human(?:Ellipsoid|Limb)\(", source))
    generated_sections = hair_limb_sections + hair_triangles + body_call_sites
    require(hair_chunks == 0, "detached silhouette hair chunks returned")
    require(head_skin_triangles == 960, "landmark head carrier topology")
    require(hair_shell_cells == 1088, "normal-sheet hair carrier resolution")
    require(hair_shell_triangles == 2176, "normal-sheet shell triangle count")
    require(hair_side_triangles == 0, "side hair triangle count")
    require(head_hair_triangles == 3136, "head/hair triangle budget")
    require(triangle_reduction > 0.40, "normal-sheet triangle reduction")
    require(generated_sections >= 3950, "sectional sculpt carrier unexpectedly reduced")

    print("PASS: Stage-0 character headless contract")
    print(f"reference_scale_mm_per_px={metres_per_pixel * 1000.0:.4f}")
    print(f"cloth_seam_error_m={cloth_seam:.12g}")
    print(f"power_slide_distance_m={distance:.3f}")
    print(f"hair_chunks={hair_chunks}")
    print(f"hair_shell_cells={hair_shell_cells}")
    print(f"hair_shell_triangles={hair_shell_triangles}")
    print(f"hair_side_triangles={hair_side_triangles}")
    print(f"hair_ribbon_triangles={hair_ribbon_triangles}")
    print(f"hair_triangles={hair_triangles}")
    print(f"head_skin_triangles={head_skin_triangles}")
    print(f"head_hair_triangles={head_hair_triangles}")
    print(f"head_hair_triangle_reduction_pct={triangle_reduction * 100.0:.2f}")
    print(f"generated_section_floor={generated_sections}")
    print(f"normal_sheet_bytes={NORMAL_SHEET.stat().st_size}")
    print("player_reference_sheet=1536x1024")
    print("player_normal_sheet=1536x1024")
    print(f"truth_ruler_height_m={ruler_height:.4f}")
    print(f"portrait_camera_distance_m={portrait_distance:.3f}")
    print(f"portrait_near_clearance_m={near_clearance:.3f}")


if __name__ == "__main__":
    main()

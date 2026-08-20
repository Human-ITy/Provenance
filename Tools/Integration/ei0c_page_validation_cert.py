#!/usr/bin/env python3
"""EI0.C corruption, parity, lineage, and atomic-publication certificate."""

from __future__ import annotations

import argparse
import hashlib
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WORLDGEN = ROOT / "Tools" / "Worldgen"
sys.path.insert(0, str(WORLDGEN))

import macro_page_contract as contract  # noqa: E402


EXPECTED_PAYLOAD_SHA = {
    "height_grid_row_major": "2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826",
    "surface_grid_row_major": "138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7",
    "water_grid_row_major": "6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36",
}


def coord(path: Path) -> tuple[int, int]:
    left, right = path.stem.removeprefix("page_").rsplit("_", 1)
    return int(left), int(right)


def emit(magic: str, values: dict[str, str]) -> str:
    return magic + "\n" + "\n".join(f"{key}={value}" for key, value in values.items()) + "\n"


def mutate(text: str, key: str, value: str, recompute_page=False) -> str:
    magic, values = contract._records(text)
    values[key] = value
    if recompute_page:
        values["page_digest"] = contract.page_digest(magic, values)
    return emit(magic, values)


def mutate_token(text: str, key: str, index: int, token: str) -> str:
    magic, values = contract._records(text)
    tokens = values[key].split(); tokens[index] = token
    values[key] = " ".join(tokens)
    return emit(magic, values)


def legacy_form(text: str) -> str:
    _, values = contract._records(text)
    keep = {key: value for key, value in values.items() if key in {
        "region_id", "region_cell", "world_seed", "generator_version", "world_identity_hash",
        "region_min_x_m", "region_min_y_m", "region_size_m", "sample_step_m", "grid_n",
        "source_digest", "height_grid_row_major", "surface_descriptor_version", "surface_step_m",
        "surface_grid_n", "surface_digest", "surface_grid_row_major", "water_descriptor_version",
        "water_step_m", "water_grid_n", "water_digest", "water_grid_row_major",
        "no_wrap_into_region", "cheap_source", *(f"neighbor_{d}" for d in
        ("n", "s", "e", "w", "ne", "nw", "se", "sw"))}}
    return emit(contract.LEGACY_MAGIC, keep)


def superseded_identity_form(text: str) -> str:
    magic, values = contract._records(text)
    values.update(contract.LEGACY_GENESIS_IDENTITY)
    values["genesis_digest"] = contract.LEGACY_GENESIS_DIGEST
    values["page_digest"] = contract.page_digest(magic, values)
    return emit(magic, values)


def assert_failure(name: str, text: str, c: tuple[int, int], expected: str):
    result = contract.validate_text(text, c, contract.GENESIS_DIGEST)
    if result.authoritative or result.failure != expected:
        raise AssertionError(f"{name}: expected {expected}, got {result.trust}/{result.failure}: {result.detail}")


def aggregate_payload(pages: list[Path], key: str) -> str:
    digest = hashlib.sha256()
    for page in pages:
        _, values = contract._records(page.read_text(encoding="utf-8"))
        digest.update(page.name.encode("utf-8")); digest.update(b"\r\n")
        digest.update(f"{key}={values[key]}".encode("utf-8")); digest.update(b"\r\n")
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine-genesis-manifest", type=Path)
    args = parser.parse_args()
    pages = sorted((ROOT / "Data" / "Worldgen" / "MacroAuthority").glob("*.mcp"))
    if len(pages) != 25:
        raise AssertionError(f"expected 25 pages, found {len(pages)}")

    start = time.perf_counter(); results = []
    for page in pages:
        result = contract.load_and_validate(page, coord(page), contract.GENESIS_DIGEST)
        if not result.authoritative:
            raise AssertionError(f"{page.name}: {result.failure}: {result.detail}")
        results.append(result)
    elapsed_ms = (time.perf_counter() - start) * 1000.0

    for key, expected in EXPECTED_PAYLOAD_SHA.items():
        actual = aggregate_payload(pages, key)
        if actual != expected:
            raise AssertionError(f"semantic payload drift for {key}: {actual}")

    sample_path = next(page for page in pages if coord(page) == (0, 0))
    text = sample_path.read_text(encoding="utf-8")
    _, values = contract._records(text)
    surface = values["surface_grid_row_major"].split()
    water = values["water_grid_row_major"].split()
    height = values["height_grid_row_major"].split()
    corruptions = [
        ("genesis_digest", mutate(text, "genesis_digest", "0"*64), contract.Failure.WRONG_GENESIS),
        ("generator_identity", mutate(text, "generator_family", "wrong.generator"), contract.Failure.WRONG_GENERATOR),
        ("material_registry", mutate(text, "material_registry_digest", "1"*64), contract.Failure.WRONG_MATERIAL_REGISTRY),
        ("page_coordinate", mutate(text, "region_cell", "1,0"), contract.Failure.WRONG_PAGE_COORD),
        ("dimensions", mutate(text, "grid_n", "64"), contract.Failure.INVALID_RANGE),
        ("sample_spacing", mutate(text, "sample_step_m", "999.0"), contract.Failure.INVALID_RANGE),
        ("surface_encoding", mutate(text, "surface_descriptor_version", "999"), contract.Failure.UNSUPPORTED_SURFACE_ENCODING),
        ("water_encoding", mutate(text, "water_descriptor_version", "999"), contract.Failure.UNSUPPORTED_WATER_ENCODING),
        ("terrain_payload", mutate_token(text, "height_grid_row_major", 0, str(float(height[0])+0.01)), contract.Failure.SOURCE_DIGEST_MISMATCH),
        ("surface_payload", mutate_token(text, "surface_grid_row_major", 0, f"{int(surface[0],16)^0x400:x}"), contract.Failure.SURFACE_DIGEST_MISMATCH),
        ("water_payload", mutate_token(text, "water_grid_row_major", 0, f"{int(water[0],16)^0x400:x}"), contract.Failure.WATER_DIGEST_MISMATCH),
        ("source_digest", mutate(text, "source_digest", "0"*16), contract.Failure.SOURCE_DIGEST_MISMATCH),
        ("surface_digest", mutate(text, "surface_digest", "0"*16), contract.Failure.SURFACE_DIGEST_MISMATCH),
        ("water_digest", mutate(text, "water_digest", "0"*16), contract.Failure.WATER_DIGEST_MISMATCH),
        ("page_digest", mutate(text, "page_digest", "0"*64), contract.Failure.PAGE_DIGEST_MISMATCH),
        ("truncation", mutate(text, "height_count", str(len(height)-1)), contract.Failure.TRUNCATED_PAYLOAD),
        ("reserved_surface", mutate_token(text, "surface_grid_row_major", 0, f"{(int(surface[0],16)&~0xf)|0xf:x}"), contract.Failure.INVALID_ENUM),
        ("reserved_water", mutate_token(text, "water_grid_row_major", 0, f"{(int(water[0],16)&~0x78)|0x78:x}"), contract.Failure.INVALID_ENUM),
        ("invalid_authority_scope", mutate_token(text, "water_grid_row_major", 0, f"{int(water[0],16)|(1<<59):x}"), contract.Failure.INVALID_ENUM),
        ("parent_lineage", mutate(text, "parent_supertile_id", "0"*16), contract.Failure.LINEAGE_MISMATCH),
    ]
    for name, artifact, expected in corruptions:
        assert_failure(name, artifact, (0, 0), expected)

    # World UUID is intentionally not a page input: distinct instances sharing G
    # receive the same authority result.  A different G is rejected.
    for _world_uuid in ("11111111-1111-4111-8111-111111111111", "22222222-2222-4222-8222-222222222222"):
        if not contract.validate_text(text, (0, 0), contract.GENESIS_DIGEST).authoritative:
            raise AssertionError("same genesis/different world UUID rejected")
    if contract.validate_text(text, (0, 0), "f"*64).failure != contract.Failure.WRONG_GENESIS:
        raise AssertionError("different genesis accepted")

    legacy = legacy_form(text)
    canonical_legacy = contract.validate_text(legacy, (0, 0), contract.GENESIS_DIGEST)
    diagnostic_legacy = contract.validate_text(legacy, (0, 0), contract.GENESIS_DIGEST, True)
    if canonical_legacy.trust != contract.Trust.QUARANTINED or diagnostic_legacy.trust != contract.Trust.DIAGNOSTIC_ONLY:
        raise AssertionError("legacy authority/diagnostic separation failed")
    if canonical_legacy.authoritative or diagnostic_legacy.authoritative:
        raise AssertionError("legacy page became authority-capable")

    superseded = superseded_identity_form(text)
    canonical_old = contract.validate_text(superseded, (0, 0), contract.GENESIS_DIGEST)
    diagnostic_old = contract.validate_text(
        superseded, (0, 0), contract.GENESIS_DIGEST, True)
    if (canonical_old.trust != contract.Trust.QUARANTINED or
            diagnostic_old.trust != contract.Trust.DIAGNOSTIC_ONLY or
            canonical_old.failure != contract.Failure.WRONG_GENESIS or
            diagnostic_old.failure != contract.Failure.WRONG_GENESIS):
        raise AssertionError("superseded identity authority/diagnostic separation failed")
    if canonical_old.authoritative or diagnostic_old.authoritative:
        raise AssertionError("superseded identity became authority-capable")

    valid = contract.validate_text(text, (0, 0), contract.GENESIS_DIGEST)
    corrupt = contract.validate_text(corruptions[8][1], (0, 0), contract.GENESIS_DIGEST)
    slot = contract.TrustedPageSlot()
    if not slot.publish_candidate(valid):
        raise AssertionError("valid initial publication failed")
    original_digest = slot.published.values["page_digest"]
    if slot.publish_candidate(corrupt) or slot.published.values["page_digest"] != original_digest:
        raise AssertionError("corrupt replacement displaced trusted page")
    quarantined_slot = contract.TrustedPageSlot()
    if quarantined_slot.publish_candidate(corrupt) or not quarantined_slot.publish_candidate(valid):
        raise AssertionError("quarantined-to-valid atomic publication failed")

    deferred = sum(1 for result in results for code in result.water_codes
                   if contract.descriptors.decode_water(contract.descriptors.WATER_ENCODING_VERSION, code).macro_authority
                   == contract.descriptors.WaterAuthority.DEFER_TO_DETAILED)
    if deferred != 361:
        raise AssertionError(f"DEFER_TO_DETAILED parity drift: {deferred}")

    if args.engine_genesis_manifest:
        import json
        product = json.loads(args.engine_genesis_manifest.read_text(encoding="utf-8"))
        manifest = {name: product[name] for name in contract.GENESIS_FIELDS}
        if (manifest != contract.GENESIS_IDENTITY or
                contract.genesis_digest(manifest) != contract.GENESIS_DIGEST or
                product.get("genesis_digest", contract.GENESIS_DIGEST) != contract.GENESIS_DIGEST):
            raise AssertionError("engine GenesisIdentity manifest differs from macro pages")

    print("EI0.C PAGE VALIDATION CERT: PASS")
    print(f"pages=25 corruption_cases={len(corruptions)} deferred_cells={deferred}")
    print(f"genesis_digest={contract.GENESIS_DIGEST}")
    print(f"descriptor_schema_digest={contract.descriptors.SCHEMA_DIGEST}")
    print(f"validation_25_pages_ms={elapsed_ms:.3f} validation_per_page_ms={elapsed_ms/25:.3f}")
    for key, value in EXPECTED_PAYLOAD_SHA.items():
        print(f"{key}_sha256={value}")


if __name__ == "__main__":
    main()

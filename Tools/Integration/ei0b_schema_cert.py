#!/usr/bin/env python3
"""Certify generated descriptor bindings against the frozen `.mcp` corpus."""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WORLDGEN = ROOT / "Tools" / "Worldgen"
sys.path.insert(0, str(WORLDGEN))

import generated_world_descriptors as schema  # noqa: E402


EXPECTED_SCHEMA_DIGEST = "8857189f2dfb5fe3ba73d470ca653248188a0717469963d64d6d5003bad6dac5"


def parse_page(path: Path):
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            values[key] = value
    return values


def legacy_surface_codes(packed: int):
    return (
        packed & 0xF, (packed >> 4) & 0x7, (packed >> 7) & 0x7,
        *((packed >> offset) & 0xF for offset in (10, 14, 18, 22, 26, 30, 34)))


def legacy_water_codes(packed: int):
    return (
        packed & 0x7, (packed >> 3) & 0xF, (packed >> 7) & 0x7,
        (packed >> 10) & 0x7, (packed >> 13) & 0x1FF,
        *((packed >> offset) & 0xF for offset in (22, 26, 30, 34, 38, 42, 46, 50)),
        (packed >> 54) & 0x3, (packed >> 56) & 0x3, (packed >> 58) & 0x1)


def certify_page(path: Path):
    page = parse_page(path)
    surface_version = int(page["surface_descriptor_version"])
    water_version = int(page["water_descriptor_version"])
    if surface_version != schema.SURFACE_ENCODING_VERSION:
        raise AssertionError(f"{path.name}: unsupported SurfaceState version")
    if water_version != schema.WATER_ENCODING_VERSION:
        raise AssertionError(f"{path.name}: unsupported WaterState version")

    surface_count = water_count = deferred_count = 0
    for token in page["surface_grid_row_major"].split():
        packed = int(token, 16)
        decoded = schema.decode_surface(surface_version, packed)
        if tuple(decoded.quantized) != legacy_surface_codes(packed)[3:]:
            raise AssertionError(f"{path.name}: SurfaceState decoder drift")
        reencoded = schema.encode_surface(
            decoded.substrate_class, decoded.lithology_class,
            decoded.dominant_surface_family, decoded.wetness, decoded.weathering,
            decoded.soil_depth, decoded.stability, decoded.organic_potential,
            decoded.exposure, decoded.roughness_proxy, version=surface_version)
        if reencoded != packed:
            raise AssertionError(f"{path.name}: SurfaceState round-trip drift")
        surface_count += 1

    for token in page["water_grid_row_major"].split():
        packed = int(token, 16)
        decoded = schema.decode_water(water_version, packed)
        legacy = legacy_water_codes(packed)
        if tuple(decoded.quantized) != legacy[4:15]:
            raise AssertionError(f"{path.name}: WaterState decoder drift")
        reencoded = schema.encode_water(
            decoded.encoded_presence, decoded.body_class, decoded.flow_regime,
            decoded.bottom_family, decoded.depth_m, decoded.discharge_proxy,
            decoded.seasonality_index, decoded.clarity, decoded.turbidity,
            decoded.suspended_sediment, decoded.mineral_load, decoded.organic_load,
            decoded.temperature_proxy, decoded.waterfall_potential,
            decoded.mineral_potential, decoded.macro_authority,
            version=water_version)
        if reencoded != packed:
            raise AssertionError(f"{path.name}: WaterState round-trip drift")
        if decoded.macro_authority == schema.WaterAuthority.DEFER_TO_DETAILED:
            if decoded.presence is not None or decoded.has_presence_conclusion:
                raise AssertionError(f"{path.name}: deferred cell asserts dry")
            deferred_count += 1
        water_count += 1
    return surface_count, water_count, deferred_count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine-root", type=Path)
    args = parser.parse_args()
    if schema.SCHEMA_DIGEST != EXPECTED_SCHEMA_DIGEST:
        raise AssertionError("unexpected generated schema digest")
    if args.engine_root:
        generator = args.engine_root / "schema_tools" / "generate_world_descriptors.py"
        subprocess.run(
            [sys.executable, str(generator), "--client-root", str(ROOT), "--check"],
            cwd=args.engine_root, check=True)

    corpus = sorted((ROOT / "Data" / "Worldgen" / "MacroAuthority").glob("*.mcp"))
    if not corpus:
        raise AssertionError("no committed MacroAuthority pages found")
    totals = [0, 0, 0]
    digest = hashlib.sha256()
    for page in corpus:
        digest.update(page.read_bytes())
        counts = certify_page(page)
        totals = [left + right for left, right in zip(totals, counts)]
    print("EI0.B SCHEMA CERT: PASS")
    print(f"schema_digest={schema.SCHEMA_DIGEST}")
    print(f"pages={len(corpus)} surface_codes={totals[0]} water_codes={totals[1]}")
    print(f"deferred_without_presence_conclusion={totals[2]}")
    print(f"corpus_sha256={digest.hexdigest()}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""EI0.E reproducible identity, semantic parity, and page reissue certificate."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import shutil
import sys
import tempfile
from pathlib import Path


CLIENT_ROOT = Path(__file__).resolve().parents[2]
CLIENT_WORLDGEN = CLIENT_ROOT / "Tools" / "Worldgen"
sys.path.insert(0, str(CLIENT_WORLDGEN))

import macro_page_contract as contract  # noqa: E402


EXPECTED_PAYLOADS = {
    "height_grid_row_major": "2e29df623ed3659825a5d151193bd5601e0ea6910d746651782b13024a0e2826",
    "surface_grid_row_major": "138a37c1341ae4dc9b3869785bda1a3331a3389a60badc8eca346ba42e7231a7",
    "water_grid_row_major": "6a6a92f581211bd4766b11887d89cd540416b4782992b43f32ad4b63f8b83a36",
}


def payload_hash(pages, key):
    digest = hashlib.sha256()
    for page in pages:
        _, values = contract._records(page.read_text(encoding="utf-8"))
        digest.update(page.name.encode("utf-8")); digest.update(b"\r\n")
        digest.update(f"{key}={values[key]}".encode("utf-8")); digest.update(b"\r\n")
    return digest.hexdigest()


def corpus_hash(pages):
    digest = hashlib.sha256()
    for page in pages:
        digest.update(page.name.encode("utf-8")); digest.update(b"\r\n")
        digest.update(page.read_bytes()); digest.update(b"\r\n")
    return digest.hexdigest()


def coord(page):
    left, right = page.stem.removeprefix("page_").rsplit("_", 1)
    return int(left), int(right)


def emit(magic, values):
    return magic + "\n" + "\n".join(
        f"{key}={value}" for key, value in values.items()) + "\n"


def superseded_page(text):
    magic, values = contract._records(text)
    values.update(contract.LEGACY_GENESIS_IDENTITY)
    values["genesis_digest"] = contract.LEGACY_GENESIS_DIGEST
    values["page_digest"] = contract.page_digest(magic, values)
    return emit(magic, values)


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine-root", type=Path, required=True)
    args = parser.parse_args(argv)
    engine = args.engine_root.resolve()
    sys.path.insert(0, str(engine))

    from engine.canonical_identity import genesis_digest, provenance_macro_genesis_identity
    from worldgen.provenance_macro_identity import (
        build_generator_manifest, canonical_json_bytes, generator_build_digest,
        load_pinned_manifest,
    )

    macro = CLIENT_WORLDGEN / "macro_authority.py"
    central = CLIENT_ROOT / "Data" / "Worldgen" / "causal_world_macro_provinces_floor.cmp"
    schema = engine / "schemas" / "world_descriptors.json"
    actual_manifest = build_generator_manifest(macro, schema, central)
    pinned_manifest = load_pinned_manifest()
    if canonical_json_bytes(actual_manifest) != canonical_json_bytes(pinned_manifest):
        raise AssertionError("pinned manifest does not reproduce from the oracle")

    identity = provenance_macro_genesis_identity()
    product = json.loads((
        CLIENT_WORLDGEN / "provenance_macro_identity.generated.json"
    ).read_text(encoding="utf-8"))
    if {name: product[name] for name in contract.GENESIS_FIELDS} != identity:
        raise AssertionError("client generated identity differs from engine derivation")
    if product["genesis_digest"] != genesis_digest(identity):
        raise AssertionError("client generated genesis digest is stale")

    with tempfile.TemporaryDirectory() as folder:
        roots = [Path(folder) / "checkout-a", Path(folder) / "nested" / "checkout-b"]
        rebuilt = []
        for root in roots:
            root.mkdir(parents=True)
            shutil.copyfile(macro, root / "macro_authority.py")
            shutil.copyfile(schema, root / "world_descriptors.json")
            shutil.copyfile(central, root / "central.cmp")
            rebuilt.append(build_generator_manifest(
                root / "macro_authority.py", root / "world_descriptors.json",
                root / "central.cmp"))
        if canonical_json_bytes(rebuilt[0]) != canonical_json_bytes(rebuilt[1]):
            raise AssertionError("checkout relocation changed generator manifest")

        macro_text = macro.read_text(encoding="utf-8")
        semantic_change = macro_text.replace(
            "ANCHOR_OUTER_M = 64000.0", "ANCHOR_OUTER_M = 65000.0", 1)
        (roots[1] / "macro_authority.py").write_text(semantic_change, encoding="utf-8")
        changed = build_generator_manifest(
            roots[1] / "macro_authority.py", roots[1] / "world_descriptors.json",
            roots[1] / "central.cmp")
        if generator_build_digest(changed) == generator_build_digest(rebuilt[0]):
            raise AssertionError("semantic generator mutation did not change build digest")

        representation_change = macro_text.replace(
            "# MV2.A regional macro authority page. Macro forcing only; not fine terrain.",
            "# representation-only Page V2 comment", 1)
        descriptor = json.loads(schema.read_text(encoding="utf-8"))
        descriptor["protocol"]["protocol_semver"] = "99.0.0"
        descriptor["descriptors"]["SurfaceState"]["encoding_version"] = 99
        descriptor["descriptors"]["WaterState"]["encoding_version"] = 99
        (roots[1] / "macro_authority.py").write_text(representation_change, encoding="utf-8")
        (roots[1] / "world_descriptors.json").write_text(
            json.dumps(descriptor, indent=2), encoding="utf-8")
        reencoded = build_generator_manifest(
            roots[1] / "macro_authority.py", roots[1] / "world_descriptors.json",
            roots[1] / "central.cmp")
        if generator_build_digest(reencoded) != generator_build_digest(rebuilt[0]):
            raise AssertionError("representation-only change renamed generator build")

    pages = sorted((CLIENT_ROOT / "Data" / "Worldgen" / "MacroAuthority").glob("*.mcp"))
    if len(pages) != 25:
        raise AssertionError("canonical page ring is not 25 pages")
    page_digests = set()
    for page in pages:
        result = contract.load_and_validate(page, coord(page), contract.GENESIS_DIGEST)
        if not result.authoritative:
            raise AssertionError(f"{page.name}: {result.failure}: {result.detail}")
        page_digests.add(result.values["page_digest"])
    if len(page_digests) != 25:
        raise AssertionError("corrected page digests are not unique")
    for key, expected in EXPECTED_PAYLOADS.items():
        actual = payload_hash(pages, key)
        if actual != expected:
            raise AssertionError(f"semantic payload drift: {key} {actual}")

    old = superseded_page(pages[12].read_text(encoding="utf-8"))
    rejected = contract.validate_text(old, coord(pages[12]), contract.GENESIS_DIGEST)
    diagnostic = contract.validate_text(
        old, coord(pages[12]), contract.GENESIS_DIGEST, diagnostic_legacy=True)
    if (rejected.trust != contract.Trust.QUARANTINED or
            diagnostic.trust != contract.Trust.DIAGNOSTIC_ONLY or
            rejected.failure != contract.Failure.WRONG_GENESIS or
            diagnostic.authoritative):
        raise AssertionError("superseded identity policy failed")

    source = macro.read_text(encoding="utf-8")
    if "genesis_digest" in source:
        raise AssertionError("static macro authority unexpectedly depends on genesis_digest")
    for namespace in (":prov:", ":range:", ":peak:", ":mwshed:", ":mchan:",
                      ":mwbody:", ":mregime:"):
        if namespace not in source:
            raise AssertionError(f"missing static-ID namespace audit target {namespace}")

    print("EI0.E REPRODUCIBLE GENESIS IDENTITY CERT: PASS")
    print(f"generator_build_digest={generator_build_digest(actual_manifest)}")
    print(f"generator_config_digest={identity['generator_config_digest']}")
    print(f"material_registry_digest={identity['material_registry_digest']}")
    print(f"old_genesis_digest={contract.LEGACY_GENESIS_DIGEST}")
    print(f"corrected_genesis_digest={contract.GENESIS_DIGEST}")
    print("relocation_locations=2 semantic_mutation=detected representation_mutation=stable")
    print("static_ids=seed_plus_absolute_feature_facts genesis_digest_dependency=none")
    for key, expected in EXPECTED_PAYLOADS.items():
        print(f"{key}_sha256={expected}")
    print(f"corrected_page_corpus_sha256={corpus_hash(pages)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

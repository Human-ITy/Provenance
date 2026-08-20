"""EI1 canonical client admission and development-parity harness."""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WORLDGEN = ROOT / "Tools" / "Worldgen"
sys.path.insert(0, str(WORLDGEN))
import macro_page_contract as page_contract


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("engine_root", type=Path)
    parser.add_argument("--development-parity", action="store_true")
    args = parser.parse_args(argv)
    sys.path.insert(0, str(args.engine_root))
    from worldgen.provenance_macro.macro_manifest import load_manifest
    from worldgen.provenance_macro_identity import canonical_macro_semantics_bytes

    cache_root = ROOT / "Data" / "Worldgen" / "MacroAuthority"
    manifest = load_manifest(cache_root / "macro_manifest.mcm", page_contract.GENESIS_DIGEST)
    assert manifest["producer_authority"] == "FABLESCRIPT_WORLDGENESIS"
    assert "world_uuid" not in manifest
    assert len(manifest["pages"]) == 25
    for entry in manifest["pages"]:
        coord = tuple(entry["page_coord"])
        path = cache_root / entry["cache_path"]
        encoded = path.read_bytes()
        assert len(encoded) == entry["payload_size"]
        assert "sha256:" + hashlib.sha256(encoded).hexdigest() == entry["cache_key"]
        page = page_contract.validate_text(
            encoded.decode("utf-8"), coord, page_contract.GENESIS_DIGEST)
        assert page.authoritative, (coord, page.failure, page.detail)
        for name in ("page_digest", "source_digest", "surface_digest", "water_digest"):
            assert page.values[name] == entry[name], (coord, name)

    main_cpp = (ROOT / "Code/Applications/ProvenanceClient/Main.cpp").read_text(encoding="utf-8")
    hello_header = (ROOT / "Code/Applications/ProvenanceClient/Ei0aHandshake.h").read_text(encoding="utf-8")
    assert "MacroManifestAuthority::Load" in main_cpp
    assert "FableScript WorldGenesis produced the immutable cache" in main_cpp
    assert "silently generate local replacement" not in main_cpp
    assert "BuildMacroWorldGenesisHelloParams" in hello_header
    assert "macro_manifest" in hello_header and "macro_page_v2" in hello_header
    if args.development_parity:
        engine_semantics = canonical_macro_semantics_bytes(
            args.engine_root / "worldgen/provenance_macro/authority.py")
        oracle_semantics = canonical_macro_semantics_bytes(WORLDGEN / "macro_authority.py")
        assert engine_semantics == oracle_semantics
    print("EI1_PROVENANCE_CLIENT PASS")
    print("macro_source=ENGINE_WORLDGEN")
    print("pages=25")
    print("genesis_digest={}".format(manifest["genesis_digest"]))
    print("manifest_digest={}".format(manifest["manifest_digest"]))
    print("local_authority_fallback=DISABLED")
    print("development_parity={}".format("PASS" if args.development_parity else "NOT_RUN"))


if __name__ == "__main__":
    main()

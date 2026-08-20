#!/usr/bin/env python3
"""Upgrade committed V1 macro pages to canonical EI0.C V2 metadata."""

from __future__ import annotations

from pathlib import Path

from macro_page_contract import GENESIS_DIGEST, Trust, upgrade_legacy_page_text, validate_text


ROOT = Path(__file__).resolve().parents[2]


def main():
    pages = sorted((ROOT / "Data" / "Worldgen" / "MacroAuthority").glob("*.mcp"))
    if len(pages) != 25:
        raise AssertionError(f"expected 25 pages, found {len(pages)}")
    for path in pages:
        stem = path.stem.removeprefix("page_")
        ri_s, rj_s = stem.rsplit("_", 1)
        coord = (int(ri_s), int(rj_s))
        upgraded = upgrade_legacy_page_text(path.read_text(encoding="utf-8"))
        result = validate_text(upgraded, coord, GENESIS_DIGEST)
        if result.trust != Trust.VALID_AUTHORITY:
            raise AssertionError(f"{path.name}: {result.failure}: {result.detail}")
        path.write_text(upgraded, encoding="utf-8", newline="\n")
    print(f"upgraded={len(pages)} genesis_digest={GENESIS_DIGEST}")


if __name__ == "__main__":
    main()

"""EI0.C canonical macro-page authority contract.

This module validates immutable macro projection pages.  Page and descriptor
schema versions are representation identities; only ``GenesisIdentity`` is
hashed into ``genesis_digest``.  The page binds to a genesis, never to a
playthrough/world UUID.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from decimal import Decimal, InvalidOperation
import hashlib
import json
import math
from pathlib import Path

try:
    import generated_world_descriptors as descriptors
except ModuleNotFoundError:  # package import in focused certs
    from . import generated_world_descriptors as descriptors


MAGIC = "PROVENANCE_MACRO_AUTHORITY_PAGE_V2"
LEGACY_MAGIC = "PROVENANCE_MACRO_AUTHORITY_PAGE_V1"
PAGE_SCHEMA_VERSION = 2
PAGE_DIGEST_ALGORITHM = "sha256-canonical-records-v1"
SOURCE_DIGEST_ALGORITHM = "fnv1a64-height-centimeters-le6-v1"
SURFACE_DIGEST_ALGORITHM = "fnv1a64-packed-le5-v1"
WATER_DIGEST_ALGORITHM = "fnv1a64-packed-le8-v1"
PAYLOAD_ENCODING = "ascii-row-major-v1"
REGION_SIZE_M = Decimal("64000.0")
REGION_HALF_M = Decimal("32000.0")
SUPERTILE_SIZE_M = Decimal("384000.0")
REGION_KEY = "causal_world_macro_provinces_floor"

FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211

GENESIS_FIELDS = (
    "identity_schema_version", "seed", "coordinate_frame_id",
    "generator_family", "generator_version", "generator_build_digest",
    "generator_config_digest", "material_registry_id",
    "material_registry_digest", "surface_grammar_id",
    "surface_grammar_version", "water_grammar_id", "water_grammar_version",
)

_IDENTITY_PRODUCT = json.loads(
    (Path(__file__).with_name("provenance_macro_identity.generated.json"))
    .read_text(encoding="utf-8"))
GENESIS_IDENTITY = {
    name: _IDENTITY_PRODUCT[name] for name in GENESIS_FIELDS
}

# Superseded identities are evidence only. They can be inspected in explicit
# diagnostic mode and can never enter canonical authority.
LEGACY_GENESIS_IDENTITY = {
    **GENESIS_IDENTITY,
    "generator_build_digest": "afa225c1bada4642a3ba6dd79948870e396293a5dd8c5595b1e8c10257a8b72a",
    "generator_config_digest": "f5ad774512951d272fe507700dd4702ac3094f253c75596936a92e1d852cf8a0",
    "material_registry_digest": "e33bf4530caf681caed2d8a0bebe5cd886fd38d4954a480fa4d458149f825c76",
}
LEGACY_GENESIS_DIGEST = "17405cbecb97d55aee9e85408e8735c7f055f6ad87369dfa94bee4762c67e994"
TOOLCHAIN_SUPERSEDED_GENESIS_IDENTITY = {
    **GENESIS_IDENTITY,
    "generator_build_digest": "5d7c1636de50e62dabbae43233e7fc4abdb6d16bc06e79874a92f0010f697d08",
}
TOOLCHAIN_SUPERSEDED_GENESIS_DIGEST = (
    "8394bfefb6955cfffec1c927721d2e6da1b4a24c5525dce4cd238640c2ecd801")
SUPERSEDED_GENESIS_IDENTITIES = (
    (LEGACY_GENESIS_IDENTITY, LEGACY_GENESIS_DIGEST, "opaque EI0.C"),
    (TOOLCHAIN_SUPERSEDED_GENESIS_IDENTITY,
     TOOLCHAIN_SUPERSEDED_GENESIS_DIGEST, "toolchain-sensitive EI0.E/EI2"),
)


def canonical_genesis_bytes(identity: dict) -> bytes:
    if set(identity) != set(GENESIS_FIELDS):
        raise ValueError("malformed GenesisIdentity")
    ordered = {name: identity[name] for name in GENESIS_FIELDS}
    return json.dumps(ordered, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def genesis_digest(identity: dict = GENESIS_IDENTITY) -> str:
    return hashlib.sha256(canonical_genesis_bytes(identity)).hexdigest()


GENESIS_DIGEST = genesis_digest()
if GENESIS_DIGEST != _IDENTITY_PRODUCT["genesis_digest"]:
    raise RuntimeError("stale generated Provenance macro identity")


class Failure:
    NONE = "NONE"
    IO_ERROR = "IO_ERROR"
    LEGACY_PAGE = "LEGACY_PAGE"
    MALFORMED_HEADER = "MALFORMED_HEADER"
    UNSUPPORTED_PAGE_SCHEMA = "UNSUPPORTED_PAGE_SCHEMA"
    WRONG_GENESIS = "WRONG_GENESIS"
    WRONG_GENERATOR = "WRONG_GENERATOR"
    WRONG_MATERIAL_REGISTRY = "WRONG_MATERIAL_REGISTRY"
    WRONG_SCHEMA_DIGEST = "WRONG_SCHEMA_DIGEST"
    WRONG_PAGE_COORD = "WRONG_PAGE_COORD"
    INVALID_RANGE = "INVALID_RANGE"
    TRUNCATED_PAYLOAD = "TRUNCATED_PAYLOAD"
    UNSUPPORTED_SURFACE_ENCODING = "UNSUPPORTED_SURFACE_ENCODING"
    UNSUPPORTED_WATER_ENCODING = "UNSUPPORTED_WATER_ENCODING"
    SOURCE_DIGEST_MISMATCH = "SOURCE_DIGEST_MISMATCH"
    SURFACE_DIGEST_MISMATCH = "SURFACE_DIGEST_MISMATCH"
    WATER_DIGEST_MISMATCH = "WATER_DIGEST_MISMATCH"
    PAGE_DIGEST_MISMATCH = "PAGE_DIGEST_MISMATCH"
    INVALID_ENUM = "INVALID_ENUM"
    LINEAGE_MISMATCH = "LINEAGE_MISMATCH"


class Trust:
    UNLOADED = "UNLOADED"
    VALIDATING = "VALIDATING"
    VALID_AUTHORITY = "VALID_AUTHORITY"
    QUARANTINED = "QUARANTINED"
    DIAGNOSTIC_ONLY = "DIAGNOSTIC_ONLY"


@dataclass
class ValidationResult:
    trust: str = Trust.QUARANTINED
    failure: str = Failure.MALFORMED_HEADER
    detail: str = ""
    values: dict[str, str] = field(default_factory=dict)
    heights: list[float] = field(default_factory=list)
    surface_codes: list[int] = field(default_factory=list)
    water_codes: list[int] = field(default_factory=list)

    @property
    def authoritative(self) -> bool:
        return self.trust == Trust.VALID_AUTHORITY


@dataclass
class TrustedPageSlot:
    """Atomic publication model: an invalid candidate never replaces trust."""

    published: ValidationResult | None = None
    last_rejected: ValidationResult | None = None

    def publish_candidate(self, candidate: ValidationResult) -> bool:
        if not candidate.authoritative:
            self.last_rejected = candidate
            return False
        self.published = candidate
        self.last_rejected = None
        return True


def _fnv_bytes(data: bytes, value: int = FNV_OFFSET) -> int:
    for byte in data:
        value ^= byte
        value = (value * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return value


def fnv_text(text: str) -> str:
    return f"{_fnv_bytes(text.encode('utf-8')):016x}"


def region_id(seed: str, ri: int, rj: int, region_key: str = REGION_KEY) -> str:
    return fnv_text(f"{region_key}:macro:{seed}:({ri},{rj})")


def supertile_cell(ri: int, rj: int) -> tuple[int, int]:
    def one(index: int) -> int:
        center = Decimal(index) * REGION_SIZE_M
        return math.floor((center + SUPERTILE_SIZE_M / 2) / SUPERTILE_SIZE_M)
    return one(ri), one(rj)


def supertile_id(seed: str, sti: int, stj: int, region_key: str = REGION_KEY) -> str:
    return fnv_text(f"{region_key}:macro:{seed}:supertile:({sti},{stj})")


def _records(text: str) -> tuple[str, dict[str, str]]:
    magic = ""
    values: dict[str, str] = {}
    for raw in text.splitlines():
        line = raw[:-1] if raw.endswith("\r") else raw
        if not line or line.startswith("#"):
            continue
        if not magic:
            magic = line
            continue
        if "=" not in line:
            raise ValueError("non-record line")
        key, value = line.split("=", 1)
        if not key or key in values:
            raise ValueError("empty or duplicate field")
        values[key] = value
    return magic, values


def canonical_page_bytes(magic: str, values: dict[str, str]) -> bytes:
    records = [magic]
    records.extend(f"{key}={values[key]}" for key in sorted(values) if key != "page_digest")
    return ("\n".join(records) + "\n").encode("utf-8")


def page_digest(magic: str, values: dict[str, str]) -> str:
    return hashlib.sha256(canonical_page_bytes(magic, values)).hexdigest()


def _height_digest(tokens: list[str]) -> str:
    value = FNV_OFFSET
    for token in tokens:
        q = int(Decimal(token) * 100) & 0xFFFFFFFFFFFFFFFF
        value = _fnv_bytes(bytes((q >> shift) & 0xFF for shift in (0, 8, 16, 24, 32, 40)), value)
    return f"{value:016x}"


def _packed_digest(codes: list[int], byte_count: int) -> str:
    value = FNV_OFFSET
    for code in codes:
        value = _fnv_bytes(bytes((code >> (8 * i)) & 0xFF for i in range(byte_count)), value)
    return f"{value:016x}"


def _fail(code: str, detail: str, values=None, trust=Trust.QUARANTINED) -> ValidationResult:
    return ValidationResult(trust=trust, failure=code, detail=detail, values=values or {})


def validate_text(text: str, requested_coord: tuple[int, int], expected_genesis: str,
                  diagnostic_legacy: bool = False) -> ValidationResult:
    try:
        magic, values = _records(text)
    except ValueError as exc:
        return _fail(Failure.MALFORMED_HEADER, str(exc))
    if magic == LEGACY_MAGIC:
        return _fail(Failure.LEGACY_PAGE, "legacy page is not authority-capable", values,
                     Trust.DIAGNOSTIC_ONLY if diagnostic_legacy else Trust.QUARANTINED)
    if magic != MAGIC:
        return _fail(Failure.MALFORMED_HEADER, "wrong page family", values)

    required = set(GENESIS_FIELDS) | {
        "macro_page_schema_version", "genesis_digest", "descriptor_schema_digest",
        "region_key", "region_id", "region_cell", "region_min_x_m", "region_min_y_m",
        "region_size_m", "sample_step_m", "grid_n", "height_count",
        "source_digest_algorithm", "source_digest", "height_grid_row_major",
        "surface_descriptor_version", "surface_step_m", "surface_grid_n", "surface_count",
        "surface_digest_algorithm", "surface_digest", "surface_grid_row_major",
        "water_descriptor_version", "water_step_m", "water_grid_n", "water_count",
        "water_digest_algorithm", "water_digest", "water_grid_row_major",
        "parent_supertile_cell", "parent_supertile_id", "payload_encoding",
        "page_digest_algorithm", "page_digest", "no_wrap_into_region", "cheap_source",
        "world_seed", "world_identity_hash",
        *(f"neighbor_{d}" for d in ("n", "s", "e", "w", "ne", "nw", "se", "sw")),
    }
    if set(values) != required:
        return _fail(Failure.MALFORMED_HEADER, "missing or unknown canonical field", values)
    try:
        if int(values["macro_page_schema_version"]) != PAGE_SCHEMA_VERSION:
            return _fail(Failure.UNSUPPORTED_PAGE_SCHEMA, "unsupported macro page schema", values)
        identity = {name: values[name] for name in GENESIS_FIELDS}
        superseded = next((entry for entry in SUPERSEDED_GENESIS_IDENTITIES
                           if identity == entry[0]), None)
        legacy_identity = superseded is not None
        if (values["generator_family"] != GENESIS_IDENTITY["generator_family"] or
                values["generator_version"] != GENESIS_IDENTITY["generator_version"] or
                (not legacy_identity and
                 (values["generator_build_digest"] != GENESIS_IDENTITY["generator_build_digest"] or
                  values["generator_config_digest"] != GENESIS_IDENTITY["generator_config_digest"]))):
            return _fail(Failure.WRONG_GENERATOR, "unexpected generator identity", values)
        if (values["material_registry_id"] != GENESIS_IDENTITY["material_registry_id"] or
                (not legacy_identity and
                 values["material_registry_digest"] != GENESIS_IDENTITY["material_registry_digest"])):
            return _fail(Failure.WRONG_MATERIAL_REGISTRY, "unexpected material registry", values)
        declared_genesis = genesis_digest(identity)
        if declared_genesis != values["genesis_digest"]:
            return _fail(Failure.WRONG_GENESIS, "page GenesisIdentity is internally inconsistent", values)
        if legacy_identity:
            if declared_genesis != superseded[1]:
                return _fail(Failure.WRONG_GENESIS, "malformed superseded identity", values)
        elif declared_genesis != expected_genesis:
            return _fail(Failure.WRONG_GENESIS, "page GenesisIdentity does not match session", values)
        if values["descriptor_schema_digest"] != descriptors.SCHEMA_DIGEST:
            return _fail(Failure.WRONG_SCHEMA_DIGEST, "descriptor schema digest mismatch", values)
        if (values["world_seed"] != values["seed"] or values["payload_encoding"] != PAYLOAD_ENCODING or
                values["no_wrap_into_region"] != "1" or
                values["cheap_source"] != "macro_analytic_no_reconstructedz"):
            return _fail(Failure.MALFORMED_HEADER, "invalid canonical page declaration", values)
        ri, rj = (int(part) for part in values["region_cell"].split(","))
        if (ri, rj) != requested_coord:
            return _fail(Failure.WRONG_PAGE_COORD, "stored coordinate differs from requested", values)
        min_x = Decimal(values["region_min_x_m"]); min_y = Decimal(values["region_min_y_m"])
        if (min_x != Decimal(ri) * REGION_SIZE_M - REGION_HALF_M or
                min_y != Decimal(rj) * REGION_SIZE_M - REGION_HALF_M or
                Decimal(values["region_size_m"]) != REGION_SIZE_M):
            return _fail(Failure.WRONG_PAGE_COORD, "absolute bounds differ from coordinate", values)
        step = Decimal(values["sample_step_m"]); n = int(values["grid_n"])
        sstep = Decimal(values["surface_step_m"]); sn = int(values["surface_grid_n"])
        wstep = Decimal(values["water_step_m"]); wn = int(values["water_grid_n"])
        if (step <= 0 or sstep <= 0 or wstep <= 0 or n < 2 or sn < 2 or wn < 2 or
                step * (n - 1) != REGION_SIZE_M or sstep * (sn - 1) != REGION_SIZE_M or
                wstep * (wn - 1) != REGION_SIZE_M):
            return _fail(Failure.INVALID_RANGE, "invalid grid dimensions or spacing", values)
        heights_s = values["height_grid_row_major"].split()
        surface_s = values["surface_grid_row_major"].split()
        water_s = values["water_grid_row_major"].split()
        if (len(heights_s) != n*n or len(surface_s) != sn*sn or len(water_s) != wn*wn or
                int(values["height_count"]) != n*n or int(values["surface_count"]) != sn*sn or
                int(values["water_count"]) != wn*wn):
            return _fail(Failure.TRUNCATED_PAYLOAD, "payload count does not match grid", values)
        heights = [float(value) for value in heights_s]
        if not all(math.isfinite(value) for value in heights):
            return _fail(Failure.INVALID_RANGE, "non-finite terrain value", values)
        surface = [int(value, 16) for value in surface_s]
        water = [int(value, 16) for value in water_s]
        if int(values["surface_descriptor_version"]) != descriptors.SURFACE_ENCODING_VERSION:
            return _fail(Failure.UNSUPPORTED_SURFACE_ENCODING, "unsupported SurfaceState encoding", values)
        if int(values["water_descriptor_version"]) != descriptors.WATER_ENCODING_VERSION:
            return _fail(Failure.UNSUPPORTED_WATER_ENCODING, "unsupported WaterState encoding", values)
        try:
            for code in surface:
                descriptors.decode_surface(descriptors.SURFACE_ENCODING_VERSION, code)
            for code in water:
                descriptors.decode_water(descriptors.WATER_ENCODING_VERSION, code)
        except descriptors.DescriptorError as exc:
            return _fail(Failure.INVALID_ENUM, str(exc), values)
        if values["source_digest_algorithm"] != SOURCE_DIGEST_ALGORITHM or _height_digest(heights_s) != values["source_digest"]:
            return _fail(Failure.SOURCE_DIGEST_MISMATCH, "terrain payload digest mismatch", values)
        if values["surface_digest_algorithm"] != SURFACE_DIGEST_ALGORITHM or _packed_digest(surface, 5) != values["surface_digest"]:
            return _fail(Failure.SURFACE_DIGEST_MISMATCH, "surface payload digest mismatch", values)
        if values["water_digest_algorithm"] != WATER_DIGEST_ALGORITHM or _packed_digest(water, 8) != values["water_digest"]:
            return _fail(Failure.WATER_DIGEST_MISMATCH, "water payload digest mismatch", values)
        if values["region_key"] != REGION_KEY or values["region_id"] != region_id(values["seed"], ri, rj):
            return _fail(Failure.LINEAGE_MISMATCH, "region lineage mismatch", values)
        directions = {"n": (0,1), "s": (0,-1), "e": (1,0), "w": (-1,0),
                      "ne": (1,1), "nw": (-1,1), "se": (1,-1), "sw": (-1,-1)}
        if any(values[f"neighbor_{d}"] != region_id(values["seed"], ri+di, rj+dj)
               for d, (di, dj) in directions.items()):
            return _fail(Failure.LINEAGE_MISMATCH, "neighbor lineage mismatch", values)
        sti, stj = supertile_cell(ri, rj)
        if (values["parent_supertile_cell"] != f"{sti},{stj}" or
                values["parent_supertile_id"] != supertile_id(values["seed"], sti, stj)):
            return _fail(Failure.LINEAGE_MISMATCH, "parent supertile lineage mismatch", values)
        if values["page_digest_algorithm"] != PAGE_DIGEST_ALGORITHM or page_digest(magic, values) != values["page_digest"]:
            return _fail(Failure.PAGE_DIGEST_MISMATCH, "whole-page digest mismatch", values)
    except (KeyError, ValueError, InvalidOperation, OverflowError) as exc:
        return _fail(Failure.MALFORMED_HEADER, str(exc), values)
    if legacy_identity:
        return ValidationResult(
            Trust.DIAGNOSTIC_ONLY if diagnostic_legacy else Trust.QUARANTINED,
            Failure.WRONG_GENESIS, "superseded {} generator identity".format(superseded[2]),
            values, heights, surface, water)
    return ValidationResult(Trust.VALID_AUTHORITY, Failure.NONE, "", values, heights, surface, water)


def reissue_identity_text(text: str) -> str:
    """Rebind a superseded V2 page without touching semantic payloads."""
    magic, values = _records(text)
    if magic != MAGIC:
        raise ValueError("identity reissue requires a V2 page")
    old_identity = {name: values[name] for name in GENESIS_FIELDS}
    superseded = next((entry for entry in SUPERSEDED_GENESIS_IDENTITIES
                       if old_identity == entry[0]), None)
    if superseded is None:
        raise ValueError("identity reissue requires a recognized superseded identity")
    if genesis_digest(old_identity) != values["genesis_digest"]:
        raise ValueError("legacy page identity is internally inconsistent")
    values.update({name: str(value) for name, value in GENESIS_IDENTITY.items()})
    values["genesis_digest"] = GENESIS_DIGEST
    values["page_digest"] = page_digest(MAGIC, values)
    comments = [line for line in text.splitlines() if line.startswith("#")]
    output = [MAGIC, *comments]
    payload_keys = {"height_grid_row_major", "surface_grid_row_major", "water_grid_row_major"}
    output.extend(
        f"{key}={values[key]}" for key in sorted(values) if key not in payload_keys)
    output.extend(f"{key}={values[key]}" for key in (
        "height_grid_row_major", "surface_grid_row_major", "water_grid_row_major"))
    return "\n".join(output) + "\n"


def upgrade_legacy_page_text(text: str) -> str:
    magic, old = _records(text)
    if magic != LEGACY_MAGIC:
        raise ValueError("upgrade input is not a legacy V1 macro page")
    ri, rj = (int(part) for part in old["region_cell"].split(","))
    sti, stj = supertile_cell(ri, rj)
    values = dict(old)
    values.update(GENESIS_IDENTITY)
    values.update({
        "macro_page_schema_version": str(PAGE_SCHEMA_VERSION),
        "genesis_digest": GENESIS_DIGEST,
        "descriptor_schema_digest": descriptors.SCHEMA_DIGEST,
        "region_key": REGION_KEY,
        "height_count": str(len(old["height_grid_row_major"].split())),
        "surface_count": str(len(old["surface_grid_row_major"].split())),
        "water_count": str(len(old["water_grid_row_major"].split())),
        "source_digest_algorithm": SOURCE_DIGEST_ALGORITHM,
        "surface_digest_algorithm": SURFACE_DIGEST_ALGORITHM,
        "water_digest_algorithm": WATER_DIGEST_ALGORITHM,
        "payload_encoding": PAYLOAD_ENCODING,
        "parent_supertile_cell": f"{sti},{stj}",
        "parent_supertile_id": supertile_id(old["world_seed"], sti, stj),
        "page_digest_algorithm": PAGE_DIGEST_ALGORITHM,
    })
    # Canonical identity seed supersedes the legacy spelling but both must agree.
    if old["world_seed"] != values["seed"]:
        raise ValueError("legacy page seed differs from canonical GenesisIdentity")
    values["page_digest"] = page_digest(MAGIC, values)

    comments = [line for line in text.splitlines() if line.startswith("#")]
    authority = [MAGIC, *comments]
    payload_keys = {"height_grid_row_major", "surface_grid_row_major", "water_grid_row_major"}
    for key in sorted(values):
        if key not in payload_keys:
            authority.append(f"{key}={values[key]}")
    for key in ("height_grid_row_major", "surface_grid_row_major", "water_grid_row_major"):
        authority.append(f"{key}={values[key]}")
    return "\n".join(authority) + "\n"


def load_and_validate(path: Path, requested_coord: tuple[int, int], expected_genesis: str,
                      diagnostic_legacy: bool = False) -> ValidationResult:
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        return _fail(Failure.IO_ERROR, str(exc))
    return validate_text(text, requested_coord, expected_genesis, diagnostic_legacy)

"""Compile the FableScript-owned geology authority bridge and parity oracle.

This module is deliberately independent of the Esoterica C++ evaluator.  It
reads the authority products for Stages 5-10, emits one canonical history
envelope, and evaluates hostile 3-D queries into a stable oracle.  Esoterica is
allowed to consume and evaluate these products; it is not allowed to replace
them with client-authored geology.
"""

from __future__ import annotations

import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "Data" / "Worldgen"
BRIDGE = DATA / "fablescript_geology_authority_bridge_v1.cgab"
ORACLE = DATA / "fablescript_geology_authority_parity_v1.tsv"
OFFSET = 14695981039346656037
PRIME = 1099511628211


def fnv1a64(data: str | bytes) -> int:
    if isinstance(data, str):
        data = data.encode("utf-8")
    value = OFFSET
    for byte in data:
        value ^= byte
        value = (value * PRIME) & 0xFFFFFFFFFFFFFFFF
    return value


def read_product(name: str) -> tuple[str, dict[str, str], list[str], list[str]]:
    text = (DATA / name).read_text(encoding="utf-8")
    scalar: dict[str, str] = {}
    events: list[str] = []
    formations: list[str] = []
    for line in text.splitlines()[1:]:
        if not line or line.startswith("#"):
            continue
        key, value = line.split("=", 1)
        if key == "event":
            events.append(value)
        elif key == "formation":
            formations.append(value)
        else:
            scalar[key] = value
    return text, scalar, events, formations


GEO_TEXT, GEO, EVENTS, FORMATIONS = read_product("causal_world_geology_kernel_floor.cwg")
EXP_TEXT, EXP, _, _ = read_product("causal_world_geologic_exposure_floor.cwe")
ERO_TEXT, ERO, _, _ = read_product("causal_world_differential_erosion_floor.cde")
INT_TEXT, INT, _, _ = read_product("causal_world_granite_intrusion_floor.cgi")
MIN_TEXT, MIN, _, _ = read_product("causal_world_contact_mineralization_floor.ccm")


def f(mapping: dict[str, str], key: str) -> float:
    return float(mapping[key])


def h(mapping: dict[str, str], key: str) -> int:
    return int(mapping[key], 16)


EVENT_BY_NAME = {row.split(",")[0]: row.split(",") for row in EVENTS}
FORMATION_ROWS = [row.split(",") for row in FORMATIONS]
SURFACES = {
    row.split(",")[0]: row.split(",")
    for row in (line.split("=", 1)[1] for line in EXP_TEXT.splitlines() if line.startswith("surface="))
}


def surface_z(x: float, y: float) -> float:
    row = SURFACES["present_erosion"]
    datum, slope_x, slope_y, amp, wavelength, azimuth = map(float, row[2:8])
    angle = math.radians(azimuth)
    along = x * math.cos(angle) + y * math.sin(angle)
    return datum + slope_x * x + slope_y * y + amp * math.sin(2 * math.pi * along / wavelength)


def host_query(x: float, y: float, z: float) -> dict[str, object]:
    axis = math.radians(f(GEO, "fold_axis_deg"))
    along = x * math.cos(axis) + y * math.sin(axis)
    across = -x * math.sin(axis) + y * math.cos(axis)
    wave = 2 * math.pi * along / f(GEO, "fold_wavelength_m")
    folded = f(GEO, "datum_m") + f(GEO, "fold_amplitude_m") * math.sin(wave)
    local_z = z - folded
    row = next((r for r in FORMATION_ROWS if float(r[3]) <= local_z < float(r[4])), None)
    if row is None:
        return {"found": False}
    slope = f(GEO, "fold_amplitude_m") * (2 * math.pi / f(GEO, "fold_wavelength_m")) * math.cos(wave)
    normal = [-slope * math.cos(axis), -slope * math.sin(axis), 1.0]
    length = math.sqrt(sum(v * v for v in normal))
    normal = [v / length for v in normal]
    event_names = row[5].split("|")
    return {
        "found": True,
        "feature": int(row[1], 16),
        "formation": row[0],
        "material": row[2],
        "normal": normal,
        "local": [along, across, local_z],
        "events": [int(EVENT_BY_NAME[name][1], 16) for name in event_names],
        "chronology": [int(EVENT_BY_NAME[name][2]) for name in event_names],
    }


def regional_work(x: float, y: float) -> float:
    angle = math.radians(f(ERO, "work_azimuth_deg"))
    along = x * math.cos(angle) + y * math.sin(angle)
    cross = -x * math.sin(angle) + y * math.cos(angle)
    amp = f(ERO, "work_amplitude_m")
    wavelength = f(ERO, "work_wavelength_m")
    return (f(ERO, "work_datum_m")
            + amp * math.sin(2 * math.pi * along / wavelength)
            + 0.35 * amp * math.sin(2 * math.pi * cross / (wavelength * 1.7)))


def reconstructed_z(x: float, y: float) -> float:
    z = surface_z(x, y)
    remaining = regional_work(x, y)
    for _ in range(1024):
        if remaining <= 1e-12:
            break
        body = host_query(x, y, z - 0.001)
        if not body["found"]:
            break
        resistance = f(ERO, "sandstone_resistance") if body["material"] == "sandstone" else f(ERO, "shale_resistance")
        dz = min(f(ERO, "integration_step_m"), remaining / resistance)
        z -= dz
        remaining -= dz * resistance
    return z


def body_field(x: float, y: float, z: float) -> float:
    dx = (x - f(INT, "center_x_m")) / f(INT, "radius_x_m")
    dy = (y - f(INT, "center_y_m")) / f(INT, "radius_y_m")
    dz = (z - f(INT, "center_z_m")) / f(INT, "radius_z_m")
    warp = f(INT, "irregularity") * math.sin(dy * 5.1 + dz * 2.7) * math.cos(dx * 4.3 - dz * 1.9)
    return dx * dx + dy * dy + dz * dz + warp


def intrusion_query(x: float, y: float, z: float, enabled: bool = True) -> dict[str, object]:
    host = host_query(x, y, z)
    if not enabled or body_field(x, y, z) > 1.0:
        return host
    normal = [f(INT, "joint_nx"), f(INT, "joint_ny"), f(INT, "joint_nz")]
    return {
        "found": True, "feature": h(INT, "feature_id"), "formation": "G01_PLUTON",
        "material": "granite", "normal": normal,
        "local": [x - f(INT, "center_x_m"), y - f(INT, "center_y_m"), z - f(INT, "center_z_m")],
        "events": [h(INT, "intrusion_event_id"), h(INT, "cooling_event_id")],
        "chronology": [int(INT["intrusion_chronology"]), int(INT["cooling_chronology"])],
    }


def contact_distance(x: float, y: float, z: float) -> float:
    scale = min(f(INT, "radius_x_m"), f(INT, "radius_y_m"), f(INT, "radius_z_m"))
    return abs(math.sqrt(max(0.0, body_field(x, y, z))) - 1.0) * scale


def permeability(host: dict[str, object], x: float, y: float, z: float) -> float:
    fluid = [f(MIN, "fluid_nx"), f(MIN, "fluid_ny"), f(MIN, "fluid_nz")]
    normal = host["normal"]
    structural = abs(sum(normal[i] * fluid[i] for i in range(3)))
    fracture = 0.5 + 0.5 * math.sin(x * 0.47 + y * 0.71 + z * 0.29 + math.sin(y * 0.19))
    return 0.38 * structural + 0.62 * fracture


def assembled_query(x: float, y: float, z: float) -> tuple[dict[str, object], float, float, bool]:
    assembled = intrusion_query(x, y, z, True)
    contact = contact_distance(x, y, z)
    host = intrusion_query(x, y, z, False)
    perm = permeability(host, x, y, z) if host.get("found") else 0.0
    admitted = bool(host.get("found")) and body_field(x, y, z) > 1.0
    admitted = admitted and host["material"] in ("sandstone", "shale")
    admitted = admitted and f(MIN, "contact_min_m") <= contact <= f(MIN, "contact_max_m")
    admitted = admitted and perm >= f(MIN, "permeability_threshold")
    if admitted:
        vein = abs(math.sin(x * 0.83 - y * 0.37 + z * 0.41 + int(host["feature"]) % 17))
        admitted = vein <= f(MIN, "vein_threshold")
    if not admitted:
        return assembled, contact, perm, False
    fluid = [f(MIN, "fluid_nx"), f(MIN, "fluid_ny"), f(MIN, "fluid_nz")]
    deposit = {
        "found": True, "feature": h(MIN, "feature_id"), "formation": "D01_CONTACT_QUARTZ",
        "material": "quartz", "normal": fluid,
        "local": [x - f(INT, "center_x_m"), y - f(INT, "center_y_m"), z - f(INT, "center_z_m")],
        "events": [h(MIN, "parent_intrusion_event_id"), h(MIN, "mineralizing_event_id")],
        "chronology": [int(INT["intrusion_chronology"]), int(MIN["mineralizing_chronology"])],
    }
    return deposit, contact, perm, True


def bridge_text() -> str:
    rows = [
        "PROVENANCE_GEOLOGY_AUTHORITY_BRIDGE_V1",
        "authority_owner=FableScript",
        "evaluator_role=Esoterica_reference_client",
        f"world_identity={GEO['world_identity']}",
        f"world_identity_hash={GEO['world_identity_hash']}",
        f"generator_id={GEO['worldgen_id']}",
        f"generator_version={GEO['worldgen_version']}",
        "schema_version=1",
        "authority_revision=1",
        f"geology_descriptor_digest={fnv1a64(GEO_TEXT):016x}",
        f"exposure_descriptor_digest={fnv1a64(EXP_TEXT):016x}",
        f"erosion_descriptor_digest={fnv1a64(ERO_TEXT):016x}",
        f"intrusion_descriptor_digest={fnv1a64(INT_TEXT):016x}",
        f"mineralization_descriptor_digest={fnv1a64(MIN_TEXT):016x}",
    ]
    for event in EVENTS:
        name, event_id, chronology, kind = event.split(",")
        rows.append(f"event={event_id},{name},{kind},{chronology},-")
    rows.extend([
        f"event={INT['intrusion_event_id']},intrude_G01,intrusion,{INT['intrusion_chronology']},fold_03",
        f"event={INT['cooling_event_id']},cool_G01,cooling,{INT['cooling_chronology']},intrude_G01",
        f"event={MIN['mineralizing_event_id']},mineralize_D01,contact_mineralization,{MIN['mineralizing_chronology']},intrude_G01",
    ])
    for formation in FORMATIONS:
        name, feature, material, lo, hi, ancestry = formation.split(",")
        rows.append(f"feature={feature},{name},formation,{material},-,folded_slab:{lo}:{hi},{ancestry}")
    rows.extend([
        f"feature={INT['feature_id']},G01_PLUTON,intrusion,granite,-,irregular_ellipsoid,intrude_G01|cool_G01",
        f"deposit_system={MIN['deposit_system_id']},DS01_CONTACT,{INT['feature_id']},{MIN['mineralizing_event_id']}",
        f"feature={MIN['feature_id']},D01_CONTACT_QUARTZ,deposit_body,quartz,{INT['feature_id']},contact_vein,mineralize_D01",
        f"deposit_body={MIN['feature_id']},{MIN['deposit_system_id']},sandstone|shale,{MIN['contact_min_m']}:{MIN['contact_max_m']}",
    ])
    payload = "\n".join(rows) + "\n"
    return payload + f"bridge_payload_digest={fnv1a64(payload):016x}\n"


def hostile_points() -> list[tuple[float, float, float]]:
    points: list[tuple[float, float, float]] = []
    # Surface, near-surface, deep, negative coordinates, tile seams, formation
    # contacts, intrusion boundary and mineralized contact candidates.
    for y in range(-32, 33, 4):
        for x in range(-32, 33, 4):
            sx, sy = x + 0.5, y + 0.5
            rz = reconstructed_z(sx, sy)
            points.extend((sx, sy, rz + dz) for dz in (-18.0, -6.001, -0.001, 0.001, 7.0))
    for i in range(257):
        x = ((i * 37) % 131) - 65 + 0.125 * (i % 7)
        y = ((i * 61) % 127) - 63 - 0.125 * (i % 5)
        z = reconstructed_z(x, y) - ((i * 17) % 360) / 10.0
        points.append((x, y, z))
    # Dense Stage-10 surface sweep guarantees admitted and rejected contact samples.
    for y in range(-24, 25):
        for x in range(-24, 25):
            points.append((x + 0.5, y + 0.5, reconstructed_z(x + 0.5, y + 0.5) - 0.001))
    return points


def oracle_text(points: list[tuple[float, float, float]]) -> str:
    header = ("x\ty\tz\tfound\tmaterial\tfeature_id\tbody\tdeposit_system_id\tdeposit_body_id"
              "\tnx\tny\tnz\tintrusion_inside\tdeposit_admitted\tcontact_distance_m"
              "\tpermeability\tchronology\tevent_ids")
    rows = [header]
    for x, y, z in points:
        sample, contact, perm, admitted = assembled_query(x, y, z)
        found = bool(sample.get("found"))
        normal = sample.get("normal", [0.0, 0.0, 0.0])
        chronology = "|".join(str(v) for v in sample.get("chronology", [])) or "-"
        event_ids = "|".join(f"{v:016x}" for v in sample.get("events", [])) or "-"
        feature = int(sample.get("feature", 0))
        rows.append("\t".join((
            f"{x:.17g}", f"{y:.17g}", f"{z:.17g}", "1" if found else "0",
            str(sample.get("material", "-")), f"{feature:016x}", str(sample.get("formation", "-")),
            MIN["deposit_system_id"] if admitted else "0000000000000000",
            MIN["feature_id"] if admitted else "0000000000000000",
            f"{normal[0]:.17g}", f"{normal[1]:.17g}", f"{normal[2]:.17g}",
            "1" if body_field(x, y, z) <= 1.0 else "0", "1" if admitted else "0",
            f"{contact:.17g}", f"{perm:.17g}", chronology, event_ids,
        )))
    payload = "\n".join(rows) + "\n"
    return f"# oracle_digest={fnv1a64(payload):016x}\n" + payload


def main() -> None:
    bridge = bridge_text()
    points = hostile_points()
    oracle = oracle_text(points)
    BRIDGE.write_text(bridge, encoding="utf-8", newline="\n")
    ORACLE.write_text(oracle, encoding="utf-8", newline="\n")
    print(BRIDGE)
    print(ORACLE)
    oracle_payload = oracle.split("\n", 1)[1]
    print(f"queries={len(points)} bridge_digest={fnv1a64(bridge):016x} "
          f"oracle_payload_digest={fnv1a64(oracle_payload):016x} "
          f"oracle_file_digest={fnv1a64(oracle):016x}")


if __name__ == "__main__":
    main()

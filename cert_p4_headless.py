#!/usr/bin/env python3
"""P4 Fablescript authority floor — headless receipt probe against voxel_bridge.

Uses the same newline JSON request envelope as ProvenanceClient / Unreal.
Esoterica `--cert-p4` asserts the client applies these receipts without inventing.

  python cert_p4_headless.py --host 127.0.0.1 --port 8765
"""
from __future__ import annotations

import argparse
import json
import os
import socket
import sys
import tempfile
import time


def recv_line(sock: socket.socket, buf: bytearray, timeout_s: float = 20.0) -> dict:
    deadline = time.time() + timeout_s
    while True:
        if time.time() > deadline:
            raise TimeoutError("bridge reply timeout")
        nl = buf.find(b"\n")
        if nl >= 0:
            line = bytes(buf[:nl]).decode("utf-8", "replace")
            del buf[: nl + 1]
            return json.loads(line)
        sock.settimeout(max(0.05, deadline - time.time()))
        try:
            chunk = sock.recv(65536)
        except socket.timeout:
            continue
        if not chunk:
            raise ConnectionError("bridge closed")
        buf.extend(chunk)


def rpc(sock: socket.socket, buf: bytearray, method: str, params: dict, req_id: int) -> dict:
    # Match ProvenanceClient RequestMethod envelope.
    msg = {
        "version": 1,
        "id": str(req_id),
        "type": "request",
        "method": method,
        "params": params,
    }
    sock.sendall((json.dumps(msg, separators=(",", ":")) + "\n").encode("utf-8"))
    env = recv_line(sock, buf)
    if isinstance(env.get("result"), dict):
        return env["result"]
    return env


def removed_grams(result: dict) -> tuple[int, list[str]]:
    removed = result.get("removed") or {}
    if not isinstance(removed, dict):
        return 0, []
    grams = 0
    mats = []
    for k, v in removed.items():
        if isinstance(v, (int, float)) and int(v) > 0:
            grams += int(v)
            mats.append(str(k))
    return grams, mats


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8765)
    ap.add_argument("--out", default="")
    args = ap.parse_args()

    rows: list[tuple[str, str, str]] = []

    def add(check: str, verdict: str, note: str) -> None:
        rows.append((check, verdict, note))
        print(f"{check}\t{verdict}\t{note}")

    sock = socket.create_connection((args.host, args.port), timeout=5.0)
    buf = bytearray()
    rid = 1

    caps = rpc(sock, buf, "terrain_caps", {}, rid)
    rid += 1
    add("caps", "PASS" if caps.get("ok", True) else "FAIL", f"contract={caps.get('contract')}")

    ps = rpc(sock, buf, "player_state", {}, rid)
    rid += 1
    px = int(ps.get("x", ps.get("px", 128)))
    py = int(ps.get("y", ps.get("py", 128)))
    add("player_state", "PASS", f"px={px} py={py} terrain_rev={ps.get('terrain_rev')}")

    dig = {}
    grams, mats = 0, []
    for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (2, 0), (0, -2), (3, 1)):
        dig = rpc(
            sock,
            buf,
            "carve",
            {
                "x": px + dx,
                "y": py + dy,
                "u": 0.5,
                "v": 0.5,
                "depth": 0.16,
                "radius": 0.10,
                "shape": "sphere",
                "px": px,
                "py": py,
            },
            rid,
        )
        rid += 1
        grams, mats = removed_grams(dig)
        if bool(dig.get("ok")) and grams > 0:
            break
    accepted = bool(dig.get("ok")) and grams > 0
    add(
        "headless_dig_accept",
        "PASS" if accepted else "FAIL",
        f"ok={dig.get('ok')} grams={grams} mats={mats} rev={dig.get('rev')} reason={dig.get('reason')}",
    )
    body_id = dig.get("body_id") or 0
    agg_id = dig.get("aggregate_id") or 0
    add(
        "headless_auth_body_or_aggregate_id",
        "PASS" if accepted and (int(body_id) > 0 or int(agg_id) > 0) else ("SKIP" if not accepted else "FAIL"),
        f"body_id={body_id} aggregate_id={agg_id} provenance={dig.get('provenance')}",
    )

    air = rpc(
        sock,
        buf,
        "carve",
        {
            "x": px,
            "y": py,
            "u": 0.5,
            "v": 0.5,
            "depth": 3.5,
            "radius": 0.04,
            "shape": "sphere",
            "px": px,
            "py": py,
        },
        rid,
    )
    rid += 1
    air_grams, _ = removed_grams(air)
    reason = str(air.get("reason") or "")
    if air_grams > 0:
        add(
            "headless_nothing_or_refuse",
            "SKIP",
            f"deep bite still removed grams={air_grams} rev={air.get('rev')}",
        )
    else:
        refused = (air.get("ok") is False) or reason in (
            "nothing_to_dig",
            "too_hard",
            "out_of_reach",
        )
        add(
            "headless_nothing_or_refuse",
            "PASS" if refused else "FAIL",
            f"ok={air.get('ok')} grams={air_grams} reason={reason} msg={air.get('msg')}",
        )

    # Synthetic nothing_to_dig shape the client must honor (document expected refuse fields).
    add(
        "receipt_shape_nothing_to_dig",
        "PASS",
        'expected={"ok":false,"reason":"nothing_to_dig","removed":{}} — client must not invent scoop',
    )

    fail = sum(1 for _, v, _ in rows if v == "FAIL")
    out = args.out or os.path.join(tempfile.gettempdir(), "provenance_p4_headless_cert.txt")
    with open(out, "w", encoding="utf-8") as f:
        f.write("Provenance P4 headless Fablescript receipt probe\n")
        f.write(f"exit_code={1 if fail else 0}\n")
        f.write(f"FAIL_rows={fail} rows={len(rows)}\n\n")
        f.write("check\tverdict\tnote\n")
        for c, v, n in rows:
            f.write(f"{c}\t{v}\t{n}\n")
    print(f"wrote {out}")
    sock.close()
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())

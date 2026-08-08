from PIL import Image
from pathlib import Path
from collections import deque

assets = Path(r"C:\Users\D-Day\.cursor\projects\c-Users-D-Day-OneDrive-Desktop-Mygame\assets")
TOOLS_SHEET = assets / "c__Users_D-Day_AppData_Roaming_Cursor_User_workspaceStorage_empty-window_images_image-2378ad73-00f0-4a9e-acb0-07b0c282ac5e.png"
MATS_SHEET = assets / "c__Users_D-Day_AppData_Roaming_Cursor_User_workspaceStorage_empty-window_images_image-2efe3462-b4d5-4fb7-a78a-7727364b2e39.png"
OUT_TOOLS = Path(r"C:\Users\D-Day\ProvenanceEsoterica\Assets\Icons\Tools")
OUT_MATS = Path(r"C:\Users\D-Day\ProvenanceEsoterica\Assets\Icons\Materials")
OUT_TOOLS.mkdir(parents=True, exist_ok=True)
OUT_MATS.mkdir(parents=True, exist_ok=True)

# Flat (row, col, name) — avoid any nested-list aliasing confusion
TOOLS = [
    (0, 0, "axe"), (0, 1, "pick"), (0, 2, "shovel"), (0, 3, "spade"),
    (1, 0, "building_hammer"), (1, 1, "knife"), (1, 2, "scythe"), (1, 3, "torch"),
    (2, 0, "bucket"), (2, 1, "rope"), (2, 2, "spear"), (2, 3, "sword"),
    (3, 0, "bow"), (3, 1, "campfire_kit"), (3, 2, "water_flask"), (3, 3, "kettle"),
]
MATS = [
    (0, 0, "dirt"), (0, 1, "clay"), (0, 2, "sand"), (0, 3, "gravel"),
    (1, 0, "stone"), (1, 1, "flint"), (1, 2, "iron"), (1, 3, "coal"),
    (2, 0, "wood_log"), (2, 1, "wood_planks"), (2, 2, "bark"), (2, 3, "sticks_tinder"),
    (3, 0, "grass_wheat"), (3, 1, "leather_hide"), (3, 2, "herbs"), (3, 3, "mushrooms"),
]


def color_dist(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])


def gather_sheet_bg(im, samples=80):
    """Checker colors from sheet margin (outside icon content)."""
    w, h = im.size
    px = im.load()
    found = []
    # Top/bottom strips and left/right gutters between cells are unreliable;
    # use the outer 6px frame of the whole sheet.
    coords = []
    for x in range(0, w, max(1, w // samples)):
        for y in range(0, 6):
            coords.append((x, y))
            coords.append((x, h - 1 - y))
    for y in range(0, h, max(1, h // samples)):
        for x in range(0, 6):
            coords.append((x, y))
            coords.append((w - 1 - x, y))
    for x, y in coords:
        r, g, b, a = px[x, y]
        if a < 8:
            continue
        if max(r, g, b) - min(r, g, b) > 12:
            continue
        found.append((r, g, b))
    # Cluster
    reps = []
    for s in found:
        hit = False
        for bucket in reps:
            if color_dist(s, bucket[0]) < 24:
                n = bucket[1]
                bucket[0] = tuple((bucket[0][j] * n + s[j]) // (n + 1) for j in range(3))
                bucket[1] = n + 1
                hit = True
                break
        if not hit:
            reps.append([list(s), 1])
    cols = [tuple(b[0]) for b in reps]
    # Force white family — previous cut left white checker tiles
    for wcol in [(255, 255, 255), (252, 252, 252), (248, 248, 248), (242, 242, 242),
                 (236, 236, 236), (230, 230, 230), (220, 220, 220), (210, 210, 210),
                 (200, 200, 200), (190, 190, 190)]:
        cols.append(wcol)
    print("bg reps", cols[:8], "count", len(cols))
    return cols


def is_bg_pixel(r, g, b, a, bg_cols, tol):
    if a < 8:
        return True
    # Only near-grey can be checker (keep rust/wood/flame)
    if max(r, g, b) - min(r, g, b) > 14:
        return False
    for bc in bg_cols:
        if color_dist((r, g, b), bc) <= tol:
            return True
    return False


def flood_remove_bg(cell, bg_cols, tol=32):
    cell = cell.convert("RGBA")
    w, h = cell.size
    px = cell.load()
    seen = [[False] * w for _ in range(h)]
    q = deque()

    def push(x, y):
        if x < 0 or y < 0 or x >= w or y >= h or seen[y][x]:
            return
        r, g, b, a = px[x, y]
        if not is_bg_pixel(r, g, b, a, bg_cols, tol):
            return
        seen[y][x] = True
        q.append((x, y))

    for x in range(w):
        push(x, 0)
        push(x, h - 1)
    for y in range(h):
        push(0, y)
        push(w - 1, y)

    while q:
        x, y = q.popleft()
        px[x, y] = (0, 0, 0, 0)
        push(x + 1, y)
        push(x - 1, y)
        push(x, y + 1)
        push(x, y - 1)

    # Peel light grey/white fringe next to transparency (do NOT peel mid metal greys)
    for _ in range(3):
        doomed = []
        for y in range(h):
            for x in range(w):
                r, g, b, a = px[x, y]
                if a < 8:
                    continue
                if max(r, g, b) - min(r, g, b) > 14:
                    continue
                lum = (r + g + b) / 3.0
                if lum < 195:  # keep darker iron/wood greys inside the silhouette
                    continue
                if not is_bg_pixel(r, g, b, a, bg_cols, tol + 10) and lum < 245:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h and px[nx, ny][3] < 8:
                        doomed.append((x, y))
                        break
        for x, y in doomed:
            px[x, y] = (0, 0, 0, 0)
    return cell


def fit_256(cell):
    bbox = cell.split()[-1].getbbox()
    if not bbox:
        return Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    pad = 2
    l, t, r, b = bbox
    cell = cell.crop((max(0, l - pad), max(0, t - pad), min(cell.width, r + pad), min(cell.height, b + pad)))
    canvas = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    scale = min(240 / cell.width, 240 / cell.height)
    nw = max(1, int(cell.width * scale))
    nh = max(1, int(cell.height * scale))
    resized = cell.resize((nw, nh), Image.Resampling.LANCZOS)
    canvas.paste(resized, ((256 - nw) // 2, (256 - nh) // 2), resized)
    # final white halo peel on canvas
    px = canvas.load()
    for _ in range(2):
        doomed = []
        for y in range(256):
            for x in range(256):
                r, g, b, a = px[x, y]
                if a < 8:
                    continue
                if max(r, g, b) - min(r, g, b) > 14:
                    continue
                if (r + g + b) / 3.0 < 210:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < 256 and 0 <= ny < 256 and px[nx, ny][3] < 8:
                        doomed.append((x, y))
                        break
        for x, y in doomed:
            px[x, y] = (0, 0, 0, 0)
    return canvas


def export(sheet_path, out_dir, entries):
    im = Image.open(sheet_path).convert("RGBA")
    bg = gather_sheet_bg(im)
    w, h = im.size
    cw, ch = w // 4, h // 4
    for row, col, name in entries:
        raw = im.crop((col * cw, row * ch, (col + 1) * cw, (row + 1) * ch))
        cut = flood_remove_bg(raw, bg, tol=34)
        out = fit_256(cut)
        dest = out_dir / f"{name}.png"
        out.save(dest)
        # stats
        px = out.load()
        opaque = white_edge = 0
        for y in range(256):
            for x in range(256):
                r, g, b, a = px[x, y]
                if a <= 10:
                    continue
                opaque += 1
                if max(r, g, b) - min(r, g, b) <= 12 and (r + g + b) / 3.0 >= 245:
                    # only count if near transparent neighbor
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < 256 and 0 <= ny < 256 and px[nx, ny][3] < 8:
                            white_edge += 1
                            break
        bbox = out.split()[-1].getbbox()
        print(f"{out_dir.name}/{name}: opaque={opaque} white_edge={white_edge} bbox={bbox}")


export(TOOLS_SHEET, OUT_TOOLS, TOOLS)
export(MATS_SHEET, OUT_MATS, MATS)

# sanity: knife must not be mostly checker
knife = Image.open(OUT_TOOLS / "knife.png")
kb = knife.split()[-1].getbbox()
print("knife bbox", kb)
axe = Image.open(OUT_TOOLS / "axe.png")
print("axe sample", axe.getpixel((128, 80)), axe.getpixel((100, 160)))
print("done")

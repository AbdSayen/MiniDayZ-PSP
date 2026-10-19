#!/usr/bin/env python3
# Extract the C2 "Tutorial" layout (Mini DayZ 1.4.1 data.js, decoded proj.pkl) into
#   data/tutorial_tilemaps.txt   (WorldTilemaps format, one map per Tilemap instance)
#   src/world/tutorial_layout.inc (placed objects of the layout, in-bounds only)
# Usage: python3 extract_tutorial.py proj.pkl map_props.inc items.json out_dir
import pickle, sys, re, json, math
pk, props_inc, items_json, out = sys.argv[1:5]
p = pickle.load(open(pk, 'rb')); T = p[3]
L = [l for l in p[5] if l[0] == 'Tutorial'][0]
LW, LH = L[1], L[2]
def inb(x, y): return -50 < x < LW + 50 and -50 < y < LH + 50
def tex(t):
    try: return T[t][6][0].split('/')[-1].replace('.png', '')
    except Exception: return None
def frame(t, anim_name=None, fi=0):
    an = T[t][7]
    a = an[0] if anim_name is None else [x for x in an if x[0] == anim_name][0]
    fr = a[7]
    if fi < 0 or fi >= len(fr): fi = 0   # C2 clamps an out-of-range initial frame to 0
    return fr[fi]
# --- tilemaps --------------------------------------------------------------------------
TILE = {475: (60, 60, 5), 411: (60, 60, 3), 221: (30, 30, 5), 202: (150, 150, 3), 199: (60, 60, 3)}
def rle(s, n):
    v = []
    for tok in s.split(','):
        if 'x' in tok:
            c, x = tok.split('x'); v += [int(x)] * int(c)
        else: v.append(int(tok))
    assert len(v) == n, (len(v), n)
    return v
maps = []
for lay in L[6]:
    for ins in lay[14]:
        t = ins[1]
        if T[t][1] != 14: continue
        g = ins[0]
        x, y, w, h = g[0], g[1], g[3], g[4]
        if x + w < 0 or y + h < 0 or x > LW or y > LH: continue
        cols, rows, data = g[13][0], g[13][1], g[13][2]
        tw, th, ac = TILE[t]
        v = rle(data, cols * rows)
        # Instance size crops the grid (C2 draws only inside the instance box).
        vc = min(cols, int(math.ceil(w / tw))); vr = min(rows, int(math.ceil(h / th)))
        cells = [[v[r * cols + c] for c in range(vc)] for r in range(vr)]
        nz = [(r, c) for r in range(vr) for c in range(vc) if cells[r][c] >= 0]
        if not nz: continue
        r0 = min(a for a, _ in nz); r1 = max(a for a, _ in nz)
        c0 = min(b for _, b in nz); c1 = max(b for _, b in nz)
        name = tex(t)
        if t == 202 and lay[0] == 'Forests': name += '|top'   # Forests layer: above objects
        maps.append((name, int(round(x + c0 * tw)), int(round(y + r0 * th)), c1 - c0 + 1, r1 - r0 + 1,
                     tw, th, ac, [cells[r][c0:c1 + 1] for r in range(r0, r1 + 1)], lay[0]))
with open(out + '/data/tutorial_tilemaps.txt', 'w') as f:
    f.write('%d\n' % len(maps))
    for m in maps:
        f.write('%s %d %d %d %d %d %d %d\n' % m[:8])
        f.write(' '.join(str(v) for row in m[8] for v in row) + '\n')
# --- objects ---------------------------------------------------------------------------
defs = []
for line in open(props_inc):
    mm = re.match(r'\s*\{(-?\d+), (-?\d+), (-?\d+), .*\}, // (\S+)', line)
    if mm and 'kPropDefs' not in line: defs.append(mm.group(4))
# kPropDefs comes after PropFrame/PropSheet/PropBase tables: keep the lines of that array only
src = open(props_inc).read()
blk = src[src.index('kPropDefs[]'):]
blk = blk[:blk.index('};')]
defs = [re.search(r'// (\S+)', l).group(1) for l in blk.split('\n') if '//' in l]
def def_idx(name): return defs.index(name)
items = {it['name'].lower(): it['id'] for it in json.load(open(items_json))['items']}
ITEM_BY_IV10 = {'m4': 'm4 carbine', 'gorka helmet': 'gorka helmet', 'fireaxe': None}
def nm(t):
    try: return frame(t)[0].split('/')[-1].replace('-sheet0.png', '').replace('.png', '')
    except Exception: return 't%d' % t
TREES = {175: 1, 198: 2, 200: 3, 569: 4, 570: 5, 574: 6, 201: 7}
PROPS = {310: 'obst_fence', 309: 'obst_block', 305: 'obst_trash_container', 306: 'obst_lavka',
         248: 'fence_horizontal', 249: 'fence_vertical', 432: 'pillar', 287: 'berry_bush-sheet1.png',
         435: 'waterpump', 204: 'car_regular_blue', 480: 'car_regular_blue_vert',
         479: 'car_regular_green_vert', 181: 'car_police'}
CARS = (204, 480, 479, 181)
BUILD = {319: 'b_hostel', 321: 'b_village_green', 322: 'b_village_brown', 508: 'b_garage_green',
         639: 'b_village_red2'}
out_b, out_p, out_t, out_i, out_z, out_c, out_misc = [], [], [], [], [], [], []
fireaxe = [k for k in items if k.replace(' ', '') == 'fireaxe']
for lay in L[6]:
    for ins in lay[14]:
        t = ins[1]; g = ins[0]; x, y = g[0], g[1]
        if T[t][1] == 14 or not inb(x, y): continue
        fr = ins[5][2] if len(ins) > 5 and isinstance(ins[5], list) and len(ins[5]) > 2 else 0
        iv = ins[3] if len(ins) > 3 else []
        if t in BUILD: out_b.append((BUILD[t] + '-sheet0', x, y))
        elif t in TREES:
            sub = TREES[t]
            f = frame(t, None, fr if t == 201 else 0)
            fi = fr if t == 201 and 0 <= fr < len(T[t][7][0][7]) else 0
            ip = f[9][0]  # Imagepoint 1
            w, h, hx, hy = f[4], f[5], f[7], f[8]
            out_t.append((sub, fi, x + (ip[1] - hx) * w, y + (ip[2] - hy) * h, x, y))
        elif t in PROPS:
            out_p.append((def_idx(PROPS[t]), x, y, fr, nm(t)))
            if t in CARS:  # car_loot_point spawned at the car's Imagepoint 1 (Tutorial start)
                f = frame(t, None, 0); ip = f[9][0]
                out_c.append((x + (ip[1] - f[7]) * f[4], y + (ip[2] - f[8]) * f[5], nm(t)))
        elif lay[0] == 'items_on_ground' and t not in (614, 434, 687):
            ivs = [v[0] if v else None for v in iv]
            if isinstance(ivs[0], str):           # consumable / ammo: [name, amount, id]
                iid = ivs[2]; cnt = ivs[1] if t == 30 else 1
            else:                                  # gear: IV10 = name
                n10 = str(ivs[10]).lower()
                iid = items[ITEM_BY_IV10.get(n10) or (fireaxe[0] if 'fireaxe' in n10 else n10)]
                cnt = 1
            out_i.append((iid, cnt, x, y, nm(t)))
        elif t == 207: out_z.append((x, y, g[6]))
        elif t in (614, 434, 687, 667, 668, 666, 172):
            out_misc.append((t, x, y, fr, g[3], g[4], iv[0][0] if iv and iv[0] else 0))
# off-layout pl12 zone IV0=2 (moved to the player when the boat is repaired)
for lay in L[6]:
    for ins in lay[14]:
        if ins[1] == 666 and not inb(ins[0][0], ins[0][1]) and ins[3][0][0] == 2:
            g = ins[0]; out_misc.append((666, g[0], g[1], 0, g[3], g[4], 2))
def F(v): return '%.2ff' % v
with open(out + '/src/world/tutorial_layout.inc', 'w') as f:
    f.write('// Generated by tools/extract_tutorial.py from data.js layout "Tutorial" (%dx%d).\n' % (LW, LH))
    f.write('// Do not edit by hand.\n')
    f.write('static const TutBuilding kTutBuildings[] = {\n')
    for b in out_b: f.write('  {"%s", %s, %s},\n' % (b[0], F(b[1]), F(b[2])))
    f.write('};\nstatic const TutProp kTutProps[] = {\n')
    for q in out_p: f.write('  {%d, %s, %s, %d}, // %s\n' % (q[0], F(q[1]), F(q[2]), q[3], q[4]))
    f.write('};\nstatic const TutTree kTutTrees[] = {\n')
    for q in out_t: f.write('  {%d, %d, %s, %s}, // origin %s,%s\n' % (q[0], q[1], F(q[2]), F(q[3]), F(q[4]), F(q[5])))
    f.write('};\nstatic const TutItem kTutItems[] = {\n')
    for q in out_i: f.write('  {%d, %d, %s, %s}, // %s\n' % (q[0], q[1], F(q[2]), F(q[3]), q[4]))
    f.write('};\nstatic const TutZedPoint kTutZedPoints[] = {\n')
    for q in out_z: f.write('  {%s, %s, %s},\n' % (F(q[0]), F(q[1]), F(q[2] * 57.29578)))
    f.write('};\nstatic const TutCarLoot kTutCarLoot[] = {\n')
    for q in out_c: f.write('  {%s, %s}, // %s Imagepoint 1\n' % (F(q[0]), F(q[1]), q[2]))
    f.write('};\nstatic const TutMisc kTutMisc[] = {\n')
    for q in out_misc: f.write('  {%d, %s, %s, %d, %s, %s, %d},\n' % (q[0], F(q[1]), F(q[2]), q[3], F(q[4]), F(q[5]), q[6]))
    f.write('};\n')
print(len(maps), 'tilemaps', [(m[0], m[3], m[4], m[9]) for m in maps])
print(len(out_b), 'buildings', len(out_p), 'props', len(out_t), 'trees', len(out_i), 'items', len(out_z), 'zr', len(out_c), 'carloot', len(out_misc), 'misc')

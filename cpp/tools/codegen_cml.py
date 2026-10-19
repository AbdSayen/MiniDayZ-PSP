# Codegen: data.js function "generate_location" (all.txt 11255..15117) -> C++ (generate_location.inc).
import sys, re, json
sys.path.insert(0, '/workspace/mdz/item5')
from parse import split_call, L as PL
def node_tree(a,b):
    root={'ind':-1,'conds':[],'acts':[],'kids':[],'hdr':'ROOT','vars':[]}
    stack=[root]
    for ln in PL[a-1:b]:
        if not ln.strip(): continue
        ind=len(ln)-len(ln.lstrip()); st=ln.strip()
        if st.startswith('EV '):
            n={'ind':ind,'conds':[],'acts':[],'kids':[],'hdr':st,'vars':[]}
            while stack[-1]['ind']>=ind: stack.pop()
            stack[-1]['kids'].append(n); stack.append(n)
        elif st[0] in '?!':
            while stack[-1]['ind']>=ind: stack.pop()
            (stack[-1]['conds'] if st[0]=='?' else stack[-1]['acts']).append(st[2:])
        elif st.startswith('VAR'):
            while stack[-1]['ind']>=ind: stack.pop()
            import ast; v=ast.literal_eval(st[4:]); stack[-1]['vars'].append(v)
    return root
A, B = int(sys.argv[1]), int(sys.argv[2])
root = node_tree(A, B)
VARS = []
def vid(n):
    if n not in VARS: VARS.append(n)
    return 'V[%d]' % VARS.index(n)
unk = {}
def note(k): unk[k] = unk.get(k, 0) + 1
LOOPS = []
TMP=[0]
PRE=[]
def hoist(code):
    TMP[0]+=1; n='t%d'%TMP[0]; PRE.append('const double %s = %s;'%(n,code)); return n
def E(e):
    if isinstance(e, bool): return '1' if e else '0'
    if isinstance(e, (int, float)): return repr(float(e)) if isinstance(e, float) else '%d.0' % e
    if isinstance(e, str): return '0.0 /*str %s*/' % e
    t = e[0]
    if t == 'var': return vid(e[1])
    if t in (0, 1): return E(e[1])
    if t == 3: return '(-(%s))' % E(e[1])
    ops = {4: '+', 5: '-', 6: '*', 7: '/', 12: '==', 13: '!=', 14: '<', 15: '<=', 16: '>', 17: '>='}
    if t in ops and len(e) == 3:
        return '(%s %s %s)' % (E(e[1]), ops[t], E(e[2]))
    if t == 8: return 'c2mod(%s, %s)' % (E(e[1]), E(e[2]))
    if t == 18: return '((%s) != 0 ? (%s) : (%s))' % (E(e[1]), E(e[2]), E(e[3]))
    if t == 23: return vid(e[1])
    if t == 19:
        fid = e[1]; args = e[2] if len(e) > 2 else []
        if fid == 80: return 'std::floor(%s)' % E(args[0])
        if fid == 187: return 'c2int(%s)' % E(args[0])
        if fid == 95:
            if len(args) == 1: a0=E(args[0]); return hoist('G.rnd() * (%s)' % a0)
            a0=E(args[0]); a1=E(args[1]); return hoist('G.rnd_range(%s, %s)' % (a0, a1))
        if fid == 82:
            vals = [E(a) for a in args]
            idx = hoist('(double)G.pick(%d)' % len(vals))
            expr = vals[-1]
            for i in range(len(vals) - 2, -1, -1): expr = '(%s == %d ? %s : %s)' % (idx, i, vals[i], expr)
            return expr
        if fid == 150: return '(double)%s' % (LOOPS[-1] if LOOPS else '0')
        note('sys%d' % fid); return '0.0 /*sys%d*/' % fid
    if t == 20:
        obj, xid = e[1], e[2]
        if obj == 518 and xid == 210: return 'G.index_of0()'
        if obj == 507 and xid == 65: return 'G.cell_x'
        if obj == 507 and xid == 66: return 'G.cell_y'
        if obj == 507 and xid == 151: return 'G.uid'
        if obj == 180 and xid == 93: return 'G.uid'
        if obj == 945 and xid in (208, 209): return 'std::floor((%s) / kTileSize)' % E(e[5][0])
        note('obj%d.%d' % (obj, xid)); return '0.0 /*obj%d.%d*/' % (obj, xid)
    if t == 21:
        obj, k = e[1], e[4]
        if obj == 507: return 'G.iv(%d)' % k
        note('iv%d.%d' % (obj, k)); return '0.0'
    note('exp%r' % (t,)); return '0.0 /*exp*/'
TMAP={475:0,681:1,682:2,771:3,945:9,221:4,199:5}
def tmid(obj):
    import re as _r
    m=_r.match(r't(\d+)',obj); return TMAP.get(int(m.group(1))) if m else None
CMP = ['==', '!=', '<', '<=', '>', '>=']
def P(a):  # param value
    if isinstance(a, list) and a and a[0] == 7 and len(a) == 2: return E(a[1])
    return E(a)
def cond(c):
    s = split_call(c)
    if not s: note('badcond ' + c[:40]); return '0'
    neg, obj, op, a = s
    r = None
    if op == 'c71' and 'mapgen' in obj: r = '(G.iv(%d) %s %s)' % (a[0][1], CMP[a[1][1]], P(a[2]))
    elif op == 'c51' and obj == 'System': r = '((%s) %s (%s))' % (P(a[0]), CMP[a[1][1]], P(a[2]))
    elif op == 'c46' and obj == 'System': r = '(%s %s (%s))' % (vid(a[0][1]), CMP[a[1][1]], P(a[2]))
    elif op == 'c186' and 't511' in obj: r = '(G.grid(%s, %s) %s (%s))' % (P(a[0]), P(a[1]), CMP[a[2][1]], P(a[3]))
    elif op == 'c188' and tmid(obj) is not None: r = '(G.tile(%d, %s, %s) %s (%s))' % (tmid(obj), P(a[0]), P(a[1]), CMP[a[2][1]], P(a[3]))
    elif op == 'c184' and tmid(obj) is not None: r = '1'
    elif op in ('c92', 'c40'): r = '1'
    else: note('cond %s.%s' % (obj, op)); r = '0'
    return '(!%s)' % r if neg else r
def act(x, out, ind):
    PRE.clear()
    s = split_call(x)
    if not s: note('badact ' + x[:40]); return
    neg, obj, op, a = s
    p = ' ' * ind
    if op == 'a41' and obj == 'System': line = p + '%s = %s;' % (vid(a[0][1]), P(a[1])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a122' and obj == 'System': line = p + '%s += %s;' % (vid(a[0][1]), P(a[1])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a203' and obj == 'System': line = p + '%s -= %s;' % (vid(a[0][1]), P(a[1])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a94' and obj == 'System' and a[0][1] in TMAP: line = p + 'G.create(%d, %s, %s);' % (TMAP[a[0][1]], P(a[2]), P(a[3])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a185' and tmid(obj) is not None: line = p + 'G.set_range(%d, %s, %s, %s, %s, %s);' % (tmid(obj), P(a[0]), P(a[1]), P(a[2]), P(a[3]), P(a[4])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a189' and tmid(obj) is not None: line = p + 'G.set_tile(%d, %s, %s, %s);' % (tmid(obj), P(a[0]), P(a[1]), P(a[2])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a191' and tmid(obj) is not None: line = p + 'G.erase_range(%d, %s, %s, %s, %s);' % (tmid(obj), P(a[0]), P(a[1]), P(a[2]), P(a[3])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a190' and tmid(obj) is not None: line = p + 'G.set_size(%d, %s, %s);' % (tmid(obj), P(a[0]), P(a[1])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a182' and 't518' in obj: line = p + 'G.set(%s, %s, %s);' % (P(a[0]), P(a[1]), P(a[2])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a198' and 't518' in obj: line = p + 'G.resize(%s);' % P(a[0]); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a94' and obj == 'System' and a[0] == [4, 207]: line = p + 'G.spawn_zed_resp(%s, %s);' % (P(a[2]), P(a[3])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a72' and 't207' in obj: line = p + 'G.zed_resp_frame(%s);' % P(a[0]); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    elif op == 'a88' and 't207' in obj: line = p + 'G.zed_resp_iv(%d, %s);' % (a[0][1], P(a[1])); out.append(p + '{ ' + ' '.join(PRE) + ' ' + line[len(p):] + ' }' if PRE else line)
    else: note('act %s.%s' % (obj, op))
CNT = [0]
def ev(n, out, ind, prev_flag):
    p = ' ' * ind
    CNT[0] += 1; me = 'r%d' % CNT[0]
    out.append(p + 'bool %s = false;' % me)
    is_or = n['hdr'].rstrip().endswith('else')
    conds = n['conds']; closers = 0; pushed = 0
    out.append(p + '{'); ind2 = ind + 2
    for v in n.get('vars', []):
        out.append(' ' * ind2 + '%s = %s;' % (vid(v[0]), E(v[2])))
    elsecond = any(split_call(c) and split_call(c)[2] == 'c70' for c in conds)
    conds = [c for c in conds if not (split_call(c) and split_call(c)[2] == 'c70')]
    if elsecond:
        out.append(' ' * ind2 + 'if (!%s) {' % (prev_flag or '0')); closers += 1; ind2 += 2
    if is_or and conds:
        PRE.clear(); cs = [cond(c) for c in conds]
        for q in PRE: out.append(' ' * ind2 + q)
        out.append(' ' * ind2 + 'if (%s) {' % ' || '.join(cs)); closers += 1; ind2 += 2
    else:
        for c in conds:
            s = split_call(c)
            if s and s[2] == 'c123':
                PRE.clear(); nexpr = P(s[3][0]); assert not PRE
                lv = 'li%d' % len(LOOPS); LOOPS.append(lv); pushed += 1
                out.append(' ' * ind2 + 'for (int %s = 0, %s_n = (int)(%s); %s < %s_n; ++%s) {' % (lv, lv, nexpr, lv, lv, lv))
            else:
                PRE.clear(); cc = cond(c)
                for q in PRE: out.append(' ' * ind2 + q)
                out.append(' ' * ind2 + 'if (%s) {' % cc)
            closers += 1; ind2 += 2
    out.append(' ' * ind2 + '%s = true;' % me)
    for x in n['acts']: act(x, out, ind2)
    prev = None
    for k in n['kids']: prev = ev(k, out, ind2, prev)
    for _ in range(closers): ind2 -= 2; out.append(' ' * ind2 + '}')
    for _ in range(pushed): LOOPS.pop()
    out.append(p + '}')
    return me
out = []
prev = None
for k in root['kids']: prev = ev(k, out, 2, prev)
hdr = ['// GENERATED by gencml/codegen_cml.py from data.js "create_map_location" (tilemap actions only) (all.txt %d-%d). Do not edit.' % (A, B),
       'static const int kCmlVars = %d;' % max(1, len(VARS)),
       '// vars: ' + ', '.join('%d=%s' % (i, v) for i, v in enumerate(VARS)),
       'static void create_map_location_tiles(CmlCtx& G, double* V) {']
open(sys.argv[3], 'w').write('\n'.join(hdr + out + ['}', '']))
json.dump({'vars': VARS}, open(sys.argv[3] + '.vars.json', 'w'))
print('vars', VARS); print('unknown', unk); print('lines', len(out))

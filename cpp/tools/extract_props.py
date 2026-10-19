import pickle,json,sys
sys.path.insert(0,'/workspace/mdz')
import dec
p=pickle.load(open('/workspace/mdz/proj.pkl','rb')); T=p[3]
root=p[6]
for k in [0,1,259,7,0,7,6]: root=root[k]
def nm(i): return dec.oname(i).split('<')[1][:-1] if '<' in dec.oname(i) else str(i)
def pv(x):
    # param value -> python
    if isinstance(x,list) and len(x)==2 and x[0] in (0,1): return x[1]
    return x
out={}
def ev(e,ctx):
    if not isinstance(e,list) or e[0]!=0: return
    ctx=dict(ctx)
    for c in e[5] or []:
        obj,cid=c[0],c[1]; prm=c[9] if len(c)>9 and c[9] else []
        if obj==501 and cid==71:
            iv=prm[0][1]; op=prm[1][1]; v=pv(prm[2][1]) if isinstance(prm[2],list) else None
            if iv==1: ctx['cat']=v
            elif iv==2: ctx['sub']=(op,v)
            elif iv==3: ctx['iv3']=v
            elif iv==6: ctx['iv6']=v
        elif cid==64: ctx['mframe']=pv(prm[1])
        elif cid==108: ctx.setdefault('ovl',[]).append(nm(prm[0][1]))
    key=None
    if 'cat' in ctx and 'sub' in ctx: key='%s:%s'%(ctx['cat'],ctx['sub'][1] if ctx['sub'][0]==0 else 'op%d_%s'%ctx['sub'])
    for a in e[6] or []:
        obj,aid=a[0],a[1]; prm=a[5] if len(a)>5 and a[5] else []
        if key is None: continue
        rec=out.setdefault(key,[])
        c2={k:v for k,v in ctx.items() if k in ('iv3','iv6','mframe','ovl')}
        if aid==73: rec.append(dict(c2,op='spawn',by=nm(obj),obj=nm(prm[0][1]),objid=prm[0][1],layer=prm[1][1],ip=prm[2][1]))
        elif aid==72: rec.append(dict(c2,op='frame',on=nm(obj),v=prm[0]))
        elif aid==53: rec.append(dict(c2,op='destroy',on=nm(obj)))
        elif aid==158: rec.append(dict(c2,op='anim',on=nm(obj),v=prm[0][1]))
        elif aid==88 and obj!=501: rec.append(dict(c2,op='setiv',on=nm(obj),iv=prm[0][1],v=prm[1][1] if not isinstance(prm[1][1],list) or prm[1][1][0]!=20 else 'uid'))
    for se in (e[7] if len(e)>7 and e[7] else []): ev(se,ctx)
ev(root,{})
json.dump(out,open('/tmp/props.json','w'),indent=0)
for k in out:
    if k.startswith(('3:','4:','5:','6:','7:1')): 
        print(k, [ (r['op'],r.get('obj',r.get('on')),r.get('v',r.get('ip')),r.get('mframe'),r.get('iv3')) for r in out[k] if r['op']!='setiv'])

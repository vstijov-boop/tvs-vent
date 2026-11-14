"""Golden cases for geometry/AshraeEngine.h from the user's DuctPressDrop ASHRAE engine
(services/ashrae/engine.py). The ASHRAE data stays outside this repository:
   python3 tools/ashrae_golden.py <hvac_app folder> <ashrae_data folder> <out.txt> [variants]
One line per case: mode, key, role, inputs, C, dP, every result variable (%.17g)."""
import csv,math,os,random,sys
app,data,out=sys.argv[1],sys.argv[2],sys.argv[3]
variants=int(sys.argv[4]) if len(sys.argv)>4 else 6
sys.path.insert(0,app)
from services.ashrae.engine import AshraeEngine
inputs={}
for r in csv.DictReader(open(os.path.join(data,'Input.csv'),encoding='utf-8-sig'),delimiter=';'):
    k=(r.get('Key') or '').strip();v=(r.get('Variable') or '').strip()
    if not k or not v:continue
    def f(x,d):
        try:return float(x)
        except (TypeError,ValueError):return d
    inputs.setdefault(k,[]).append((v,f(r.get('SIDefault'),0.0),f(r.get('SIMinimum'),0.0),f(r.get('SIMaximum'),0.0)))
rnd=random.Random(1234)
lines=[]
for mode in ('clamp','linear'):
    eng=AshraeEngine(data,os.path.join(data,'tables'))
    eng.ekstrapolacija=mode;eng.interpolacija='pv' if mode=='clamp' else 'linear'
    for key in sorted(eng.equations):
        par=inputs.get(key,[])
        cases=[{v:d for v,d,lo,hi in par}]
        for _ in range(variants):
            c={}
            for v,d,lo,hi in par:
                if hi>lo:
                    x=rnd.uniform(lo,hi)
                    if rnd.random()<0.15:x=rnd.uniform(lo-0.3*(hi-lo),hi+0.3*(hi-lo))   # outside: clamp / extrapolation
                    c[v]=round(x,3)
                else:c[v]=d
            cases.append(c)
        for c in cases:
            for role in ('branch','main'):
                eng._loss_memo.clear()
                C,dP=eng.loss(key,c,role)
                cv=eng.calculate(key,c)
                res=[]
                for n in eng.result_vars(key):
                    v=cv.get(n)
                    if isinstance(v,(int,float)) and not isinstance(v,bool):res.append('%s=%.17g'%(n,v))
                lines.append('\t'.join([mode,key,role,','.join('%s=%.17g'%(k,v) for k,v in sorted(c.items())),'%.17g'%C,'%.17g'%dP,','.join(res)]))
open(out,'w').write('\n'.join(lines)+'\n')
print(len(lines),'cases')

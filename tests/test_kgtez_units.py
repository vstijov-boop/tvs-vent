"""Evaluate the actual numeric KGTEZ functions with a small AutoLISP test harness.
CAD/COM I/O is stubbed; this does not simulate AutoCAD geometry or its UI.
"""
import math
import re
import unittest
from collections import ChainMap
from pathlib import Path

class Symbol(str):
    pass

def parse(source):
    tokens = re.findall(r';[^\n]*|"(?:\\.|[^"\\])*"|[()]|\'|[^\s()\';]+', source)
    tokens = iter(t for t in tokens if not t.startswith(';'))
    def atom(t):
        if t.startswith('"'):
            return t[1:-1].replace('\\"','"').replace('\\n','\n')
        try:
            return float(t) if '.' in t or 'e' in t.lower() else int(t)
        except ValueError:
            return Symbol(t.lower())
    def form(t):
        if t == '(':
            result=[]
            for t in tokens:
                if t == ')': return result
                result.append(form(t))
            raise ValueError('unbalanced parentheses')
        if t == "'": return [Symbol('quote'),form(next(tokens))]
        if t == ')': raise ValueError('unexpected closing parenthesis')
        return atom(t)
    return [form(t) for t in tokens]

class Harness:
    def __init__(self):
        self.env={'nil':None,'t':True,'pi':math.pi}
        self.functions={}
        source=Path(__file__).parents[1]/'Installer/TVS_TOOLS/KGTEZ.lsp'
        self.forms=parse(source.read_text())
        for f in self.forms:
            if f and f[0]=='defun': self.functions[f[1]]=f
            elif f and f[0]=='setq': self.eval(f,self.env)
        self.env.update({
            '+':lambda *v:sum(v), '-':lambda a,b:a-b,
            '*':lambda *v:math.prod(v), '/':lambda a,b:a/b,
            '=':lambda a,b:a==b, '<=':lambda a,b:a<=b,
            '1+':lambda a:a+1, 'max':max, 'min':min,
            'list':lambda *a:list(a),'car':lambda a:a[0] if a else None,
            'cdr':lambda a:a[1] if isinstance(a,tuple) else a[1:] if a else None,
            'cadr':lambda a:a[1], 'nth':lambda i,a:a[i],
            'cons':lambda a,b:[a]+(b or []),
            'assoc':lambda k,a:next((e for e in (a or []) if e[0]==k),None),
            'subst':lambda n,o,a:[n if e==o else e for e in a],
            'equal':lambda a,b,t=0:abs(a-b)<=t if isinstance(a,(int,float)) else a==b,
            'fix':int,'itoa':str,'strcat':lambda *v:''.join(v),
            'rtos':lambda v,mode,prec:f'{v:.{prec}f}',
            'member':lambda v,a:v in a,
            'vlax-get-property':lambda o,p:o[str(p)],
            'vlax-property-available-p':lambda o,p:str(p) in o,
            'numberp':lambda x:isinstance(x,(int,float)), '>':lambda a,b:a>b,
            'type':lambda x:'variant' if isinstance(x,dict) else 'real' if isinstance(x,float) else 'int' if isinstance(x,int) else 'str' if isinstance(x,str) else 'nil',
            'vlax-variant-value':lambda x:x['variant'],
            'namedobjdict':lambda:None,
            'dictsearch':lambda *a:self.record,
        })
        self.record=None
    def call(self,name,*args):
        if name in self.env and callable(self.env[name]): return self.env[name](*args)
        f=self.functions[name];params=f[2];split=params.index('/') if '/' in params else len(params)
        local=dict(zip(params[:split],args));local.update({k:None for k in params[split+1:]})
        env=ChainMap(local,self.env)
        result=None
        for body in f[3:]:result=self.eval(body,env)
        return result
    def eval(self,x,e):
        if isinstance(x,Symbol):return e[x]
        if not isinstance(x,list):return x
        if not x:return None
        op=x[0]
        if op=='quote':return x[1]
        if op=='setq':
            for k,v in zip(x[1::2],x[2::2]):
                value=self.eval(v,e)
                target=e.maps[0] if isinstance(e,ChainMap) and k in e.maps[0] else self.env
                target[k]=value
            return value
        if op=='if':return self.eval(x[2],e) if self.eval(x[1],e) else self.eval(x[3],e) if len(x)>3 else None
        if op=='progn':
            result=None
            for body in x[1:]:result=self.eval(body,e)
            return result
        if op=='cond':
            for case in x[1:]:
                if self.eval(case[0],e):
                    value=None
                    for body in case[1:]:value=self.eval(body,e)
                    return value
            return None
        return self.call(op,*(self.eval(v,e) for v in x[1:]))

class KgtezUnits(unittest.TestCase):
    def setUp(self):self.h=Harness()
    def test_record_and_legacy_default(self):
        for value,expected in [(None,1.0),(1.0,1.0),(10.0,10.0),(1000.0,1000.0),(0,1.0),(100,1.0)]:
            self.h.record=None if value is None else [(40,value)]
            self.assertEqual(self.h.call('tvs-drawing-unit-factor'),expected)
    def test_actual_elbow_geometry_conversion_expression(self):
        def find(node):
            if isinstance(node,list):
                if len(node)>1 and node[0]=='cons' and isinstance(node[1],list) and node[1][:2]==['*','*tvs-mm-per-unit*']:return node[1]
                for c in node:
                    result=find(c)
                    if result:return result
        expression=find(self.h.functions['tvs-elbow-rg']);self.assertIsNotNone(expression)
        for factor in [1.0,10.0,1000.0]:
            self.h.env['*tvs-mm-per-unit*']=factor
            self.assertAlmostEqual(self.h.eval(expression,ChainMap({'ed':[(40,150.0/factor)]},self.h.env)),150.0)
    def test_channel_and_fittings_mass_invariant(self):
        for factor in [1.0,10.0,1000.0]:
            self.h.env.update({'*tvs-mm-per-unit*':factor,'*tvs-rows*':None})
            # Updated COM returns physical millimetres in every drawing.
            obj={'dpr':400.0,'sizeb':200.0,'length':3000.0,'angle':90.0,'dpr2':300.0,'sizeb2':200.0}
            pipe=self.h.call('tvs-info',obj,'TVS_Pipe')
            self.assertEqual(pipe[2],'400x200');self.assertAlmostEqual(pipe[3],3.6);self.assertEqual(pipe[5],3.0)
            self.h.call('tvs-add',pipe)
            self.assertAlmostEqual(self.h.env['*tvs-rows*'][0][9],16.956)
            self.h.call('tvs-add',pipe)
            self.assertAlmostEqual(self.h.env['*tvs-rows*'][0][9],33.912)
            self.assertEqual(self.h.env['*tvs-rows*'][0][6],2)
            self.h.env['tvs-elbow-rg']=lambda obj:[150.0,550.0]
            elbow=self.h.call('tvs-info',obj,'TVS_TAP')
            self.assertAlmostEqual(elbow[3],2*(400+200)*350*math.pi/2/1e6)
            trans=self.h.call('tvs-info',obj,'TVS_TRANS')
            self.assertAlmostEqual(trans[3],3.3)
            wye=self.h.call('tvs-info',dict(obj,length=50.0),'TVS_WYE')
            self.assertAlmostEqual(wye[3],0.47)
            self.assertIsNone(self.h.call('tvs-info',dict(obj,sizeb=0),'TVS_Pipe'))

if __name__=='__main__':unittest.main()

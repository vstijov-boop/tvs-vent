"""Compare C++ table sizing with KGTEZ's actual paper-height and scale functions."""
import subprocess
import tempfile
import unittest
from pathlib import Path
from test_kgtez_units import Harness

class ScaleHarness(Harness):
    def eval(self,value,env):
        if isinstance(value,list) and value and value[0]=='and':
            return all(self.eval(v,env) for v in value[1:])
        return super().eval(value,env)

class TableSizing(unittest.TestCase):
    def test_cpp_lisp_height_and_defaults(self):
        root=Path(__file__).parents[1]
        with tempfile.TemporaryDirectory() as tmp:
            exe=str(Path(tmp)/'sizing')
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-pedantic',
                str(root/'tests/test_table_and_selection.cpp'),'-o',exe],check=True)
            subprocess.run([exe],check=True)
            rows=subprocess.check_output([exe,'--fixtures'],text=True).splitlines()
        self.assertEqual(len(rows),15)
        h=ScaleHarness()
        self.assertEqual(h.env['*tvs-th*'],2.5)
        for row in rows:
            units,scale,expected=map(float,row.split())
            self.assertAlmostEqual(h.call('tvs-table-text-height',2.5,scale,units),expected)
        h.env.update({'numberp':lambda n:isinstance(n,(int,float)), '>':lambda a,b:a>b})
        for viewport,key,default in [(2,'TVS_TABLE_SCALE_MODEL',50),(1,'TVS_TABLE_SCALE_PAPER',1)]:
            h.env['getvar']=lambda name,v=viewport:v
            self.assertEqual(h.call('tvs-table-scale-record'),key)
            for saved,wanted in [(None,default),(0,default),(-1,default),(20,20),(50,50),(100,100)]:
                h.record=None if saved is None else [(40,saved)]
                self.assertEqual(h.call('tvs-table-scale-default'),wanted)

if __name__=='__main__': unittest.main()

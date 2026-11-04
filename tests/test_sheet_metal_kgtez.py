"""Cross-check the compiled C++ implementation against the actual KGTEZ formulas."""
import subprocess
import tempfile
import unittest
from pathlib import Path
from test_kgtez_units import Harness

class SheetMetalParity(unittest.TestCase):
    def test_cpp_matches_kgtez(self):
        root = Path(__file__).parents[1]
        with tempfile.TemporaryDirectory() as tmp:
            executable = str(Path(tmp)/'sheet-metal')
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-pedantic',
                str(root/'tests/test_sheet_metal.cpp'),'-o',executable],check=True)
            subprocess.run([executable],check=True)
            rows = subprocess.check_output([executable,'--fixtures'],text=True).splitlines()
        self.assertEqual(len(rows),56)
        names = ['TVS_Pipe','TVS_TAP','TVS_TRANS','TVS_WYE']
        h = Harness()
        for line in rows:
            kind,a,b,a2,b2,length,radius,angle,area,thickness,mass = map(float,line.split())
            obj = {'dpr':a,'sizeb':b,'dpr2':a2,'sizeb2':b2,'length':length,'angle':angle}
            # KGTEZ reads min/max radii of the same bend geometry.
            h.env['tvs-elbow-rg'] = lambda o, r=radius, width=a:[r,r+width]
            inf = h.call('tvs-info',obj,names[int(kind)])
            self.assertAlmostEqual(inf[3],area,places=12)
            h.env['*tvs-rows*'] = None
            h.call('tvs-add',inf)
            row = h.env['*tvs-rows*'][0]
            self.assertAlmostEqual(row[5],thickness,places=12)
            self.assertAlmostEqual(row[9],mass,places=10)

if __name__ == '__main__':
    unittest.main()

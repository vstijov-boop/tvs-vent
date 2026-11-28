"""Plan 9 (2026-11-28): ASHRAE oznaka fitinga u specifikaciji (dovod / odvod)."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Spec(unittest.TestCase):
 def test_labels(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_ashrae_spec.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_wiring(self):
  s=text('TVS_Ventilation_ARX/SPEC.cpp')
  self.assertLess(s.index('#include "../geometry/AshraeKeys.h"'),s.index('#define TeePants 9'))
  for k in ('tvsPressure::elbowSpec(','tvsPressure::junctionSpec(','tvsPressure::transitionSpec('):self.assertIn(k,s)
  self.assertIn('#include "AshraeKeys.h"',text('geometry/PressureDrop.h'))
  self.assertTrue(text('geometry/AshraeKeys.h').isascii())
if __name__=='__main__':unittest.main()

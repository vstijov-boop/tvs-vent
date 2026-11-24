"""MIRROR koljena gore / dolje (geometry/RoundElbow.h mirrorVertical, TVS_TAP::subTransformBy; plan 23)."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class ElbowMirror(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_elbow_mirror.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_wiring(self):
  t=text('TVS_Ventilation_DBX/TVS_TAP.cpp')
  self.assertIn('tvsRoundElbow::mirrorVertical(ThisRound != 0',t);self.assertIn('planMirror && Form == Form_Direct',t)
  self.assertTrue(all(c<128 for c in (ROOT/'geometry/RoundElbow.h').read_bytes()))
if __name__=='__main__':unittest.main()

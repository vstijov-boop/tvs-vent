"""MIRROR of TVS fittings: plan elbows reverse the arc (start = mirrored end direction),
side-flush prelazi swap the flush side (TransType 2 <-> 3)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def body(path,name):
    s=(ROOT/path).read_text(encoding='utf-8-sig');i=s.index(name);return s[i:s.index('return (Acad::eOk);',i)]
class Mirror(unittest.TestCase):
 def test_elbow(self):
  b=body('TVS_Ventilation_DBX/TVS_TAP.cpp','Acad::ErrorStatus TVS_TAP::subTransformBy')
  self.assertIn('xform.det() < 0',b);self.assertIn('Startvector.rotateBy(Swectangle, Normvector);',b)
  self.assertLess(b.index('rotateBy'),b.index('Startvector.transformBy(xform)'))
 def test_transition(self):
  b=body('TVS_Ventilation_DBX/TVS_TRANS.cpp','Acad::ErrorStatus TVS_TRANS::subTransformBy')
  self.assertIn('if (TransType == 2) TransType = 3; else if (TransType == 3) TransType = 2;',b)
if __name__=='__main__':unittest.main()

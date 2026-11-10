"""Faza 6: TVS elementi se ne skaliraju (TransformGuard.h); MIRROR ostaje; kanal nulte duzine bez NaN."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Guard(unittest.TestCase):
 def test_guard(self):
  g=text('TVS_Ventilation_DBX/TransformGuard.h')
  self.assertIn('std::fabs(v.length()-1)>1e-6',g)          # samo jedinicne ose: pomjeranje, rotacija, ogledalo
  self.assertIn('return Acad::eNotApplicable;',g)
  for c in ('Pipe','TAP','TRANS','WYE'):
   s=text('TVS_Ventilation_DBX/TVS_%s.cpp'%c);i=s.index('TVS_%s::subTransformBy(const AcGeMatrix3d & xform)'%c)
   self.assertIn('if (!tvsTransform::rigid(xform)) return tvsTransform::refuse();',s[i:i+300],c)
 def test_mirror_kept(self):
  self.assertIn('if (xform.det() < 0) { if (TransType == 2) TransType = 3;',text('TVS_Ventilation_DBX/TVS_TRANS.cpp'))
  self.assertIn('if (xform.det() < 0 && std::fabs(std::fabs(Normvector.z) - 1) < 1e-8)',text('TVS_Ventilation_DBX/TVS_TAP.cpp'))
 def test_zero_length(self):
  s=text('TVS_Ventilation_DBX/TVS_Pipe.cpp')
  self.assertNotIn('curViewUnitSize / Length;',s);self.assertNotIn('SizeA/Length/2',s);self.assertNotIn(')/Length/2',s)
if __name__=='__main__':unittest.main()

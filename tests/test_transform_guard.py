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
   self.assertIn('const double scale = tvsTransform::accepted(xform);',s[i:i+400],c)
   self.assertIn('if (scale <= 0) return tvsTransform::refuse();',s[i:i+500],c)
   self.assertIn('if (scale != 1)',s[i:i+1600],c)                                    # dimenzije prate dozvoljenu razmjeru
 def test_mirror_kept(self):
  self.assertIn('if (xform.det() < 0) { if (TransType == 2) TransType = 3;',text('TVS_Ventilation_DBX/TVS_TRANS.cpp'))
  self.assertIn('if (xform.det() < 0 && std::fabs(std::fabs(Normvector.z) - 1) < 1e-8)',text('TVS_Ventilation_DBX/TVS_TAP.cpp'))
 def test_zero_length(self):
  s=text('TVS_Ventilation_DBX/TVS_Pipe.cpp')
  self.assertNotIn('curViewUnitSize / Length;',s);self.assertNotIn('SizeA/Length/2',s);self.assertNotIn(')/Length/2',s)
 def test_units_between_drawings(self):
  g=text('TVS_Ventilation_DBX/TransformGuard.h')
  self.assertIn('GetEnvironmentVariableW(L"TVS_SCALE_OK"',g);self.assertIn('return uniform(m,s)&&scalingAllowed()?s:0;',g)
  w=text('TVS_Ventilation_ARX/ScaleWindow.h')
  for c in ('PASTECLIP','INSERT','EXPLODE'):self.assertIn('L"%s"'%c,w)
  self.assertNotIn('L"SCALE"',w)                                                 # SCALE ostaje zabranjen
  self.assertIn('tvsUnits::store(tvsUnits::factor(from),to)',w)                  # kopija nosi jedinice izvora
  self.assertIn('acdbGetUnitsConversion(from->insunits(),to->insunits(),ratio)',w)
  self.assertIn('r.l *= scale; r.second *= scale; r.radius2 *= scale;',text('TVS_Ventilation_DBX/TVS_WYE.cpp'))
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('tvsScaleWindow::start();',e);self.assertIn('tvsScaleWindow::stop();',e);self.assertIn('TVS_PRERACUNAJ, TVS_PRERACUNAJ',e)
 def test_move_section_diagnostics(self):
  m=text('TVS_Ventilation_ARX/TVSMoveCommand.h')
  self.assertIn('p->Elev+tvsTransAlign::endOffset(tvsTransAlign::read(p),',m)   # prelaz poravnat gore/dolje: osa na kraju
  self.assertIn('inline void sectionProblem(',m);self.assertEqual(m.count('sectionProblem(nodes,bodies,result,'),2)
  g=text('TVS_Ventilation_ARX/MoveNetworkGeometry.h')
  self.assertEqual(g.count('out.other=c.body;out.port=static_cast<int>(j);out.otherPort=c.port;'),2)
if __name__=='__main__':unittest.main()

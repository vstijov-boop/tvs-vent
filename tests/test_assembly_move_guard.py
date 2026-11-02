"""Whole-assembly MOVE/ROTATE wiring (source contract; AutoCAD runtime not executed here)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class AssemblyMoveGuard(unittest.TestCase):
 def test_wiring(self):
  g=(ROOT/'TVS_Ventilation_ARX/AssemblyMoveGuard.h').read_text()
  self.assertIn('endSSGet(Acad::PromptStatus status,int,AcEdSelectionSetService&,const AcDbObjectIdArray& ids) override',g)
  self.assertIn('if(tvsBypassData::members(o.object(),all))need.assign(all,all+7);',g,'bypass: all seven members')
  self.assertIn('need.assign(r.members,r.members+3)',g,'standalone offset: V1 triple')
  self.assertIn('if(complete){for(const AcDbObjectId& n:need)tvsOffsetData::permitAssemblyTransform(n);}else ++partial();',g,'all or nothing')
  r=(ROOT/'TVS_Ventilation_ARX/OffsetIntegrationReactor.h').read_text()
  self.assertIn('virtual void commandWillStart(const ACHAR*){tvsAssemblyMove::begin();}',r)
  for k in ('commandEnded','commandCancelled','commandFailed'):self.assertRegex(r,k+r'\(const ACHAR\*[^)]*\)\{tvsAssemblyMove::end\(\);')
  e=(ROOT/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_text(encoding='utf-8',errors='replace')
  self.assertIn('tvsAssemblyMove::attachAll(true)',e);self.assertIn('tvsAssemblyMove::attachAll(false)',e)
  self.assertIn('tvsAssemblyMove::detach(',e)
  for f in ('TVS_Pipe.cpp','TVS_TAP.cpp'):
   t=(ROOT/'TVS_Ventilation_DBX'/f).read_text(encoding='utf-8',errors='replace')
   self.assertIn('if(!tvsOffsetData::allowTransform(this,xform)){tvsOffsetData::noteTransformRefusal();return Acad::eNotApplicable;}',t)
  d=(ROOT/'TVS_Ventilation_ARX/BypassPreviewCommand.cpp').read_text(encoding='utf-8',errors='replace')
  self.assertIn('AcDbObjectPointer<TVS_Pipe> live(r.obstacle,AcDb::kForRead);',d,'detail uses the current obstacle')
if __name__=='__main__':unittest.main()

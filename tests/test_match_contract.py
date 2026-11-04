"""TVS_MATCH wiring (source contracts)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig',errors='replace')
class MatchContract(unittest.TestCase):
 def test_contracts(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('Ventilation_ARX, TVS_MATCH, TVS_MATCH, ACRX_CMD_MODAL',e);self.assertIn('tvsMatch::run();',e);self.assertIn('#include "MatchCommand.h"',e)
  m=text('TVS_Ventilation_ARX/MatchCommand.h')
  self.assertIn('tvsTeeFit::farEnd(AcDbObjectId::kNull,widened[i],true,tol,f);tvsTeeFit::farEnd(AcDbObjectId::kNull,widened[i],false,tol,f);',m,'width step at both ends gets a transition')
  self.assertIn('const double axis=tr?targetAxis(src,tr->SizeAp1,tr->SizeBp1,o.reference):targetAxis(src,t->SizeA,t->SizeB,o.reference);',m)
  self.assertIn('if(pipe->ThisRound!=src.round)++skippedShape;',m,'no round/rectangular conversion')
  self.assertIn('if(!ask(whole))return;',m,'settings in a dialog, not the command line')
  self.assertIn('IDD_MATCHSET DIALOGEX',text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc'));self.assertIn('#define IDD_MATCHSET',text('TVS_Ventilation_ARX/resource.h'))
  self.assertIn('if(whole){if(!network(db,tol,targets))return;}',m)
  self.assertIn('const int shifted=tvsElevationShift::shift(targets,delta,src.id);',m,'Mreza: whole network shifted, level changes kept')
  self.assertIn('if(t->put_Elevation(t->Elev+delta)!=Acad::eOk)continue;',text('TVS_Ventilation_ARX/ElevationShift.h'))
  self.assertIn('if(o.elevation&&!whole){',m)
  self.assertIn('trans->put_SizeAp1(src.A);trans->put_SizeBp1(src.B);trans->put_SizeAp2(src.A);trans->put_SizeBp2(src.B);',m,'Dimenzije on a prelaz: both ends')
  self.assertIn('if(tvsElevationShift::side(db,offsetBefore[i].first,1,tol,ids,loop)&&!loop)tvsElevationShift::shift(ids,d,src.id);',m,'Mreza continuous across a prelaz')
  for line in m.splitlines():self.assertFalse(line.rstrip().endswith('L"'))
if __name__=='__main__':unittest.main()

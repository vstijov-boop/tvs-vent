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
  self.assertIn('acedInitGet(0,L"Dimenzije Kota Referenca Tip Prikaz prOtok Sloj");',m,'keyword letters unique (R = Referenca, O = prOtok)')
  self.assertIn('if(rc==RTNORM&&!_wcsicmp(kw,L"Mreza")){if(!network(db,tol,targets))return;whole=true;}',m)
  self.assertIn('const int shifted=tvsElevationShift::shift(targets,delta,src.id);',m,'Mreza: whole network shifted, level changes kept')
  self.assertIn('if(t->put_Elevation(t->Elev+delta)!=Acad::eOk)continue;',text('TVS_Ventilation_ARX/ElevationShift.h'))
  self.assertIn('if(o.elevation&&!whole){',m)
  for line in m.splitlines():self.assertFalse(line.rstrip().endswith('L"'))
if __name__=='__main__':unittest.main()

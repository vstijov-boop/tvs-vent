"""Flex cuffs (manzetne): filed flags, drawing hook, marking/toggle commands and the
specification item (source contracts; geometry is executed in test_flex_hose)."""
import pathlib,unittest
from entry_source import entry_source
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig',errors='replace')
class FlexCuffs(unittest.TestCase):
 def test_contracts(self):
  d=text('TVS_Ventilation_DBX/FlexCuffData.h')
  self.assertIn('L"TVS_FLEX_CUFF_V1"',d);self.assertIn('inline double lengthMm(){return 40;}',d)
  for f,needle in (('TVS_Ventilation_DBX/TVS_Pipe.cpp','tvsFlex::straight(FirstPoint.x, FirstPoint.y, axis.x / length, axis.y / length, length, SizeA, tvsFlex::defaultPitch(), hose, cuffs)'),
                   ('TVS_Ventilation_DBX/TVS_TAP.cpp','tvsFlex::defaultPitch(), hose, cuffs)')):
   self.assertIn(needle,text(f))
  c=text('TVS_Ventilation_ARX/FlexCuffCommand.h')
  self.assertIn('if(q.id!=id&&q.flex&&std::hypot(q.p.x-ends[k].x,q.p.y-ends[k].y)<=tol)cuff[k]=false;',c,'only flex continuation suppresses a cuff')
  e=entry_source()
  for cmd in ('TVS_FLEX_MANZETNE','TVS_FLEX_MANZETNA'):self.assertIn('Ventilation_ARX, %s, %s, ACRX_CMD_MODAL'%(cmd,cmd),e)
  self.assertIn('cuff.addCuffs(pEnt, spec.SizeA, int(cuffStart) + int(cuffEnd));',e)
  s=text('TVS_Ventilation_ARX/SPEC.cpp');self.assertIn('#define FlexCuff 7',s);self.assertIn('setUnit1(_T("kom"));\n\tsetUnit2(_T(""));\n\tsetParam1(count, TypeInt);',s)
if __name__=='__main__':unittest.main()

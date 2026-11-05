"""Etaz follows a changed duct elevation (OffsetRefit.h, TVS_ETAZ_KOTA): wiring contract."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class OffsetRefit(unittest.TestCase):
 def test_wiring(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('Ventilation_ARX, TVS_ETAZ_KOTA, TVS_ETAZ_KOTA, ACRX_CMD_MODAL',e)
  self.assertIn('tvsOffsetRefit::attachAll();',e);self.assertIn('tvsOffsetRefit::detachAll();',e)
  self.assertIn('tvsOffsetRefit::attach(acdbHostApplicationServices()->workingDatabase());',e)
  r=text('TVS_Ventilation_ARX/OffsetIntegrationReactor.h');self.assertIn('tvsOffsetRefit::commandEnded(command);',r)
  s=text('TVS_Ventilation_ARX/OffsetRefit.h')
  # same solver/adapter inputs as tvsOffsetData::validateCore, bypass triples left alone
  self.assertIn('tvsOffset::Input in((elevB-elevA)*f,p->SizeA*f,bend*f,a->get_radius()*f,a->Swectangle,0);',s)
  self.assertIn('tvsBypassData::marked(a.object())',s);self.assertIn('tvsOffsetData::WriteScope scope;',s)
  self.assertIn('Prilagoditi etaz? [Da/Ne] <Da>',s);self.assertIn('o->isUndoing()',s)
  self.assertEqual(s.count('{'),s.count('}'))
if __name__=='__main__':unittest.main()

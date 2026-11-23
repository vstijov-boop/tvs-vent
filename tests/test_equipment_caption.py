"""TVS_NATPIS za opremu: overrule na AcDbBlockReference (EquipmentCaption.h)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Caption(unittest.TestCase):
 def test_overrule(self):
  s=text('TVS_Ventilation_ARX/EquipmentCaption.h')
  self.assertIn('class Overrule : public AcGiDrawableOverrule',s)
  self.assertIn('AcRxOverrule::addOverrule(AcDbBlockReference::desc(),&instance(),false)',s)
  self.assertIn('AcRxOverrule::removeOverrule(AcDbBlockReference::desc(),&instance())',s)
  self.assertIn('isPlotGeneration()',s);self.assertIn('busy()=true;es=ref->getGeomExtents(ext);busy()=false;',s)
  self.assertIn('tvsDuctCaption::enabled(db)',s)
  self.assertIn('std::wstring s=L"+ ";',s);self.assertIn('text=L"- "+n;',s)
  self.assertEqual(s.count('{'),s.count('}'))
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/EquipmentCaption.h').read_bytes().startswith(b'\xef\xbb\xbf'))
 def test_registered(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('#include "EquipmentCaption.h"',e)
  self.assertIn('tvsEquipmentCaption::start();',e);self.assertIn('tvsEquipmentCaption::stop();',e)
 def test_apply_all_report(self):
  d=text('TVS_Ventilation_ARX/EquipmentDialog.h')
  self.assertIn('Ostalih blokova istog naziva: %d (povezanih %d).',d)
if __name__=='__main__':unittest.main()

"""TVS_POPRAVI: popravka zapisa crteza starijeg builda (RepairCommand.h, korisnik 24.11.)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Repair(unittest.TestCase):
 def test_command(self):
  s=text('TVS_Ventilation_ARX/RepairCommand.h')
  self.assertIn('tvsTee::isCone(r.second,w->SizeAotv,r.l)',s)
  self.assertIn('t->connectors[c].connectionID=AcDbObjectId::kNull',s)
  self.assertIn('tvsEquipmentStore::anchor(b.object(),r)',s)
  self.assertIn('Popraviti? [Da/Ne] <Da>: ',s);self.assertIn('acedInitGet(0,L"Da Ne")',s)
  self.assertEqual(s.count('{'),s.count('}'))
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/RepairCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
 def test_registered(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('#include "RepairCommand.h"',e);self.assertIn('Ventilation_ARX, TVS_POPRAVI, TVS_POPRAVI, ACRX_CMD_MODAL, NULL)',e)
  self.assertIn('tvsRepair::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
if __name__=='__main__':unittest.main()

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
  self.assertIn('tvsTeeOpenEnds::write(t.object(),true)',s);self.assertIn('r.terminal()&&r.mount!=tvsEquipment::Bottom',s)
  self.assertIn('Popraviti? [Da/Ne] <Da>: ',s);self.assertIn('acedInitGet(0,L"Da Ne")',s)
  self.assertEqual(s.count('{'),s.count('}'))
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/RepairCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
 def test_open_ends(self):
  w=text('TVS_Ventilation_DBX/TVS_WYE.cpp');self.assertIn('if (tvsTeeOpenEnds::read(this)) {',w)
  g=text('TVS_Ventilation_ARX/GrilleCommand.h');self.assertIn('tvsTeeOpenEnds::write(w,true)',g)
  h=(ROOT/'TVS_Ventilation_DBX/TeeOpenEnds.h').read_bytes();self.assertTrue(all(c<128 for c in h))
 def test_registered(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('#include "RepairCommand.h"',e);self.assertIn('Ventilation_ARX, TVS_POPRAVI, TVS_POPRAVI, ACRX_CMD_MODAL, NULL)',e)
  self.assertIn('tvsRepair::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
 def test_trace(self):
  t=text('TVS_Ventilation_ARX/Trace.h');self.assertIn('std::fflush(file());',t);self.assertIn('TVS_trag.log',t)
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp');self.assertIn('Ventilation_ARX, TVS_TRAG, TVS_TRAG, ACRX_CMD_MODAL, NULL)',e)
  for p,n in (('TVS_Ventilation_ARX/commands/ConnectHelpers.inl','ConnectWithTap'),('TVS_Ventilation_ARX/SizeFollow.h','SizeFollow::commandEnded'),('TVS_Ventilation_ARX/SizeFollow.h','SizeFollow::settle'),
              ('TVS_Ventilation_ARX/OffsetRefit.h','OffsetRefit::commandEnded'),('TVS_Ventilation_ARX/EquipmentCaption.h','EquipmentCaption::worldDraw')):
   self.assertIn('tvsTrace::Scope trace_("'+n+'");',text(p),p)
if __name__=='__main__':unittest.main()

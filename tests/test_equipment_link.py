"""TVS_OPREMA / TVS_OPREMA_ODVEZI: povezivanje nacrtanih blokova opreme u mrezu
(geometry/EquipmentLink.h + TVS_Ventilation_ARX/EquipmentLinkCommand.h)."""
import pathlib,re,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class EquipmentLink(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_equipment_link.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_header_ascii(self):
  (ROOT/'geometry/EquipmentLink.h').read_bytes().decode('ascii')
 def test_command_file(self):
  raw=(ROOT/'TVS_Ventilation_ARX/EquipmentLinkCommand.h').read_bytes()
  self.assertTrue(raw.startswith(b'\xef\xbb\xbf'))
  c=text('TVS_Ventilation_ARX/EquipmentLinkCommand.h')
  self.assertIn('#include "../geometry/EquipmentLink.h"',c);self.assertIn('#include "EquipmentStore.h"',c)
  # the block never moves: no transformBy / setPosition on the block reference
  self.assertNotIn('setPosition',c);self.assertNotRegex(c,r'ref->transformBy|b->transformBy')
  # one transaction per block, preflight before it, record through the shared store
  self.assertIn('tvsAction::Transaction tx;',c);self.assertIn('tvsAction::writable(touched)',c)
  self.assertIn('tvsEquipmentStore::write(b,rec)',c);self.assertIn('tvsEquipmentStore::readDefault(db,name.c_str(),def)',c)
  self.assertIn('tvsEquipmentStore::remove(ref.object())',c)
  self.assertIn('rec.mmPerUnit=f;',c);self.assertIn('ref->blockTransform()',c)
  self.assertIn('AcDbAttributeDefinition::cast(e)',c)          # attribute definitions not in the footprint
  self.assertIn('isFromExternalReference()',c)
  # duct ends only through put_FirstPoint / put_Lastpoint, the cut piece appended in the transaction
  self.assertIn('p->put_FirstPoint(to):p->put_Lastpoint(to)',c);self.assertIn('tx.append(space,n,pieceId)',c)
  self.assertNotIn('TVS_Pipe::add_new',c)                       # add_new posts with open/close (no rollback)
  self.assertIn('acedInitGet(0,L"Izaberi Sve Tolerancija")',c);self.assertIn('acedInitGet(0,L"Da Ne")',c)
 def test_registered(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('#include "EquipmentLinkCommand.h"',e)
  i=e.index('TVS_DPD_IZVOZ, TVS_DPD_IZVOZ, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)')
  rest=e[i:].split('\n')
  self.assertIn('TVS_OPREMA, TVS_OPREMA, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)',rest[1])
  self.assertIn('TVS_OPREMA_ODVEZI, TVS_OPREMA_ODVEZI, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)',rest[2])
  n=text('TVS_Ventilation_ARX/commands/NewCommands.inl')
  self.assertIn('static void Ventilation_ARXTVS_OPREMA(void)',n);self.assertIn('tvsEquipmentLinkCmd::execute();',n)
  self.assertIn('static void Ventilation_ARXTVS_OPREMA_ODVEZI(void)',n);self.assertIn('tvsEquipmentLinkCmd::unlink();',n)
  self.assertLess(n.index('Ventilation_ARXTVS_DPD_IZVOZ'),n.index('Ventilation_ARXTVS_OPREMA(void)'))
if __name__=='__main__':unittest.main()

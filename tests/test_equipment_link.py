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
 def test_terminal_rules(self):
  c=text('TVS_Ventilation_ARX/EquipmentLinkCommand.h')
  # base point rule: insertion point + free elbow outlets go to the 5-argument link
  self.assertIn('at=lk::Pt(ref->position().x,ref->position().y);',c)
  self.assertIn('lk::Result r=lk::link(block,at,ducts,outlets,o);',c);self.assertNotIn('lk::link(block,ducts,o)',c)
  self.assertIn('base.bottom=true;',c)
  # elbow Gore / Dolje outlet: form, 90 deg, level Elev -/+ rc, RoundElbow.h for round elbows
  self.assertIn('#include "../geometry/RoundElbow.h"',c)
  self.assertIn('form!=Form_Up&&form!=Form_Down',c);self.assertIn('tap->get_radius()+(round?tap->SizeA:tap->SizeB)/2',c)
  self.assertIn('tap->Elev+(form==Form_Up?1:-1)*rc*(1-std::cos(tap->Swectangle))',c);self.assertIn('tvsRoundElbow::ports(',c)
  # bottom grille: Dno / Linija once per block name, remembered as the default; neck from the default or the footprint
  self.assertIn('acedInitGet(0,L"Dno Linija")',c);self.assertIn('[Dno/Linija] <Linija>',c)
  self.assertIn('tvsEquipmentStore::writeDefault(db,name.c_str(),d)',c)
  self.assertIn('rec.mount=r.ports==1?r.mount:tvsEquipment::AtEnd;',c);self.assertIn('lk::footprintMm(block,f,w,h);',c)
  self.assertIn('TVS_OPREMA_SVOJSTVA',c)
  # kinds not set: hint + the properties dialog for those blocks
  self.assertIn('Podesiti vrstu sada? [Da/Ne] <Da>',c);self.assertIn('tvsEquipmentDialog::run(IDD_EQUIPMENT,t.unset);',c)
  self.assertIn('#include "EquipmentDialog.h"',c)
  d=text('TVS_Ventilation_ARX/EquipmentDialog.h')
  self.assertIn('inline void run(UINT idd,const std::vector<AcDbObjectId>& ids){',d)
  self.assertRegex(d,r'inline void run\(UINT idd\)\{\n    std::vector<AcDbObjectId> ids;if\(!pick\(ids\)\)return;\n    run\(idd,ids\);\n\}')
  for f in (c,d):
   code=re.sub(r'//.*','',f)
   for w in ('near','far','small','min','max'):self.assertNotRegex(code,r'\b%s\b'%w)
  # acedInitGet keywords plain ASCII
  for k in re.findall(r'acedInitGet\(0,L"([^"]*)"\)',c):k.encode('ascii')
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

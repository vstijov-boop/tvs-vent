"""Oprema na mrezi: TVS_OPREMA_SVOJSTVA (geometry/EquipmentEdit.h, EquipmentDialog.h), specifikacija povezanih blokova, ribbon."""
import pathlib,re,subprocess,tempfile,unittest,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class EquipmentEdit(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_equipment_edit.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_header_ascii(self):
  (ROOT/'geometry/EquipmentEdit.h').read_bytes().decode('ascii')
 def test_bom(self):
  for f in ('TVS_Ventilation_ARX/EquipmentDialog.h','TVS_Ventilation_ARX/SPEC.cpp','TVS_Ventilation_ARX/SPEC.h','TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc','TVS_Ventilation_ARX/resource.h',
            'TVS_Ventilation_ARX/acrxEntryPoint.cpp','TVS_Ventilation_ARX/commands/NewCommands.inl','TVS_Ventilation_ARX/commands/Specification.inl'):
   b=(ROOT/f).read_bytes();self.assertEqual(b[:3],b'\xef\xbb\xbf',f);self.assertNotIn(b'\r\n',b,f)
 def test_dialog_resource(self):
  r=text('TVS_Ventilation_ARX/resource.h');rc=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc')
  self.assertRegex(r,r'#define IDD_EQUIPMENT\s+122')
  self.assertEqual(len(re.findall(r'(?m)^#define IDD_\w+\s+122\s*$',r)),1)
  body=rc.split('IDD_EQUIPMENT DIALOGEX')[1].split('\nEND')[0]
  for i in range(1701,1723):self.assertRegex(body,r'\b%d,'%i)
  for y in ('2020','2022'):self.assertIn('<ResourceCompile Include="TVS_Ventilation_ARX.rc" />',text('TVS_Ventilation_ARX/TVS_Ventilation_ARX%s.vcxproj'%y))
  h=text('TVS_Ventilation_ARX/EquipmentDialog.h')
  for i in range(1701,1723):self.assertIn('=%d'%i,h)
 def test_dialog_source(self):
  h=text('TVS_Ventilation_ARX/EquipmentDialog.h')
  self.assertIn('CAcModuleResourceOverride',h)
  self.assertIn('tvsEquipmentStore::read(',h);self.assertIn('Blok nije povezan u mrežu (TVS_OPREMA).',h)
  self.assertIn('tvsEquipmentStore::writeDefault(',h);self.assertIn('getBlockReferenceIds(',h)
  self.assertIn('tvsAction::writable(',h);self.assertIn('tvsAction::Transaction',h);self.assertIn('tx.commit()',h)
  self.assertIn('tvsDecimal::DDX_Plain(',h)
  for f in ('nextTag(','validate(','merge(','apply(','typeDefault(','applyType('):self.assertIn('tvsEquipmentEdit::'+f,h)
  code=re.sub(r'//.*','',h)
  self.assertNotIn('_wtof(',code)
  for w in ('near','far','small','min','max'):self.assertNotRegex(code,r'\b%s\b'%w)
 def test_command(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  pad='ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_PAD, TVS_PAD, ACRX_CMD_MODAL, NULL)\n'
  self.assertIn(pad+'ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OPREMA_SVOJSTVA, TVS_OPREMA_SVOJSTVA, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)',e)
  self.assertIn('#include "EquipmentDialog.h"',e)
  n=text('TVS_Ventilation_ARX/commands/NewCommands.inl')
  self.assertIn('static void Ventilation_ARXTVS_OPREMA_SVOJSTVA(void)',n);self.assertIn('tvsEquipmentDialog::run(IDD_EQUIPMENT);',n)
 def test_specification(self):
  s=text('TVS_Ventilation_ARX/SPEC.cpp')
  self.assertIn('bool SPEC::addEquipment(AcDbEntity * pEnt)',s);self.assertIn('tvsEquipmentStore::read(pEnt, rec)',s)
  self.assertIn('tvsEquipmentEdit::specName(rec)',s);self.assertIn('tvsEquipmentEdit::sizeText(rec, d)',s)
  self.assertIn('wcsncpy_s(sPos, rec.tag.c_str(), _TRUNCATE);',s);self.assertIn('wcscpy_s(sUnit, _T("kom"));',s)
  self.assertLess(s.index('#include "EquipmentStore.h"'),s.index('#define None 0'))   # before the SPEC.cpp macros
  self.assertIn('bool addEquipment(AcDbEntity * pEnt);',text('TVS_Ventilation_ARX/SPEC.h'))
  i=text('TVS_Ventilation_ARX/commands/Specification.inl')
  self.assertEqual(i.count('spec.addEquipment(pEnt)'),1)   # TVS_SPEC / TVS_EXCELSPEC; TVS_BLCKMNGR (atributi) ostaje
  self.assertEqual(i.count('if (spec.addEquipment(pEnt) || spec.addBlock(pEnt))'),1)
 def test_ribbon(self):
  with zipfile.ZipFile(ROOT/'Installer/TVS_VENT/TVS_VENT.cuix') as z:
   names=z.namelist();mg=z.read('MenuGroup.cui').decode('utf-8','ignore')
   for c in ('OPREMA','OPREMA_ODVEZI','OPREMA_SVOJSTVA'):
    self.assertIn('^C^C_TVS_%s;'%c,mg)
    for n in ('ToolbarRoot.cui','PopMenuRoot.cui','RibbonRoot.cui'):self.assertIn('MenuMacroID="MMU_TVS_%s"'%c,z.read(n).decode('utf-8','ignore'))
    for s in (16,32):
     self.assertIn('TVS_%s_%d.png'%(c,s),names)
     self.assertEqual(z.read('TVS_%s_%d.png'%(c,s)),(ROOT/'Installer/TVS_VENT/icons'/('TVS_%s_%d.png'%(c,s))).read_bytes())
if __name__=='__main__':unittest.main()

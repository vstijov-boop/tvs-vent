"""TVS_RESETKA: prikljucenje resetke na kanal (geometry/GrilleConnect.h, TVS_Ventilation_ARX/GrilleCommand.h,
dijalog IDD_GRILLE, meni / ribbon)."""
import pathlib,re,subprocess,tempfile,unittest,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class GrilleConnect(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_grille_connect.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_header_ascii(self):
  (ROOT/'geometry/GrilleConnect.h').read_bytes().decode('ascii')
 def test_bom(self):
  for f in ('TVS_Ventilation_ARX/GrilleCommand.h','TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc','TVS_Ventilation_ARX/resource.h',
            'TVS_Ventilation_ARX/acrxEntryPoint.cpp','TVS_Ventilation_ARX/commands/NewCommands.inl'):
   b=(ROOT/f).read_bytes();self.assertEqual(b[:3],b'\xef\xbb\xbf',f);self.assertNotIn(b'\r\n',b,f)
 def test_dialog_resource(self):
  r=text('TVS_Ventilation_ARX/resource.h');rc=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc')
  self.assertRegex(r,r'#define IDD_GRILLE\s+123')
  self.assertEqual(len(re.findall(r'(?m)^#define IDD_\w+\s+123\s*$',r)),1)
  body=rc.split('IDD_GRILLE DIALOGEX')[1].split('\nEND')[0]
  for i in range(1801,1820):self.assertRegex(body,r'\b%d,'%i)
  h=text('TVS_Ventilation_ARX/GrilleCommand.h')
  for i in range(1801,1820):self.assertIn('=%d'%i,h)
  for t in ('Lijevo','Desno','Dno','Ravni T komad (TK0)','Štucna','Konusna štucna','Direktno na stranicu','Okrugli ogranak (D)','Priključi'):self.assertIn(t,body)
 def test_command_source(self):
  h=text('TVS_Ventilation_ARX/GrilleCommand.h')
  self.assertIn('#include "../geometry/GrilleConnect.h"',h);self.assertIn('#include "EquipmentStore.h"',h)
  self.assertIn('CAcModuleResourceOverride',h)
  # one transaction, preflight before it, nothing through add_new (open / close, no rollback)
  self.assertIn('tvsAction::Transaction tx;',h);self.assertIn('tvsAction::writable(touched)',h);self.assertIn('tx.commit()',h)
  self.assertNotIn('::add_new',re.sub(r'//.*','',h));self.assertIn('tx.append(space,w,teeId)',h);self.assertIn('tx.append(space,s,stubId)',h);self.assertIn('tx.append(space,rest,restId)',h)
  self.assertIn('tvsEquipmentLinkCmd::newPiece(d,runEnd)',h);self.assertIn('d->put_Lastpoint(runStart)',h)
  # stucna as the drawing commands make it: record type Tap, L, cone base, margin 25 mm (radius2)
  self.assertIn('r.type=tvsTee::Tap;r.l=y.lengthPl;r.second=y.cone;r.radius2=o.margin;',h)
  self.assertIn('o.tapMinimum=tvsTeeData::tapMinimumMm()/f',h);self.assertIn('o.margin=tvsTee::tapMarginMm()/f',h)
  # the block moves with its attributes (transformBy), record written through the shared store
  self.assertIn('ref->transformBy(m)',h);self.assertIn('gc::turnAngle(',h)
  self.assertIn('tvsEquipmentStore::write(ref,rec)',h);self.assertIn('tvsEquipmentStore::write(b,rec)',h)
  self.assertIn('tvsEquipmentStore::readDefault(db,name.c_str(),def)',h)
  self.assertIn('rec.mount=tvsEquipment::Bottom;',h);self.assertIn('rec.mount=tvsEquipment::AtEnd;',h);self.assertIn('rec.mmPerUnit=f;',h)
  self.assertIn('q.dx=0;q.dy=0;',h)
  # stub length in the registry next to the other TVS_Vent values
  self.assertIn('L"Software\\\\TVS_Vent"',h);self.assertIn('L"ResetkaOgranak"',h)
  self.assertIn('tvsDecimal::DDX_Real(',h)
  self.assertIn('acedInitGet(0,L"Najblizi")',h)
  code=re.sub(r'//.*','',h)
  self.assertNotIn('_wtof(',code)
  for w in ('near','far','small','min','max'):self.assertNotRegex(code,r'\b%s\b'%w)
 def test_registered(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  self.assertIn('#include "GrilleCommand.h"',e)
  svo='ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_OPREMA_SVOJSTVA, TVS_OPREMA_SVOJSTVA, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)\n'
  self.assertIn(svo+'ACED_ARXCOMMAND_ENTRY_AUTO(CTVS_Ventilation_ARXApp, Ventilation_ARX, TVS_RESETKA, TVS_RESETKA, ACRX_CMD_MODAL | ACRX_CMD_USEPICKSET, NULL)\n',e)
  n=text('TVS_Ventilation_ARX/commands/NewCommands.inl')
  self.assertIn('static void Ventilation_ARXTVS_RESETKA(void)',n);self.assertIn('tvsGrilleCmd::execute(IDD_GRILLE, globalLengthW);',n)
  self.assertLess(n.index('Ventilation_ARXTVS_OPREMA_SVOJSTVA'),n.index('Ventilation_ARXTVS_RESETKA'))
  self.assertLess(n.index('Ventilation_ARXTVS_RESETKA'),n.index('Ventilation_ARXTVS_DPD_IZVOZ'))
 def test_ribbon(self):
  with zipfile.ZipFile(ROOT/'Installer/TVS_VENT/TVS_VENT.cuix') as z:
   names=z.namelist();mg=z.read('MenuGroup.cui').decode('utf-8','ignore')
   self.assertIn('^C^C_TVS_RESETKA;',mg)
   for n in ('ToolbarRoot.cui','PopMenuRoot.cui','RibbonRoot.cui'):self.assertIn('MenuMacroID="MMU_TVS_RESETKA"',z.read(n).decode('utf-8','ignore'))
   for s in (16,32):
    self.assertIn('TVS_RESETKA_%d.png'%s,names)
    self.assertEqual(z.read('TVS_RESETKA_%d.png'%s),(ROOT/'Installer/TVS_VENT/icons'/('TVS_RESETKA_%d.png'%s)).read_bytes())
  self.assertIn("('RESETKA', 'Priključenje rešetke'",text('tools/add_new_commands_cuix.py'))
if __name__=='__main__':unittest.main()

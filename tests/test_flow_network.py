"""TVS_PROTOK: sabiranje protoka (geometry/FlowNetwork.h) i komanda."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Flow(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_flow_network.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_ascii(self):
  for h in ('FlowNetwork.h','DuctSizing.h','DionicaPlan.h'):(ROOT/'geometry'/h).read_bytes().decode('ascii')
 def test_plan_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'p'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_dionica_plan.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_sizing_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'s'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_duct_sizing.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_velocities(self):
  c=text('TVS_Ventilation_ARX/FlowCommand.h')
  self.assertEqual(c.count('sizes(db,net,r,el,f,tol,sizingDialog,net.nodes[root].id,el[root].ports[port]);'),3)          # posle upisa, bez izmjene i uz Ne
  self.assertIn('L"TVS_RUCNO"',c);self.assertIn('tvsSizingDialog::show(in,def,sizingDialog)',c);self.assertIn('L"CMDECHO"',text('TVS_Ventilation_ARX/CheckCommand.h'));self.assertIn('acdbRegApp(lockApp)',c)
  self.assertIn('tvsDuctSizing::widthOnly(',c);self.assertIn('tvsDuctSizing::widthHeight(',c);self.assertIn('tvsDuctSizing::diameter(',c)
 def test_tee_dialog_duct_behind_prelaz(self):   # PLAN 22: Kanal u nastavku otkljucan i iza prelaza
  t=text('TVS_Ventilation_ARX/TeeTypeDialog.h')
  self.assertIn('const bool duct=d.neighbour==1||d.neighbour==2;',t);self.assertIn('enable(IdDuctB,(d.neighbour==1||d.neighbour==2)&&!d.duct.round);',t)
  self.assertIn('if(x.neighbour==2&&!tvsTeePort::same(x.duct,x.ductWas,1e-9)){',t)
  self.assertLess(t.index('if(x.neighbour==2&&!tvsTeePort::same'),t.index('        tvsTeePort::ensure(s.id,x.k,x.lengthMm);'))
 def test_tee_dialog_common_height(self):   # PLAN 22: visina TK-O / TK-G otkljucana, ulazni kanal je preuzima
  t=text('TVS_Ventilation_ARX/TeeTypeDialog.h')
  self.assertIn('enable(IdTeeB,!d.tee.round);',t);self.assertIn('if(d.lockB){heightNew=v/f;',t)
  self.assertIn('if(heightChanged){',t);self.assertIn('p->put_SizeB(d.heightNew);',t)
  self.assertLess(t.index('w->SizeBotv=d.heightNew;else w->SizeBpr=d.heightNew;'),t.index('if(d.choice==Branch){w->SizeAotv=b.tee.a;w->SizeBotv=w->SizeBpr;'))
 def test_dialog(self):
  d=text('TVS_Ventilation_ARX/SizingDialog.h');self.assertTrue((ROOT/'TVS_Ventilation_ARX/SizingDialog.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  self.assertIn('tvsDuctSizing::candidates(',d);self.assertIn('tvsDuctSizing::friction(',d);self.assertIn('tvsDuctSizing::parse(',d)
  self.assertIn('CAcModuleResourceOverride',d);self.assertNotIn('put_Size',d)
  rc=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc');self.assertIn('IDD_SIZING DIALOGEX',rc);self.assertIn('#define IDD_SIZING',text('TVS_Ventilation_ARX/resource.h'))
  block=rc[rc.index('IDD_SIZING DIALOGEX'):];block=block[:block.index('\nEND')]
  import re
  ids=set(int(x) for x in re.findall(r'[\s,](16\d\d),',block))
  for k in re.findall(r'Id\w+=(16\d\d)',d):self.assertIn(int(k),ids,k)                                  # prijedlog, bez promjene dimenzija
 def test_command(self):
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/FlowCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  c=text('TVS_Ventilation_ARX/FlowCommand.h')
  self.assertIn('tvsFlowNetwork::solve(el,root,port,tol)',c)
  self.assertIn('Upisati protoke',c);self.assertIn('recalculateMleader',c)
  self.assertIn('tvsMove::bodiesFor(',c)   # etaz / obilazak as one element
  self.assertIn('TVS_PROTOK, TVS_PROTOK',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('#include "FlowCommand.h"',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('tvsFlow::execute(IDD_SIZING);',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('PROTOK',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_PROTOK_%d.png'%s)).exists())
if __name__=='__main__':unittest.main()

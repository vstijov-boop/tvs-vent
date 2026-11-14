"""TVS_PAD: pad pritiska po dionicama (geometry/PressureDrop.h, PressureSheet.h) i komanda."""
import os,pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
FLAGS=['g++','-std=c++14','-Wall','-Wextra','-Werror','-Wno-misleading-indentation']
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Pressure(unittest.TestCase):
 def test_ascii(self):
  for h in ('PressureDrop.h','PressureSheet.h','AshraeEngine.h'):(ROOT/'geometry'/h).read_bytes().decode('ascii')
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(FLAGS+[str(ROOT/'tests/test_pressure_drop.cpp'),'-o',str(exe)],check=True)
   args=[str(exe)]
   if os.environ.get('TVS_ASHRAE_DATA'):args+=[os.environ['TVS_ASHRAE_DATA'],str(pathlib.Path(d)/'dump.txt')]
   self.assertIn('OK',subprocess.run(args,check=True,capture_output=True,text=True).stdout)
 def test_flow_ports(self):   # FlowNetwork: entry / via / order for the dionice
  f=text('geometry/FlowNetwork.h');self.assertIn('r.via[j]=static_cast<int>(p);',f);self.assertIn('r.order=order;',f)
 def test_command(self):
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/PressureCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  c=text('TVS_Ventilation_ARX/PressureCommand.h')
  self.assertIn('tvsFlowNetwork::solve(el,root,port,tol)',c);self.assertIn('tvsExcel::write(tvsSheet::build(res,info,o))',c)
  self.assertIn('L"AshraeData"',c);self.assertIn('[Dovod/Odvod/Podaci]',c);self.assertNotIn('put_',c)   # nothing written into the drawing
  self.assertIn('TVS_PAD, TVS_PAD',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'));self.assertIn('#include "PressureCommand.h"',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('tvsPad::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('PAD',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_PAD_%d.png'%s)).exists())
 def test_excel_writer(self):
  x=text('TVS_Ventilation_ARX/ExcelWriter.h')
  self.assertIn('L"Formula",arr',x);self.assertIn('L"FormatConditions"',x);self.assertIn('L"AddLine"',x);self.assertIn('CoUninitialize',x);self.assertIn('Invoke(id,IID_NULL,0x0409,',x);self.assertNotIn('LOCALE_USER_DEFAULT',x)
  self.assertLess(x.index('every sheet named first'),x.index('cells(d,s);'))
  self.assertLess(x.index('every Var released before CoUninitialize'),x.index('app->Release();'))
if __name__=='__main__':unittest.main()

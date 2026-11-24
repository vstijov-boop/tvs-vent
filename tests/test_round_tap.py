"""Stucna sa okruglim ogrankom: valjkasta (SR5-11) i konusna (SR5-12) - oblik, prompt, proracun."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class RoundTap(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(ROOT/'tests/test_round_tap.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_wiring(self):
  t=text('TVS_Ventilation_ARX/TeeCommand.h')
  self.assertIn('[Stucna/Konusna/Ravni/oKreni]',t);self.assertIn('[Stucna/Konusna/Ravni]',t);self.assertIn('L"SR5-12")==0)wcscpy_s(kw,n,L"Konusna")',t)
  self.assertIn('tapRecord(r,w1,round,second<0,f)',t)
  for f in ('TVS_Ventilation_ARX/FlowCommand.h','TVS_Ventilation_ARX/SizeFollow.h','TVS_Ventilation_ARX/TeeTypeDialog.h'):
   self.assertIn('tvsTeeCommand::tapRecord(',text(f),f)
  self.assertIn('tee.cone = ThisRoundotv && tvsTee::isCone(r.second, SizeAotv, r.l)',text('TVS_Ventilation_DBX/TVS_WYE.cpp'))
  self.assertIn('Konusna štucna SR5-12',text('TVS_Ventilation_ARX/SPEC.cpp'))
  self.assertIn('tvsTee::isCone(rec.second,p->SizeAotv,rec.l)',text('TVS_Ventilation_ARX/PressureCommand.h'))
if __name__=='__main__':unittest.main()

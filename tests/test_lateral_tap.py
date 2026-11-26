"""Kosa stucna 45 okrugli na okrugli (plan 8, korisnik 26.11.): geometrija + povezivanje."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Lateral(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_lateral_tap.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_wiring(self):
  w=text('TVS_Ventilation_DBX/TVS_WYE.cpp');self.assertIn('tee.lateral = r.neck;',w)
  c=text('TVS_Ventilation_ARX/commands/ConnectHelpers.inl');self.assertIn('return ConnectLateral(pPipe1, pPipe2);',c);self.assertIn('tvsTeeFit::lateralRecord(r, A, w, f);',c);self.assertIn('return ConnectLateral(pPipe1, pPipe2, true);',c);self.assertIn('tvsTeeDialog::planElbow(wyeId, face, d, turn, sec,',c)
  dh=text('TVS_Ventilation_ARX/commands/DrawHelpers.inl');self.assertIn('return roundTeeDuringDrawing(A1, A2, pipi, pt1); }',dh);self.assertIn(': lateralDuringDrawing(A1, A2, pipi, pt1);',dh);self.assertIn('[Koljeno/Bez] <Koljeno>',dh)
  p=text('geometry/PressureDrop.h');self.assertIn('return exhaust?"ED5-2":"SD5-1";',p)
  self.assertIn('t.lateral=(rec.type==tvsTee::Tap||rec.type==tvsTee::Pants)&&rec.neck>0',text('TVS_Ventilation_ARX/PressureCommand.h'))
  for f in ('SizeFollow.h','FlowCommand.h','TeeEditDialog.h'):self.assertIn('tvsTeeFit::lateralLength(',text('TVS_Ventilation_ARX/'+f))
  self.assertIn('Kosa račva TK 45° (Soko)',text('TVS_Ventilation_ARX/SPEC.cpp'))
if __name__=='__main__':unittest.main()

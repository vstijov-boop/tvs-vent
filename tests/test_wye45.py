"""Racva Y45 okrugla (Soko str. 36, korisnik 26.11.): geometrija, tabela + povezivanje."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Wye45(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_wye45.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_cxx98(self):
  subprocess.run(['g++','-std=c++98','-fsyntax-only','-x','c++','-'],input='#include "geometry/TeeShape.h"\n#include "geometry/SokoPants.h"\nint main(){return 0;}\n',text=True,cwd=ROOT,check=True)
 def test_ascii(self):
  for f in ('geometry/TeeShape.h','geometry/SokoPants.h'):(ROOT/f).read_bytes().decode('ascii')
 def test_wiring(self):
  w=text('TVS_Ventilation_DBX/TVS_WYE.cpp');self.assertIn('else if (r.neck > 0 && ThisRoundpr && ThisRoundotv) {   // gacasta racva Y45',w);self.assertIn('tvsTee::wyePorts(tee, lp, ld);',w)
  n=text('TVS_Ventilation_ARX/TeeNeighbours.h');self.assertIn('inline bool wyeRecord(',n);self.assertIn('if(isWye(w,r)){',n)
  dh=text('TVS_Ventilation_ARX/commands/DrawHelpers.inl');self.assertIn('static bool wyeDuringDrawing(',dh);self.assertIn('[Kosa/Y45]',dh);self.assertIn('[Okomito/Paralelno/Bez] <Bez>',dh)
  self.assertIn('rec.type==tvsTee::Pants)&&rec.neck>0',text('TVS_Ventilation_ARX/PressureCommand.h'))
  self.assertIn('if(t.lateral)in=seedJunction(',text('geometry/PressureDrop.h'))
  self.assertIn('Račva Y45 (Soko), okrugla',text('TVS_Ventilation_ARX/SPEC.cpp'))
  t=text('TVS_Ventilation_ARX/TeeTypeDialog.h');self.assertIn('tvsTeeFit::isWye(w.object(),r))return false;',t)
if __name__=='__main__':unittest.main()

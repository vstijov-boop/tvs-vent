"""Meni u gripu slobodnog prikljucka fitinga (plan 19 faza 2, korisnik 26.11.)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class PortGripMenu(unittest.TestCase):
 def test_menu(self):
  d=text('TVS_Ventilation_ARX/commands/Draw.inl')
  self.assertIn('static int portGripMenu(bool duct = false, bool round = false)',d);self.assertIn('const int choice = portGripMenu();',d)
  for t in ('Nastavi crtanje','Nastavi drugom dimenzijom (prelaz)...','Umetni koljeno','Odustani'):self.assertIn(t,d)
  self.assertIn('if (choice == 2) dimension();',d);self.assertIn('elbow(choice % 1000 - 100, choice >= 1000);',d)
  self.assertIn('TPM_RETURNCMD',d)
  for x in ('Spoji sa kanalom koljenom...','Spoji sa kanalom račvom...','Promjena visine','Koljeno nadolje','Račva Y45','TK-G gaćasti'):self.assertIn(x,d)
  self.assertIn('if (!fromMenu) acedGetInput(resultss);',d);self.assertIn('pendingStep3().clear(); pendingTee().clear();',d)
  h=text('TVS_Ventilation_ARX/commands/DrawHelpers.inl');self.assertIn('takePendingTee(L"Kosa", L"Y45", NULL, kw, 32)',h);self.assertIn('takePendingTee(L"Ogranak", L"Gacasti", L"Stucna", kw, 32)',h)
  self.assertIn('const int m = ductEndMenu(id, index == 101);',d);self.assertIn('drawFromEnd(id, at, dir, sec, elev, db, choice);',d)
 def test_wye_directions(self):
  self.assertIn('tvsTee::wyePorts(t,lp,ld);',text('TVS_Ventilation_DBX/PortGrips.h'))
  self.assertIn('tvsTee::wyePorts(t,lp,ld);out.out=',text('TVS_Ventilation_ARX/TeePort.h'))
if __name__=='__main__':unittest.main()

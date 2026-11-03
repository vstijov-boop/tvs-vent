"""T komad TK-O / TK-G wiring (source contracts): record, WYE draw/frame, sheet area,
specification, Properties name, KGTEZ native area and the TVS_TK_TIP command."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig',errors='replace')
class TeeContract(unittest.TestCase):
 def test_contracts(self):
  d=text('TVS_Ventilation_DBX/TeeData.h');self.assertIn('L"TVS_TEE_V1"',d);self.assertIn('const short types[]={1,70,40,40,40};',d)
  w=text('TVS_Ventilation_DBX/TVS_WYE.cpp')
  self.assertIn('tee.R1 = LengthPl - r.l + tee.w1;',w,'R1 follows LengthPl, so ports and drawing agree')
  self.assertIn('origin = Basepoint - u * (LengthPl + tee.w1 / 2); ey = u; ex = v;',w,'TK-O: inlet at the run start')
  self.assertIn('origin = Basepoint + v * (SizeApr / 2 + LengthPl); ey = -v; ex = u;',w,'TK-G: inlet = TVS branch end')
  self.assertIn('if (!This1D && teeShape(tee, o, ex, ey)) {',w)
  e=text('TVS_Ventilation_DBX/TVS_Entity.cpp');self.assertIn('areaM2 = tee.type == tvsTee::Branch ? tvsTee::area(mm, b, b2) : tvsTee::area(mm, b2, b);',e)
  s=text('TVS_Ventilation_ARX/SPEC.cpp');self.assertIn('#define TeeBranch 8',s);self.assertIn('parameters1[6]=param1.Extra;',s);self.assertIn('while (i<7)',s)
  self.assertIn('L"T komad sa ogrankom TK-O"',s);self.assertIn('L"Gaćasti T komad TK-G"',s)
  c=text('TVS_Ventilation_DBX/ComWye.cpp');self.assertIn('L"T komad sa ogrankom (TK-O)"',c)
  k=(ROOT/'Installer/TVS_TOOLS/KGTEZ.lsp').read_bytes().decode('latin-1')
  i=k.index('((= on "TVS_WYE")');self.assertIn("'SheetArea",k[i:i+1500],'KGTEZ uses the TVS sheet area for T komadi')
  t=text('TVS_Ventilation_ARX/TeeCommand.h');self.assertIn('w->LengthPl=tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1);',t)
  # Svojstva dialog: extra width (TK-O prolaz C, TK-G lijevi ogranak) and L follows the branch width.
  f=text('TVS_Ventilation_ARX/Func.cpp');self.assertEqual(f.count('teeGather.add(Wyeie);'),2);self.assertEqual(f.count('tvsTeeEdit::prepare(dg, teeGather);'),2)
  self.assertEqual(f.count('tvsTeeEdit::apply(Wyeie, teeGather, dg, cSizeA == false, cSizeA2 == false, cLengthW == false);'),2)
  r=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc');self.assertIn('EDITTEXT        1050,187,86,40,12',r);self.assertIn('"Prolaz C",10050',r)
  g=text('TVS_Ventilation_ARX/dgAllEdit.cpp');self.assertIn('DDX_MillimetreString(pDX, IDC_TeeSecond, TeeSecond);',g);self.assertIn('L"Lijevi ogr."',g)
  a=text('TVS_Ventilation_ARX/TeeEditDialog.h');self.assertIn('if(r.type==tvsTee::Pants)r.radius2=tvsTee::defaultRadius(v);',a)
  self.assertIn('w->put_Length(tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1));',a)
  self.assertIn('Ventilation_ARX, TVS_TK_TIP, TVS_TK_TIP, ACRX_CMD_MODAL',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
if __name__=='__main__':unittest.main()

"""Faza 5: decimalni zarez u starim dijalozima (DecimalText.h), Svojstva: Odustani i zakljucan sloj."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Decimal(unittest.TestCase):
 def test_header(self):
  self.assertEqual((ROOT/'TVS_Ventilation_ARX/DecimalText.h').read_bytes()[:3],b'\xef\xbb\xbf')   # poruka sa dijakritikom
  h=text('TVS_Ventilation_ARX/DecimalText.h')
  self.assertIn("s.Replace(L',',L'.')",h);self.assertIn('dx->Fail();',h)
 def test_no_wtof(self):
  for f in ('dgPipeSize.cpp','dgAllEdit.cpp','dgMLeadProperty.cpp'):
   s=re.sub(r'(?m)^\s*//.*$','',text('TVS_Ventilation_ARX/'+f))
   self.assertNotIn('_wtof(',s,f)
 def test_ddx(self):
  p=text('TVS_Ventilation_ARX/dgPipeSize.cpp')
  self.assertNotRegex(p,r'(?m)^\s*DDX_Text\(pDX, IDC_(Flow|Speed|i|TapRadiusVariableParameter|SpiroC),')
  a=text('TVS_Ventilation_ARX/dgAllEdit.cpp')
  for x in ('Flow','Swectangle','TapRadiusVariableParameter'):self.assertIn('tvsDecimal::DDX_Number(pDX, IDC_%s, %s);'%(x,x),a)
 def test_svojstva(self):
  f=text('TVS_Ventilation_ARX/Func.cpp')
  self.assertIn('if (dg.DoModal() != IDOK) return;   // Odustani',f)
  self.assertIn('Nije promijenjeno %d elemenata (zaklju',f)
if __name__=='__main__':unittest.main()

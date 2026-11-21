"""Prijedlog 4 (REVIEW_2026-11b): numericka polja dijaloga kroz jedan sloj (geometry/NumericField.h + DecimalText.h)."""
import pathlib,re,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
ARX=ROOT/'TVS_Ventilation_ARX'
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
def code(p):return re.sub(r'(?m)//.*$','',text(p))   # bez komentara
class NumericField(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_numeric_field.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_header_pure(self):
  h=(ROOT/'geometry/NumericField.h').read_bytes().decode('ascii')   # bez BOM, kompajlira se i van MSVC
  for bad in ('CString','afx','acad','AcDb','std::min','std::max'):self.assertNotIn(bad,h)
  self.assertNotRegex(h,r'\b(near|far|small|min|max)\b')
 def test_wrapper(self):
  d=text('TVS_Ventilation_ARX/DecimalText.h')
  self.assertEqual((ARX/'DecimalText.h').read_bytes()[:3],b'\xef\xbb\xbf')
  self.assertIn('#include "../geometry/NumericField.h"',d)
  self.assertIn('return tvsNumeric::parse(text.GetString(),v);',d)                 # jedan parser
  for f in ('DDX_Scaled','DDX_Millimetres','DDX_Degrees','DDX_Plain','differs'):self.assertIn('inline ',d);self.assertIn(f+'(',d)
  self.assertIn('tvsNumeric::edited(stored.GetString(),typed.GetString(),scale,digits,v)',d)
 def test_svojstva_no_string_compare(self):
  f=code('TVS_Ventilation_ARX/Func.cpp')
  for name in ('double','int'):
   i=f.index('void func::sequal(%s &var,'%name);body=f[i:f.index('\n}',i)]
   self.assertNotIn('Svar1 == Svar2',body);self.assertNotIn('atoi',body);self.assertNotIn('atof',body)
   self.assertIn('tvsDecimal::differs(Svar1, Svar2, after)',body)
  i=f.index('void func::strfil(double &myvar,');body=f[i:f.index('\n}',i)]
  self.assertNotIn('Format(',body);self.assertIn('tvsDecimal::exact(myvar)',body)
  a=code('TVS_Ventilation_ARX/dgAllEdit.cpp')
  self.assertNotIn('SwectangleShown',a);self.assertNotIn('SwectangleRadians',a);self.assertNotIn('normalized(',a)
  self.assertNotIn('.Format(L"%.15g"',a);self.assertNotIn('M_PI',a)                 # ugao i mm samo kroz sloj
  self.assertIn('tvsDecimal::DDX_Millimetres(dx, id, value, tvsUnits::factor(), positive);',a)
  for x in ('SizeA','SizeA2'):self.assertIn('DDX_MillimetreString(pDX, IDC_%s, %s, true);'%(x,x),a)   # S-2: dimenzija > 0
  self.assertNotIn('Swectangle',text('TVS_Ventilation_ARX/dgAllEdit.h').split('CString Swectangle;')[1].split('\n')[1])
  t=code('TVS_Ventilation_ARX/TeeEditDialog.h')
  self.assertNotIn('dg.TeeSecond!=',t);self.assertNotIn('wcstod',t)
  self.assertIn('tvsNumeric::differs(g.text().GetString(),dg.TeeSecond.GetString(),v)',t)
 def test_other_dialogs_one_parser(self):
  for f in ('ElevRefSwitch.h','dgChangeZ.cpp','SectionDialog.h','TeeEditDialog.h'):
   s=code('TVS_Ventilation_ARX/'+f)
   self.assertNotIn('wcstod',s,f);self.assertNotIn('_wtof',s,f);self.assertRegex(s,r'tvsNumeric::(parse|differs)\(',f)
 def test_encoding(self):
  for f,bom in (('DecimalText.h',1),('dgAllEdit.cpp',1),('Func.cpp',1),('dgChangeZ.cpp',1),('SectionDialog.h',1),('dgAllEdit.h',0),('TeeEditDialog.h',0),('ElevRefSwitch.h',0)):
   b=(ARX/f).read_bytes();self.assertEqual(b[:3]==b'\xef\xbb\xbf',bool(bom),f);self.assertNotIn(b'\r\n',b,f)
   if not bom:b.decode('ascii')
if __name__=='__main__':unittest.main()

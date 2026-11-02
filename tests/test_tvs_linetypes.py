"""ARX creates missing tvs_hidden/tvs_centerline with the TVS_LineType.lin patterns."""
import pathlib,re,unittest
root=pathlib.Path(__file__).resolve().parents[1]
class Linetypes(unittest.TestCase):
 def test_patterns_and_hooks(self):
  h=(root/'TVS_Ventilation_ARX/TvsLinetypes.h').read_text()
  lin=(root/'Installer/Resources/TVS_LineType.lin').read_bytes().decode('latin-1')
  for name in ('tvs_hidden','tvs_centerline'):
   pattern=re.search(r'\*'+name+r',[^\n]*\n\s*A,\s*([-\d., ]+)',lin).group(1)
   dashes=[float(x) for x in pattern.replace(' ','').strip(',').split(',')]
   m=re.search(r'\{L"'+name+r'",L"[^"]*",(\d),\{([-\d.,]+)\}\}',h)
   self.assertIsNotNone(m,name);values=[float(x) for x in m.group(2).split(',')][:int(m.group(1))]
   self.assertEqual(values,dashes,name)
  self.assertIn('if(table->has(p.name))continue;',h,'existing linetypes are never replaced')
  src=(root/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_text(encoding='utf-8',errors='replace')
  self.assertEqual(src.count('tvsLinetypes::ensureCurrent();'),2,'on ARX load and on each drawing load')
if __name__=='__main__':unittest.main()

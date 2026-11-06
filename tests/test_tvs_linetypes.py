"""ARX creates missing tvs_hidden/tvs_centerline with the TVS_LineType.lin patterns."""
import pathlib,re,unittest
from entry_source import entry_source
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
  src=entry_source()
  self.assertEqual(src.count('tvsLinetypes::ensureCurrent();'),2,'on ARX load and on each drawing load')
  dbx=root/'TVS_Ventilation_DBX'
  entity=(dbx/'TVS_Entity.cpp').read_text(encoding='utf-8',errors='replace')
  self.assertIn('pEnt->setLinetypeScale(this->linetypeScale() / tvsUnits::factor(pDb));',entity,'mm pattern: element scale / drawing units')
  self.assertNotIn('/factor',h,'ARX-created records stay in mm like the .lin file')
  for f in ('TVS_Entity.cpp','TVS_WYE.cpp','TVS_TRANS.cpp','TVS_TAP.cpp','TVS_Pipe.cpp'):
   live=[l for l in (dbx/f).read_text(encoding='utf-8',errors='replace').splitlines() if not l.strip().startswith('//')]
   for l in live:
    if 'tvs_centerline' in l or 'tvs_hidden' in l:self.assertIn('setAuxLinetype(',l,f+': '+l.strip())
if __name__=='__main__':unittest.main()

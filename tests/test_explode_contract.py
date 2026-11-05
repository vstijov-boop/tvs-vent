"""TVS_ERASE (explode): every TVS entity explodes into what it draws (ListOfEntity), so TK-O,
TK-G, stucna and side-flush prelazi keep their shape in drawings sent without TVS_VENT."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class ExplodeContract(unittest.TestCase):
 def test_explode_uses_drawn_geometry(self):
  for name in ('TVS_Pipe','TVS_TAP','TVS_WYE','TVS_TRANS'):
   s=(ROOT/'TVS_Ventilation_DBX'/(name+'.cpp')).read_text(encoding='utf-8-sig',errors='replace')
   m=re.search(name+r'::subExplode\(AcDbVoidPtrArray & entitySet\) const\s*\{(.*?)\n\}',s,re.S)
   self.assertTrue(m,name);body=m.group(1)
   self.assertIn('ListOfEntity',body,name);self.assertIn('->clone()',body,name)
if __name__=='__main__':unittest.main()

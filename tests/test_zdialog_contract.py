"""Plan 5: prosireni dijalog promjene kote (dgChangeZ): postojece/novo stanje, ugao, rezultat, skica."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class ZDialog(unittest.TestCase):
 def test_resource(self):
  rc=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc');m=re.search(r'IDD_HANGEZDG DIALOGEX(.*?)\nEND',rc,re.S);self.assertTrue(m)
  for c in ('IDC_ZOldTop','IDC_StartZ','IDC_ZOldBottom','IDC_ZRefTop','IDC_ZRefAxis','IDC_ZRefBottom','IDC_NextZ','IDC_Axis','IDC_ZResult','IDC_ZSketch'):self.assertIn(c,m.group(1))
  h=text('TVS_Ventilation_ARX/resource.h')
  for c in ('IDC_ZOldTop','IDC_ZSketch'):self.assertRegex(h,r'#define %s\s+\d+'%c)
 def test_dialog(self):
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/dgChangeZ.cpp').read_bytes().startswith(b'\xef\xbb\xbf'),'non-ASCII text needs the BOM')
  s=text('TVS_Ventilation_ARX/dgChangeZ.cpp')
  self.assertIn('axis = v / tvsUnits::factor() - (ref == 1 ? h : ref == 2 ? -h : 0);',s)   # Vrh/Dno -> axis
  self.assertIn('if (evaluate && !feasible)',s)
  o=text('TVS_Ventilation_ARX/OffsetIntegrationCommand.h')
  self.assertIn('dg.height=snapshot.physical.b;',o);self.assertIn('tvsOffsetOperation::prepare(snapshot.physical,q)',o)
if __name__=='__main__':unittest.main()

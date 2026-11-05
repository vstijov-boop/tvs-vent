"""Prelaz/kanal dimenzije na spoju (SizeFollow.h, TVS_DIM_PRATI) i izvoz bez TVS (ExportCommand.h, TVS_IZVOZ)."""
import pathlib,unittest,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class FollowExport(unittest.TestCase):
 def test_wiring(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  for c in ('TVS_DIM_PRATI','TVS_IZVOZ'):self.assertIn('Ventilation_ARX, %s, %s, ACRX_CMD_MODAL'%(c,c),e)
  self.assertIn('tvsSizeFollow::start();',e);self.assertIn('tvsSizeFollow::stop();',e)
  self.assertIn('tvsSizeFollow::attach(acdbHostApplicationServices()->workingDatabase());',e)
 def test_follow_rules(self):
  s=text('TVS_Ventilation_ARX/SizeFollow.h')
  self.assertIn('!c.CompareNoCase(L"TVS_CHANGE")',s)          # only user edits (Svojstva, Properties)
  self.assertIn('if(applying()||o->isUndoing()||!userEdit())return;',s)
  self.assertIn('tvsTransAlignCmd::afterEdit(t.object(),was,-1,-1);',s)
  self.assertEqual(s.count('{'),s.count('}'))
 def test_export(self):
  s=text('TVS_Ventilation_ARX/ExportCommand.h')
  self.assertIn('db->wblock(raw)',s);self.assertIn('o->explode(parts)',s);self.assertIn('copy->saveAs(path)',s)
  self.assertNotIn('acdbEntDel',s)                            # the open drawing is never changed
  with zipfile.ZipFile(ROOT/'Installer/TVS_VENT/TVS_VENT.cuix') as z:
   self.assertIn('^C^C_TVS_IZVOZ;',z.read('MenuGroup.cui').decode('utf-8'))
if __name__=='__main__':unittest.main()

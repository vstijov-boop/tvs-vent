"""Plan 22 (2026-11-28): TK-O / TK-G imaju jednu visinu - visinu ulaza, i u Svojstvima i u paleti."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Heights(unittest.TestCase):
 def test_rule(self):
  t=text('TVS_Ventilation_ARX/TeeEditDialog.h')
  self.assertIn('if(r.type==tvsTee::Branch&&std::fabs(w->SizeBotv-w->SizeBpr)>1e-9){w->put_SizeBotv(w->SizeBpr);return true;}',t)
  self.assertIn('if(r.type==tvsTee::Pants&&std::fabs(w->SizeBpr-w->SizeBotv)>1e-9){w->put_SizeBpr(w->SizeBotv);return true;}',t)
  self.assertIn('if(w->ThisRoundpr||w->ThisRoundotv)return false;',t)
 def test_properties_dialog(self):
  f=text('TVS_Ventilation_ARX/Func.cpp')
  self.assertEqual(f.count('tvsTeeEdit::keepHeights(Wyeie)) ++heightsKept;'),2)
  for site in f.split('tvsTeeEdit::keepHeights(Wyeie)')[:2]:self.assertIn('put_SizeBotv(SizeB2);',site[-400:])   # after the generic fields
  d=text('TVS_Ventilation_ARX/dgAllEdit.cpp')
  self.assertIn('fSizeB2.SetReadOnly(TRUE);',d);self.assertIn('fSizeB.SetReadOnly(TRUE);',d)
  self.assertIn(', TeeHeightFree(true)',d);self.assertIn('TeeHeightFree = !rect;',d)
 def test_palette(self):
  c=text('TVS_Ventilation_DBX/ComWye.cpp')
  self.assertIn('if (teeType == tvsTee::Branch) { pSq->put_SizeBotv(newVal);',c)
  self.assertIn('if (teeType == tvsTee::Pants) { pSq->put_SizeBpr(newVal);',c)
if __name__=='__main__':unittest.main()

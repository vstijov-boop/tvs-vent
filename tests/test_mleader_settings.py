"""TVS label settings: default pipe format, persisted settings, style/scale wiring (source contracts)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig',errors='replace')
class MLeaderSettings(unittest.TestCase):
 def test_contracts(self):
  m=text('TVS_Ventilation_ARX/MLeaderSettings.h')
  self.assertIn(r'pipeLeader = L"[D]mm\\P[L]m\\U+00B3/h";',m,'default shown as [D]mm\\P[L]m\\U+00B3/h')
  self.assertIn('L"Software\\\\TVS_Vent\\\\MLeader"',m)
  for v in ('PipeFormat','BlockFormat','Style','StyleSource','Scale'):self.assertIn('L"%s"'%v,m)
  self.assertIn('style = L"TVS_MleaderStyle";',m)
  t=text('TVS_Ventilation_ARX/MLeaderStyleTools.h')
  self.assertIn('acedFindFile(L"TVS_Oznake.dwg",found)',t);self.assertIn('TVS.bundle\\\\Contents\\\\Resources\\\\TVS_Oznake.dwg',t)
  self.assertIn('const bool standard=current.isNull()||current==find(db,L"Standard");',t,'never overrides a user-chosen current style')
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp');self.assertEqual(e.count('tvsMLeaderStyle::ensureCurrentDocument('),2)
  l=text('TVS_Ventilation_ARX/TVSLeadCommand.cpp')
  self.assertIn('pEnt->setMLeaderStyle(styleId);',l)
  self.assertIn('const double scale = m.scale > 0 ? m.scale / tvsUnits::factor(db) : 1;',l,'paper mm x scale / units')
  self.assertIn('mText->setTextHeight(style->textHeight() * scale);',l,'absolute MText height = style height x overall scale')
  self.assertLess(l.index('const double scale ='),l.index('mText->setTextHeight(style->textHeight() * scale);'))
  self.assertLess(l.index('pEnt->setMLeaderStyle(styleId);'),l.index('pEnt->addLeader(i);'))
  d=text('TVS_Ventilation_ARX/dgMLeadProperty.cpp')
  self.assertIn('tvsMLeaderStyle::import(',d);self.assertIn('m.save();',d)
  r=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc')
  for c in ('COMBOBOX        1201','PUSHBUTTON      "Iz crteža...",1202','EDITTEXT        1203'):self.assertIn(c,r)
if __name__=='__main__':unittest.main()

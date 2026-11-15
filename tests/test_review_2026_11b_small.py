"""Pregled 2026-11b, tacka 1 plana: N3, N4, N7, N8, N10, S-11 (provjere u izvoru)."""
import pathlib,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Small(unittest.TestCase):
 def test_n3_units(self):
  s=text('TVS_Ventilation_ARX/commands/NewCommands.inl')
  for call in ('tvsSizeFollow::run(IDD_RUN, IDD_TEETYPE);','tvsFlexCuffCommand::run();','tvsExport::run();'):
   i=s.index(call);self.assertIn('syncTVSGlobalUnits();',s[i-120:i],call)
 def test_n4_per_drawing(self):
  s=text('TVS_Ventilation_ARX/OffsetRefit.h')
  self.assertIn('inline void DbReactor::goodbye(const AcDbDatabase* db){forget(db);',s)
  self.assertIn('if(posted()&&nowMs()-postedAt()<5000)return;',s)
  self.assertIn('(pending()[i].database()==db?work:other)',s)
 def test_n7_read_only(self):
  s=text('TVS_Ventilation_ARX/SPEC.cpp');g=s[s.index('bool SPEC::GetAtt'):s.index('bool SPEC::addBlock')]
  self.assertIn('kForRead',g);self.assertNotIn('kForWrite',g)
  self.assertLess(g.index('wcscpy_s(pVal'),g.index('pAttr->close();'))
 def test_n8_default_applies(self):
  s=text('TVS_Ventilation_ARX/OffsetIntegrationCommand.h')
  self.assertIn('[Primijeni/Otkazi] <Primijeni>',s);self.assertNotIn('<Otkazi>',s);self.assertNotIn('rise=%',s)
 def test_n10_flex_length(self):
  self.assertIn('Length=tvsUnits::mm(pTap->Radius + pTap->SizeA / 2, pTap)*pTap->Swectangle/1000;',text('TVS_Ventilation_ARX/SPEC.cpp'))
  self.assertIn('count = tvsUnits::metres((Radius + SizeA / 2)*Swectangle, this);',text('TVS_Ventilation_DBX/TVS_TAP.cpp'))
 def test_s11_numbers(self):
  s=text('TVS_Ventilation_ARX/MSExcel.cpp');self.assertIn('static bool plainNumber(LPCTSTR t, double& out)',s);self.assertIn('COleVariant oleValue(number);',s)
if __name__=='__main__':unittest.main()

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
  self.assertLess(g.index('wcsncpy_s(pVal'),g.index('pAttr->close();'))
 def test_n8_default_applies(self):
  s=text('TVS_Ventilation_ARX/OffsetIntegrationCommand.h')
  self.assertIn('[Primijeni/Otkazi] <Primijeni>',s);self.assertNotIn('<Otkazi>',s);self.assertNotIn('rise=%',s)
 def test_n10_flex_length(self):
  self.assertIn('Length=tvsUnits::mm(pTap->Radius + pTap->SizeA / 2, pTap)*pTap->Swectangle/1000;',text('TVS_Ventilation_ARX/SPEC.cpp'))
  self.assertIn('count = tvsUnits::metres((Radius + SizeA / 2)*Swectangle, this);',text('TVS_Ventilation_DBX/TVS_TAP.cpp'))
 def test_s11_numbers(self):
  s=text('TVS_Ventilation_ARX/MSExcel.cpp');self.assertIn('static bool plainNumber(LPCTSTR t, double& out)',s);self.assertIn('COleVariant oleValue(number);',s)
class Leftovers(unittest.TestCase):
 def test_k5_truncate(self):
  s=text('TVS_Ventilation_ARX/SPEC.cpp');self.assertNotIn('wcscpy_s(pVal,512',s);self.assertNotIn('wcscat_s(sTypeSize',s)
  self.assertIn('wcsncat_s(achText, pText, _TRUNCATE);',text('TVS_Ventilation_ARX/MSExcel.cpp'))
 def test_v12(self):
  s=text('TVS_Ventilation_ARX/SPEC.cpp');self.assertNotIn('%s", LPCTSTR(',s)
  seg=s[s.index('bool SPEC::setAtribToEnt'):s.index('bool SPEC::setAtribListToEnt')];self.assertEqual(seg.count('delete pAttrIter;'),2);self.assertIn('return false;',seg)
  m=text('TVS_Ventilation_ARX/commands/Misc.inl');self.assertNotIn('pBlkRef->close();',m);self.assertIn('SPEC::getBlockName(idBlkRef, pName)',m)
class OffsetProofSplit(unittest.TestCase):
 def test_helpers_out_of_test_header(self):
  b=text('TVS_Ventilation_ARX/BuildTransaction.h');t=text('TVS_Ventilation_ARX/TVSTestOffset.h')
  for d in ('inline AcGePoint3d pt(','inline AcDbObjectId modelId(','class BuildTransaction','inline void initialize('):
   self.assertIn(d,b);self.assertNotIn(d,t)
  for h in ('OffsetService.h','SizeFollow.h','OffsetRefit.h','BypassService.h','OffsetIntegrationCommand.h'):
   self.assertIn('#include "BuildTransaction.h"',text('TVS_Ventilation_ARX/'+h),h)
 def test_test_offset_only_in_dev(self):
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp');i=e.index('#include "TVSTestOffset.h"')
  self.assertIn('#if defined(TVS_OFFSET_PROOF)&&defined(TVS_DEV)',e[i-60:i])
  n=text('TVS_Ventilation_ARX/commands/NewCommands.inl');i=n.index('tvsOffsetProof::create()')
  self.assertIn('#if defined(TVS_OFFSET_PROOF)&&defined(TVS_DEV)',n[:i]);self.assertNotIn('#ifdef TVS_OFFSET_PROOF',n)
 def test_release_without_proof(self):
  for y in ('2020','2022'):
   w=text('.github/workflows/build-autocad-%s.yml'%y);self.assertNotIn('TVSOffsetProof',w);self.assertNotIn('TVSDev',w)
   v=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX%s.vcxproj'%y)
   self.assertIn("Condition=\"'$(TVSDev)'=='true'\"",v);self.assertIn('<PreprocessorDefinitions>TVS_OFFSET_INTEGRATION;%(',v)
  self.assertIn('/p:TVSDev=true',text('.github/workflows/dev-tvs-dev.yml'))
class K4Connect(unittest.TestCase):
 def test_connect_by_id(self):
  c=text('TVS_Ventilation_ARX/commands/Connect.inl')
  self.assertIn('static bool selectTwoPipes(',c);self.assertEqual(c.count('selectTwoPipes(id1, id2, pt1, pt2)'),2)
  self.assertNotIn('ConnectWithWye(pEnt1, pEnt2, pt1, pt2);\n\t}\n\n\tstatic void Ventilation_ARXTVS_CONNECTT(void)\n\t{\n\t\tif (ActivationErrorMessage() != pError_Ok) return;\n\t\tsyncTVSGlobalUnits();\n\t\tif (!currentLayerWritable()) return;\n\t\t// Add',c)
  self.assertEqual(c.count('{'),c.count('}'))
  h=text('TVS_Ventilation_ARX/commands/ConnectHelpers.inl')
  self.assertEqual(h.count('// K4 / reopen: both ducts writable before anything is drawn'),2)
  s=text('TVS_Ventilation_ARX/commands/Specification.inl')
  self.assertNotIn('pEnt->close();\n\t\t\t\t\t\tif ((br = AcDbBlockReference::cast(pEnt))',s)
  self.assertEqual(s.count('pEnt->close();   // K4: after addBlock'),2)
if __name__=='__main__':unittest.main()

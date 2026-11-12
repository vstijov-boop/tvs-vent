"""Izdanje: razvojne / test komande registrovane samo uz TVS_DEV; komande koje mijenjaju crtez nisu transparentne."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
DEV=('TVS_HEATFLOOR','TVS_TEST','TVS_TRANS','TVS_WYE','TVS_CONNECTTest','TVS_DUCT','TVS_SpecificationOld','TVS_Flex','TVS_Show','TVSSomeParts',
     'MyCommand','MyPickFirst','MySessionCmd','TVS_OFFSET_TEST_BAD','TVS_OFFSET_FAIL_NEXT','TVS_TEST_OFFSET','TVS_BYPASS_DETAIL_TEST','TVS_BYPASS_DETAIL_CHECK',
     'TVS_BYPASS_PREVIEW','TVS_OFFSET_STATUS','TVS_OFFSET_POLICY')
def release(s):
    """Lines outside #ifdef TVS_DEV / #if ...defined(TVS_DEV) blocks (nesting counted)."""
    out=[];stack=[]
    for l in s.split('\n'):
        t=l.strip()
        if t.startswith('#if'):stack.append('TVS_DEV' in t);continue
        if t.startswith('#endif'):stack.pop() if stack else None;continue
        if not any(stack)and not t.startswith('//'):out.append(l)
    return '\n'.join(out)
class Release(unittest.TestCase):
 def test_dev_commands(self):
  r=release(text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  for c in DEV:
   self.assertNotRegex(r,r'ENTRY_AUTO\(CTVS_Ventilation_ARXApp, \w+, %s, '%c,c)
   self.assertNotIn('L"%s",L"%s"'%(c,c),r)
  self.assertNotIn('ACED_ADSSYMBOL_ENTRY_AUTO(CTVS_Ventilation_ARXApp, MyLispFunction',r)
  self.assertNotIn('virtual AcRx::AppRetCode On_(void *pkt)',r)
 def test_modal(self):
  r=release(text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  t=[m for m in re.findall(r'ENTRY_AUTO\(CTVS_Ventilation_ARXApp, \w+, (\w+), \w+, ([^,]+),',r) if 'TRANSPARENT' in m[1]]
  self.assertEqual([c for c,_ in t],['TVS_SETTINGS'])   # podesavanja i usred crtanja
 def test_menu_commands_registered(self):
  import zipfile
  r=release(text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')).upper()
  with zipfile.ZipFile(ROOT/'Installer/TVS_VENT/TVS_VENT.cuix') as z:
   m=z.read('MenuGroup.cui').decode('utf-8','ignore')
  for c in set(re.findall(r'\^C\^C_?(TVS_\w+)',m)):
   self.assertTrue(', %s, '%c in r or 'L"%s"'%c in r,c)
 def test_phase4(self):
  c=text('TVS_Ventilation_ARX/commands/ConnectHelpers.inl');d=text('TVS_Ventilation_ARX/commands/DrawHelpers.inl');e=text('TVS_Ventilation_ARX/commands/Edit.inl')
  for s in (c,d,e,text('TVS_Ventilation_ARX/commands/Connect.inl'),text('TVS_Ventilation_ARX/commands/Draw.inl')):
   self.assertNotRegex(s,r'(?m)^\s*acdbOpenAcDbEntity\(')          # svako otvaranje provjereno (reopen)
  self.assertIn('static bool reopen(AcDbObject* o)',c);self.assertIn('static bool currentLayerWritable()',c)
  self.assertNotIn('pipi->LastPoint = ',d+e)                         # kraj kanala samo kroz put_Lastpoint
  self.assertIn('const double stub = 100.0 / tvsUnits::factor();',d)
  self.assertIn('const double minLength = 50.0 / tvsUnits::factor();',e)   # TVS_CUT u mm
  self.assertIn('es = postToDatabase(eSet, owner);\n\t\t\tif (es == Acad::eOk) es = pEnt->erase();',e)   # TVS_ERASE: original ide tek kad su dijelovi u crtezu
  self.assertIn('if (tvsAlignCmd::inAssembly(id)) { ++assembly; continue; }',e)   # V7: clan etaza / obilaska se ne razbija sam
  self.assertIn('TVS_RAZBIJ, TVS_RAZBIJ',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
 def test_release_defines_and_units(self):
  for y in ('2020','2022'):self.assertNotIn('TVS_BYPASS_FAULT_INJECTION',text('TVS_Ventilation_ARX/TVS_Ventilation_ARX%s.vcxproj'%y))
  e=text('TVS_Ventilation_ARX/acrxEntryPoint.cpp')
  for f in ('tvsBypassCommandEntry','tvsBypassPreviewCommandEntry','tvsBypassExistingDetailEntry'):self.assertIn('static void %s(){syncTVSGlobalUnits();'%f,e)
  self.assertIn('Ventilation_ARXTVS_OFFSET(){syncTVSGlobalUnits();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
 def test_no_windows_macro_identifiers(self):
  # near / far / small are Windows macros (MSVC): not as names of parameters or variables
  import glob,re
  pat=re.compile(r'\b(double|int|bool|float|auto|const)\s+(near|far|small)\b')
  for f in glob.glob(str(ROOT/'TVS_Ventilation_ARX/*.h'))+glob.glob(str(ROOT/'TVS_Ventilation_ARX/commands/*.inl'))+glob.glob(str(ROOT/'TVS_Ventilation_DBX/*.h'))+glob.glob(str(ROOT/'geometry/*.h')):
   self.assertIsNone(pat.search(pathlib.Path(f).read_text(encoding='utf-8-sig',errors='ignore')),f)
 def test_no_broken_string_literals(self):
  # a raw newline inside L"..." (python escapes written into C++) does not compile
  import glob
  for f in glob.glob(str(ROOT/'TVS_Ventilation_ARX/**/*.[hc]*'),recursive=True)+glob.glob(str(ROOT/'TVS_Ventilation_ARX/commands/*.inl'))+glob.glob(str(ROOT/'TVS_Ventilation_DBX/*.[hc]*')):
   if f.endswith(('.rc','.rc2'))or'_i.'in f:continue
   for n,l in enumerate(pathlib.Path(f).read_text(encoding='utf-8-sig',errors='ignore').split('\n'),1):
    self.assertFalse(l.rstrip().endswith('L"') and not l.strip().startswith('//'),'%s:%d'%(f,n))
if __name__=='__main__':unittest.main()

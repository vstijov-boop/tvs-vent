"""Izdanje: razvojne / test komande registrovane samo uz TVS_DEV; komande koje mijenjaju crtez nisu transparentne."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
DEV=('TVS_TEST','TVS_TRANS','TVS_WYE','TVS_CONNECTTest','TVS_DUCT','TVS_SpecificationOld','TVS_Flex','TVS_Show','TVSSomeParts',
     'MyCommand','MyPickFirst','MySessionCmd','TVS_OFFSET_TEST_BAD','TVS_OFFSET_FAIL_NEXT','TVS_TEST_OFFSET','TVS_BYPASS_DETAIL_TEST','TVS_BYPASS_DETAIL_CHECK')
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
if __name__=='__main__':unittest.main()

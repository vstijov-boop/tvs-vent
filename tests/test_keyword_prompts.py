"""Opcije u [..] komandne linije su samo kljucne rijeci: AutoCAD ih nudi za klik / dinamicki unos,
pa "Ogranak (TK-O)" salje "TK-O" koje acedInitGet ne zna (TVS_CONNECTW / Racva: Invalid option keyword)."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Prompts(unittest.TestCase):
 def test_no_parentheses_in_options(self):
  bad=[]
  for p in list((ROOT/'TVS_Ventilation_ARX').rglob('*.h'))+list((ROOT/'TVS_Ventilation_ARX').rglob('*.inl'))+list((ROOT/'TVS_Ventilation_ARX').rglob('*.cpp')):
   for n,line in enumerate(p.read_text(encoding='utf-8-sig',errors='replace').splitlines(),1):
    if line.lstrip().startswith('//'):continue
    for s in re.findall(r'(?:L|_T\()"((?:[^"\\]|\\.)*)"',line):
     for opt in re.findall(r'\[([^\]]*)\]',s):
      if '/' in opt and any(c in opt for c in '( =')and'%'not in opt:bad.append('%s:%d %s'%(p.relative_to(ROOT),n,opt))
  self.assertEqual(bad,[])
 def test_tee_aliases(self):
  t=(ROOT/'TVS_Ventilation_ARX/TeeCommand.h').read_text(encoding='utf-8-sig')
  self.assertIn('TK-O TK-G SR5-3',t);self.assertEqual(t.count('typeAlias(kw,32)'),3)
  self.assertIn('typeAlias(kw, 32)',(ROOT/'TVS_Ventilation_ARX/commands/DrawHelpers.inl').read_text(encoding='utf-8-sig'))
if __name__=='__main__':unittest.main()

"""acrxEntryPoint.cpp split into commands/*.inl (plan 10): every fragment is included once
inside the app class, keeps its UTF-8 BOM (prompts with č/ć) and closes its own #if blocks."""
import pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
ARX=ROOT/'TVS_Ventilation_ARX'
class EntrySplit(unittest.TestCase):
 def test_fragments(self):
  entry=(ARX/'acrxEntryPoint.cpp').read_text(encoding='utf-8-sig')
  included=re.findall(r'^\t#include "commands/(\w+)\.inl"',entry,re.M)
  files=sorted(p.stem for p in (ARX/'commands').glob('*.inl'))
  self.assertEqual(sorted(included),files);self.assertEqual(len(set(included)),len(included))
  cls=entry.index('class CTVS_Ventilation_ARXApp');end=entry.index('IMPLEMENT_ARX_ENTRYPOINT')
  for name in included:self.assertTrue(cls<entry.index('commands/'+name+'.inl')<end)
  for p in (ARX/'commands').glob('*.inl'):
   self.assertTrue(p.read_bytes().startswith(b'\xef\xbb\xbf'),p.name)
   depth=0
   for line in p.read_text(encoding='utf-8-sig').split('\n'):
    s=line.strip()
    if re.match(r'#\s*if',s):depth+=1
    elif re.match(r'#\s*endif',s):depth-=1
    self.assertGreaterEqual(depth,0,p.name)
   self.assertEqual(depth,0,p.name)
if __name__=='__main__':unittest.main()

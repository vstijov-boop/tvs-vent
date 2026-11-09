"""TVS_DIM_PRATI: pravila spoja po visini (geometry/SizeFollowRules.h) i njihova upotreba u SizeFollow.h."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Rules(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_size_follow_rules.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_header_ascii(self):
  (ROOT/'geometry/SizeFollowRules.h').read_bytes().decode('ascii')   # compiled without BOM
 def test_used(self):
  s=text('TVS_Ventilation_ARX/SizeFollow.h')
  self.assertIn('#include "../geometry/SizeFollowRules.h"',s)
  self.assertEqual(s.count('tvsFollowRules::joined('),2)             # kanal -> prelaz, prelaz -> kanal
  self.assertIn('L"Da Ne Prelaz"',s)                                 # dionica: prelaz na spoju umjesto cijele dionice
  self.assertIn('joints(nodes,runs[k].first',s)
  self.assertIn('it->first.database()==db',s)                        # stanje po crtezu
  self.assertNotIn('q<500',s)
  self.assertEqual(s.count('{'),s.count('}'))
if __name__=='__main__':unittest.main()

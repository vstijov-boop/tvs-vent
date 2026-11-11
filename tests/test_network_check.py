"""TVS_PROVJERA: pravila provjere mreze (geometry/NetworkCheck.h) i komanda."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Check(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_network_check.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_ascii(self):(ROOT/'geometry/NetworkCheck.h').read_bytes().decode('ascii')
 def test_command(self):
  c=text('TVS_Ventilation_ARX/CheckCommand.h')
  self.assertIn('tvsNetworkCheck::check(items,f,all);',c);self.assertIn('L"Sljedeci Prethodni Kraj"',c)
  self.assertIn('TVS_PROVJERA, TVS_PROVJERA',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn("('PROVJERA',",text('tools/add_new_commands_cuix.py'))
if __name__=='__main__':unittest.main()

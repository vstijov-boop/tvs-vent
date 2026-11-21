"""Oprema na mrezi: zapis i model pada pritiska (geometry/Equipment.h)."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Equipment(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-pedantic',str(ROOT/'tests/test_equipment.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_header_ascii(self):
  (ROOT/'geometry/Equipment.h').read_bytes().decode('ascii')
if __name__=='__main__':unittest.main()

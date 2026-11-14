"""ASHRAE motor (geometry/AshraeEngine.h): izmisljeni podaci uvijek; poredjenje sa DuctPressDrop
motorom samo lokalno, kad postoje TVS_ASHRAE_DATA (folder ashrae_data) i TVS_DPD_APP (hvac_app)."""
import os,pathlib,subprocess,sys,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
FLAGS=['g++','-std=c++14','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation']
class Ashrae(unittest.TestCase):
 def test_ascii(self):(ROOT/'geometry/AshraeEngine.h').read_bytes().decode('ascii')
 def test_no_data_in_repo(self):   # ASHRAE tabele ne idu u javni repozitorijum
  for p in ROOT.rglob('*'):
   if '.git' in p.parts:continue
   self.assertFalse(p.suffix.upper()=='.TBL' or p.name in('Equation.csv','Constants.csv'),str(p))
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(FLAGS+[str(ROOT/'tests/test_ashrae_engine.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 @unittest.skipUnless(os.environ.get('TVS_ASHRAE_DATA') and os.environ.get('TVS_DPD_APP'),'bez ASHRAE podataka')
 def test_golden(self):
  with tempfile.TemporaryDirectory() as d:
   g=pathlib.Path(d)/'g.txt';exe=pathlib.Path(d)/'a'
   subprocess.run([sys.executable,str(ROOT/'tools/ashrae_golden.py'),os.environ['TVS_DPD_APP'],os.environ['TVS_ASHRAE_DATA'],str(g)],check=True)
   subprocess.run(FLAGS+[str(ROOT/'tests/ashrae_golden.cpp'),'-o',str(exe)],check=True)
   r=subprocess.run([str(exe),os.environ['TVS_ASHRAE_DATA'],str(g)],capture_output=True,text=True)
   self.assertIn('OK',r.stdout,r.stdout[-3000:])
if __name__=='__main__':unittest.main()

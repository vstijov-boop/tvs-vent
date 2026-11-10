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
  self.assertIn('tvsRunDialog::show(survey,banner,ch,ids().run)',s)  # dionica: dijalog umjesto pitanja u komandnoj liniji
  self.assertNotIn('ask(L"',s)                                       # obilazak dionice vise nista ne pita
  self.assertIn('joints(nodes,starts[k]',s)                          # samo kanal: upozorenje / prelaz na spoju
  self.assertIn('tvsTeeDialog::openArriving(arrivals[k].tee,ids().tee,arrivals[k].port,arrivals[k].in)',s)
  self.assertIn('it->first.database()==db',s)                        # stanje po crtezu
  self.assertNotIn('q<500',s)
  self.assertEqual(s.count('{'),s.count('}'))
 def test_run_dialog(self):
  d=text('TVS_Ventilation_ARX/RunDialog.h');rc=text('TVS_Ventilation_ARX/TVS_Ventilation_ARX.rc')
  self.assertEqual(d.count('{'),d.count('}'))
  self.assertIn('IDD_RUN DIALOGEX',rc);self.assertIn('#define IDD_RUN',text('TVS_Ventilation_ARX/resource.h'))
  block=rc[rc.index('IDD_RUN DIALOGEX'):];block=block[:block.index('\nEND')]
  for i in range(1501,1515):self.assertIn(',%d,'%i if i not in(1502,1503,1504,1505) else ' %d,'%i,block)
  self.assertIn('tvsSizeFollow::run(IDD_RUN, IDD_TEETYPE);',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  t=text('TVS_Ventilation_ARX/TeeTypeDialog.h')
  self.assertIn('inline bool openArriving(const AcDbObjectId& id,UINT idd,int k,const Section& in)',t)
  self.assertIn('if(d.newInlet&&(d.choice==Pants)==(d.inletChoice==Pants))',t)
 def test_run_sketch(self):
  (ROOT/'geometry/RunSketch.h').read_bytes().decode('ascii')
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Wno-misleading-indentation','-Werror',str(ROOT/'tests/run_sketch_svg.cpp'),'-o',str(exe)],check=True)
   subprocess.run([str(exe),d],check=True)
   svg=(pathlib.Path(d)/'run1.svg').read_text()
   self.assertIn('#1B6CA5',svg);self.assertIn('#FFE9BF',svg);self.assertIn('T komad',svg)   # izabrani kanal, T komad sa dijalogom, oznaka granice
if __name__=='__main__':unittest.main()

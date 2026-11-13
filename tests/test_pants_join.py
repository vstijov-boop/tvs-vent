"""TVS_SPOJ_TKG: TK-G izmedju ulaza i dva ogranka (geometry/PantsJoin.h) i komanda."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class Pants(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_pants_join.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_ascii(self):(ROOT/'geometry/PantsJoin.h').read_bytes().decode('ascii')
 def test_command(self):
  c=text('TVS_Ventilation_ARX/PantsJoinCommand.h')
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/PantsJoinCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  self.assertIn('tvsPantsJoin::plan(din,dbr,l,50.0/f,prefer)',c);self.assertIn('r.type=tvsTee::Pants;',c)
  self.assertIn('[Da/Zamijeni/Ne] <Da>',c);self.assertIn('restore(saved,tee);',c)
  self.assertIn('ports=tvsTeeFit::ports(o.object());',c)   # stvarni prikljucci T komada
  self.assertIn('TVS_SPOJ_TKG, TVS_SPOJ_TKG',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('tvsPantsJoinCmd::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('SPOJ_TKG',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_SPOJ_TKG_%d.png'%s)).exists())
if __name__=='__main__':unittest.main()

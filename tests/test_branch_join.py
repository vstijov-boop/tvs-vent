"""TVS_SPOJ_TKO: plan (geometry/BranchJoin.h), TK-O sa pomjerenim prolazom, komanda."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class BranchJoin(unittest.TestCase):
 def test_cpp(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t'
   subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_branch_join.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_ascii(self):(ROOT/'geometry/BranchJoin.h').read_bytes().decode('ascii')
 def test_offset_in_entity(self):
  w=text('TVS_Ventilation_DBX/TVS_WYE.cpp');self.assertIn('tee.xo = r.neck;',w);self.assertIn('ports[1] = Basepoint + u * half + v * tee.xo;',w)
  self.assertIn('const int last=r.neck!=0?5:4;',text('TVS_Ventilation_DBX/TeeData.h'))
 def test_command(self):
  c=text('TVS_Ventilation_ARX/BranchJoinCommand.h')
  self.assertTrue((ROOT/'TVS_Ventilation_ARX/BranchJoinCommand.h').read_bytes().startswith(b'\xef\xbb\xbf'))
  self.assertIn('tvsBranchJoin::plan(din,dpr,dbr,align,l,50.0/f)',c);self.assertIn('r.type=tvsTee::Branch;r.l=l;r.second=pr.a;r.neck=p.xo;',c)
  self.assertIn('[Da/Ivica/Ne] <Da>',c);self.assertIn('tvsPantsJoinCmd::restore(saved,tee);',c)
  self.assertIn('TVS_SPOJ_TKO, TVS_SPOJ_TKO',text('TVS_Ventilation_ARX/acrxEntryPoint.cpp'))
  self.assertIn('tvsBranchJoinCmd::execute();',text('TVS_Ventilation_ARX/commands/NewCommands.inl'))
  self.assertIn("('SPOJ_TKO',",text('tools/add_new_commands_cuix.py'))
  for s in (16,32):self.assertTrue((ROOT/('Installer/TVS_VENT/icons/TVS_SPOJ_TKO_%d.png'%s)).exists())
if __name__=='__main__':unittest.main()

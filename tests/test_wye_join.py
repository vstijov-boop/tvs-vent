"""TVS_SPOJ_TKG za okrugle kanale (Y45 / kosa racva) i TVS_MOVE / TVS_STRETCH za kose racve, Y45 i okrugla segmentna koljena (plan 25)."""
import pathlib,subprocess,tempfile,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def text(p):return (ROOT/p).read_text(encoding='utf-8-sig')
class WyeJoin(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as d:
   exe=pathlib.Path(d)/'t';subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror',str(ROOT/'tests/test_wye_join.cpp'),'-o',str(exe)],check=True)
   self.assertIn('OK',subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout)
 def test_cxx98_ascii(self):
  subprocess.run(['g++','-std=c++98','-fsyntax-only','-x','c++','-'],input='#include "geometry/WyeJoin.h"\nint main(){return 0;}\n',text=True,cwd=ROOT,check=True)
  (ROOT/'geometry/WyeJoin.h').read_bytes().decode('ascii')
 def test_command(self):
  s=text('TVS_Ventilation_ARX/PantsJoinCommand.h')
  self.assertIn('if(in.round&&br[0].round&&br[1].round){const Pick all[3]={in,br[0],br[1]};roundJoin(db,all);return;}',s)
  self.assertIn('tvsWyeJoin::lateralParts(d,2.0/f,m,b)',s);self.assertIn('tvsWyeJoin::wye(d[0],br,r.radius2,r.l,1.0,50.0/f)',s)
 def test_move(self):
  s=text('TVS_Ventilation_ARX/TVSMoveCommand.h')
  self.assertIn('&&(angled||std::fabs(mainAxis.dot(branchAxis))<1e-6))n.kind=4;',s)
  self.assertIn('n.kind=2;n.segmented=true;',s);self.assertIn('inline bool segmentedPorts(const TVS_TAP* p,const Point& middle,Point ports[2]){',s)
  self.assertIn('(n.kind==2&&!n.segmented)',text('TVS_Ventilation_ARX/SizeFollow.h'))
  self.assertIn('nd.kind!=2||nd.segmented',text('TVS_Ventilation_ARX/FlowCommand.h'))
if __name__=='__main__':unittest.main()

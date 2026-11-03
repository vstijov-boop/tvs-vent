"""TVS_PRESJEK (P1): section line cuts of the TVS network (geometry/Section.h) + wiring."""
import pathlib,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
CPP=r"""
#include "%s"
#include <cassert>
#include <cstdio>
using namespace tvsSection;
static bool near(double a,double b,double e=1e-9){return std::fabs(a-b)<=e;}
int main(){
 // duct along x at y=0 (400x200, axis z=2500); section line along y through x=1000
 Member d;d.a=P(0,0);d.b=P(2000,0);d.A1=d.A2=400;d.B1=d.B2=200;d.z=2500;d.tag=1;
 Cut c;assert(cut(P(1000,-500),P(1000,1500),d,c));
 assert(near(c.s,500)&&near(c.sinA,1)&&near(c.w,400)&&near(c.h,200)&&near(c.z,2500));
 // oblique 30 deg: section width A / sin
 const double k=std::sin(3.14159265358979323846/6);
 assert(cut(P(1000-1000*std::cos(3.14159265358979323846/6),-1000*k),P(1000+1000*std::cos(3.14159265358979323846/6),1000*k),d,c));
 assert(near(c.sinA,k,1e-9)&&near(c.w,400/k,1e-6));
 // parallel or outside: no cut
 assert(!cut(P(0,100),P(2000,100),d,c));assert(!cut(P(3000,-500),P(3000,500),d,c));
 // transition: width interpolated along the member
 Member t=d;t.A2=200;assert(cut(P(1500,-1),P(1500,1),t,c));assert(near(c.A,250));
 // round duct
 Member r=d;r.round=true;r.A1=r.A2=250;r.B1=r.B2=0;r.z=3000;r.tag=2;r.a=P(0,300);r.b=P(2000,300);
 std::vector<Member> net;net.push_back(d);net.push_back(r);
 // elbow chords meeting at the middle: one cut only
 Member e1;e1.a=P(500,600);e1.b=P(1000,600);e1.A1=e1.A2=300;e1.B1=e1.B2=300;e1.z=2000;e1.tag=3;
 Member e2=e1;e2.a=P(1000,600);e2.b=P(1000,900);net.push_back(e1);net.push_back(e2);
 std::vector<Cut> v=cuts(P(1000,-500),P(1000,1500),net,false,1);
 assert(v.size()==3&&near(v[0].s,500)&&near(v[1].s,800)&&v[1].round&&near(v[1].h,250)&&v[2].tag==3);
 // flip: measured from the other end, order reversed
 std::vector<Cut> fl=cuts(P(1000,-500),P(1000,1500),net,true,1);
 assert(fl.size()==3&&near(fl[2].s,1500)&&fl[0].tag==3);
 // overlap test
 Cut x1;x1.s=0;x1.w=400;x1.z=0;x1.h=200;Cut x2=x1;x2.s=300;assert(overlap(x1,x2));x2.z=300;assert(!overlap(x1,x2));
 std::puts("PASS SECTION cuts");
}
"""
class Section(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as f:
   cpp=pathlib.Path(f)/'s.cpp';cpp.write_text(CPP%(ROOT/'geometry/Section.h').as_posix())
   target=pathlib.Path(f)/'s';compile_cpp(cpp,target);subprocess.run([str(executable(target))],check=True)
 def test_contracts(self):
  e=(ROOT/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_text(encoding='utf-8-sig')
  self.assertIn('Ventilation_ARX, TVS_PRESJEK, TVS_PRESJEK, ACRX_CMD_MODAL',e);self.assertIn('tvsSectionCmd::run();',e)
  s=(ROOT/'TVS_Ventilation_ARX/SectionCommand.h').read_text(encoding='utf-8-sig')
  self.assertIn('const int n=build(btr.object(),p);refreshReferences(btr.object());',s,'Osvjezi rebuilds the block from stored parameters')
  self.assertIn('const short types[]={1,1,10,10,70,40,40,40};',s)
  for line in s.splitlines():self.assertFalse(line.rstrip().endswith('L"'))
if __name__=='__main__':unittest.main()

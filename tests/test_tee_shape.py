"""T komadi TK-O / TK-G (geometry/TeeShape.h): block geometry, TVS_WYE port equivalence, area."""
import pathlib,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
CPP=r'''
#include "%s"
#include <cassert>
#include <cstdio>
using namespace tvsTee;
static bool near(double a,double b,double e=1e-9){return std::fabs(a-b)<=e;}
static P at(const Arc& c,double ang){return P(c.c.x+c.r*std::cos(ang),c.c.y+c.r*std::sin(ang));}
int main(int argc,char**){
 // Block defaults: A = C = w = 100, R = 1.75 w = 175, collars 25, flange 25.
 Tee o;o.type=Branch;o.A=100;o.C=100;o.w1=100;o.R1=defaultRadius(100);o.l=25;o.flange=25;
 Shape s;assert(shape(o,s));
 assert(near(runLength(o),300)&&near(lengthPl(25,175,100),100));        // as in the block (L = 300)
 const Arc& in=s.arcs[0];const Arc& outA=s.arcs[1];
 assert(near(in.c.x,125)&&near(in.c.y,25)&&near(in.r,75)&&near(outA.r,175));   // centre (125,25), Ri 75
 P p=at(in,in.a0);assert(near(p.x,125)&&near(p.y,100));p=at(in,in.a1);assert(near(p.x,50)&&near(p.y,25));
 p=at(outA,outA.a0);assert(near(p.x,125)&&near(p.y,200));p=at(outA,outA.a1);assert(near(p.x,-50,1e-6));  // reaches the far wall
 // Ports equal the TVS_WYE model: run half = LengthPl + w/2 around the branch centre line,
 // branch face at A/2 + LengthPl. Branch centre line y = l + Ri + w/2 = L/2.
 const double Lp=lengthPl(o.l,o.R1,o.w1);assert(near(o.l+(o.R1-o.w1)+o.w1/2,runLength(o)/2));
 bool face=false;for(const Seg& g:s.lines)if(near(g.a.x,o.A/2+Lp)&&near(g.b.x,o.A/2+Lp)&&near(g.a.y,100-25)&&near(g.b.y,200+25))face=true;assert(face);
 // TK-G: both branches, outer arcs meet on the axis.
 Tee g=o;g.type=Pants;g.w2=100;g.R2=175;Shape t;assert(shape(g,t));assert(t.arcs.size()==4);
 p=at(t.arcs[1],t.arcs[1].a1);assert(near(p.x,0,1e-9));p=at(t.arcs[3],t.arcs[3].a0);assert(near(p.x,0,1e-9));
 // Different widths and a narrow branch (outer arc cannot reach the far wall: stops at 180 deg).
 Tee n=o;n.w1=60;n.R1=defaultRadius(60);Shape u;assert(shape(n,u));p=at(u.arcs[1],u.arcs[1].a1);assert(p.x>-50&&near(u.arcs[1].a1,pi()));
 // Area: branch quarter elbow + collar, run by mean perimeter minus opening.
 const double b=200,bb=200;const double ref=(2*(100+200)*300-100*200+2*(100+200)*(175-50)*pi()/2+2*(100+200)*25)/1e6;
 assert(near(area(o,b,bb),ref,1e-12));
 const double pants=(2*(100+200)*25+2*(2*(100+200)*(175-50)*pi()/2+2*(100+200)*25))/1e6;assert(near(area(g,b,bb),pants,1e-12));
 // Invalid input draws nothing.
 Shape z;Tee bad=o;bad.R1=90;assert(!shape(bad,z));bad=o;bad.type=Straight;assert(!shape(bad,z));
 if(argc>1){
  for(const Seg& q:s.lines)std::printf("L %%f %%f %%f %%f\n",q.a.x,q.a.y,q.b.x,q.b.y);
  for(const Arc& c:s.arcs)std::printf("A %%f %%f %%f %%f %%f\n",c.c.x,c.c.y,c.r,c.a0,c.a1);
  for(const Seg& q:t.lines)std::printf("l %%f %%f %%f %%f\n",q.a.x,q.a.y,q.b.x,q.b.y);
  for(const Arc& c:t.arcs)std::printf("a %%f %%f %%f %%f %%f\n",c.c.x,c.c.y,c.r,c.a0,c.a1);
  return 0;}
 std::puts("PASS TEE SHAPE TK-O/TK-G block geometry, WYE ports, area");
}
'''
def build(folder):
 cpp=pathlib.Path(folder)/'tee.cpp';cpp.write_text(CPP%(ROOT/'geometry/TeeShape.h').as_posix())
 target=pathlib.Path(folder)/'tee';compile_cpp(cpp,target);return executable(target)
class TeeShape(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as f:subprocess.run([str(build(f))],check=True)
if __name__=='__main__':unittest.main()

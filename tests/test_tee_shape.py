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
 // Defaults: A = C = w = 100, outer R = 1.5 w = 150 (centre line R = w), collars 25, flange 25.
 Tee o;o.type=Branch;o.A=100;o.C=100;o.w1=100;o.R1=defaultRadius(100);o.l=25;o.flange=25;
 Shape s;assert(shape(o,s));
 assert(near(defaultRadius(100)-50,100)&&near(runLength(o),250)&&near(lengthPl(25,150,100),75));
 const Arc& in=s.arcs[0];const Arc& outA=s.arcs[1];
 assert(near(in.c.x,100)&&near(in.c.y,25)&&near(in.r,50)&&near(outA.r,150));   // centre (100,25), Ri 50
 P p=at(in,in.a0);assert(near(p.x,100)&&near(p.y,75));p=at(in,in.a1);assert(near(p.x,50)&&near(p.y,25));
 const double qy=25+std::sqrt(150.0*150-50*50);
 p=at(outA,outA.a0);assert(near(p.x,100)&&near(p.y,175));p=at(outA,outA.a1);assert(near(p.x,50,1e-6)&&near(p.y,qy,1e-6));  // stops at the near run wall
 // Near run wall is open between the inner arc and the outer arc; no line crosses the opening.
 bool wall=false;for(const Seg& g:s.lines){if(near(g.a.x,50)&&near(g.b.x,50)){if(std::max(g.a.y,g.b.y)>25+1e-9)assert(!(std::min(g.a.y,g.b.y)<qy-1e-6));if(near(g.a.y,qy,1e-6)&&near(g.b.y,225))wall=true;}}assert(wall);
 // Ports equal the TVS_WYE model: run half = LengthPl + w/2 around the branch centre line,
 // branch face at A/2 + LengthPl. Branch centre line y = l + Ri + w/2 = L/2.
 const double Lp=lengthPl(o.l,o.R1,o.w1);assert(near(o.l+(o.R1-o.w1)+o.w1/2,runLength(o)/2));
 bool face=false;for(const Seg& g:s.lines)if(near(g.a.x,o.A/2+Lp)&&near(g.b.x,o.A/2+Lp)&&near(g.a.y,75-25)&&near(g.b.y,175+25))face=true;assert(face);
 // TK-G: both branches, outer arcs meet on the axis.
 Tee g=o;g.type=Pants;g.w2=100;g.R2=150;Shape t;assert(shape(g,t));assert(t.arcs.size()==4);
 p=at(t.arcs[1],t.arcs[1].a1);assert(near(p.x,0,1e-9));p=at(t.arcs[3],t.arcs[3].a0);assert(near(p.x,0,1e-9));
 // TK-G with different branch widths: both branch axes on one line (TVS run), fixed collars,
 // the narrower branch starts after a straight inlet wall; outer arcs meet in a cusp.
 const double cases[][3]={{100,100,60},{100,60,100},{100,60,60},{150,100,60}};
 for(const auto& cs:cases){
  Tee k=o;k.type=Pants;k.A=cs[0];k.w1=cs[1];k.w2=cs[2];k.R1=defaultRadius(k.w1);k.R2=defaultRadius(k.w2);
  Shape h;assert(shape(k,h)&&pantsFeasible(k.A,k.w1,k.w2,25));const PantsFrame pf=pantsFrame(k);
  assert(near(pf.Y,25+(std::max)(k.w1,k.w2))&&near(pf.Y-k.w1/2,pantsLengthPl(25,k.w1,k.w2)));
  // fixed collars l = 25: faces at a + Ri + l, both centred on the common axis Y
  double mid[2]={0,0};int faces=0;const double fx[2]={k.A/2+k.R1-k.w1+25,k.A/2+k.R2-k.w2+25};
  for(const Seg& g:h.lines)if(near(g.a.x,g.b.x)&&std::fabs(g.b.y-g.a.y)>1&&near(std::fabs(g.a.x),fx[g.a.x>0?0:1])){
    ++faces;mid[g.a.x>0?0:1]=(g.a.y+g.b.y)/2;}
  assert(faces==2&&near(mid[0],pf.Y)&&near(mid[1],pf.Y));
  assert(near(fx[k.w1>=k.w2?0:1],k.w1>=k.w2?pf.X:fx[1])&&fx[0]<=pf.X+1e-9);
  P e1=at(h.arcs[1],h.arcs[1].a1),e2=at(h.arcs[3],h.arcs[3].a0);assert(near(e1.x,e2.x,1e-6)&&near(e1.y,e2.y,1e-6));
  if(argc>2)for(const Seg& q:h.lines)std::printf("G%%d %%f %%f %%f %%f\n",(int)(&cs-cases),q.a.x,q.a.y,q.b.x,q.b.y);
  if(argc>2)for(const Arc& c:h.arcs)std::printf("H%%d %%f %%f %%f %%f %%f\n",(int)(&cs-cases),c.c.x,c.c.y,c.r,c.a0,c.a1);
 }
 // Branches too narrow for the inlet (not used in practice): no TK-G drawing.
 assert(!pantsFeasible(200,60,100,25)&&!pantsFeasible(200,100,60,25)&&!pantsFeasible(300,60,60,25));
 // Narrow branch and a tapered run (C < A): the outer arc still ends on the near wall.
 Tee n=o;n.w1=60;n.R1=defaultRadius(60);n.C=80;Shape u;assert(shape(n,u));p=at(u.arcs[1],u.arcs[1].a1);
 {const double L=runLength(n),tt=(p.y-25)/(L-50);assert(tt>0&&tt<1&&near(p.x,50+tt*(40-50),1e-6));}
 // Area: branch quarter elbow + collar, run by mean perimeter minus opening.
 const double b=200,bb=200;const double ref=(2*(100+200)*250-100*200+2*(100+200)*(150-50)*pi()/2+2*(100+200)*25)/1e6;
 assert(near(area(o,b,bb),ref,1e-12));
 const double pants=(2*(100+200)*25+2*(2*(100+200)*(150-50)*pi()/2+2*(100+200)*25))/1e6;assert(near(area(g,b,bb),pants,1e-12));
 // Stucna SR5-3: L = 0.25 w, at least 75 mm; straight main 2L + w; branch face at a + L
 // from y = L to L + w; 45 deg entry (a,0) -> (a+L,L) upstream; main continuous, no end lines.
 assert(near(tapLength(200,75),75)&&near(tapLength(400,75),100)&&near(tapLength(250,75),75));
 Tee k;k.type=Tap;k.A=500;k.C=500;k.w1=400;k.l=tapLength(400,75);k.flange=25;Shape ts;assert(shape(k,ts)&&ts.arcs.empty());
 assert(near(runLength(k),2*100+400));
 bool chamfer=false,tface=false,open=true;wall=false;
 for(const Seg& g:ts.lines){
  if(near(g.a.x,250)&&near(g.a.y,0)&&near(g.b.x,350)&&near(g.b.y,100))chamfer=true;
  if(near(g.a.x,350)&&near(g.b.x,350)&&near(g.a.y,100-25)&&near(g.b.y,500+25))tface=true;
  if(near(g.a.y,g.b.y)&&std::min(g.a.x,g.b.x)<0)open=false;   // no end lines across the main
  if(near(g.a.x,250)&&near(g.b.x,250)&&near(g.a.y,0)&&near(g.b.y,600))wall=true;}
 assert(wall);
 assert(chamfer&&tface&&open);
 {const double bb=300,L=100,w=400;const double ref=(2*(500+300)*600-(w+L)*bb+L*(2*w+L)+bb*L+bb*L*std::sqrt(2.0))/1e6;assert(near(area(k,300,bb),ref,1e-12));}
 // Invalid input draws nothing.
 Shape z;Tee bad=o;bad.R1=90;assert(!shape(bad,z));bad=o;bad.type=Straight;assert(!shape(bad,z));
 if(argc>1){
  for(const Seg& q:s.lines)std::printf("L %%f %%f %%f %%f\n",q.a.x,q.a.y,q.b.x,q.b.y);
  for(const Arc& c:s.arcs)std::printf("A %%f %%f %%f %%f %%f\n",c.c.x,c.c.y,c.r,c.a0,c.a1);
  for(const Seg& q:t.lines)std::printf("l %%f %%f %%f %%f\n",q.a.x,q.a.y,q.b.x,q.b.y);
  for(const Arc& c:t.arcs)std::printf("a %%f %%f %%f %%f %%f\n",c.c.x,c.c.y,c.r,c.a0,c.a1);
  return 0;}
 std::puts("PASS TEE SHAPE TK-O/TK-G/stucna block geometry, WYE ports, area");
}
'''
def build(folder):
 cpp=pathlib.Path(folder)/'tee.cpp';cpp.write_text(CPP%(ROOT/'geometry/TeeShape.h').as_posix())
 target=pathlib.Path(folder)/'tee';compile_cpp(cpp,target);return executable(target)
class TeeShape(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as f:subprocess.run([str(build(f))],check=True)
if __name__=='__main__':unittest.main()

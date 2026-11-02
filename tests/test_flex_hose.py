"""Flexible hose plan geometry (convex beads): envelope, joints and cell counts, compiled."""
import pathlib,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
CPP=r'''
#include "%s"
#include <cassert>
#include <cstdio>
using namespace tvsFlex;
// Points of polyline segment a->b with bulge (AcDbPolyline convention).
static void sample(const Vertex& a,const Vertex& b,int k,double* xs,double* ys){
 const double dx=b.x-a.x,dy=b.y-a.y,c=std::hypot(dx,dy),th=4*std::atan(a.bulge);
 if(a.bulge==0){for(int i=0;i<=k;++i){xs[i]=a.x+dx*i/k;ys[i]=a.y+dy*i/k;}return;}
 // positive bulge = CCW arc: centre left of the chord at distance r*cos(th/2)
 const double r=c/(2*std::sin(std::fabs(th)/2)),off=r*std::cos(std::fabs(th)/2)*(th>0?1:-1);
 const double Cx=(a.x+b.x)/2-dy/c*off,Cy=(a.y+b.y)/2+dx/c*off,a0=std::atan2(a.y-Cy,a.x-Cx);
 for(int i=0;i<=k;++i){const double t=a0+th*i/k;xs[i]=Cx+r*std::cos(t);ys[i]=Cy+r*std::sin(t);}
}
static double maxOffset(const std::vector<Vertex>& e,double nx,double ny,double ox,double oy){double m=-1e300,xs[33],ys[33];
 for(size_t j=0;j+1<e.size();++j){sample(e[j],e[j+1],32,xs,ys);for(int i=0;i<=32;++i){double v=(xs[i]-ox)*nx+(ys[i]-oy)*ny;if(v>m)m=v;}}return m;}
static double maxRadius(const std::vector<Vertex>& e,double cx,double cy,bool outer){double m=outer?-1e300:1e300,xs[33],ys[33];
 for(size_t j=0;j+1<e.size();++j){sample(e[j],e[j+1],32,xs,ys);for(int i=0;i<=32;++i){double v=std::hypot(xs[i]-cx,ys[i]-cy);if(outer?v>m:v<m)m=v;}}return m;}
int main(int argc,char**){
 const double D=250,k=defaultPitch();Drawing s,e;
 assert(straight(0,0,1,0,1200,D,k,s));
 const int n=cellCount(1200,D,k);assert(n==24&&s.edges[0].size()==25&&s.rings.size()==25);
 // Envelope: bead tops reach exactly D/2 on both sides, never beyond.
 assert(std::fabs(maxOffset(s.edges[0],0,1,0,0)-D/2)<1e-6&&std::fabs(maxOffset(s.edges[1],0,-1,0,0)-D/2)<1e-6);
 // Valleys inside by h, ring lines at valley width.
 const double h=depth(k*D,D);assert(std::fabs(s.edges[0][3].y-(D/2-h))<1e-9&&std::fabs(s.rings[2].y0-s.rings[2].y1-(D-2*h))<1e-9);
 // Elbow 90 deg, R = 1.0 D, starting where the straight ends (centre above the end point).
 assert(elbow(1200,D,D,-3.141592653589793/2,3.141592653589793/2,D,k,e));
 assert(std::fabs(maxRadius(e.edges[0],1200,D,true)-(D+D/2))<1e-6&&std::fabs(maxRadius(e.edges[1],1200,D,false)-(D-D/2))<1e-6);
 // Joint: last straight valley = first elbow valley (same side), within 1e-9.
 assert(std::hypot(s.edges[1].back().x-e.edges[0].front().x,s.edges[1].back().y-e.edges[0].front().y)<1e-9);
 assert(std::hypot(s.edges[0].back().x-e.edges[1].front().x,s.edges[0].back().y-e.edges[1].front().y)<1e-9);
 if(argc>1){ // dump for the visual check
  for(const Drawing* g:{&s,&e}){for(int j=0;j<2;++j){std::printf("E");for(auto& v:g->edges[j])std::printf(" %%.6f %%.6f %%.9f",v.x,v.y,v.bulge);std::printf("\n");}
   for(auto& l:g->rings)std::printf("R %%.6f %%.6f %%.6f %%.6f\n",l.x0,l.y0,l.x1,l.y1);}return 0;}
 // Degenerate input draws nothing (caller keeps the old drawing).
 assert(!straight(0,0,1,0,0,D,k,s)&&!elbow(0,0,D/2,0,1,D,k,e)&&!elbow(0,0,D,0,0,D,k,e));
 assert(cellCount(1e9,D,k)==2000);
 std::puts("PASS FLEX HOSE envelope D, valleys, elbow radii, straight/elbow joint, limits");
}
'''
def build(folder):
 cpp=pathlib.Path(folder)/'flex.cpp';cpp.write_text(CPP%(ROOT/'geometry/FlexHose.h').as_posix())
 target=pathlib.Path(folder)/'flex';compile_cpp(cpp,target);return executable(target)
class FlexHose(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as folder:subprocess.run([str(build(folder))],check=True)
if __name__=='__main__':unittest.main()

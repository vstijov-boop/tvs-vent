"""TVS_PRESJEK (P1): section line cuts of the TVS network (geometry/Section.h) + wiring."""
import pathlib,subprocess,tempfile,unittest
from entry_source import entry_source
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
 // depth: duct parallel to the section 500 behind it, partly behind a cut duct
 { std::vector<Member> m2;
   Member c=d;c.a=P(500,-100);c.b=P(500,2000);c.A1=c.A2=200;c.B1=c.B2=300;c.z=1000;c.tag=10;m2.push_back(c);   // crosses the line y=0 at x=500
   Member q=d;q.a=P(0,500);q.b=P(2000,500);q.A1=q.A2=400;q.B1=q.B2=200;q.z=1000;q.tag=11;m2.push_back(q);    // parallel, 500 behind
   Member out3=q;out3.a=P(0,3000);out3.b=P(2000,3000);out3.tag=12;m2.push_back(out3);                           // outside the depth
   const P s1(0,0),s2(2000,0);   // looking +y (left of +x)
   std::vector<Cut> cc=cuts(s1,s2,m2,false,1);assert(cc.size()==1&&near(cc[0].s,500));
   std::vector<View> vv=views(s1,s2,m2,false,1000,1);
   bool par=false,outside=false;for(size_t i=0;i<vv.size();++i){if(vv[i].tag==11){par=near(vv[i].x0,0)&&near(vv[i].x1,2000)&&near(vv[i].zc0,1000)&&near(vv[i].zc1,1000)&&near(vv[i].h,200)&&near(vv[i].closest,500);}if(vv[i].tag==12)outside=true;}
   assert(par&&!outside);
   std::vector<Edge> ee=edges(cc,vv);
   // the parallel duct's top edge (z=1100) is hidden between x=400 and x=600 by the cut 200x300 at z=1000
   bool hidden=false,left=false;for(size_t i=0;i<ee.size();++i){const Prim& g=ee[i].g;const bool gh=ee[i].hidden;
     if(near(g.y0,1100)&&near(g.y1,1100)&&gh&&near((std::min)(g.x0,g.x1),400,1e-4)&&near((std::max)(g.x0,g.x1),600,1e-4))hidden=true;   // change points refined
     if(near(g.y0,1100)&&!gh&&near((std::min)(g.x0,g.x1),0)&&near((std::max)(g.x0,g.x1),400,1e-4))left=true;}
   assert(hidden&&left);
   // flipped: looking -y, nothing behind (members at +y are in front of the plane)
   assert(views(s1,s2,m2,true,1000,1).size()==1);   // only the crossing duct's part at y<0
 }
 // Z prelaz seen from the side: level 2000 -> sloped -> level 2600, parallel to the section 300 behind
 { std::vector<Member> z3;const P s1(0,0),s2(3000,0);
   Member l1;l1.a=P(0,300);l1.b=P(1000,300);l1.A1=l1.A2=400;l1.B1=l1.B2=200;l1.z=2000;l1.tag=1;z3.push_back(l1);
   Member sl=l1;sl.a=P(1000,300);sl.b=P(1600,300);sl.sloped=true;sl.za=2000;sl.zb=2600;sl.tag=2;z3.push_back(sl);
   Member l2=l1;l2.a=P(1600,300);l2.b=P(3000,300);l2.z=2600;l2.tag=3;z3.push_back(l2);
   Member riser=l1;riser.a=riser.b=P(2500,100);riser.sloped=true;riser.za=2600;riser.zb=3200;riser.tag=4;z3.push_back(riser);
   std::vector<View> v=views(s1,s2,z3,false,1000,1);assert(v.size()==4);
   bool slope=false,vert=false;for(size_t i=0;i<v.size();++i){
     if(v[i].tag==2){const double o=100/std::sqrt(2.0);slope=near(v[i].x0,1000-o,1e-6)&&near(v[i].x1,1600+o,1e-6)&&near(v[i].zc0,2000)&&near(v[i].zc1,2600);}
     if(v[i].tag==4)vert=v[i].vertical&&near(v[i].x0,2300)&&near(v[i].x1,2700)&&near(v[i].zc0,2900)&&near(v[i].h,600);}
   assert(slope&&vert);
   // sloped top edge drawn as one straight (not stepped) visible line
   std::vector<Edge> e=edges(std::vector<Cut>(),v);bool top=false;
   const double o=100/std::sqrt(2.0);   // thickness perpendicular to the 45 deg axis
   for(size_t i=0;i<e.size();++i)if(!e[i].hidden&&near(e[i].g.x0,1000+o,1e-6)&&near(e[i].g.y0,2000-o,1e-6)&&near(e[i].g.x1,1600+o,1e-6)&&near(e[i].g.y1,2600-o,1e-6))top=true;   // bottom edge
   assert(top);
   // cut through the sloped piece: elevation interpolated
   Cut c;assert(cut(P(1300,0),P(1300,1000),sl,c)&&near(c.z,2300));
 }
 // vertical elbow seen from the side: true arc band, level end tangent horizontal
 { Member bnd;bnd.a=P(1000,300);bnd.b=P(1200,300);bnd.A1=bnd.A2=400;bnd.B1=bnd.B2=200;bnd.sloped=true;bnd.bend=true;bnd.levelA=true;bnd.za=2000;bnd.zb=2200;bnd.tag=7;
   std::vector<Member> one(1,bnd);std::vector<View> v=views(P(0,0),P(3000,0),one,false,1000,1);
   assert(v.size()==1&&v[0].bend&&v[0].outline.size()==4&&v[0].outline[0].arc&&v[0].outline[2].arc);
   const Prim& o=v[0].outline[0];assert(near(o.r,200+100)&&near(o.cx,1000)&&near(o.cy,2200));   // axis radius 200 (quarter), outer 300
   const P s0=o.at(0),s1=o.at(1);assert(near(s0.x,1000,1e-9)&&near(s0.y,1900,1e-9)&&near(s1.x,1300,1e-9)&&near(s1.y,2200,1e-9));
   std::vector<Edge> e=edges(std::vector<Cut>(),v);bool arc=false;for(size_t i=0;i<e.size();++i)if(e[i].g.arc&&!e[i].hidden)arc=true;assert(arc);
 }
 // horizontal elbow chords at one level merge into one box (no inner vertical lines)
 { Member c1;c1.a=P(500,300);c1.b=P(800,300);c1.A1=c1.A2=200;c1.B1=c1.B2=200;c1.z=1000;c1.tag=9;
   Member c2=c1;c2.a=P(800,300);c2.b=P(800,600);
   std::vector<Member> two;two.push_back(c1);two.push_back(c2);
   std::vector<View> v=views(P(0,0),P(3000,0),two,false,1000,1);assert(v.size()==1&&near(v[0].x0,500)&&near(v[0].x1,900));
 }
 // gap: duct passing under a cut duct
 { std::vector<Cut> cc(1);cc[0].s=1000;cc[0].w=400;cc[0].h=200;cc[0].z=2500;cc[0].tag=1;
   View low;low.x0=0;low.x1=3000;low.zc0=low.zc1=2000;low.h=200;low.tag=2;low.closest=100;low.quad();
   std::vector<View> vv(1,low);std::vector<Gap> g=gaps(cc,vv,10);
   assert(g.size()==1&&near(g[0].low,2100)&&near(g[0].high,2400)&&near(g[0].lowBottom,1900)&&near(g[0].x,1000));
 }
 // prelaz 400x300 -> 400x200, ravno uz gornju ivicu: trapezoid with a level top edge
 { Member tr;tr.a=P(1000,300);tr.b=P(1300,300);tr.A1=tr.A2=400;tr.B1=300;tr.B2=200;tr.taper=true;tr.sloped=true;tr.za=2000;tr.zb=2050;tr.tag=21;
   std::vector<Member> one(1,tr);std::vector<View> v=views(P(0,0),P(3000,0),one,false,1000,1);
   assert(v.size()==1&&near(v[0].h,300)&&near(v[0].h1,200)&&!v[0].level());
   const Prim& top=v[0].outline[2];assert(near(top.y0,2150)&&near(top.y1,2150));      // level top
   const Prim& bot=v[0].outline[0];assert(near(bot.y0,1850)&&near(bot.y1,1950));      // rising bottom
   Cut c;assert(cut(P(1150,0),P(1150,1000),tr,c)&&near(c.z,2025)&&near(c.h,250)&&near(c.z+c.h/2,2150));
 }
 // prelaz uz jednu ivicu: skewed axis, faces square to the duct -> no widening along a parallel section
 { Member tr;tr.a=P(1000,300);tr.b=P(1300,325);tr.dir=P(1,0);tr.A1=400;tr.A2=350;tr.B1=tr.B2=200;tr.taper=true;tr.z=2000;tr.tag=31;
   std::vector<Member> one(1,tr);std::vector<View> v=views(P(0,0),P(3000,0),one,false,1000,1);
   assert(v.size()==1&&near(v[0].x0,1000,1e-9)&&near(v[0].x1,1300,1e-9));
 }
 std::puts("PASS SECTION cuts");
}
"""
class Section(unittest.TestCase):
 def test_geometry(self):
  with tempfile.TemporaryDirectory() as f:
   cpp=pathlib.Path(f)/'s.cpp';cpp.write_text(CPP%(ROOT/'geometry/Section.h').as_posix())
   target=pathlib.Path(f)/'s';compile_cpp(cpp,target);subprocess.run([str(executable(target))],check=True)
 def test_contracts(self):
  e=entry_source()
  self.assertIn('Ventilation_ARX, TVS_PRESJEK, TVS_PRESJEK, ACRX_CMD_MODAL',e);self.assertIn('tvsSectionDialog::run(IDD_SECTION);',e)
  s=(ROOT/'TVS_Ventilation_ARX/SectionCommand.h').read_text(encoding='utf-8-sig')
  self.assertIn('const int n=build(btr.object(),p);refreshReferences(btr.object());',s,'Osvjezi rebuilds the block from stored parameters')
  self.assertIn('const short types[]={1,1,10,10,70,40,40,40};',s)
  self.assertIn('const double th=0.7*p.scale/f',s,'1:50 in mm -> 35')
  self.assertIn('levelMark(btr,x,y+hh,true,level(c.z+hh,f),th,style,layerId);',s,'visinske kote as level marks')
  for line in s.splitlines():self.assertFalse(line.rstrip().endswith('L"'))
  import re
  g=(ROOT/'geometry/Section.h').read_text()
  for macro in ('near','far','min','max'):self.assertIsNone(re.search(r'[^:(\w]'+macro+r'\b(?!\))',g.replace('(std::min)','').replace('(std::max)','')),'Windows macro name '+macro)
if __name__=='__main__':unittest.main()

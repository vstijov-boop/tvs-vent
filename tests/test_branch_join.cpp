#include "../geometry/BranchJoin.h"
#include "../geometry/TeeShape.h"
#include <cassert>
#include <cstdio>
using namespace tvsBranchJoin;
static Duct D(double ax,double ay,double bx,double by,double w,bool farJ=false){Duct d;d.a=V(ax,ay);d.b=V(bx,by);d.width=w;d.farJoined=farJ;return d;}
static bool eq(double a,double b){return std::fabs(a-b)<1e-6;}
int main(){
    const double l=25,m=50;
    // the user's drawing (13.11.): inlet from above (x 0..200 -> axis 100, w 200), prolaz below narrower (w 150, axis 125: right edges in line),
    // branch to the left (w 150, axis y = -400)
    Duct in=D(100,3000,100,0,200),pr=D(125,-1000,125,-4000,150),br=D(-2000,-400,-300,-400,150);
    {Plan p=plan(in,pr,br,0,l,m);assert(p.error==Ok);
     assert(eq(p.u.x,0)&&eq(p.u.y,-1)&&eq(p.v.x,-1)&&eq(p.v.y,0));
     assert(eq(p.offset,-25)&&p.suggested==1);                  // right edge = the edge opposite the branch
     assert(p.moved&&eq(p.shift.x,-25));                         // axis: prolaz moved onto the inlet axis
     assert(eq(p.base.x,100)&&eq(p.base.y,-400)&&eq(p.lengthPl,25+75));
     assert(eq(p.inletPort.y,-400+175)&&eq(p.prolazPort.y,-400-175)&&eq(p.prolazPort.x,100));
     assert(eq(p.branchPort.x,100-(100+100))&&eq(p.branchPort.y,-400));}
    {Plan p=plan(in,pr,br,1,l,m);assert(p.error==Ok&&!p.moved&&eq(p.xo,-25)&&eq(p.prolazPort.x,125));}   // already in line: nothing moves
    {Plan p=plan(in,pr,br,2,l,m);assert(p.error==Ok&&p.moved&&eq(p.xo,25)&&eq(p.shift.x,-50)&&eq(p.prolazPort.x,75));}
    // prolaz joined further on and not where it has to be: refused; in line already: fine
    {Duct pj=pr;pj.farJoined=true;assert(plan(in,pj,br,0,l,m).error==ProlazJoined);assert(plan(in,pj,br,1,l,m).error==Ok);}
    // refusals
    {Duct b2=D(-2000,-400,-300,-700,150);assert(plan(in,pr,b2,0,l,m).error==NotPerpendicular);}
    {Duct p2=D(125,-1000,900,-4000,150);assert(plan(in,p2,br,0,l,m).error==NotParallel);}
    {Duct i2=D(100,-300,100,-200,200);assert(plan(i2,pr,br,0,l,m).error==InletShort);}
    {Duct b2=D(-130,-400,-120,-400,150);assert(plan(in,pr,b2,0,l,m).error==BranchShort);}
    {Duct p2=D(125,-560,125,-600,150);assert(plan(in,p2,br,1,l,m).error==ProlazShort);}
    // the TK-O shape with an offset prolaz: one run wall straight (edge in line)
    {tvsTee::Tee t;t.type=tvsTee::Branch;t.A=200;t.C=150;t.w1=150;t.R1=225;t.l=25;t.xo=-25;tvsTee::Shape s;assert(tvsTee::shape(t,s));
     bool straight=false;for(size_t k=0;k<s.lines.size();++k){const tvsTee::Seg& g=s.lines[k];if(eq(g.a.x,-100)&&eq(g.b.x,-100)&&g.b.y-g.a.y>100)straight=true;}
     assert(straight);
     t.xo=25;assert(tvsTee::shape(t,s));}
    std::puts("branch join OK");return 0;
}

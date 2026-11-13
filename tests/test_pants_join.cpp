#include "../geometry/PantsJoin.h"
#include <cassert>
#include <cstdio>
using namespace tvsPantsJoin;
static Duct D(double ax,double ay,double bx,double by,double w,bool farJ=false){Duct d;d.a=V(ax,ay);d.b=V(bx,by);d.width=w;d.farJoined=farJ;return d;}
static bool near1(double a,double b){return std::fabs(a-b)<1e-6;}
int main(){
    const double l=100,minDuct=50;
    // inlet along +x ending at x=6000 (y=0), branch up at x=6300 (w300), branch down at x=6250 (w200): like the drawing
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,400,6300,3000,300),D(6250,-400,6250,-3000,200)};
     Plan p=plan(in,br,l,minDuct);assert(p.error==Ok);
     assert(p.moved==1&&near1(p.shift.x,50)&&near1(p.shift.y,0));        // narrower one onto the wider's axis
     assert(near1(p.base.x,6300)&&near1(p.base.y,0));
     assert(near1(p.u.x,0)&&near1(p.u.y,1)&&near1(p.v.x,-1));
     assert(near1(p.inletPort.x,6300-(l+300))&&near1(p.inletPort.y,0)); // l + max(w1,w2)
     assert(near1(p.port[0].y,200+150+100)&&near1(p.port[1].y,-(200+100+100)));
     assert(p.inletNear==1&&p.branchNear[0]==0&&p.branchNear[1]==0);}
    // far end of the narrower one joined: the wider moves
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,400,6300,3000,300),D(6250,-400,6250,-3000,200,true)};
     Plan p=plan(in,br,l,minDuct);assert(p.error==Ok&&p.moved==0&&near1(p.shift.x,-50)&&near1(p.base.x,6250));}
    // both joined and not aligned: refused; aligned: fine
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,400,6300,3000,300,true),D(6250,-400,6250,-3000,200,true)};
     assert(plan(in,br,l,minDuct).error==BothJoined);
     br[1]=D(6300,-400,6300,-3000,200,true);Plan p=plan(in,br,l,minDuct);assert(p.error==Ok&&p.moved==-1);}
    // forced choice (Zamijeni)
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,400,6300,3000,300),D(6250,-400,6250,-3000,200)};
     Plan p=plan(in,br,l,minDuct,0);assert(p.error==Ok&&p.moved==0&&near1(p.base.x,6250));}
    // not perpendicular / same side
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,400,6600,3000,300),D(6250,-400,6250,-3000,200)};assert(plan(in,br,l,minDuct).error==NotPerpendicular);
     br[0]=D(6300,400,6300,3000,300);br[1]=D(6250,500,6250,3000,200);assert(plan(in,br,l,minDuct).error==NotOpposite);}
    // inlet too short
    {Duct in=D(5880,0,6000,0,400);Duct br[2]={D(6300,400,6300,3000,300),D(6300,-400,6300,-3000,200)};assert(plan(in,br,l,minDuct).error==InletShort);}
    // branch too short (face beyond its far end)
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,300,6300,420,300),D(6300,-400,6300,-3000,200)};assert(plan(in,br,l,minDuct).error==BranchShort);}
    // ends given the other way round
    {Duct in=D(6000,0,0,0,400);Duct br[2]={D(6300,3000,6300,400,300),D(6300,-3000,6300,-400,200)};
     Plan p=plan(in,br,l,minDuct);assert(p.error==Ok&&p.inletNear==0&&p.branchNear[0]==1&&p.branchNear[1]==1&&p.moved==-1);}
    std::puts("pants join OK");return 0;
}

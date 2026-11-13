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
    // Vrat: nothing moves, the T komad on the axis nearer the inlet, the farther branch gets e
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6300,400,6300,3000,300),D(6250,-400,6250,-3000,200)};
     Plan p=plan(in,br,l,minDuct,-1,true);assert(p.error==Ok&&p.moved==-1&&near1(p.base.x,6250));
     assert(near1(p.e[0],50)&&near1(p.e[1],0)&&near1(p.neck,-50));
     assert(near1(p.port[0].x,6300)&&near1(p.port[1].x,6250));          // both faces on their own axes
     assert(near1(p.inletPort.x,6250-(l+300)));
     // both branches joined further on: Vrat is the way
     br[0].farJoined=br[1].farJoined=true;p=plan(in,br,l,minDuct,-1,true);assert(p.error==Ok&&near1(p.e[0],50));
     // the other way round: branch 1 farther
     Duct b2[2]={D(6250,400,6250,3000,300),D(6300,-400,6300,-3000,200)};p=plan(in,b2,l,minDuct,-1,true);
     assert(p.error==Ok&&near1(p.e[1],50)&&near1(p.neck,50)&&near1(p.base.x,6250));}
    // the shape with a vrat is a real TK-G (cusp found) and its ports match the plan
    {tvsTee::Tee t;t.type=tvsTee::Pants;t.A=400;t.w1=300;t.w2=200;t.R1=450;t.R2=300;t.l=100;t.e1=50;tvsTee::Shape s;assert(tvsTee::shape(t,s));
     const tvsTee::PantsFrame f=tvsTee::pantsFrame(t);assert(near1(f.d1,50)&&near1(f.d2,100));}
    // long vrat: the outer arcs do not meet any more; drawn with the straight wall (13.11.), any length
    for(double e:{150.0,400.0,1500.0}){
        for(int side=0;side<2;++side){tvsTee::Tee t;t.type=tvsTee::Pants;t.A=400;t.w1=300;t.w2=250;t.R1=450;t.R2=375;t.l=25;
            if(side)t.e2=e;else t.e1=e;tvsTee::Shape s;assert(tvsTee::shape(t,s));
            // the straight wall is on x = s(a - w_far), from the nearer arc up to the farther centre height
            bool wall=false;const double xs=side?-(200-250):(200-300);
            for(size_t k=0;k<s.lines.size();++k)if(near1(s.lines[k].a.x,xs)&&near1(s.lines[k].b.x,xs))wall=true;
            if(e>=400)assert(wall);}}
    {Duct in=D(0,0,6000,0,400);Duct br[2]={D(6800,400,6800,3000,300),D(6250,-400,6250,-3000,250)};
     Plan p=plan(in,br,l,minDuct,-1,true);assert(p.error==Ok&&near1(p.e[0],550));}
    std::puts("pants join OK");return 0;
}

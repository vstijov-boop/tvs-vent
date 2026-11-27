// TVS_SPOJ_TKG za okrugle kanale: racva Y45 i kosa racva TK izmedju tri nacrtana kanala (geometry/WyeJoin.h).
#include "../geometry/WyeJoin.h"
#include <cstdio>
#include <cmath>
static int fails=0;
static void near(double a,double b,double tol,const char* w){if(std::fabs(a-b)>tol){std::printf("FAIL %s: %.4f != %.4f\n",w,a,b);++fails;}}
static void check(bool c,const char* w){if(!c){std::printf("FAIL %s\n",w);++fails;}}
typedef tvsWyeJoin::V V;
static tvsWyeJoin::Duct duct(V a,V b,double w){tvsWyeJoin::Duct d;d.a=a;d.b=b;d.width=w;return d;}
int main(){
    const double r=std::sqrt(0.5),s=40,L2=280;
    // inlet along +y ending at (0,0); Y45 200 / 140, branches start near (0, 500) going out at 45 deg
    const double yO=tvsTee::wyeAxisY(200,140,s,45);
    tvsWyeJoin::Duct in=duct(V(0,-2000),V(0,0),200),br[2];
    // branch axes through (0, 600) (exact O for inlet face at 600 - yO)
    br[0]=duct(V(10*r,600+10*r),V(1500*r,600+1500*r),140);     // right
    br[1]=duct(V(-1500*r,600+1500*r),V(-10*r,600+10*r),140);   // left, reversed order
    tvsWyeJoin::Plan p=tvsWyeJoin::wye(in,br,s,L2,1.0,50);
    check(p.error==tvsWyeJoin::Ok,"wye ok");check(p.moved==-1,"nothing moved");check(p.right==0,"right = branch 0");
    near(p.inletPort.y,600-yO,1e-6,"inlet face");near(p.inletPort.x,0,1e-9,"inlet x");
    near(p.base.y,600,1e-6,"base = O");near(p.u.x,1,1e-12,"u right");near(p.v.y,-1,1e-12,"v to inlet");
    near(p.port[0].x,L2*r,1e-6,"right face x");near(p.port[1].x,-L2*r,1e-6,"left face x");near(p.port[1].y,600+L2*r,1e-6,"left face y");
    check(p.inletNear==1&&p.branchNear[0]==0&&p.branchNear[1]==1,"near ends");
    // TVS_WYE port model matches the shape: base + v (w1/2 + LengthPl) = inlet face
    near(p.base.y-(70+p.lengthPl),p.inletPort.y,1e-6,"port model");
    // the left branch 100 off along the inlet: it is moved (narrower or equal: branch 0 kept? equal widths -> branch 1 moves)
    br[1]=duct(V(-1500*r,700+1500*r),V(-10*r,700+10*r),140);
    p=tvsWyeJoin::wye(in,br,s,L2,1.0,50);check(p.error==tvsWyeJoin::Ok&&p.moved==1,"moved left");near(p.shift.y,-100,1e-6,"shift");
    br[1].farJoined=true;p=tvsWyeJoin::wye(in,br,s,L2,1.0,50);check(p.error==tvsWyeJoin::Ok&&p.moved==0,"far joined: the other moves");near(p.shift.y,100,1e-6,"shift up");
    br[0].farJoined=true;p=tvsWyeJoin::wye(in,br,s,L2,1.0,50);check(p.error==tvsWyeJoin::BothJoined,"both joined");
    br[0].farJoined=br[1].farJoined=false;
    // not 45 deg
    tvsWyeJoin::Duct b2[2]={duct(V(10,600),V(1500,600),140),br[1]};p=tvsWyeJoin::wye(in,b2,s,L2,1.0,50);check(p.error==tvsWyeJoin::NotAngled,"90 deg refused");
    // same side
    b2[0]=br[0];b2[1]=br[0];b2[1].a.y+=300;b2[1].b.y+=300;p=tvsWyeJoin::wye(in,b2,s,L2,1.0,50);check(p.error==tvsWyeJoin::SameSide,"same side");
    // inlet too short
    tvsWyeJoin::Duct shortIn=duct(V(0,540),V(0,560),200);p=tvsWyeJoin::wye(shortIn,br,s,L2,1.0,50);check(p.error==tvsWyeJoin::InletShort,"inlet short");
    // user's drawing 2026-11-27: inlet along +x, two branches on one vertical line (perpendicular) -> Y45 + koljena 45
    {tvsWyeJoin::Duct in2=duct(V(-3000,0),V(-500,0),200),b3[2];
     b3[0]=duct(V(400,150),V(400,3000),150);b3[1]=duct(V(400,-150),V(400,-3000),150);
     const double T[2]={50,50},L=205,yO2=tvsTee::wyeAxisY(200,150,40,45);
     tvsWyeJoin::Plan q=tvsWyeJoin::wye(in2,b3,40,L,1.0,50,-1,T);
     check(q.error==tvsWyeJoin::Ok&&q.elbow[0]&&q.elbow[1]&&q.moved==-1,"perpendicular: elbows");
     check(q.right==1,"right = down");
     near(q.port[0].x,400,1e-6,"exit on branch line 0");near(q.port[1].x,400,1e-6,"exit on branch line 1");
     near(q.inletPort.x,400-(L+50)*r-yO2,1e-6,"inlet face");near(q.port[0].y,-q.port[1].y,1e-6,"symmetric");
     near(q.face[0].x-q.inletPort.x,yO2+L*r,1e-6,"Y45 face");
     near(q.base.x-q.inletPort.x,yO2,1e-6,"base = O");
     check(tvsWyeJoin::wye(in2,b3,40,L,1.0,50).error==tvsWyeJoin::NotAngled,"no T: refused");}
    // kosa: main 0..1000 and 1000..3000 (gap free), branch at 45 from (1500, 300)
    tvsWyeJoin::Duct d3[3]={duct(V(0,0),V(1000,0),250),duct(V(3000,0),V(1200,0),250),duct(V(1600+10*r,10*r),V(1600+1500*r,1500*r),160)};
    int m[2],b=-1;check(tvsWyeJoin::lateralParts(d3,1.0,m,b)&&b==2&&m[0]==0&&m[1]==1,"parts");
    tvsWyeJoin::Duct mm[2]={d3[m[0]],d3[m[1]]};
    tvsWyeJoin::Lateral l=tvsWyeJoin::lateral(mm,d3[b],200,150,300,1.0,50);
    check(l.error==tvsWyeJoin::Ok,"lateral ok");near(l.base.x,1600,1e-6,"X");near(l.run0.x,1400,1e-6,"run0");near(l.run1.x,1750,1e-6,"run1");
    check(l.up==0&&l.nearEnd[0]==1&&l.nearEnd[1]==1&&l.nearEnd[2]==0,"lateral ends");near(l.face.x,1600+300*r,1e-6,"face");
    tvsWyeJoin::Duct mx[2]={duct(V(0,0),V(1000,0),250),duct(V(3000,0),V(1200,0),200)};
    check(tvsWyeJoin::lateral(mx,d3[2],200,150,300,1.0,50).error==tvsWyeJoin::MainSizes,"main sizes");
    if(fails)return 1;
    std::printf("OK\n");return 0;
}

// MIRROR of vertical elbows (geometry/RoundElbow.h mirrorVertical): the mirrored plan ports of the
// old elbow are the plan ports of the new one, for 30/45/60/90 deg and mirror lines at any angle.
#include "../geometry/RoundElbow.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace tvsRoundElbow;
static bool eq(P a,P b){return std::fabs(a.x-b.x)<1e-7&&std::fabs(a.y-b.y)<1e-7;}
// mirror about the line through (px,py) at angle phi
static Map mirror(double px,double py,double phi){
    const double c=std::cos(2*phi),s=std::sin(2*phi);Map m;m.a=c;m.b=s;m.c=s;m.d=-c;
    m.tx=px-(c*px+s*py);m.ty=py-(s*px-c*py);return m;
}
int main(){
    const double deg=3.14159265358979323846/180;
    const double angles[4]={30,45,60,90};
    for(int a=0;a<4;++a)for(int h=0;h<12;++h)for(int l=0;l<6;++l){
        const double t=angles[a]*deg,sa=h*30*deg+7*deg,rc=250+40*l;
        const P mid(1000+13*h,-400+7*l),start(std::cos(sa)*2,std::sin(sa)*2);
        const Map m=mirror(300,-50,l*31*deg+5*deg);
        // round: ports of the mirrored elbow = mirrored ports
        P ma,mc;ports(mid,start.x,start.y,t,rc,ma,mc);
        P mid2,start2;mirrorVertical(true,mid,start,t,rc,m,mid2,start2);
        P ma2,mc2;ports(mid2,start2.x,start2.y,t,rc,ma2,mc2);
        assert(eq(ma2,apply(m,ma))&&eq(mc2,apply(m,mc)));
        assert(std::fabs(std::hypot(start2.x,start2.y)-2)<1e-12);
        // rectangular: MC = MiddlePoint, MA = M - u rc sin t
        mirrorVertical(false,mid,start,t,rc,m,mid2,start2);
        const double sb=std::atan2(start2.y,start2.x);
        const P raOld(mid.x+std::sin(sa)*rc*std::sin(t),mid.y-std::cos(sa)*rc*std::sin(t));
        const P raNew(mid2.x+std::sin(sb)*rc*std::sin(t),mid2.y-std::cos(sb)*rc*std::sin(t));
        assert(eq(mid2,apply(m,mid))&&eq(raNew,apply(m,raOld)));
    }
    std::puts("elbow mirror OK");
    return 0;
}

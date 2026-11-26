// Kosa stucna 45 (geometry/TeeShape.h): branch walls start on the main wall, the face is
// perpendicular to the branch axis at Basepoint + d (A/2 + LengthPl), the run covers the
// footprint with the margin, LengthPl <-> collar round trip.
#include "../geometry/TeeShape.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace tvsTee;
static bool eq(double a,double b){return std::fabs(a-b)<1e-9;}
int main(){
    const double As[3]={250,400,630},ws[3]={100,200,250},degs[3]={30,45,60};
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)for(int k=0;k<3;++k){
        Tee t;t.type=Tap;t.round=true;t.A=As[i];t.C=As[i];t.w1=ws[j];t.l=75;t.margin=25;t.lateral=degs[k];t.flange=10;
        if(t.w1>t.A)continue;
        assert(valid(t));Shape s;assert(shape(t,s));assert(s.lines.size()==5&&s.axis.size()==1);
        double before,after;tapRun(t,before,after);
        const double a=t.A/2,th=t.lateral*pi()/180,L=lateralLengthPl(t.A,t.w1,t.lateral,t.l);
        assert(eq(lateralCollar(t.A,t.w1,t.lateral,L),t.l));
        const P B(0,before),d(std::sin(th),std::cos(th));
        const P face(B.x+d.x*(a+L),B.y+d.y*(a+L));   // TVS_WYE branch port
        // walls: from x = a to the face line; parallel to d at distance w/2 from the axis
        for(int w=2;w<4;++w){const Seg& g=s.lines[w];assert(eq(g.a.x,a));
            const double vx=g.b.x-g.a.x,vy=g.b.y-g.a.y,len=std::hypot(vx,vy);assert(len>0&&eq(vx/len,d.x)&&eq(vy/len,d.y));
            const double off=(g.a.x-B.x)*d.y-(g.a.y-B.y)*d.x;assert(eq(std::fabs(off),t.w1/2));
            assert(eq((g.b.x-face.x)*d.x+(g.b.y-face.y)*d.y,0));}
        // downstream wall ends `l` beyond the main wall
        assert(eq(std::hypot(s.lines[3].b.x-s.lines[3].a.x,s.lines[3].b.y-s.lines[3].a.y),t.l));
        // axis to the face centre, face across it
        assert(eq(s.axis[0].b.x,face.x)&&eq(s.axis[0].b.y,face.y));
        // run: footprint + margin on both sides
        double y0,y1;lateralFootprint(t.A,t.w1,t.lateral,y0,y1);
        assert(eq(s.lines[2].a.y,before+y0)&&eq(s.lines[3].a.y,before+y1)&&eq(before+y0,25)&&eq(before+after-(before+y1),25));
        assert(eq(s.lines[0].b.y,before+after));
    }
    Tee bad;bad.type=Tap;bad.round=false;bad.A=400;bad.w1=200;bad.l=75;bad.lateral=45;assert(!valid(bad));   // round only
    std::puts("lateral tap OK");
    return 0;
}

// SDK-independent tests for the uzduzni presjek (geometry/Profile.h).
#include <cassert>
#include <cmath>
#define near
#include "../geometry/Profile.h"
using namespace tvsProfile;
static Member duct(P a,P b,double z,int tag,double A=400,double B=200){
    Member m;m.a=a;m.b=b;m.A1=m.A2=A;m.B1=m.B2=B;m.z=m.za=m.zb=z;m.tag=tag;m.kind=1;return m;}
static Member chord(P mid,P port,double z,int tag,int kind,double A=400,double B=200){
    Member m=duct(mid,port,z,tag,A,B);m.kind=kind;return m;}
static bool eq(double a,double b,double e=1e-6){return std::fabs(a-b)<=e;}
int main(){
    const double pi=3.14159265358979323846;
    std::vector<Member> ms;
    ms.push_back(duct(P(0,0),P(1000,0),2500,1));                      // 1 duct
    ms.push_back(chord(P(1200,0),P(1000,0),2500,2,2));                 // 2 plan elbow 90, R = 200
    ms.push_back(chord(P(1200,0),P(1200,200),2500,2,2));
    ms.push_back(duct(P(1200,1000),P(1200,200),2500,3));               // 3 duct drawn backwards
    Member s=duct(P(1200,1000),P(1200,1600),2500,4);s.sloped=true;s.za=2500;s.zb=2800;ms.push_back(s); // 4 sloped piece
    ms.push_back(duct(P(500,-500),P(500,500),2800,5,300,300));         // 5 crossing duct
    ms.push_back(duct(P(1000,0),P(1000,-800),2500,6));                 // 6 branch at the elbow port (joined, not crossing)
    std::vector<int> tags;
    assert(route(ms,1,4,2,tags));assert(tags.size()==4&&tags[0]==1&&tags[1]==2&&tags[2]==3&&tags[3]==4);
    std::vector<Piece> pc=develop(ms,tags,2);assert(pc.size()==4);
    assert(eq(pc[0].s0,0)&&eq(pc[0].s1,1000));
    assert(pc[1].kind==Elbow&&eq(pc[1].s1-pc[1].s0,200*pi/2));
    assert(eq(pc[2].s1-pc[2].s0,800)&&eq(pc[2].z0,2500));
    assert(pc[3].sloped()&&eq(pc[3].z0,2500)&&eq(pc[3].z1,2800));
    std::vector<Cross> x=crossings(ms,tags,pc,2);assert(x.size()==1&&eq(x[0].s,500)&&eq(x[0].c.z,2800)&&x[0].c.tag==5);
    double z=0;assert(bottomAt(pc,1000,1e-6,z)&&eq(z,2400));
    assert(bottomAt(pc,pc.back().s1,1e-6,z)&&eq(z,2700));
    std::vector<double> st=stations(pc,50);assert(st.size()==5&&eq(st.front(),0)&&eq(st.back(),pc.back().s1));
    // outline of the sloped piece: thickness perpendicular to the axis
    std::vector<P> o=outline(pc[3]);const double l=std::hypot(600.0,300.0);
    assert(eq(o[3].x-o[0].x,-300/l*200)&&eq(o[3].y-o[0].y,600/l*200));
    // reversed route: same lengths
    assert(route(ms,4,1,2,tags));pc=develop(ms,tags,2);assert(eq(pc.back().s1,1000+200*pi/2+800+600));
    // T komad straight through and into the branch
    std::vector<Member> t;
    t.push_back(duct(P(0,0),P(1000,0),2500,1));
    t.push_back(chord(P(1200,0),P(1000,0),2500,2,4));t.push_back(chord(P(1200,0),P(1400,0),2500,2,4));t.push_back(chord(P(1200,0),P(1200,300),2500,2,4,200,200));
    t.push_back(duct(P(1400,0),P(2000,0),2500,3));t.push_back(duct(P(1200,300),P(1200,900),2500,4,200,200));
    assert(route(t,1,3,2,tags)&&tags.size()==3);pc=develop(t,tags,2);assert(pc[1].kind==Tee&&eq(pc[1].s1-pc[1].s0,400)&&eq(pc.back().s1,2000));
    assert(route(t,1,4,2,tags)&&tags.size()==3);pc=develop(t,tags,2);assert(eq(pc[1].s1-pc[1].s0,200+300)&&eq(pc.back().s1,1000+500+600));
    assert(!route(t,1,99,2,tags));
    // elements at different z at the same plan point are not joined
    std::vector<Member> r;r.push_back(duct(P(0,0),P(1000,0),2500,1));r.push_back(duct(P(1000,0),P(2000,0),3000,2));
    assert(!route(r,1,2,2,tags));
    return 0;
}

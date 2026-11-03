#pragma once
// Poprecni presjek (TVS_PRESJEK, faza P1): which plan members of the TVS network a section
// line cuts, and where. Plan coordinates in drawing units; a member is one straight piece
// (duct, transition, or a chord of an elbow / T komad from its middle to a port) with the
// duct section at its ends and the axis elevation z. No AutoCAD dependency (tests).
#include <vector>
#include <cmath>
#include <algorithm>
namespace tvsSection {
struct P {double x,y;P(double a=0,double b=0):x(a),y(b){}};
inline double cross(const P& a,const P& b){return a.x*b.y-a.y*b.x;}
struct Member {P a,b;double A1,B1,A2,B2;bool round;double z;int tag;
    Member():A1(0),B1(0),A2(0),B2(0),round(false),z(0),tag(0){}};
struct Cut {double s,w,h,A,B,z,sinA;bool round;int tag;Cut():s(0),w(0),h(0),A(0),B(0),z(0),sinA(1),round(false),tag(0){}};
// Section line p1 -> p2 against one member. s = distance from p1 along the line. In the
// section the duct appears A / sin(angle) wide (oblique cut), limited to 5 A.
inline bool cut(const P& p1,const P& p2,const Member& m,Cut& c){
    const P d(p2.x-p1.x,p2.y-p1.y),e(m.b.x-m.a.x,m.b.y-m.a.y);
    const double ld=std::sqrt(d.x*d.x+d.y*d.y),le=std::sqrt(e.x*e.x+e.y*e.y);
    if(ld<1e-12||le<1e-12)return false;
    const double den=cross(d,e);if(std::fabs(den)<1e-9*ld*le)return false;   // parallel
    const P w(m.a.x-p1.x,m.a.y-p1.y);
    const double t=cross(w,e)/den,u=cross(w,d)/den;
    if(t<0||t>1||u<0||u>1)return false;
    c=Cut();c.s=t*ld;c.sinA=std::fabs(den)/(ld*le);c.tag=m.tag;c.round=m.round;c.z=m.z;
    c.A=m.A1+(m.A2-m.A1)*u;c.B=m.B1+(m.B2-m.B1)*u;
    const double k=c.sinA<0.2?0.2:c.sinA;
    c.w=c.A/k;c.h=m.round?c.A:c.B;
    return true;
}
// All cuts, ordered along the view (flip = looking from the other side: s measured from p2),
// one per element tag at the same place (elbow chords meet at the middle).
inline std::vector<Cut> cuts(const P& p1,const P& p2,const std::vector<Member>& members,bool flip,double tol){
    std::vector<Cut> out;const double L=std::sqrt((p2.x-p1.x)*(p2.x-p1.x)+(p2.y-p1.y)*(p2.y-p1.y));
    for(size_t i=0;i<members.size();++i){Cut c;if(!cut(p1,p2,members[i],c))continue;if(flip)c.s=L-c.s;
        bool dup=false;for(size_t j=0;j<out.size();++j)if(out[j].tag==c.tag&&std::fabs(out[j].s-c.s)<=tol&&std::fabs(out[j].z-c.z)<=tol)dup=true;
        if(!dup)out.push_back(c);}
    std::sort(out.begin(),out.end(),[](const Cut& a,const Cut& b){return a.s<b.s;});
    return out;
}
// Ducts whose section boxes overlap in the view (same place, crossing elevations).
inline bool overlap(const Cut& a,const Cut& b){
    return std::fabs(a.s-b.s)*2<a.w+b.w&&std::fabs(a.z-b.z)*2<a.h+b.h;
}
}

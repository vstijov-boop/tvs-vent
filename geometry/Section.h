#pragma once
// Poprecni presjek (TVS_PRESJEK): which plan members of the TVS network a section line cuts,
// and what is seen behind it. Plan coordinates in drawing units; a member is one straight
// piece (duct, transition, or a chord of an elbow / T komad from its middle to a port) with
// the duct section at its ends and the axis elevation at its ends (za -> zb: sloped pieces of
// a Z prelaz / obilazak; za = zb for level ducts). A member with no plan length and za != zb
// is a vertical duct. No AutoCAD dependency (tests).
#include <vector>
#include <cmath>
#include <algorithm>
#include <utility>
namespace tvsSection {
struct P {double x,y;P(double a=0,double b=0):x(a),y(b){}};
inline double cross(const P& a,const P& b){return a.x*b.y-a.y*b.x;}
struct Member {P a,b;double A1,B1,A2,B2;bool round;double z,za,zb;bool sloped;int tag;
    Member():A1(0),B1(0),A2(0),B2(0),round(false),z(0),za(0),zb(0),sloped(false),tag(0){}
    double zAt(double u)const{return sloped?za+(zb-za)*u:z;}
    double planLength()const{return std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y));}
    bool vertical(double tol)const{return planLength()<=tol&&sloped&&std::fabs(zb-za)>tol;}};
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
    c=Cut();c.s=t*ld;c.sinA=std::fabs(den)/(ld*le);c.tag=m.tag;c.round=m.round;c.z=m.zAt(u);
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

// ---- Dubina presjeka: members behind the section plane, inside the zone p1, p2,
// p2 + n*depth, p1 + n*depth (n = looking direction). Each is seen as a quad in the section:
// x0..x1 along the section, axis elevation zc0 at x0 and zc1 at x1 (sloped pieces), height h.
struct View {double x0,x1,zc0,zc1,h,closest;int tag;bool round,vertical;
    View():x0(0),x1(0),zc0(0),zc1(0),h(0),closest(0),tag(0),round(false),vertical(false){}
    double zc(double x)const{return x1-x0>1e-12?zc0+(zc1-zc0)*(x-x0)/(x1-x0):zc0;}
    bool inside(double x,double z,double eps)const{return x>x0+eps&&x<x1-eps&&std::fabs(z-zc(x))<h/2-eps;}};
struct Edge {double x0,y0,x1,y1;bool hidden;Edge(double a=0,double b=0,double c=0,double d=0,bool hh=false):x0(a),y0(b),x1(c),y1(d),hidden(hh){}};
inline P unit(const P& v){const double l=std::sqrt(v.x*v.x+v.y*v.y);return l>0?P(v.x/l,v.y/l):P();}
// Looking direction: left of p1->p2, or right when flipped (as in the section view).
inline P lookDir(const P& p1,const P& p2,bool flip){const P d=unit(P(p2.x-p1.x,p2.y-p1.y));return flip?P(d.y,-d.x):P(-d.y,d.x);}
inline std::vector<View> views(const P& p1,const P& p2,const std::vector<Member>& members,bool flip,double depth,double tol){
    std::vector<View> out;
    const P d=unit(P(p2.x-p1.x,p2.y-p1.y)),n=lookDir(p1,p2,flip);
    const double L=std::sqrt((p2.x-p1.x)*(p2.x-p1.x)+(p2.y-p1.y)*(p2.y-p1.y));
    for(size_t i=0;i<members.size();++i){const Member& m=members[i];
        const double A=(m.A1+m.A2)/2,B=m.round?A:(m.B1+m.B2)/2;
        double ua=(m.a.x-p1.x)*d.x+(m.a.y-p1.y)*d.y,va=(m.a.x-p1.x)*n.x+(m.a.y-p1.y)*n.y;
        double ub=(m.b.x-p1.x)*d.x+(m.b.y-p1.y)*d.y,vb=(m.b.x-p1.x)*n.x+(m.b.y-p1.y)*n.y;
        if(m.vertical(tol)){   // riser: seen when its footprint reaches the zone (also at the plane)
            const double v=(va+vb)/2,u=(ua+ub)/2;if(v<-A/2||v>depth+A/2)continue;
            double lo=u-A/2,hi=u+A/2;if(hi<0||lo>L)continue;
            lo=(std::max)(lo,0.0);hi=(std::min)(hi,L);
            View w;w.tag=m.tag;w.round=m.round;w.vertical=true;w.closest=(std::max)(v-A/2,0.0);
            w.x0=flip?L-hi:lo;w.x1=flip?L-lo:hi;w.zc0=w.zc1=(m.za+m.zb)/2;w.h=std::fabs(m.zb-m.za);
            out.push_back(w);continue;
        }
        if(!(depth>0))continue;
        // clip the axis to 0 <= v <= depth
        double t0=0,t1=1;const double dv=vb-va;
        if(std::fabs(dv)<1e-12){if(va<0||va>depth)continue;}
        else{double ta=(0-va)/dv,tb=(depth-va)/dv;if(ta>tb)std::swap(ta,tb);t0=(std::max)(t0,ta);t1=(std::min)(t1,tb);if(t0>t1)continue;}
        const double u0=ua+(ub-ua)*t0,u1=ua+(ub-ua)*t1,v0=va+dv*t0,v1=va+dv*t1;
        const double z0=m.zAt(t0),z1=m.zAt(t1);
        // footprint: axis ends +- half width across the member (|sin| member / section)
        const P e=unit(P(m.b.x-m.a.x,m.b.y-m.a.y));const double across=std::fabs(e.x*d.y-e.y*d.x);
        const bool rising=u1>=u0;const double ulo=rising?u0:u1,uhi=rising?u1:u0,zlo=rising?z0:z1,zhi=rising?z1:z0;
        double lo=ulo-A/2*across,hi=uhi+A/2*across;
        if(hi<0||lo>L)continue;
        lo=(std::max)(lo,0.0);hi=(std::min)(hi,L);
        if(hi-lo<1e-9)continue;
        // axis elevation over the seen interval (slope only along the axis projection)
        auto zAtU=[&](double u){return uhi-ulo>1e-12?zlo+(zhi-zlo)*((std::min)((std::max)(u,ulo),uhi)-ulo)/(uhi-ulo):zlo;};
        View w;w.tag=m.tag;w.round=m.round;w.closest=(std::min)(v0,v1);w.h=B;
        if(!flip){w.x0=lo;w.x1=hi;w.zc0=zAtU(lo);w.zc1=zAtU(hi);}
        else{w.x0=L-hi;w.x1=L-lo;w.zc0=zAtU(hi);w.zc1=zAtU(lo);}
        out.push_back(w);
    }
    std::sort(out.begin(),out.end(),[](const View& a,const View& b){return a.closest<b.closest;});
    return out;
}
// Outline edges of the views; a part of an edge is hidden when a nearer quad (another
// element, or a cut at the section plane) covers it. Quads of one element do not hide each
// other. Edges are sampled, so sloped pieces are handled like level ones.
inline std::vector<Edge> edges(const std::vector<Cut>& cuts,const std::vector<View>& views){
    std::vector<View> cover(views);
    for(size_t i=0;i<cuts.size();++i){View c;c.x0=cuts[i].s-cuts[i].w/2;c.x1=cuts[i].s+cuts[i].w/2;c.zc0=c.zc1=cuts[i].z;c.h=cuts[i].h;c.closest=-1;c.tag=cuts[i].tag;cover.push_back(c);}
    std::vector<Edge> out;const double eps=1e-9;const int N=64;
    for(size_t i=0;i<views.size();++i){const View& v=views[i];
        const double ends[4][4]={{v.x0,v.zc0-v.h/2,v.x1,v.zc1-v.h/2},{v.x0,v.zc0+v.h/2,v.x1,v.zc1+v.h/2},
                                 {v.x0,v.zc0-v.h/2,v.x0,v.zc0+v.h/2},{v.x1,v.zc1-v.h/2,v.x1,v.zc1+v.h/2}};
        for(int k=0;k<4;++k){
            const double ax=ends[k][0],az=ends[k][1],bx=ends[k][2],bz=ends[k][3];
            auto hiddenAt=[&](double t){const double x=ax+(bx-ax)*t,z=az+(bz-az)*t;
                for(size_t j=0;j<cover.size();++j){const View& o=cover[j];if(o.tag==v.tag||!(o.closest<v.closest-eps))continue;if(o.inside(x,z,eps))return true;}
                return false;};
            double from=0;bool state=hiddenAt(0.5/N);
            for(int s=1;s<=N;++s){
                const bool hid=s<N?hiddenAt((s+0.5)/N):state;
                if(s<N&&hid==state)continue;
                double to=1;
                if(s<N){double lo=(s-0.5)/N,hi=(s+0.5)/N;   // exact change point by bisection
                    for(int it=0;it<40;++it){const double mid=(lo+hi)/2;if(hiddenAt(mid)==state)lo=mid;else hi=mid;}to=(lo+hi)/2;}
                out.push_back(Edge(ax+(bx-ax)*from,az+(bz-az)*from,ax+(bx-ax)*to,az+(bz-az)*to,state));
                from=to;state=hid;
            }
        }
    }
    return out;
}
}

#pragma once
// Poprecni presjek (TVS_PRESJEK, faza P1): which plan members of the TVS network a section
// line cuts, and where. Plan coordinates in drawing units; a member is one straight piece
// (duct, transition, or a chord of an elbow / T komad from its middle to a port) with the
// duct section at its ends and the axis elevation z. No AutoCAD dependency (tests).
#include <vector>
#include <cmath>
#include <algorithm>
#include <utility>
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

// ---- Dubina presjeka (P1b): members behind the section plane, inside the zone
// p1, p2, p2 + n*depth, p1 + n*depth (n = looking direction), seen as boxes in the section.
struct View {double x0,x1,z0,z1,near;int tag;bool round;View():x0(0),x1(0),z0(0),z1(0),near(0),tag(0),round(false){}};
struct Edge {double x0,y0,x1,y1;bool hidden;Edge(double a=0,double b=0,double c=0,double d=0,bool h=false):x0(a),y0(b),x1(c),y1(d),hidden(h){}};
inline P unit(const P& v){const double l=std::sqrt(v.x*v.x+v.y*v.y);return l>0?P(v.x/l,v.y/l):P();}
// Looking direction: left of p1->p2, or right when flipped (as in the section view).
inline P lookDir(const P& p1,const P& p2,bool flip){const P d=unit(P(p2.x-p1.x,p2.y-p1.y));return flip?P(d.y,-d.x):P(-d.y,d.x);}
inline std::vector<View> views(const P& p1,const P& p2,const std::vector<Member>& members,bool flip,double depth){
    std::vector<View> out;if(!(depth>0))return out;
    const P d=unit(P(p2.x-p1.x,p2.y-p1.y)),n=lookDir(p1,p2,flip);
    const double L=std::sqrt((p2.x-p1.x)*(p2.x-p1.x)+(p2.y-p1.y)*(p2.y-p1.y));
    for(size_t i=0;i<members.size();++i){const Member& m=members[i];
        double ua=(m.a.x-p1.x)*d.x+(m.a.y-p1.y)*d.y,va=(m.a.x-p1.x)*n.x+(m.a.y-p1.y)*n.y;
        double ub=(m.b.x-p1.x)*d.x+(m.b.y-p1.y)*d.y,vb=(m.b.x-p1.x)*n.x+(m.b.y-p1.y)*n.y;
        // clip the axis to 0 <= v <= depth
        double t0=0,t1=1;const double dv=vb-va;
        if(std::fabs(dv)<1e-12){if(va<0||va>depth)continue;}
        else{double ta=(0-va)/dv,tb=(depth-va)/dv;if(ta>tb)std::swap(ta,tb);t0=(std::max)(t0,ta);t1=(std::min)(t1,tb);if(t0>t1)continue;}
        const double u0=ua+(ub-ua)*t0,u1=ua+(ub-ua)*t1,v0=va+dv*t0,v1=va+dv*t1;
        const double A=(m.A1+m.A2)/2,B=m.round?A:(m.B1+m.B2)/2;
        // footprint corners: axis ends +- half width across the member
        const P e=unit(P(m.b.x-m.a.x,m.b.y-m.a.y));const double across=std::fabs(e.x*d.y-e.y*d.x);   // |sin| member / section
        double lo=(std::min)(u0,u1)-A/2*across,hi=(std::max)(u0,u1)+A/2*across;
        if(hi<0||lo>L)continue;
        lo=(std::max)(lo,0.0);hi=(std::min)(hi,L);
        if(hi-lo<1e-9)continue;
        View w;w.tag=m.tag;w.round=m.round;w.near=(std::min)(v0,v1);
        w.x0=flip?L-hi:lo;w.x1=flip?L-lo:hi;w.z0=m.z-B/2;w.z1=m.z+B/2;out.push_back(w);
    }
    std::sort(out.begin(),out.end(),[](const View& a,const View& b){return a.near<b.near;});
    return out;
}
// Splits [a,b] into pieces covered / not covered by the intervals `cover` (merged inside).
inline void split(double a,double b,std::vector<std::pair<double,double> > cover,std::vector<std::pair<double,double> >& seen,std::vector<std::pair<double,double> >& hid){
    std::sort(cover.begin(),cover.end());double x=a;
    for(size_t i=0;i<cover.size()&&x<b;++i){double c0=(std::max)(cover[i].first,a),c1=(std::min)(cover[i].second,b);
        if(c1<=x)continue;
        if(c0>x){seen.push_back(std::make_pair(x,c0));x=c0;}
        if(c1>x){hid.push_back(std::make_pair(x,c1));x=c1;}}
    if(x<b)seen.push_back(std::make_pair(x,b));
}
// Outline edges of the views; a part of an edge is hidden when a nearer box (another element,
// or a cut at the section plane) covers it. Boxes of the same element do not hide each other.
inline std::vector<Edge> edges(const std::vector<Cut>& cuts,const std::vector<View>& views){
    struct Box {double x0,x1,z0,z1,near;int tag;};std::vector<Box> boxes;
    for(size_t i=0;i<cuts.size();++i){Box b={cuts[i].s-cuts[i].w/2,cuts[i].s+cuts[i].w/2,cuts[i].z-cuts[i].h/2,cuts[i].z+cuts[i].h/2,-1,cuts[i].tag};boxes.push_back(b);}
    for(size_t i=0;i<views.size();++i){Box b={views[i].x0,views[i].x1,views[i].z0,views[i].z1,views[i].near,views[i].tag};boxes.push_back(b);}
    std::vector<Edge> out;const double eps=1e-9;
    for(size_t i=0;i<views.size();++i){const View& v=views[i];
        const double ex[4][4]={{v.x0,v.z0,v.x1,v.z0},{v.x0,v.z1,v.x1,v.z1},{v.x0,v.z0,v.x0,v.z1},{v.x1,v.z0,v.x1,v.z1}};
        for(int k=0;k<4;++k){const bool horizontal=k<2;const double fixed=horizontal?ex[k][1]:ex[k][0];
            const double a=horizontal?ex[k][0]:ex[k][1],b=horizontal?ex[k][2]:ex[k][3];
            std::vector<std::pair<double,double> > cover;
            for(size_t j=0;j<boxes.size();++j){const Box& o=boxes[j];if(o.tag==v.tag||!(o.near<v.near-eps))continue;
                if(horizontal){if(fixed>o.z0+eps&&fixed<o.z1-eps)cover.push_back(std::make_pair(o.x0,o.x1));}
                else{if(fixed>o.x0+eps&&fixed<o.x1-eps)cover.push_back(std::make_pair(o.z0,o.z1));}}
            std::vector<std::pair<double,double> > seen,hid;split(a,b,cover,seen,hid);
            for(size_t q=0;q<seen.size();++q)out.push_back(horizontal?Edge(seen[q].first,fixed,seen[q].second,fixed,false):Edge(fixed,seen[q].first,fixed,seen[q].second,false));
            for(size_t q=0;q<hid.size();++q)out.push_back(horizontal?Edge(hid[q].first,fixed,hid[q].second,fixed,true):Edge(fixed,hid[q].first,fixed,hid[q].second,true));
        }
    }
    return out;
}
}

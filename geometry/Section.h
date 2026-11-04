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
// bend: vertical elbow (Gore / Nadolje) between a and b; levelA = its level end is a.
// taper: prelaz (height and axis change along it, end faces vertical).
// dir: plan direction of the duct when it differs from a->b (prelaz uz jednu ivicu: its axis is
// skewed, its faces are square to the duct direction); (0,0) = a->b.
struct Member {P a,b,dir;double A1,B1,A2,B2;bool round;double z,za,zb;bool sloped,bend,levelA,taper;int tag;
    Member():A1(0),B1(0),A2(0),B2(0),round(false),z(0),za(0),zb(0),sloped(false),bend(false),levelA(true),taper(false),tag(0){}
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
// p2 + n*depth, p1 + n*depth (n = looking direction). Each is seen as a closed outline in the
// section (lines and true arcs): straight pieces as quads (sloped ones too), vertical elbows
// as arc bands, risers as vertical strips.
struct Prim {bool arc;double x0,y0,x1,y1,cx,cy,r,a0,a1,k;   // arc: (cx + k r cos a, cy + r sin a), a0 -> a1
    Prim():arc(false),x0(0),y0(0),x1(0),y1(0),cx(0),cy(0),r(0),a0(0),a1(0),k(1){}
    static Prim line(double ax,double ay,double bx,double by){Prim q;q.x0=ax;q.y0=ay;q.x1=bx;q.y1=by;return q;}
    static Prim arcOf(double cx,double cy,double r,double a0,double a1,double k){Prim q;q.arc=true;q.cx=cx;q.cy=cy;q.r=r;q.a0=a0;q.a1=a1;q.k=k;return q;}
    P at(double t)const{if(!arc)return P(x0+(x1-x0)*t,y0+(y1-y0)*t);const double a=a0+(a1-a0)*t;return P(cx+k*r*std::cos(a),cy+r*std::sin(a));}
    Prim sub(double t0,double t1)const{if(!arc){const P p=at(t0),q=at(t1);return line(p.x,p.y,q.x,q.y);}Prim q=*this;q.a0=a0+(a1-a0)*t0;q.a1=a0+(a1-a0)*t1;return q;}
};
// h1 > 0: height at x1 differs from h (prelaz seen from the side: trapezoid).
struct View {double x0,x1,zc0,zc1,h,h1,closest;int tag;bool round,vertical,bend;std::vector<Prim> outline;std::vector<P> poly;
    View():x0(0),x1(0),zc0(0),zc1(0),h(0),h1(0),closest(0),tag(0),round(false),vertical(false),bend(false){}
    bool level()const{return !bend&&!vertical&&std::fabs(zc0-zc1)<1e-9&&!(h1>0&&std::fabs(h1-h)>1e-9);}
    double zc(double x)const{return x1-x0>1e-12?zc0+(zc1-zc0)*(x-x0)/(x1-x0):zc0;}
    void quad(){outline.clear();const double e=h1>0?h1:h;
        outline.push_back(Prim::line(x0,zc0-h/2,x1,zc1-e/2));outline.push_back(Prim::line(x1,zc1-e/2,x1,zc1+e/2));
        outline.push_back(Prim::line(x1,zc1+e/2,x0,zc0+h/2));outline.push_back(Prim::line(x0,zc0+h/2,x0,zc0-h/2));seal();}
    void seal(){poly.clear();for(size_t i=0;i<outline.size();++i){const int n=outline[i].arc?24:1;for(int k=0;k<n;++k)poly.push_back(outline[i].at(double(k)/n));}
        if(!poly.empty()){x0=x1=poly[0].x;for(size_t i=1;i<poly.size();++i){x0=(std::min)(x0,poly[i].x);x1=(std::max)(x1,poly[i].x);}}}
    bool inside(double x,double z,double eps)const{   // strictly inside the outline
        bool in=false;for(size_t i=0,j=poly.size()-1;i<poly.size();j=i++){const P& a=poly[i];const P& b=poly[j];
            if((a.y>z)!=(b.y>z)){const double xc=a.x+(z-a.y)*(b.x-a.x)/(b.y-a.y);if(x<xc)in=!in;}}
        if(!in)return false;
        for(size_t i=0,j=poly.size()-1;i<poly.size();j=i++){const P& a=poly[i];const P& b=poly[j];   // not on the boundary
            const double dx=b.x-a.x,dy=b.y-a.y,l2=dx*dx+dy*dy;double t=l2>0?((x-a.x)*dx+(z-a.y)*dy)/l2:0;t=t<0?0:t>1?1:t;
            if(std::hypot(a.x+dx*t-x,a.y+dy*t-z)<=eps*(1+std::fabs(x)+std::fabs(z)))return false;}
        return true;}
};
struct Edge {Prim g;bool hidden;Edge():hidden(false){}Edge(const Prim& p,bool h):g(p),hidden(h){}};
inline P unit(const P& v){const double l=std::sqrt(v.x*v.x+v.y*v.y);return l>0?P(v.x/l,v.y/l):P();}
// Looking direction: left of p1->p2, or right when flipped (as in the section view).
inline P lookDir(const P& p1,const P& p2,bool flip){const P d=unit(P(p2.x-p1.x,p2.y-p1.y));return flip?P(d.y,-d.x):P(-d.y,d.x);}
// Vertical elbow seen from the side: circle through both axis ends, tangent level at its level
// end; band of the duct height around it. kx = plan direction projected on the section axis.
inline bool bendView(const Member& m,double xa,double kx,double closest,View& w){
    const double Ls=m.planLength(),h=m.round?(m.A1+m.A2)/2:(m.B1+m.B2)/2;
    double Ss=0,Sz=m.za,Ts=Ls,Tz=m.zb,tan=1;if(!m.levelA){Ss=Ls;Sz=m.zb;Ts=0;Tz=m.za;tan=-1;}
    const double dz=Tz-Sz;if(std::fabs(dz)<1e-9||Ls<1e-9)return false;
    const double nz=dz>0?1:-1,ds=Ts-Ss,r=(ds*ds+dz*dz)/(2*dz*nz);
    const double Cs=Ss,Cz=Sz+nz*r;(void)tan;
    const double aS=std::atan2(Sz-Cz,Ss-Cs);double aT=std::atan2(Tz-Cz,Ts-Cs),dA=aT-aS;
    const double pi=3.14159265358979323846;while(dA>pi)dA-=2*pi;while(dA<=-pi)dA+=2*pi;aT=aS+dA;
    const double cx=xa+Cs*kx,ro=r+h/2,ri=(std::max)(r-h/2,0.0);
    w=View();w.tag=m.tag;w.round=m.round;w.bend=true;w.closest=closest;w.h=h;w.zc0=Sz;w.zc1=Tz;
    w.outline.push_back(Prim::arcOf(cx,Cz,ro,aS,aT,kx));
    {const P o=w.outline.back().at(1);const Prim in=Prim::arcOf(cx,Cz,ri,aT,aS,kx);const P i0=in.at(0);
     w.outline.push_back(Prim::line(o.x,o.y,i0.x,i0.y));w.outline.push_back(in);
     const P i1=in.at(1),o0=w.outline[0].at(0);w.outline.push_back(Prim::line(i1.x,i1.y,o0.x,o0.y));}
    w.seal();return true;
}
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
            w.x0=flip?L-hi:lo;w.x1=flip?L-lo:hi;w.zc0=w.zc1=(m.za+m.zb)/2;w.h=std::fabs(m.zb-m.za);w.quad();
            out.push_back(w);continue;
        }
        if(m.bend){   // vertical elbow: whole arc band when any part is in the zone
            if((std::min)(va,vb)>depth||(std::max)(va,vb)<-A/2)continue;
            const P e=unit(P(m.b.x-m.a.x,m.b.y-m.a.y));double kx=e.x*d.x+e.y*d.y;
            if(std::fabs(kx)<0.05)continue;                          // seen end-on: covered by the ducts it joins
            double xa=ua;if(flip){xa=L-ua;kx=-kx;}
            View w;if(bendView(m,xa,kx,(std::max)((std::min)(va,vb),0.0),w)&&w.x1>0&&w.x0<L)out.push_back(w);
            continue;
        }
        if(!(depth>0))continue;
        // clip the axis to 0 <= v <= depth
        double t0=0,t1=1;const double dv=vb-va;
        if(std::fabs(dv)<1e-12){if(va<0||va>depth)continue;}
        else{double ta=(0-va)/dv,tb=(depth-va)/dv;if(ta>tb)std::swap(ta,tb);t0=(std::max)(t0,ta);t1=(std::min)(t1,tb);if(t0>t1)continue;}
        const double u0=ua+(ub-ua)*t0,u1=ua+(ub-ua)*t1,v0=va+dv*t0,v1=va+dv*t1;
        const double z0=m.zAt(t0),z1=m.zAt(t1);
        // footprint: axis ends +- half width across the member (|sin| member / section)
        const P e=unit(m.dir.x!=0||m.dir.y!=0?m.dir:P(m.b.x-m.a.x,m.b.y-m.a.y));const double across=std::fabs(e.x*d.y-e.y*d.x);
        const bool rising=u1>=u0;const double ulo=rising?u0:u1,uhi=rising?u1:u0,zlo=rising?z0:z1,zhi=rising?z1:z0;
        double lo=ulo-A/2*across,hi=uhi+A/2*across;
        if(hi<0||lo>L)continue;
        lo=(std::max)(lo,0.0);hi=(std::min)(hi,L);
        if(hi-lo<1e-9)continue;
        auto zAtU=[&](double u){return uhi-ulo>1e-12?zlo+(zhi-zlo)*((std::min)((std::max)(u,ulo),uhi)-ulo)/(uhi-ulo):zlo;};
        View w;w.tag=m.tag;w.round=m.round;w.closest=(std::min)(v0,v1);w.h=B;
        if(!flip){w.x0=lo;w.x1=hi;w.zc0=zAtU(lo);w.zc1=zAtU(hi);}
        else{w.x0=L-hi;w.x1=L-lo;w.zc0=zAtU(hi);w.zc1=zAtU(lo);}
        if(m.taper){   // height along the transition, at the seen ends
            auto hAt=[&](double t){return m.round?m.A1+(m.A2-m.A1)*t:m.B1+(m.B2-m.B1)*t;};
            const double hlo=rising?hAt(t0):hAt(t1),hhi=rising?hAt(t1):hAt(t0);
            w.h=flip?hhi:hlo;w.h1=flip?hlo:hhi;
        }
        double kx=e.x*d.x+e.y*d.y;if(flip)kx=-kx;
        if(!m.taper&&m.sloped&&std::fabs(w.zc1-w.zc0)>1e-9&&std::fabs(kx)>0.05){
            // sloped piece: thickness and end faces perpendicular to its axis (true shape in the
            // member's vertical plane, projected: plan offsets scale by kx)
            const double ax0=flip?L-u0:u0,ax1=flip?L-u1:u1,ds=(ax1-ax0)/kx,dz=z1-z0,len=std::sqrt(ds*ds+dz*dz);
            if(len>1e-12){const double ns=-dz/len*B/2,nz=ds/len*B/2,ox=ns*kx;
                w.outline.clear();
                w.outline.push_back(Prim::line(ax0-ox,z0-nz,ax1-ox,z1-nz));w.outline.push_back(Prim::line(ax1-ox,z1-nz,ax1+ox,z1+nz));
                w.outline.push_back(Prim::line(ax1+ox,z1+nz,ax0+ox,z0+nz));w.outline.push_back(Prim::line(ax0+ox,z0+nz,ax0-ox,z0-nz));
                w.seal();out.push_back(w);continue;}
        }
        w.quad();out.push_back(w);
    }
    // one outline per element at one level (elbow / T komad chords -> one box, no inner lines)
    std::vector<View> merged;std::vector<int> used(out.size(),0);
    for(size_t i=0;i<out.size();++i){if(used[i])continue;used[i]=1;View w=out[i];
        if(w.level()){bool grown=true;while(grown){grown=false;
            for(size_t j=0;j<out.size();++j){if(used[j])continue;const View& o=out[j];
                if(o.tag==w.tag&&o.level()&&std::fabs(o.zc0-w.zc0)<=tol&&std::fabs(o.h-w.h)<=tol&&o.x0<=w.x1+tol&&o.x1>=w.x0-tol){
                    w.x0=(std::min)(w.x0,o.x0);w.x1=(std::max)(w.x1,o.x1);w.closest=(std::min)(w.closest,o.closest);used[j]=1;grown=true;}}}
            w.quad();}
        merged.push_back(w);}
    std::sort(merged.begin(),merged.end(),[](const View& a,const View& b){return a.closest<b.closest;});
    return merged;
}
// Section boxes of the cuts as covering outlines (at the plane: in front of everything).
inline std::vector<View> cover(const std::vector<Cut>& cuts,const std::vector<View>& views){
    std::vector<View> c(views);
    for(size_t i=0;i<cuts.size();++i){View q;q.x0=cuts[i].s-cuts[i].w/2;q.x1=cuts[i].s+cuts[i].w/2;q.zc0=q.zc1=cuts[i].z;q.h=cuts[i].h;q.closest=-1;q.tag=cuts[i].tag;q.quad();c.push_back(q);}
    return c;
}
inline bool covered(const std::vector<View>& c,int tag,double closest,double x,double z){
    for(size_t j=0;j<c.size();++j){const View& o=c[j];if(o.tag==tag||!(o.closest<closest-1e-9))continue;if(o.inside(x,z,1e-9))return true;}
    return false;
}
// Outline primitives of the views; a part is hidden when a nearer outline (another element, or
// a cut at the section plane) covers it. Change points are refined; arcs stay arcs.
inline std::vector<Edge> edges(const std::vector<Cut>& cuts,const std::vector<View>& views){
    const std::vector<View> c=cover(cuts,views);std::vector<Edge> out;const int N=64;
    for(size_t i=0;i<views.size();++i){const View& v=views[i];
        for(size_t k=0;k<v.outline.size();++k){const Prim& g=v.outline[k];
            auto hiddenAt=[&](double t){const P q=g.at(t);return covered(c,v.tag,v.closest,q.x,q.y);};
            double from=0;bool state=hiddenAt(0.5/N);
            for(int s=1;s<=N;++s){
                const bool hid=s<N?hiddenAt((s+0.5)/N):state;
                if(s<N&&hid==state)continue;
                double to=1;
                if(s<N){double lo=(s-0.5)/N,hi=(s+0.5)/N;
                    for(int it=0;it<40;++it){const double mid=(lo+hi)/2;if(hiddenAt(mid)==state)lo=mid;else hi=mid;}to=(lo+hi)/2;}
                out.push_back(Edge(g.sub(from,to),state));
                from=to;state=hid;
            }
        }
    }
    return out;
}
// Vertical clear distances in the section: for a lower box (cut or level straight piece), the
// nearest box above it over a common x range. lower top -> upper bottom, at x.
struct Gap {double x,low,high,lowBottom;int lowTag,highTag;};
inline std::vector<Gap> gaps(const std::vector<Cut>& cuts,const std::vector<View>& views,double minOverlap){
    struct Box {double x0,x1,z0,z1;int tag;};std::vector<Box> b;
    for(size_t i=0;i<cuts.size();++i){Box q={cuts[i].s-cuts[i].w/2,cuts[i].s+cuts[i].w/2,cuts[i].z-cuts[i].h/2,cuts[i].z+cuts[i].h/2,cuts[i].tag};b.push_back(q);}
    for(size_t i=0;i<views.size();++i)if(views[i].level()){Box q={views[i].x0,views[i].x1,views[i].zc0-views[i].h/2,views[i].zc0+views[i].h/2,views[i].tag};b.push_back(q);}
    std::vector<Gap> out;
    for(size_t i=0;i<b.size();++i){int best=-1;double bz=0;
        for(size_t j=0;j<b.size();++j){if(j==i||b[j].tag==b[i].tag)continue;
            const double o0=(std::max)(b[i].x0,b[j].x0),o1=(std::min)(b[i].x1,b[j].x1);
            if(o1-o0<minOverlap||!(b[j].z0>b[i].z1+1e-9))continue;
            if(best<0||b[j].z0<bz){best=static_cast<int>(j);bz=b[j].z0;}}
        if(best<0)continue;
        const Box& u=b[best];const double x=((std::max)(b[i].x0,u.x0)+(std::min)(b[i].x1,u.x1))/2;
        bool dup=false;for(size_t k=0;k<out.size();++k)if(out[k].lowTag==b[i].tag&&out[k].highTag==u.tag)dup=true;
        if(!dup){Gap g={x,b[i].z1,u.z0,b[i].z0,b[i].tag,u.tag};out.push_back(g);}
    }
    return out;
}
}

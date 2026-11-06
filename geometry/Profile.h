#pragma once
// Uzduzni presjek (TVS_PRESJEK -> Uzduzni, faza P2): the route of the TVS network between two
// elements, developed into one vertical plane. Station s = developed plan length from the
// route start (plan elbow by its centre-line arc, T komad by its chords through the middle),
// z = axis elevation. Input: the plan members of geometry/Section.h (one member per duct,
// prelaz or vertical elbow; chords middle -> port for plan elbows and T komadi). Drawing units.
// No AutoCAD dependency (tests).
#include "Section.h"
#include <map>
#include <deque>
namespace tvsProfile {
using tvsSection::P;
using tvsSection::Member;
enum Kind {Duct=0,Taper=1,Bend=2,Riser=3,Elbow=4,Tee=5};
// One element of the route between stations s0 and s1. h = height seen in the profile (B, or D
// when round). path = plan polyline of the piece in the route direction (crossings).
struct Piece {int kind,tag;double s0,s1,z0,z1,h0,h1,A0,A1,B0,B1;bool round;Member bend;std::vector<P> path;
    Piece():kind(Duct),tag(0),s0(0),s1(0),z0(0),z1(0),h0(0),h1(0),A0(0),A1(0),B0(0),B1(0),round(false){}
    bool sloped()const{return std::fabs(z1-z0)>1e-9&&kind!=Taper&&kind!=Riser;}
    double zAt(double s)const{return s1-s0>1e-12?z0+(z1-z0)*(s-s0)/(s1-s0):z0;}
    double hAt(double s)const{return s1-s0>1e-12?h0+(h1-h0)*(s-s0)/(s1-s0):h0;}};
struct Port {P p;double z;};
inline bool touches(const Port& a,const Port& b,double tol){return std::hypot(a.p.x-b.p.x,a.p.y-b.p.y)<=tol&&std::fabs(a.z-b.z)<=tol;}
inline std::map<int,std::vector<int> > byTag(const std::vector<Member>& ms){
    std::map<int,std::vector<int> > out;for(size_t i=0;i<ms.size();++i)out[ms[i].tag].push_back(static_cast<int>(i));return out;}
// Connection ports of an element: both ends of a single member, the port ends (b) of chords.
inline std::vector<Port> ports(const std::vector<Member>& ms,const std::vector<int>& idx){
    std::vector<Port> out;
    if(idx.size()==1){const Member& m=ms[idx[0]];Port a={m.a,m.zAt(0)},b={m.b,m.zAt(1)};out.push_back(a);out.push_back(b);}
    else for(size_t k=0;k<idx.size();++k){const Member& m=ms[idx[k]];Port b={m.b,m.zAt(1)};out.push_back(b);}
    return out;
}
inline bool joined(const std::vector<Port>& a,const std::vector<Port>& b,double tol,int* pa=NULL,int* pb=NULL){
    for(size_t i=0;i<a.size();++i)for(size_t j=0;j<b.size();++j)if(touches(a[i],b[j],tol)){
        if(pa)*pa=static_cast<int>(i);
        if(pb)*pb=static_cast<int>(j);
        return true;}
    return false;
}
// Shortest chain of elements (tags) from `first` to `last` through joined ports (plan and z).
inline bool route(const std::vector<Member>& ms,int first,int last,double tol,std::vector<int>& tags){
    tags.clear();const std::map<int,std::vector<int> > groups=byTag(ms);
    if(!groups.count(first)||!groups.count(last))return false;
    std::map<int,std::vector<Port> > pt;
    for(std::map<int,std::vector<int> >::const_iterator g=groups.begin();g!=groups.end();++g)pt[g->first]=ports(ms,g->second);
    std::map<int,int> from;from[first]=first;std::deque<int> open;open.push_back(first);
    while(!open.empty()&&!from.count(last)){
        const int t=open.front();open.pop_front();
        for(std::map<int,std::vector<Port> >::const_iterator o=pt.begin();o!=pt.end();++o)
            if(!from.count(o->first)&&joined(pt[t],o->second,tol)){from[o->first]=t;open.push_back(o->first);}
    }
    if(!from.count(last))return false;
    for(int t=last;;t=from[t]){tags.insert(tags.begin(),t);if(t==first)break;}
    return true;
}
inline Member reversed(const Member& m){
    Member r=m;std::swap(r.a,r.b);std::swap(r.za,r.zb);std::swap(r.A1,r.A2);std::swap(r.B1,r.B2);r.levelA=!m.levelA;return r;}
inline double height(const Member& m,bool end){return m.round?(end?m.A2:m.A1):(end?m.B2:m.B1);}
// Developed route: one piece per element, stations in the route direction.
inline std::vector<Piece> develop(const std::vector<Member>& ms,const std::vector<int>& tags,double tol){
    std::vector<Piece> out;const std::map<int,std::vector<int> > groups=byTag(ms);double s=0;
    std::vector<std::vector<Port> > pt;
    for(size_t i=0;i<tags.size();++i)pt.push_back(ports(ms,groups.at(tags[i])));
    for(size_t i=0;i<tags.size();++i){
        const std::vector<int>& idx=groups.at(tags[i]);const std::vector<Port>& own=pt[i];
        int entry=-1,exit=-1;
        if(i>0)joined(own,pt[i-1],tol,&entry);
        if(i+1<tags.size())joined(own,pt[i+1],tol,&exit);
        auto farthest=[&](int from){int best=-1;double d=-1;
            for(size_t k=0;k<own.size();++k)if(static_cast<int>(k)!=from){const double g=std::hypot(own[k].p.x-own[from].p.x,own[k].p.y-own[from].p.y);if(g>d){d=g;best=static_cast<int>(k);}}
            return best;};
        if(entry<0&&exit<0){entry=0;exit=own.size()>1?1:0;}
        else if(entry<0)entry=farthest(exit);
        else if(exit<0)exit=farthest(entry);
        if(entry<0||exit<0)continue;
        Piece p;p.tag=tags[i];p.s0=s;
        if(idx.size()==1){
            const Member o=entry==0?ms[idx[0]]:reversed(ms[idx[0]]);
            const double L=o.planLength();
            p.kind=o.bend?Bend:o.vertical(tol)?Riser:o.taper?Taper:Duct;
            p.s1=p.kind==Riser?s:s+L;p.z0=o.zAt(0);p.z1=o.zAt(1);p.h0=height(o,false);p.h1=height(o,true);
            p.A0=o.A1;p.A1=o.A2;p.B0=o.B1;p.B1=o.B2;p.round=o.round;p.bend=o;
            p.path.push_back(o.a);p.path.push_back(o.b);
        }else{   // chords from the middle: plan elbow or T komad
            const Member& ce=ms[idx[entry]];const Member& cx=ms[idx[exit]];const P M=ce.a,E=ce.b,X=cx.b;
            const P u(E.x-M.x,E.y-M.y),v(X.x-M.x,X.y-M.y);const double lu=std::hypot(u.x,u.y),lv=std::hypot(v.x,v.y);
            double ang=3.14159265358979323846;if(lu>1e-12&&lv>1e-12){double c=(u.x*v.x+u.y*v.y)/(lu*lv);c=c<-1?-1:c>1?1:c;ang=std::acos(c);}
            const double pi=3.14159265358979323846;double L=lu+lv;
            const bool tee=idx.size()>=3||ce.kind==4;
            if(ang>pi-1e-3)L=std::hypot(X.x-E.x,X.y-E.y);                       // straight through (prolaz)
            else if(!tee){const double sweep=pi-ang,t=std::tan(sweep/2);if(t>1e-9)L=lu/t*sweep;}   // centre-line arc
            p.kind=tee?Tee:Elbow;p.s1=s+L;p.z0=p.z1=ce.z;p.h0=height(ce,false);p.h1=height(cx,false);
            p.A0=ce.A1;p.A1=cx.A1;p.B0=ce.B1;p.B1=cx.B1;p.round=ce.round;
            p.path.push_back(E);if(ang<=pi-1e-3)p.path.push_back(M);p.path.push_back(X);
        }
        out.push_back(p);s=p.s1;
    }
    return out;
}
// Ducts of other routes crossed by the route in plan: their cut at station s (P1 cut box).
struct Cross {double s;tvsSection::Cut c;};
inline double segDistance(const P& q,const P& a,const P& b){
    const double dx=b.x-a.x,dy=b.y-a.y,l=dx*dx+dy*dy;double t=l>0?((q.x-a.x)*dx+(q.y-a.y)*dy)/l:0;t=t<0?0:t>1?1:t;
    return std::hypot(a.x+dx*t-q.x,a.y+dy*t-q.y);}
inline std::vector<Cross> crossings(const std::vector<Member>& ms,const std::vector<int>& tags,const std::vector<Piece>& pieces,double tol){
    std::vector<Cross> out;
    for(size_t k=0;k<pieces.size();++k){const Piece& p=pieces[k];if(p.path.size()<2||p.s1-p.s0<=tol)continue;
        double total=0;for(size_t j=1;j<p.path.size();++j)total+=std::hypot(p.path[j].x-p.path[j-1].x,p.path[j].y-p.path[j-1].y);
        if(total<=tol)continue;
        double along=0;
        for(size_t j=1;j<p.path.size();++j){const P a=p.path[j-1],b=p.path[j];const double l=std::hypot(b.x-a.x,b.y-a.y);
            for(size_t i=0;i<ms.size();++i){const Member& m=ms[i];
                if(std::find(tags.begin(),tags.end(),m.tag)!=tags.end())continue;
                if(segDistance(m.a,a,b)<=tol||segDistance(m.b,a,b)<=tol)continue;   // joined, not crossing
                tvsSection::Cut c;if(!tvsSection::cut(a,b,m,c))continue;
                Cross x;x.c=c;x.s=p.s0+(along+c.s)/total*(p.s1-p.s0);
                bool dup=false;for(size_t q=0;q<out.size();++q)if(out[q].c.tag==c.tag&&std::fabs(out[q].s-x.s)<=tol)dup=true;
                if(!dup)out.push_back(x);
            }
            along+=l;
        }
    }
    std::sort(out.begin(),out.end(),[](const Cross& a,const Cross& b){return a.s<b.s;});
    return out;
}
// Outline of a straight piece in the profile (x = s, y = z): sloped ducts with their thickness
// perpendicular to the axis, prelaz and level pieces with vertical end faces.
inline std::vector<P> outline(const Piece& p){
    std::vector<P> q;
    if(p.kind==Riser){const double w=(p.round?p.A0:p.A0)/2,lo=(std::min)(p.z0,p.z1),hi=(std::max)(p.z0,p.z1);
        q.push_back(P(p.s0-w,lo));q.push_back(P(p.s0+w,lo));q.push_back(P(p.s0+w,hi));q.push_back(P(p.s0-w,hi));return q;}
    double nx=0,ny=1;
    if(p.sloped()){const double dx=p.s1-p.s0,dz=p.z1-p.z0,l=std::hypot(dx,dz);if(l>1e-12){nx=-dz/l;ny=dx/l;}}
    q.push_back(P(p.s0-nx*p.h0/2,p.z0-ny*p.h0/2));q.push_back(P(p.s1-nx*p.h1/2,p.z1-ny*p.h1/2));
    q.push_back(P(p.s1+nx*p.h1/2,p.z1+ny*p.h1/2));q.push_back(P(p.s0+nx*p.h0/2,p.z0+ny*p.h0/2));
    return q;
}
// Bottom elevation at station s (table): the piece starting there, else the one ending there.
inline bool bottomAt(const std::vector<Piece>& pieces,double s,double tol,double& z){
    for(int pass=0;pass<2;++pass)for(size_t k=0;k<pieces.size();++k){const Piece& p=pieces[k];if(p.kind==Riser)continue;
        const bool hit=pass==0?(s>=p.s0-tol&&s<p.s1-tol):(s>=p.s0-tol&&s<=p.s1+tol);
        if(hit){const double c=(std::max)(p.s0,(std::min)(p.s1,s));z=p.zAt(c)-p.hAt(c)/2;return true;}}
    return false;
}
// Table stations: every piece boundary, at least `spacing` apart (the end always kept).
inline std::vector<double> stations(const std::vector<Piece>& pieces,double spacing){
    std::vector<double> all;for(size_t k=0;k<pieces.size();++k){all.push_back(pieces[k].s0);all.push_back(pieces[k].s1);}
    std::sort(all.begin(),all.end());std::vector<double> out;
    for(size_t k=0;k<all.size();++k){
        if(out.empty()||all[k]-out.back()>=spacing)out.push_back(all[k]);
        else if(k+1==all.size()){if(out.size()>1)out.back()=all[k];else out.push_back(all[k]);}   // keep the end
    }
    return out;
}
}

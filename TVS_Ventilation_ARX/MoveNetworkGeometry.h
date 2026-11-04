#pragma once
#include "MoveGeometry.h"
#include <vector>
#include <utility>
#include <cstddef>

namespace tvsMoveNetwork {
using std::size_t;
using tvsMoveGeometry::Point;
enum Kind { Blocked, Pipe, Rigid };
struct Section {
    double a,b,elevation;bool round;
    Section(double pa=0,double pb=0,bool pr=false,double pe=0):a(pa),b(pb),elevation(pe),round(pr){}
};
struct Body {Kind kind;std::vector<Point> ports,axes,junctions;std::vector<Section> sections;Body():kind(Blocked){}};
struct Contact {int body,port,count;Contact():body(-1),port(-1),count(0){}};
enum Error { Ok,Invalid,Ambiguous,Unsupported,SectionMismatch,NotTangent,NonPlanar,TooShort,SideBranch,NewContact,Cycle,TooMany };
struct Result {std::vector<int> masks;std::vector<std::vector<Point> > ports;int changed;Error error;int problem;Result():changed(0),error(Invalid),problem(-1){}};
inline Contact contact(const std::vector<Body>& bodies,int owner,int port,double tol){
    Contact c;
    for(size_t i=0;i<bodies.size();++i)if(static_cast<int>(i)!=owner)
        for(size_t j=0;j<bodies[i].ports.size();++j)
            if(tvsMoveGeometry::pointsNear(bodies[owner].ports[port],bodies[i].ports[j],tol)){
                c.body=static_cast<int>(i);c.port=static_cast<int>(j);++c.count;
            }
    return c;
}
inline bool compatible(const Section& a,const Section& b,double tol){
    return a.round==b.round && tvsMoveGeometry::finite(a.a)&&tvsMoveGeometry::finite(b.a)
        &&a.a>0&&b.a>0&&(a.round||(a.b>0&&b.b>0))
        &&std::fabs(a.a-b.a)<=tol&&(a.round||std::fabs(a.b-b.b)<=tol)
        &&std::fabs(a.elevation-b.elevation)<=tol;
}
inline bool interior(const Point& q,const Point& a,const Point& b,double tol){
    return tvsMoveGeometry::onSegment(q,a,b,tol)
        &&!tvsMoveGeometry::pointsNear(q,a,tol)&&!tvsMoveGeometry::pointsNear(q,b,tol);
}
// Coordinates, delta, minimum length and contact tolerance all use WCS drawing units.
// Plan a bounded translation through a tree of rigid fittings and straight pipes.
// Pipes parallel to the translation absorb it by changing length; other pipes carry it on.
inline bool plan(const std::vector<Body>& bodies,int selected,const Point& delta,double minimum,double tol,Result& out){
    out=Result();
    if(selected<0||static_cast<size_t>(selected)>=bodies.size()||bodies[selected].kind!=Pipe
        ||!tvsMoveGeometry::finite(minimum)||minimum<=0||!tvsMoveGeometry::finite(tol)||tol<=0)return false;
    Point direction;
    if(!tvsMoveGeometry::unit(delta,direction)||!tvsMoveGeometry::valid(delta)||delta.length()<=tol
        ||std::fabs(direction.z)>1e-8)return false;
    out.masks.resize(bodies.size(),0);out.ports.resize(bodies.size());
    for(size_t i=0;i<bodies.size();++i)out.ports[i]=bodies[i].ports;
    std::vector<std::pair<int,int> > pending;pending.push_back(std::make_pair(selected,0));
    for(size_t next=0;next<pending.size();++next){
        int index=pending[next].first,port=pending[next].second;
        out.problem=index;
        const Body& body=bodies[index];
        if(body.kind==Blocked||body.ports.size()<2||body.ports.size()>3||body.sections.size()!=body.ports.size()||body.axes.size()!=body.ports.size()){
            out.error=Unsupported;return false;}
        if(out.masks[index]&(1<<port))continue;
        int wanted=(1<<static_cast<int>(body.ports.size()))-1;
        if(body.kind==Pipe){
            if(body.ports.size()!=2){out.error=Invalid;return false;}
            Point axis;
            if(!tvsMoveGeometry::unit(body.ports[1]-body.ports[0],axis)){out.error=TooShort;return false;}
            if(index!=selected&&std::fabs(axis.dot(direction))>=1-1e-8)wanted=1<<port;
        }
        if(!out.masks[index]&&++out.changed>256){out.error=TooMany;return false;}
        int added=wanted&~out.masks[index];out.masks[index]|=wanted;
        for(size_t j=0;j<body.ports.size();++j){
            if(!tvsMoveGeometry::valid(body.ports[j])||std::fabs(body.ports[j].z-bodies[selected].ports[0].z)>tol){
                out.error=NonPlanar;return false;}
            if(!(added&(1<<static_cast<int>(j))))continue;
            out.ports[index][j]=body.ports[j]+delta;
            if(!tvsMoveGeometry::valid(out.ports[index][j])){out.error=Invalid;return false;}
            Contact c=contact(bodies,index,static_cast<int>(j),tol);
            if(c.count>1){out.error=Ambiguous;return false;}
            if(c.count==1){
                if(bodies[c.body].kind==Blocked){out.error=Unsupported;return false;}
                if(static_cast<size_t>(c.port)>=bodies[c.body].sections.size()
                    ||!compatible(body.sections[j],bodies[c.body].sections[c.port],tol)){
                    out.error=SectionMismatch;return false;}
                Point aAxis,bAxis;
                if(bodies[c.body].axes.size()!=bodies[c.body].ports.size()
                    ||!tvsMoveGeometry::unit(body.axes[j],aAxis)
                    ||!tvsMoveGeometry::unit(bodies[c.body].axes[c.port],bAxis)
                    ||std::fabs(aAxis.dot(bAxis))<1-1e-6){out.error=NotTangent;return false;}
                pending.push_back(std::make_pair(c.body,c.port));
            }
        }
    }
    int edges=0;
    for(size_t i=0;i<bodies.size();++i)if(out.masks[i]){
        out.problem=static_cast<int>(i);
        const Body& body=bodies[i];
        for(size_t j=0;j<body.ports.size();++j){
            Contact c=contact(bodies,static_cast<int>(i),static_cast<int>(j),tol);
            if(c.count>1){out.error=Ambiguous;return false;}
            if(c.count==1&&c.body>static_cast<int>(i)&&out.masks[c.body])++edges;
        }
        if(body.kind==Pipe){
            Point axis;
            if(!tvsMoveGeometry::unit(body.ports[1]-body.ports[0],axis)
                ||(out.ports[i][1]-out.ports[i][0]).dot(axis)<minimum){out.error=TooShort;return false;}
            // A fitting attached halfway along an unsplit pipe cannot be propagated safely.
            for(size_t k=0;k<bodies.size();++k)if(k!=i)
                for(size_t p=0;p<bodies[k].ports.size();++p)
                    if(interior(bodies[k].ports[p],body.ports[0],body.ports[1],tol)
                        ||interior(out.ports[k][p],out.ports[i][0],out.ports[i][1],tol)){
                        out.error=SideBranch;return false;}
            for(size_t k=0;k<bodies.size();++k)if(k!=i)
                for(size_t p=0;p<bodies[k].junctions.size();++p){
                    Point old=bodies[k].junctions[p];Point moved=out.masks[k]?old+delta:old;
                    if(interior(old,body.ports[0],body.ports[1],tol)
                        ||interior(moved,out.ports[i][0],out.ports[i][1],tol)){
                        out.error=SideBranch;return false;}
                }
        }
        for(size_t j=0;j<body.ports.size();++j)if(out.masks[i]&(1<<static_cast<int>(j)))
            for(size_t k=0;k<bodies.size();++k)if(k!=i)
                for(size_t p=0;p<bodies[k].ports.size();++p)
                    if(tvsMoveGeometry::pointsNear(out.ports[i][j],out.ports[k][p],tol)
                        &&!tvsMoveGeometry::pointsNear(body.ports[j],bodies[k].ports[p],tol)){
                        out.error=NewContact;return false;}
    }
    if(edges>=out.changed){out.error=Cycle;return false;}
    out.problem=-1;out.error=Ok;return true;
}
// Explicit endpoint selection: never propagate movement outside the selection.
inline bool planSelection(const std::vector<Body>& bodies,const std::vector<int>& masks,
                          const Point& delta,double minimum,double tol,Result& out){
    out=Result();
    if(masks.size()!=bodies.size()||!tvsMoveGeometry::valid(delta)
        ||!tvsMoveGeometry::finite(minimum)||minimum<=0
        ||!tvsMoveGeometry::finite(tol)||tol<=0||delta.length()<=tol)return false;
    if(std::fabs(delta.z)>tol){out.error=NonPlanar;return false;}
    out.masks=masks;out.ports.resize(bodies.size());
    double plane=0;bool havePlane=false;
    for(size_t i=0;i<bodies.size();++i){
        const Body& b=bodies[i];out.ports[i]=b.ports;
        if(!masks[i])continue;
        out.problem=static_cast<int>(i);
        if(b.kind==Blocked||b.ports.size()<2||b.ports.size()>3
            ||b.axes.size()!=b.ports.size()||b.sections.size()!=b.ports.size()){
            out.error=Unsupported;return false;}
        int full=(1<<static_cast<int>(b.ports.size()))-1;
        if((masks[i]&~full)||(b.kind==Rigid&&masks[i]!=full)){
            out.error=Unsupported;return false;}
        if(++out.changed>256){out.error=TooMany;return false;}
        if(!havePlane){plane=b.ports[0].z;havePlane=true;}
        for(size_t j=0;j<b.ports.size();++j){
            if(!tvsMoveGeometry::valid(b.ports[j])||std::fabs(b.ports[j].z-plane)>tol){
                out.error=NonPlanar;return false;}
            if(masks[i]&(1<<static_cast<int>(j)))out.ports[i][j]=b.ports[j]+delta;
            if(!tvsMoveGeometry::valid(out.ports[i][j]))return false;
        }
        if(b.kind==Pipe){
            Point oldAxis,newAxis;
            if(b.ports.size()!=2||!tvsMoveGeometry::unit(b.ports[1]-b.ports[0],oldAxis)
                ||!tvsMoveGeometry::unit(out.ports[i][1]-out.ports[i][0],newAxis)){
                out.error=TooShort;return false;}
            if(oldAxis.dot(newAxis)<1-1e-8){out.error=NotTangent;return false;}
        }
    }
    if(!out.changed)return false;
    for(size_t i=0;i<bodies.size();++i)if(masks[i])
        for(size_t j=0;j<bodies[i].ports.size();++j){
            out.problem=static_cast<int>(i);
            Contact c=contact(bodies,static_cast<int>(i),static_cast<int>(j),tol);
            if(c.count>1){out.error=Ambiguous;return false;}
            if(c.count==1){
                const Body& b=bodies[c.body];
                if(b.kind==Blocked||b.sections.size()!=b.ports.size()||b.axes.size()!=b.ports.size()){
                    out.error=Unsupported;return false;}
                if(!compatible(bodies[i].sections[j],b.sections[c.port],tol)){
                    out.error=SectionMismatch;return false;}
                Point aAxis,bAxis;
                if(!tvsMoveGeometry::unit(bodies[i].axes[j],aAxis)||!tvsMoveGeometry::unit(b.axes[c.port],bAxis)
                    ||std::fabs(aAxis.dot(bAxis))<1-1e-6){out.error=NotTangent;return false;}
                if(bool(masks[i]&(1<<static_cast<int>(j)))!=bool(masks[c.body]&(1<<c.port))){
                    out.error=Unsupported;return false;}
            }
        }

    for(size_t i=0;i<bodies.size();++i)if(out.masks[i]){
        out.problem=static_cast<int>(i);
        const Body& body=bodies[i];
        for(size_t j=0;j<body.ports.size();++j){
            out.problem=static_cast<int>(i);
            Contact c=contact(bodies,static_cast<int>(i),static_cast<int>(j),tol);
            if(c.count>1){out.error=Ambiguous;return false;}

        }
        if(body.kind==Pipe){
            Point axis;
            if(!tvsMoveGeometry::unit(body.ports[1]-body.ports[0],axis)
                ||(out.ports[i][1]-out.ports[i][0]).dot(axis)<minimum){out.error=TooShort;return false;}
            // A fitting attached halfway along an unsplit pipe cannot be propagated safely.
            for(size_t k=0;k<bodies.size();++k)if(k!=i)
                for(size_t p=0;p<bodies[k].ports.size();++p)
                    if(interior(bodies[k].ports[p],body.ports[0],body.ports[1],tol)
                        ||interior(out.ports[k][p],out.ports[i][0],out.ports[i][1],tol)){
                        out.error=SideBranch;return false;}
            for(size_t k=0;k<bodies.size();++k)if(k!=i)
                for(size_t p=0;p<bodies[k].junctions.size();++p){
                    Point old=bodies[k].junctions[p];Point moved=out.masks[k]?old+delta:old;
                    if(interior(old,body.ports[0],body.ports[1],tol)
                        ||interior(moved,out.ports[i][0],out.ports[i][1],tol)){
                        out.error=SideBranch;return false;}
                }
        }
        for(size_t j=0;j<body.ports.size();++j)if(out.masks[i]&(1<<static_cast<int>(j)))
            for(size_t k=0;k<bodies.size();++k)if(k!=i)
                for(size_t p=0;p<bodies[k].ports.size();++p)
                    if(tvsMoveGeometry::pointsNear(out.ports[i][j],out.ports[k][p],tol)
                        &&!tvsMoveGeometry::pointsNear(body.ports[j],bodies[k].ports[p],tol)){
                        out.error=NewContact;return false;}
    }
    out.problem=-1;out.error=Ok;return true;
}
}

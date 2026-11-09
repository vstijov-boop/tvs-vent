#pragma once
// TVS_ALIGN: pure plan geometry (no AutoCAD), WCS drawing units, rotation about the vertical axis only.
// The element reference (pipe edge / axis, or the axis / edge of one fitting port) is a point `anchor`
// (on that reference, nearest the user's click) and a direction. The target is the infinite carrier
// line of a LINE, straight polyline segment, XLINE or RAY, projected to plan.
//   1. rotate about `anchor` by the smallest angle (|angle| <= 90 deg) that makes the reference parallel
//      to the target: the element never flips by 180 deg;
//   2. translate perpendicular to the target only, so the anchor lands at the signed distance
//      side * offset from the target: no sliding along the target.
// Physical elevation (z) is never changed.
#include <cmath>
#include <cfloat>
#include "MoveGeometry.h"
namespace tvsAlign {
using tvsMoveGeometry::Point;
struct Line {Point p,d;};   // point and unit plan direction (z = 0)
inline bool line(const Point& a,const Point& b,Line& out){
    Point d(b.x-a.x,b.y-a.y,0);if(!tvsMoveGeometry::unit(d,out.d))return false;
    out.p=Point(a.x,a.y,0);return tvsMoveGeometry::valid(out.p);
}
inline bool lineFromDirection(const Point& base,const Point& direction,Line& out){
    return line(base,Point(base.x+direction.x,base.y+direction.y,0),out);
}
inline Point normal(const Line& l){return Point(-l.d.y,l.d.x,0);}
// Signed plan distance of p from the line (positive on the left of its direction).
inline double distance(const Line& l,const Point& p){return (Point(p.x,p.y,0)-l.p).dot(normal(l));}
// Side (+1 / -1) of a point; a point on the line keeps `fallback`.
inline int side(const Line& l,const Point& p,double tol,int fallback){
    const double s=distance(l,p);return s>tol?1:s<-tol?-1:(fallback<0?-1:1);
}
struct Plan {double angle;Point pivot,delta;bool rotate;};
// angleTol in radians: below it the directions count as parallel (Samo pomjeri).
inline bool plan(const Point& anchor,const Point& direction,const Line& target,double offset,int sideSign,
                 double angleTol,Plan& out){
    Point e;if(!tvsMoveGeometry::valid(anchor)||!tvsMoveGeometry::unit(Point(direction.x,direction.y,0),e))return false;
    if(!(offset>=0&&offset<DBL_MAX)||!(angleTol>=0))return false;
    const double pi=3.14159265358979323846;
    double a=std::atan2(e.x*target.d.y-e.y*target.d.x,e.dot(target.d));   // e -> target direction
    if(a>pi/2)a-=pi;else if(a<=-pi/2)a+=pi;                               // smallest turn, no flip
    out.rotate=std::fabs(a)>angleTol;out.angle=out.rotate?a:0;out.pivot=anchor;
    const double want=(sideSign<0?-1.0:1.0)*offset;
    out.delta=normal(target)*(want-distance(target,anchor));
    return tvsMoveGeometry::valid(out.delta);
}
// Same transform as the AutoCAD matrix in AlignCommand.h: rotate about pivot (plan), then translate.
inline Point apply(const Plan& p,const Point& q){
    const double c=std::cos(p.angle),s=std::sin(p.angle),x=q.x-p.pivot.x,y=q.y-p.pivot.y;
    return Point(p.pivot.x+c*x-s*y+p.delta.x,p.pivot.y+s*x+c*y+p.delta.y,q.z);
}
inline Point turn(const Plan& p,const Point& v){
    const double c=std::cos(p.angle),s=std::sin(p.angle);return Point(c*v.x-s*v.y,s*v.x+c*v.y,v.z);
}
// Reference on one port of a fitting (or a pipe end): axis, or the edge on the click side / the other.
enum Reference {PickedEdge,OtherEdge,Axis};
inline bool portReference(const Point& port,const Point& axis,double width,const Point& cursor,Reference mode,Point& anchor){
    Point d;if(!tvsMoveGeometry::valid(port)||!tvsMoveGeometry::unit(Point(axis.x,axis.y,0),d))return false;
    anchor=port;if(mode==Axis)return true;
    if(!(width>0))return false;
    const Point n(-d.y,d.x,0);double s=(Point(cursor.x,cursor.y,port.z)-port).dot(n)>=0?1.0:-1.0;
    if(mode==OtherEdge)s=-s;
    anchor=port+n*(s*width/2);return true;
}
}

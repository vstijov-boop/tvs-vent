#pragma once
#include <cmath>
#include <cfloat>

namespace tvsMoveGeometry {
struct Point {
    double x,y,z;
    Point(double px=0,double py=0,double pz=0):x(px),y(py),z(pz){}
    Point operator+(const Point& b)const{return Point(x+b.x,y+b.y,z+b.z);}
    Point operator-(const Point& b)const{return Point(x-b.x,y-b.y,z-b.z);}
    Point operator*(double s)const{return Point(x*s,y*s,z*s);}
    double dot(const Point& b)const{return x*b.x+y*b.y+z*b.z;}
    double length()const{return std::sqrt(dot(*this));}
};
inline bool finite(double a){return a>=-DBL_MAX && a<=DBL_MAX;}
inline bool valid(const Point& p){return finite(p.x)&&finite(p.y)&&finite(p.z);}
inline bool unit(const Point& p,Point& out){double l=p.length();if(!finite(l)||l<=0)return false;out=p*(1/l);return valid(out);}
inline bool pointsNear(const Point& a,const Point& b,double tol){return (a-b).length()<=tol;}
inline bool onSegment(const Point& p,const Point& a,const Point& b,double tol){
    Point v=b-a;double l=v.dot(v);if(l<=0)return pointsNear(p,a,tol);
    double t=(p-a).dot(v)/l;if(t<0)t=0;if(t>1)t=1;
    return pointsNear(p,a+v*t,tol);
}
// Reconstruct smooth elbow axis ports from stored tangent intersection, not display caches.
inline bool elbowPorts(const Point& middle,const Point& start,double sweep,double axisRadius,Point ports[2]){
    Point u;if(!valid(middle)||!unit(Point(start.x,start.y,0),u)||!finite(sweep)
        ||sweep<=1e-6||sweep>=3.14159265358979323846-1e-6||!finite(axisRadius)||axisRadius<=0)return false;
    double a=std::atan2(u.y,u.x),half=sweep/2,c=std::cos(half);
    Point center=middle-Point(std::cos(a+half),std::sin(a+half),0)*(axisRadius/c);
    ports[0]=center+u*axisRadius;
    ports[1]=center+Point(std::cos(a+sweep),std::sin(a+sweep),0)*axisRadius;
    return valid(ports[0])&&valid(ports[1]);
}
struct Plan {Point selected[2], attached[2], delta[2];double offset;};
// Neighbor fixed ends stay fixed. Each elbow slides along its neighbor's axis.
// For oblique elbows the two translations differ; the selected axis stays parallel.
inline bool plan(const Point selected[2],const Point attached[2],const Point fixed[2],
                 const Point& base,const Point& target,double minimum,double tol,Plan& result){
    if(!valid(base)||!valid(target)||!valid(selected[0])||!valid(selected[1])
        ||!finite(minimum)||minimum<=0||!finite(tol)||tol<=0)return false;
    Point direction;if(!unit(selected[1]-selected[0],direction)||std::fabs(direction.z)>1e-8)return false;
    Point normal(-direction.y,direction.x,0);
    result.offset=(target-base).dot(normal);
    if(!finite(result.offset)||std::fabs(result.offset)<=tol)return false;
    for(int i=0;i<2;++i){
        if(!valid(selected[i])||!valid(attached[i])||!valid(fixed[i])
            ||std::fabs(selected[i].z-selected[0].z)>tol
            ||std::fabs(attached[i].z-selected[0].z)>tol
            ||std::fabs(fixed[i].z-selected[0].z)>tol)return false;
        Point v;if(!unit(attached[i]-fixed[i],v)||std::fabs(v.z)>1e-8)return false;
        double d=v.dot(normal);if(std::fabs(d)<1e-6)return false;
        result.delta[i]=v*(result.offset/d);
        result.attached[i]=attached[i]+result.delta[i];
        result.selected[i]=selected[i]+result.delta[i];
        // Signed remaining length prohibits crossing/reversing the fixed end.
        if(!valid(result.attached[i])||!valid(result.selected[i])
            ||(result.attached[i]-fixed[i]).dot(v)<minimum)return false;
    }
    return (result.selected[1]-result.selected[0]).dot(direction)>=minimum;
}
}

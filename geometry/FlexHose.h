#pragma once
// Flexible duct (air hose) plan drawing, principle of HVAC_Duct.lsp dfpipe but with convex
// beads: the hose is a row of cells of pitch p = k*D (cell count rounded so they fit the
// piece exactly). Each edge is a polyline of arcs between valleys (depth h inside D/2) whose
// tops touch D/2, so the outer size stays D; thin ring lines join the valleys. Pure 2D,
// no SDK: bulges follow AcDbPolyline (positive = counter-clockwise arc).
#include <vector>
#include <cmath>
namespace tvsFlex {
struct Vertex {double x,y,bulge;Vertex(double a=0,double b=0,double c=0):x(a),y(b),bulge(c){}};
struct Line {double x0,y0,x1,y1;Line(double a=0,double b=0,double c=0,double d=0):x0(a),y0(b),x1(c),y1(d){}};
struct Drawing {std::vector<Vertex> edges[2];std::vector<Line> rings;};
inline double defaultPitch(){return 0.2;}   // pitch / D
inline int cellCount(double length,double d,double k){
    if(!(length>0)||!(d>0)||!(k>0))return 0;
    const double n=std::floor(length/(k*d)+0.5);
    return n<1?1:n>2000?2000:static_cast<int>(n);
}
inline double depth(double pitch,double d){const double a=.3*pitch,b=.12*d;return a<b?a:b;}
// Straight piece from (sx,sy) along unit (ux,uy), length L, diameter D.
inline bool straight(double sx,double sy,double ux,double uy,double length,double d,double k,Drawing& out){
    out=Drawing();const int n=cellCount(length,d,k);if(!n)return false;
    const double p=length/n,h=depth(k*d,d),r=d/2,nx=-uy,ny=ux; // h from the nominal pitch: joints match
    for(int side=0;side<2;++side){const double sg=side?-1:1;
        // left edge (sg=+1) travels +u and bulges left -> negative bulge; right edge positive.
        const double bulge=-sg*2*h/p;
        for(int j=0;j<=n;++j){const double t=j*p;
            out.edges[side].push_back(Vertex(sx+ux*t+sg*nx*(r-h),sy+uy*t+sg*ny*(r-h),j<n?bulge:0));}}
    for(int j=0;j<=n;++j){const double t=j*p;
        out.rings.push_back(Line(sx+ux*t+nx*(r-h),sy+uy*t+ny*(r-h),sx+ux*t-nx*(r-h),sy+uy*t-ny*(r-h)));}
    return true;
}
// Plan elbow: centre (cx,cy), axis radius R, start angle a0, counter-clockwise sweep theta.
inline bool elbow(double cx,double cy,double radius,double a0,double theta,double d,double k,Drawing& out){
    out=Drawing();const double r=d/2;if(!(radius>r)||!(theta>0))return false;
    const int n=cellCount(radius*theta,d,k);if(!n)return false;
    const double step=theta/n,h=depth(k*d,d);
    // outer edge (side 0): valleys at R+r-h, tops at R+r, travels CCW -> bulges right (positive)
    // inner edge (side 1): valleys at R-r+h, tops at R-r, bulges left (negative)
    const double rv[2]={radius+r-h,radius-r+h},top[2]={radius+r,radius-r};
    for(int side=0;side<2;++side){
        const double chord=2*rv[side]*std::sin(step/2),mid=rv[side]*std::cos(step/2);
        double s=side?mid-top[side]:top[side]-mid;if(!(s>0))s=1e-9*d;
        const double bulge=(side?-1:1)*2*s/chord;
        for(int j=0;j<=n;++j){const double a=a0+j*step;
            out.edges[side].push_back(Vertex(cx+rv[side]*std::cos(a),cy+rv[side]*std::sin(a),j<n?bulge:0));}}
    for(int j=0;j<=n;++j){const double a=a0+j*step;
        out.rings.push_back(Line(cx+rv[0]*std::cos(a),cy+rv[0]*std::sin(a),cx+rv[1]*std::cos(a),cy+rv[1]*std::sin(a)));}
    return true;
}
}

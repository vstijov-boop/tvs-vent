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
struct Drawing {std::vector<Vertex> edges[2];std::vector<Line> rings;std::vector<std::vector<Vertex> > cuffs;}; // cuffs: closed outlines
// Cuff (manžetna) at a piece end: fixed length along the axis, slightly wider than D (clamp).
struct Cuffs {double start,end,extra;Cuffs():start(0),end(0),extra(0){}Cuffs(double s,double e,double x):start(s),end(e),extra(x){}};
// Cuffs must leave room for at least one cell; otherwise they shrink to a quarter of the piece each.
inline void fitCuffs(double length,double d,double k,Cuffs& c){
    if(c.start<0)c.start=0;
    if(c.end<0)c.end=0;
    if(c.start+c.end>length-k*d){const double q=length/4;if(c.start>q)c.start=q;if(c.end>q)c.end=q;}
}
inline double defaultPitch(){return 0.2;}   // pitch / D
inline int cellCount(double length,double d,double k){
    if(!(length>0)||!(d>0)||!(k>0))return 0;
    const double n=std::floor(length/(k*d)+0.5);
    return n<1?1:n>2000?2000:static_cast<int>(n);
}
inline double depth(double pitch,double d){const double a=.3*pitch,b=.12*d;return a<b?a:b;}
// Straight piece from (sx,sy) along unit (ux,uy), length L, diameter D.
inline bool straight(double sx,double sy,double ux,double uy,double length,double d,double k,Drawing& out,Cuffs c=Cuffs()){
    out=Drawing();if(!(length>0)||!(d>0))return false;
    fitCuffs(length,d,k,c);
    const double from=c.start,span=length-c.start-c.end;const int n=cellCount(span,d,k);if(!n)return false;
    const double p=span/n,h=depth(k*d,d),r=d/2,nx=-uy,ny=ux; // h from the nominal pitch: joints match
    for(int side=0;side<2;++side){const double sg=side?-1:1;
        // left edge (sg=+1) travels +u and bulges left -> negative bulge; right edge positive.
        const double bulge=-sg*2*h/p;
        for(int j=0;j<=n;++j){const double t=from+j*p;
            out.edges[side].push_back(Vertex(sx+ux*t+sg*nx*(r-h),sy+uy*t+sg*ny*(r-h),j<n?bulge:0));}}
    for(int j=0;j<=n;++j){const double t=from+j*p;
        out.rings.push_back(Line(sx+ux*t+nx*(r-h),sy+uy*t+ny*(r-h),sx+ux*t-nx*(r-h),sy+uy*t-ny*(r-h)));}
    const double w=r+c.extra;
    for(int e=0;e<2;++e){const double a=e?length-c.end:0,b=e?length:c.start;if(!(b>a))continue;std::vector<Vertex> q;
        q.push_back(Vertex(sx+ux*a+nx*w,sy+uy*a+ny*w));q.push_back(Vertex(sx+ux*b+nx*w,sy+uy*b+ny*w));
        q.push_back(Vertex(sx+ux*b-nx*w,sy+uy*b-ny*w));q.push_back(Vertex(sx+ux*a-nx*w,sy+uy*a-ny*w));out.cuffs.push_back(q);}
    return true;
}
// Plan elbow: centre (cx,cy), axis radius R, start angle a0, counter-clockwise sweep theta.
inline bool elbow(double cx,double cy,double radius,double a0,double theta,double d,double k,Drawing& out,Cuffs c=Cuffs()){
    out=Drawing();const double r=d/2;if(!(radius>r)||!(theta>0))return false;
    fitCuffs(radius*theta,d,k,c);
    const double first=a0+c.start/radius,sweep=theta-(c.start+c.end)/radius;
    const int n=cellCount(radius*sweep,d,k);if(!n||!(sweep>0))return false;
    const double step=sweep/n,h=depth(k*d,d);
    // outer edge (side 0): valleys at R+r-h, tops at R+r, travels CCW -> bulges right (positive)
    // inner edge (side 1): valleys at R-r+h, tops at R-r, bulges left (negative)
    const double rv[2]={radius+r-h,radius-r+h},top[2]={radius+r,radius-r};
    for(int side=0;side<2;++side){
        const double chord=2*rv[side]*std::sin(step/2),mid=rv[side]*std::cos(step/2);
        double s=side?mid-top[side]:top[side]-mid;if(!(s>0))s=1e-9*d;
        const double bulge=(side?-1:1)*2*s/chord;
        for(int j=0;j<=n;++j){const double a=first+j*step;
            out.edges[side].push_back(Vertex(cx+rv[side]*std::cos(a),cy+rv[side]*std::sin(a),j<n?bulge:0));}}
    for(int j=0;j<=n;++j){const double a=first+j*step;
        out.rings.push_back(Line(cx+rv[0]*std::cos(a),cy+rv[0]*std::sin(a),cx+rv[1]*std::cos(a),cy+rv[1]*std::sin(a)));}
    // cuffs: annular sectors, radii R+-(r+extra), arc length c on the axis
    const double ro=radius+r+c.extra,ri=radius-r-c.extra;
    for(int e=0;e<2;++e){const double len=e?c.end:c.start;if(!(len>0))continue;
        const double b0=e?a0+theta-len/radius:a0,b1=e?a0+theta:a0+len/radius,bu=std::tan((b1-b0)/4);std::vector<Vertex> q;
        q.push_back(Vertex(cx+ro*std::cos(b0),cy+ro*std::sin(b0),bu));q.push_back(Vertex(cx+ro*std::cos(b1),cy+ro*std::sin(b1),0));
        q.push_back(Vertex(cx+ri*std::cos(b1),cy+ri*std::sin(b1),-bu));q.push_back(Vertex(cx+ri*std::cos(b0),cy+ri*std::sin(b0),0));out.cuffs.push_back(q);}
    return true;
}
}

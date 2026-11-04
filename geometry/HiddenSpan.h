#pragma once
// Plan-view occlusion of a lower straight duct edge by an upper straight duct (bypass only).
// Pure 2D, no SDK: the upper duct's plan footprint is the rectangle of its axis +-width/2;
// clip() returns the parameter interval [t0,t1] of segment a-b inside it (Cyrus-Beck).
#include <cmath>
namespace tvsHidden {
struct P {double x,y;P(double a=0,double b=0):x(a),y(b){}};
inline bool footprint(P f,P l,double width,P q[4]){
 const double dx=l.x-f.x,dy=l.y-f.y,len=std::sqrt(dx*dx+dy*dy);
 if(!(len>0)||!(width>0))return false;
 const double nx=-dy/len*width/2,ny=dx/len*width/2;
 q[0]=P(f.x+nx,f.y+ny);q[1]=P(l.x+nx,l.y+ny);q[2]=P(l.x-nx,l.y-ny);q[3]=P(f.x-nx,f.y-ny);return true;
}
inline bool clip(P a,P b,const P q[4],double& t0,double& t1){
 t0=0;t1=1;const double dx=b.x-a.x,dy=b.y-a.y;
 // orientation of the quad (both windings accepted)
 double area=0;for(int i=0;i<4;++i){const P& u=q[i];const P& v=q[(i+1)%4];area+=u.x*v.y-v.x*u.y;}
 if(area==0)return false;
 const double s=area>0?1:-1;
 for(int i=0;i<4;++i){const P& u=q[i];const P& v=q[(i+1)%4];
  const double ex=v.x-u.x,ey=v.y-u.y;            // inside: s*cross(e, p-u) >= 0
  const double num=s*(ex*(a.y-u.y)-ey*(a.x-u.x)),den=s*(ex*dy-ey*dx);
  if(den==0){if(num<0)return false;continue;}
  const double t=-num/den;
  if(den>0){if(t>t0)t0=t;}else{if(t<t1)t1=t;}
  if(t0>=t1)return false;}
 return t1-t0>1e-9;
}
}

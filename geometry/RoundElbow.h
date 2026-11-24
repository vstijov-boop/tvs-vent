#pragma once
// TVS vertical (Form_Up/Form_Down) elbow plan parameterisation, from TVS_TAP::subWorldDraw:
//  rectangular: CenterPoint = MiddlePoint - (A/2 + R, (B/2 + R)*sin(t)), rotated about MiddlePoint
//               -> MA = M - u*rc*sin(t), MC = M                       (rc = R + B/2)
//  round (constant/variable radius): CenterPoint = M - rc/cos(t/2) * dir(sa + t/2)   (rc = R + A/2)
//               MA = C + rc*dir(sa), MC = C + rc*dir(sa) + u*rc*sin(t)
// sa = heading of Startvector, u = (-sin sa, cos sa). Both give the same two plan ports for the
// same centerline; only MiddlePoint differs. These helpers convert between the two, so a round
// elbow can be placed on the ports computed by the shared (rectangular) offset adapter.
#include <cmath>
namespace tvsRoundElbow {
struct P {double x,y;P(double a=0,double b=0):x(a),y(b){}};
// Plan ports (MA = horizontal face at Elev, MC = sloped face) of a round vertical TVS elbow.
inline void ports(P middle,double sx,double sy,double theta,double rc,P& ma,P& mc){
 const double sa=std::atan2(sy,sx),k=rc/std::cos(theta/2);
 const P c(middle.x-k*std::cos(sa+theta/2),middle.y-k*std::sin(sa+theta/2));
 ma=P(c.x+rc*std::cos(sa),c.y+rc*std::sin(sa));
 mc=P(ma.x-std::sin(sa)*rc*std::sin(theta),ma.y+std::cos(sa)*rc*std::sin(theta));
}
// MIRROR of a vertical elbow (user 2026-11-23: round elbows gore / dolje came out wrong). The plan
// figure is symmetric about its axis line - the horizontal run, direction u = rot90(Startvector),
// through MA. A mirror keeps the elbow an elbow: the axis line is mirrored (point and direction) and
// Startvector = rot-90(u'); MiddlePoint follows from the parameterisation above. Plan map
// p' = (a x + b y + tx, c x + d y + ty) with a d - b c < 0. round: rc = R + A/2 (rectangular: the
// axis point is MiddlePoint itself, rc unused). Startvector keeps its length.
struct Map {double a,b,c,d,tx,ty;};
inline P apply(const Map& m,P q){return P(m.a*q.x+m.b*q.y+m.tx,m.c*q.x+m.d*q.y+m.ty);}
inline P turn(const Map& m,P v){return P(m.a*v.x+m.b*v.y,m.c*v.x+m.d*v.y);}
inline void mirrorVertical(bool round,P middle,P start,double theta,double rc,const Map& m,P& middleOut,P& startOut){
 const double len=std::sqrt(start.x*start.x+start.y*start.y),sa=std::atan2(start.y,start.x);
 const P u(-std::sin(sa),std::cos(sa));P v=turn(m,u);const double lv=std::sqrt(v.x*v.x+v.y*v.y);v=P(v.x/lv,v.y/lv);
 startOut=P(v.y*len,-v.x*len);const double sb=std::atan2(startOut.y,startOut.x);
 if(!round){middleOut=apply(m,middle);return;}
 const double k=rc/std::cos(theta/2);
 const P c(middle.x-k*std::cos(sa+theta/2),middle.y-k*std::sin(sa+theta/2));
 const P axis=apply(m,P(c.x+rc*std::cos(sa),c.y+rc*std::sin(sa)));   // MA: on the axis line
 const P c2(axis.x-rc*std::cos(sb),axis.y-rc*std::sin(sb));
 middleOut=P(c2.x+k*std::cos(sb+theta/2),c2.y+k*std::sin(sb+theta/2));
}
// MiddlePoint of a round elbow whose MC port is `mc` (= rectangular MiddlePoint).
inline P middleFromMc(P mc,double sx,double sy,double theta,double rc){
 const double sa=std::atan2(sy,sx),k=rc/std::cos(theta/2);
 const P ma(mc.x+std::sin(sa)*rc*std::sin(theta),mc.y-std::cos(sa)*rc*std::sin(theta));
 const P c(ma.x-rc*std::cos(sa),ma.y-rc*std::sin(sa));
 return P(c.x+k*std::cos(sa+theta/2),c.y+k*std::sin(sa+theta/2));
}
}

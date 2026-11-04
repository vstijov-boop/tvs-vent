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
// MiddlePoint of a round elbow whose MC port is `mc` (= rectangular MiddlePoint).
inline P middleFromMc(P mc,double sx,double sy,double theta,double rc){
 const double sa=std::atan2(sy,sx),k=rc/std::cos(theta/2);
 const P ma(mc.x+std::sin(sa)*rc*std::sin(theta),mc.y-std::cos(sa)*rc*std::sin(theta));
 const P c(ma.x-rc*std::cos(sa),ma.y-rc*std::sin(sa));
 return P(c.x+k*std::cos(sa+theta/2),c.y+k*std::sin(sa+theta/2));
}
}

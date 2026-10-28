#include "../TVS_Ventilation_ARX/ElbowDirection.h"
#include <cassert>
#include <cmath>

static bool close(double a, double b) { return std::fabs(a-b) < 1e-9; }
int main() {
    const double pi = 3.14159265358979323846;
    const double scales[] = {1.0, 0.1, 0.001}; // mm geometry -> mm, cm, m
    const double turns[] = {15, 45, 90, 135, 165, -15, -45, -90, -135, -165};
    const double rotations[] = {0, 0.37, 1.9, 3.8};
    for (unsigned si=0; si<3; ++si) {
        const double scale = scales[si];
        for (unsigned ri=0; ri<4; ++ri) {
            const double rotation = rotations[ri];
            const double ix=std::cos(rotation), iy=std::sin(rotation);
            for (unsigned ti=0; ti<10; ++ti) {
                const double turn=turns[ti]*pi/180;
                const double ox=std::cos(rotation+turn), oy=std::sin(rotation+turn);
                // 3 m and 2 m legs: the previous 10-unit offset failed in metres.
                const double bx=5000*scale, by=-7000*scale;
                double angle=0, sx=0, sy=0;
                assert(tvsGeometry::elbowDirection(bx-3000*scale*ix, by-3000*scale*iy,
                    bx,by,bx+2000*scale*ox,by+2000*scale*oy,angle,sx,sy));
                assert(close(angle,std::fabs(turn)));
                assert(close(sx*sx+sy*sy,1));
                // Reconstruct the actual TVS arc centre and its tangent endpoints.
                const double direction=std::atan2(sy,sx);
                const double radius=350*scale;
                const double centreX=bx-radius/std::cos(angle/2)*std::cos(direction+angle/2);
                const double centreY=by-radius/std::cos(angle/2)*std::sin(direction+angle/2);
                const double tangent=radius*std::tan(angle/2);
                const double firstX=centreX+radius*sx, firstY=centreY+radius*sy;
                const double lastX=centreX+radius*std::cos(direction+angle);
                const double lastY=centreY+radius*std::sin(direction+angle);
                if (turn>0) {
                    assert(close(firstX/scale,(bx-tangent*ix)/scale));
                    assert(close(firstY/scale,(by-tangent*iy)/scale));
                    assert(close(lastX/scale,(bx+tangent*ox)/scale));
                    assert(close(lastY/scale,(by+tangent*oy)/scale));
                } else {
                    assert(close(lastX/scale,(bx-tangent*ix)/scale));
                    assert(close(lastY/scale,(by-tangent*iy)/scale));
                    assert(close(firstX/scale,(bx+tangent*ox)/scale));
                    assert(close(firstY/scale,(by+tangent*oy)/scale));
                }
            }
        }
    }
    double a=0,x=0,y=0;
    assert(!tvsGeometry::elbowDirection(0,0,0,0,1,1,a,x,y));
    assert(!tvsGeometry::elbowDirection(0,0,1,0,1,0,a,x,y));
    assert(!tvsGeometry::elbowDirection(0,0,1,0,2,0,a,x,y));
    assert(!tvsGeometry::elbowDirection(0,0,1,0,0,0,a,x,y));
}

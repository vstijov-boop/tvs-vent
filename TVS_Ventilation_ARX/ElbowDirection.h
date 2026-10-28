#pragma once
#include <cmath>

namespace tvsGeometry {
// Direction of the first radial vector of a counter-clockwise elbow arc.
// Work with unit vectors: no construction offset or minimum segment length.
inline bool elbowDirection(double ax, double ay, double bx, double by,
                           double cx, double cy, double& angle,
                           double& startX, double& startY) {
    const double ux = bx - ax, uy = by - ay;
    const double vx = cx - bx, vy = cy - by;
    const double incoming = std::sqrt(ux*ux + uy*uy);
    const double outgoing = std::sqrt(vx*vx + vy*vy);
    if (incoming == 0.0 || outgoing == 0.0) return false;
    const double ix = ux/incoming, iy = uy/incoming;
    const double ox = vx/outgoing, oy = vy/outgoing;
    const double cross = ix*oy - iy*ox;
    const double dot = ix*ox + iy*oy;
    // Straight continuations and reversals have no finite elbow at this vertex.
    if (std::fabs(cross) <= 1.0e-10) return false;
    angle = std::atan2(std::fabs(cross), dot);
    if (cross > 0.0) {
        startX = iy;
        startY = -ix;
    } else {
        startX = -oy;
        startY = ox;
    }
    return true;
}
}

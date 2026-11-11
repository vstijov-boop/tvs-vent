#pragma once
// TVS_DIM_PRATI: pure rules of the size follow (no AutoCAD types; tests/test_size_follow_rules.cpp).
// Units as given (one unit for all arguments).
#include <cmath>
namespace tvsFollowRules {
// Height of a section: rectangular B, round (or B not set) the diameter A.
inline double height(double a,double b,bool round){return (round||!(b>0))?a:b;}
// Two sections with axis levels c1 / c2 and heights h1 / h2 meet in height when their vertical
// spans overlap by more than tol. Ducts stacked one above the other (ends on the same plan
// point, spans apart or only touching) do not meet. Without heights: the axes must agree.
inline bool spansMeet(double c1,double h1,double c2,double h2,double tol){
    if(!(h1>0)||!(h2>0))return std::fabs(c1-c2)<=tol;
    return (h1+h2)/2-std::fabs(c1-c2)>tol;
}
// A joint found in plan is taken as it is when the element is the only one at that point
// (older drawings: levels of fittings were not always kept); with more elements on the same
// plan point the one that meets in height is the neighbour.
inline bool joined(int othersAtPoint,double c1,double h1,double c2,double h2,double tol){
    return othersAtPoint<=1||spansMeet(c1,h1,c2,h2,tol);
}
}

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
// Neighbour of a port (TVS_DIM_PRATI walk and joints, review D1). Contacts: elements with a port
// on the same plan point. One contact: a fitting is taken whatever its level (old drawings), a
// duct only when it meets in height. Several: the one that meets in height.
// Returns the contact index, FreeEnd, Several (more than one meets) or LevelGap (none meets).
struct Contact {double level,height;bool fitting;};
enum {FreeEnd=-1,Several=-2,LevelGap=-3};
inline int neighbour(const Contact* c,int n,double level,double height,double tol){
    if(n<=0)return FreeEnd;
    if(n==1)return (c[0].fitting||spansMeet(level,height,c[0].level,c[0].height,tol))?0:LevelGap;
    int found=-1,meet=0;
    for(int i=0;i<n;++i)if(spansMeet(level,height,c[i].level,c[i].height,tol)){found=i;++meet;}
    return meet==1?found:meet==0?LevelGap:Several;
}
}

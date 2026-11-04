#ifndef TVS_OFFSET_OPERATION_H
#define TVS_OFFSET_OPERATION_H
#include "OffsetAdapter.h"

// Shared physical plan for standalone and drawing entry points. No CAD/UI/globals.
namespace tvsOffsetOperation {
enum End { First=0,Last=1 };
enum Status { Ready,NoChange,Invalid,TooShort,UnsupportedGeometry };
struct Snapshot {
    tvsOffset::WorldPoint first,last;
    double a,b,elevation,factor;
    End end;
    Snapshot():a(0),b(0),elevation(0),factor(0),end(Last){}
};
struct Request {
    double elevation,angleDegrees,ri,minimumStraight,minimumProjection;
    Request():elevation(0),angleDegrees(45),ri(100),minimumStraight(100),minimumProjection(.1){}
};
struct Plan {
    Status status;tvsOffset::Status geometryStatus;
    tvsOffset::Result geometry;tvsOffsetAdapter::Result dto;
    tvsOffset::WorldPoint remainingFirst,remainingLast,exitFirst,exitLast;
    double acceptedElevation,angleDegrees;
    Plan():status(Invalid),geometryStatus(tvsOffset::InvalidInput),acceptedElevation(0),angleDegrees(0){}
};
inline Plan prepare(const Snapshot& s,const Request& r){
    Plan p;
    if((s.end!=First&&s.end!=Last)||!tvsOffsetAdapter::finitePoint(s.first)||!tvsOffsetAdapter::finitePoint(s.last)
        ||!tvsOffset::finite(s.a)||s.a<=0||!tvsOffset::finite(s.b)||s.b<=0
        ||(s.factor!=1&&s.factor!=10&&s.factor!=1000)||!tvsOffset::finite(s.elevation)
        ||!tvsOffset::finite(r.elevation)||!tvsOffset::finite(r.angleDegrees)
        ||!tvsOffset::finite(r.ri)||r.ri<=0||!tvsOffset::finite(r.minimumStraight)||r.minimumStraight<0
        ||!tvsOffset::finite(r.minimumProjection)||r.minimumProjection<=0
        ||std::fabs(s.first.z-s.last.z)*s.factor>.001)return p;
    const tvsOffset::WorldPoint q=s.end==First?s.first:s.last,fixed=s.end==First?s.last:s.first;
    const double dx=q.x-fixed.x,dy=q.y-fixed.y,length=std::sqrt(dx*dx+dy*dy);
    if(!tvsOffset::finite(length)||length*s.factor<=.001)return p;
    if(std::fabs(r.elevation-s.elevation)*s.factor<=.001){p.status=NoChange;return p;}
    const tvsOffset::Input input((r.elevation-s.elevation)*s.factor,s.a*s.factor,s.b*s.factor,
        r.ri,r.angleDegrees*tvsOffset::pi()/180,r.minimumStraight);
    tvsOffset::Policy policy;policy.minimumProjection=r.minimumProjection;
    p.geometry=tvsOffset::solve(input,policy);p.geometryStatus=p.geometry.status;
    if(!p.geometry.hasGeometry()||p.geometry.projectedLength<=0){p.status=UnsupportedGeometry;return p;}
    if(length*s.factor-p.geometry.axis[1].s<1){p.status=TooShort;return p;}
    const double heading=std::atan2(dy,dx);
    p.dto=tvsOffsetAdapter::adapt(input,p.geometry,tvsOffsetAdapter::Placement(q,heading,s.factor,s.elevation*s.factor));
    if(!p.dto.ready())return p;
    // Keep the original stored endpoint order. Only the chosen end is shortened.
    p.remainingFirst=s.end==First?p.dto.incomingLast:s.first;
    p.remainingLast=s.end==Last?p.dto.incomingLast:s.last;
    p.exitFirst=p.dto.outgoingFirst;
    p.exitLast=tvsOffset::WorldPoint(p.exitFirst.x+300*dx/length/s.factor,
        p.exitFirst.y+300*dy/length/s.factor,p.exitFirst.z);
    p.acceptedElevation=r.elevation;p.angleDegrees=r.angleDegrees;p.status=Ready;return p;
}
}
#endif

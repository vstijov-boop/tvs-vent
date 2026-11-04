#ifndef TVS_OFFSET_ADAPTER_H
#define TVS_OFFSET_ADAPTER_H
#include "OffsetGeometry.h"

// DTO adapter only. No SDK objects, redraw, globals, storage, or call to solve().
namespace tvsOffsetAdapter {
using tvsOffset::WorldPoint;
enum Form { Up,Down };
enum Status { Ready,NoGeometry,InvalidParameters,InconsistentResult,UnrepresentablePlacement };
struct Placement {
    WorldPoint firstMiddle; // existing command's LastPoint Q, in drawing/model units
    double heading,mmPerUnit,startElevationMm;
    Placement(const WorldPoint& q,double angle,double factor,double elevation)
        :firstMiddle(q),heading(angle),mmPerUnit(factor),startElevationMm(elevation){}
};
struct Elbow {
    WorldPoint middle,startVector,normal,expectedMA,expectedMC;
    double sizeA,sizeB,innerRadius,angle,elevation; // lengths/elevation in model units, angle in rad
    Form form;
    Elbow():sizeA(0),sizeB(0),innerRadius(0),angle(0),elevation(0),form(Up){}
};
struct Segment {
    WorldPoint first,last;
    double firstElevationMm,lastElevationMm,physicalLengthMm,projectedLengthMm;
    Segment():firstElevationMm(0),lastElevationMm(0),physicalLengthMm(0),projectedLengthMm(0){}
};
struct Result {
    Status status;Elbow elbows[2];Segment middle;
    WorldPoint incomingLast,outgoingFirst;
    Result():status(NoGeometry){}
    bool ready()const{return status==Ready;}
};
inline bool finitePoint(const WorldPoint& p){return tvsOffset::finite(p.x)&&tvsOffset::finite(p.y)&&tvsOffset::finite(p.z);}
inline bool close(double a,double b,double e){return tvsOffset::finite(a)&&tvsOffset::finite(b)&&std::fabs(a-b)<=e;}
inline Result adapt(const tvsOffset::Input& input,const tvsOffset::Result& geometry,const Placement& placement){
    Result failure;
    if(!geometry.hasGeometry())return failure;
    failure.status=InvalidParameters;
    if(!finitePoint(placement.firstMiddle)||!tvsOffset::finite(placement.heading)
        ||!tvsOffset::finite(placement.mmPerUnit)||placement.mmPerUnit<=0
        ||!tvsOffset::finite(placement.startElevationMm)
        ||!tvsOffset::finite(input.width)||input.width<=0||!tvsOffset::finite(input.bendSize)||input.bendSize<=0
        ||!tvsOffset::finite(input.innerRadius)||input.innerRadius<0
        ||!tvsOffset::finite(input.angle)||input.angle<=0||input.angle>=tvsOffset::pi()/2
        ||!tvsOffset::finite(input.deltaZ)||input.deltaZ==0)return failure;
    // Caller must pass the Input/Result pair from the SAME solve. These are defensive
    // structural checks, not an independent offset calculation or a provenance proof.
    failure.status=InconsistentResult;
    if(!tvsOffset::finite(geometry.epsilon)||geometry.epsilon<=0
        ||!close(geometry.radius,input.innerRadius+input.bendSize/2,geometry.epsilon)
        ||!close(geometry.axis[3].z,input.deltaZ,geometry.epsilon)
        ||!tvsOffset::finite(geometry.straightLength)||geometry.straightLength<0
        ||!tvsOffset::finite(geometry.projectedLength)||geometry.projectedLength<0)return failure;
    for(int j=0;j<4;++j)if(!tvsOffset::finite(geometry.axis[j].s)||!tvsOffset::finite(geometry.axis[j].z)
        ||!tvsOffset::finite(geometry.axis[j].t)||geometry.axis[j].t!=0)return failure;
    double f=placement.mmPerUnit,c=std::cos(placement.heading),s=std::sin(placement.heading);
    WorldPoint origin(placement.firstMiddle.x-geometry.axis[1].s*c/f,
                      placement.firstMiddle.y-geometry.axis[1].s*s/f,placement.firstMiddle.z);
    WorldPoint ports[4];failure.status=UnrepresentablePlacement;
    for(int j=0;j<4;++j){
        // Discard physical local z ONLY in the drawing projection. Keep elevation below.
        if(!tvsOffset::place(tvsOffset::Point(geometry.axis[j].s,0,geometry.axis[j].t),
                             origin,placement.heading,f,ports[j]))return failure;
    }
    Result out;
    out.incomingLast=ports[0];out.outgoingFirst=ports[3];
    for(int j=0;j<2;++j){Elbow& e=out.elbows[j];
        e.sizeA=input.width/f;e.sizeB=input.bendSize/f;e.innerRadius=input.innerRadius/f;e.angle=input.angle;
        e.middle=ports[j+1];e.normal=WorldPoint(0,0,1);
        e.startVector=j==0?WorldPoint(s,-c,0):WorldPoint(-s,c,0);
        e.form=(input.deltaZ>0)==(j==0)?Up:Down;
        e.elevation=(placement.startElevationMm+geometry.axis[j==0?0:3].z)/f;
        e.expectedMA=ports[j==0?0:3];e.expectedMC=ports[j+1];
        if(!tvsOffset::finite(e.sizeA)||e.sizeA<=0||!tvsOffset::finite(e.sizeB)||e.sizeB<=0
            ||!tvsOffset::finite(e.innerRadius)||!tvsOffset::finite(e.elevation))return failure;
    }
    out.middle.first=ports[1];out.middle.last=ports[2];
    out.middle.firstElevationMm=placement.startElevationMm+geometry.axis[1].z;
    out.middle.lastElevationMm=placement.startElevationMm+geometry.axis[2].z;
    out.middle.physicalLengthMm=geometry.straightLength;out.middle.projectedLengthMm=geometry.projectedLength;
    if(!tvsOffset::finite(out.middle.firstElevationMm)||!tvsOffset::finite(out.middle.lastElevationMm))return failure;
    // Reject placements where floating-point precision destroys a NONZERO projected span.
    // Lmin=0 / zero-length middle stays representable as a mathematical DTO.
    const double expected=geometry.projectedLength/f;
    const double dx=ports[2].x-ports[1].x,dy=ports[2].y-ports[1].y;
    if(!close(std::sqrt(dx*dx+dy*dy),expected,geometry.epsilon/f)
        ||(expected>0&&dx==0&&dy==0))return failure;
    out.status=Ready;return out;
}
}
#endif

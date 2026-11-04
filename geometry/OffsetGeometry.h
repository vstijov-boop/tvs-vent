#ifndef TVS_OFFSET_GEOMETRY_H
#define TVS_OFFSET_GEOMETRY_H
#include <cmath>
#include <cfloat>

// Pure C++98, physical millimetres/radians. No CAD SDK or persistent state.
namespace tvsOffset {
inline double pi(){return 3.1415926535897932384626433832795;}
inline bool finite(double x){return x>=-DBL_MAX && x<=DBL_MAX;}
inline double larger(double a,double b){return a>b?a:b;}
struct Point {double s,z,t;Point(double a=0,double b=0,double c=0):s(a),z(b),t(c){}};
struct Projection {double u,v;Projection(double a=0,double b=0):u(a),v(b){}};
inline Projection plan(const Point& p){return Projection(p.s,p.t);}
inline Projection profile(const Point& p){return Projection(p.s,p.z);}
enum Status { Ok,NoChange,InvalidInput,Unsupported90,TooShort,ProjectionTooSmall,NumericalFailure };
struct Policy {
    double absoluteTolerance,relativeTolerance,angleTolerance,minimumAngle,minimumProjection;
    Policy():absoluteTolerance(.001),relativeTolerance(1e-10),angleTolerance(1e-8),
        minimumAngle(pi()/1800),minimumProjection(0){}
};
struct Input {
    double deltaZ,width,bendSize,innerRadius,angle,minimumStraight;
    Input(double rise,double transverse,double depth,double ri,double radians,double lmin)
        :deltaZ(rise),width(transverse),bendSize(depth),innerRadius(ri),angle(radians),minimumStraight(lmin){}
};
struct Result {
    Status status;
    Point axis[4],centers[2],corners[4][4];
    double radius,straightLength,projectedLength,totalRun,arcLength,epsilon;
    // arcLength is ONE centreline arc. Corners bound the four joint planes.
    Result():status(InvalidInput),radius(0),straightLength(0),projectedLength(0),totalRun(0),arcLength(0),epsilon(0){}
    bool hasGeometry()const{return status==Ok;}
};
inline bool validPolicy(const Policy& p){
    return finite(p.absoluteTolerance)&&p.absoluteTolerance>0
        &&finite(p.relativeTolerance)&&p.relativeTolerance>=0&&p.relativeTolerance<1
        &&finite(p.angleTolerance)&&p.angleTolerance>0&&p.angleTolerance<pi()/4
        &&finite(p.minimumAngle)&&p.minimumAngle>p.angleTolerance&&p.minimumAngle<pi()/2-p.angleTolerance
        &&finite(p.minimumProjection)&&p.minimumProjection>=0;
}
inline Result solve(const Input& in,const Policy& policy=Policy()){
    Result out;
    if(!validPolicy(policy)||!finite(in.deltaZ)||!finite(in.width)||in.width<=0
        ||!finite(in.bendSize)||in.bendSize<=0||!finite(in.innerRadius)||in.innerRadius<0
        ||!finite(in.angle)||in.angle<=0||in.angle>pi()/2+policy.angleTolerance
        ||!finite(in.minimumStraight)||in.minimumStraight<0)return out;
    // Ri=0 is allowed as a zero-radius inner contour, not a miter fitting.
    // R remains strictly positive because bendSize>0. No fabrication claim.
    double r=in.innerRadius+in.bendSize/2;
    double scale=larger(larger(std::fabs(in.deltaZ),r),larger(in.width,larger(in.bendSize,in.minimumStraight)));
    double eps=larger(policy.absoluteTolerance,policy.relativeTolerance*scale);
    if(!finite(r)||!finite(eps)||r<=0){out.status=NumericalFailure;return out;}
    if(std::fabs(in.angle-pi()/2)<=policy.angleTolerance){out.status=Unsupported90;return out;}
    if(in.angle<policy.minimumAngle)return out;
    if(std::fabs(in.deltaZ)<=eps){out.status=NoChange;return out;}
    double sn=std::sin(in.angle),cs=std::cos(in.angle),half=std::sin(in.angle/2);
    double oneRise=(2*half*half)*r;
    double remaining=std::fabs(in.deltaZ)-2*oneRise;
    // Never round a negative remainder or a short physical piece upward.
    if(remaining<0){out.status=TooShort;return out;}
    double length=remaining/sn;
    if(!finite(length)){out.status=NumericalFailure;return out;}
    if(length<in.minimumStraight){out.status=TooShort;return out;}
    double projected=length*cs,reach=r*sn,run=2*reach+projected;
    if(projected<policy.minimumProjection){out.status=ProjectionTooSmall;return out;}
    double arc=r*in.angle;
    if(!finite(projected)||!finite(reach)||!finite(run)||!finite(arc)){
        out.status=NumericalFailure;return out;}
    double sign=in.deltaZ>0?1:-1;
    Result result;result.status=Ok;result.radius=r;result.straightLength=length;
    result.projectedLength=projected;result.totalRun=run;result.arcLength=arc;result.epsilon=eps;
    result.axis[0]=Point();result.axis[1]=Point(reach,sign*oneRise);
    result.axis[2]=Point(reach+projected,in.deltaZ-sign*oneRise);
    result.axis[3]=Point(run,in.deltaZ);
    result.centers[0]=Point(0,sign*r);result.centers[1]=Point(run,in.deltaZ-sign*r);
    for(int j=0;j<4;++j){
        double ns=(j==1||j==2)?-sign*sn:0,nz=(j==1||j==2)?cs:1;
        for(int k=0;k<4;++k){double side=(k&1)?1:-1,vertical=(k&2)?1:-1;
            Point q(result.axis[j].s+vertical*in.bendSize/2*ns,
                    result.axis[j].z+vertical*in.bendSize/2*nz,side*in.width/2);
            if(!finite(q.s)||!finite(q.z)||!finite(q.t)){out.status=NumericalFailure;return out;}
            result.corners[j][k]=q;
        }
    }
    return result;
}
// Optional placement adapter. origin.z is an absolute reference only; solve never sees it.
// Units are mm per model unit. Heading rotates the local s,t plane about WCS Z.
struct WorldPoint {double x,y,z;WorldPoint(double a=0,double b=0,double c=0):x(a),y(b),z(c){}};
inline bool place(const Point& p,const WorldPoint& origin,double heading,double mmPerUnit,WorldPoint& out){
    if(!finite(p.s)||!finite(p.z)||!finite(p.t)||!finite(origin.x)||!finite(origin.y)||!finite(origin.z)
        ||!finite(heading)||!finite(mmPerUnit)||mmPerUnit<=0)return false;
    double c=std::cos(heading),s=std::sin(heading);
    WorldPoint candidate(origin.x+(p.s*c-p.t*s)/mmPerUnit,
                         origin.y+(p.s*s+p.t*c)/mmPerUnit,origin.z+p.z/mmPerUnit);
    if(!finite(candidate.x)||!finite(candidate.y)||!finite(candidate.z))return false;
    out=candidate;return true;
}
}
#endif

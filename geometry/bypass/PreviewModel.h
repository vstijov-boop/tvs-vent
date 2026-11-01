#ifndef TVS_BYPASS_PREVIEW_MODEL_H
#define TVS_BYPASS_PREVIEW_MODEL_H
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>
namespace tvsBypassPreview {
struct Point {double x,y,z;Point(double a=0,double b=0,double c=0):x(a),y(b),z(c){}};
struct Segment {Point first,last;double a,b,elevation;bool rectangular,rigid,ordinary;
 Segment():a(0),b(0),elevation(0),rectangular(false),rigid(false),ordinary(false){} };
struct Snapshot {Segment route,obstacle;};
struct Frame {
 bool valid;std::string reason;Point origin;double ux,uy,factor,width,height,z0,obstacleWidth,obstacleHeight,obstacleZ,tmin,tmax,left,right;
 Frame():valid(false),ux(0),uy(0),factor(0),width(0),height(0),z0(0),obstacleWidth(0),obstacleHeight(0),obstacleZ(0),tmin(0),tmax(0),left(0),right(0){}
 Point world(double s,double t)const{return Point(origin.x+(s*ux-t*uy)/factor,origin.y+(s*uy+t*ux)/factor,origin.z);}
};
inline bool finite(double v){return std::isfinite(v)&&std::fabs(v)<=1e9;}
inline Frame adapt(const Snapshot& s,double factor,bool reverse){
 Frame f;f.factor=factor;
 if(factor!=1&&factor!=10&&factor!=1000){f.reason="units must be confirmed as mm/cm/m";return f;}
 const Segment* parts[]={&s.route,&s.obstacle};
 for(int j=0;j<2;++j){const Segment& p=*parts[j];
  const double v[]={p.first.x,p.first.y,p.first.z,p.last.x,p.last.y,p.last.z,p.a,p.b,p.elevation};
  for(unsigned k=0;k<9;++k)if(!finite(v[k])){f.reason="nonfinite/out-of-domain object data";return f;}
  if(!p.rectangular||!p.rigid||!p.ordinary||p.a<=0||p.b<=0){f.reason="only ordinary rigid rectangular straight TVS channels";return f;}
  if(std::fabs(p.last.z-p.first.z)*factor>.001){f.reason="channel is not horizontal in WCS";return f;}
 }
 Point start=reverse?s.route.last:s.route.first,end=reverse?s.route.first:s.route.last;
 double dx=end.x-start.x,dy=end.y-start.y,len=std::hypot(dx,dy);
 double ox=s.obstacle.last.x-s.obstacle.first.x,oy=s.obstacle.last.y-s.obstacle.first.y,ol=std::hypot(ox,oy);
 if(len*factor<=.001||ol*factor<=.001){f.reason="zero/too-short axis";return f;}
 f.ux=dx/len;f.uy=dy/len;
 if(std::fabs((ox/ol)*f.ux+(oy/ol)*f.uy)>1e-10){f.reason="MVP requires perpendicular axes (dot tolerance 1e-10)";return f;}
 double along=(s.obstacle.first.x-start.x)*f.ux+(s.obstacle.first.y-start.y)*f.uy;
 f.origin=Point(start.x+along*f.ux,start.y+along*f.uy,start.z);
 double t1=(-(s.obstacle.first.x-f.origin.x)*f.uy+(s.obstacle.first.y-f.origin.y)*f.ux)*factor;
 double t2=(-(s.obstacle.last.x-f.origin.x)*f.uy+(s.obstacle.last.y-f.origin.y)*f.ux)*factor;
 f.tmin=std::fmin(t1,t2);f.tmax=std::fmax(t1,t2);
 f.left=-along*factor;f.right=(len-along)*factor;
 // Both native endpoints and elevation ALREADY denote axes. No A/2 or B/2 offset.
 f.width=s.route.a*factor;f.height=s.route.b*factor;f.z0=s.route.elevation*factor;
 f.obstacleWidth=s.obstacle.a*factor;f.obstacleHeight=s.obstacle.b*factor;f.obstacleZ=s.obstacle.elevation*factor;
 f.valid=true;return f;
}
struct Revision {
 std::uint64_t input,result;bool ready;
 Revision():input(1),result(0),ready(false){}
 void invalidate(){++input;ready=false;}
 void accept(){result=input;ready=true;}
 bool current()const{return ready&&input==result;}
};
struct Options {
 double ri,insulation,obstacleInsulation,clearance,reserve,minSlope,minBridge,minProjection,left,right;
 double floor,ceiling,timeMs,calls,cells,depthStep,bridgeStep,maxDepth,depthLevels,bridgeLevels,positions,seedSolves,proposals,inset,wTolerance,dTolerance;
 int angle;bool above,useFloor,useCeiling,confirmed;
 Options():ri(100),insulation(0),obstacleInsulation(0),clearance(50),reserve(5),minSlope(100),minBridge(100),minProjection(.1),left(0),right(0),
 floor(0),ceiling(0),timeMs(5000),calls(64),cells(300000),depthStep(10),bridgeStep(25),maxDepth(100000),depthLevels(8),bridgeLevels(5),positions(5),seedSolves(64),proposals(512),inset(1),wTolerance(.01),dTolerance(.01),angle(45),above(false),useFloor(false),useCeiling(false),confirmed(false){}
};
struct Vertex {double s,z;Vertex(double a=0,double b=0):s(a),z(b){}};
enum CurveKind {PhysicalOutline,JointSymbol,AxisLine,DiagnosticLine};
struct Curve {int member;bool insulation;int kind;std::vector<Vertex> points;Curve():member(0),insulation(false),kind(PhysicalOutline){} };
struct View {
 bool ready;std::string status,details;Frame frame;std::vector<Curve> profile,plan;
 double D,C,s0,W,zPass,gapLo,gapHi,elapsedMs;double slopePhysical[2],slopeProjected[2];int criticalRole;std::vector<Vertex> axis;Vertex witnessBody,witnessObstacle;unsigned calls;std::uint64_t cells;
 View():ready(false),D(0),C(0),s0(0),W(0),zPass(0),gapLo(0),gapHi(0),elapsedMs(0),criticalRole(-1),calls(0),cells(0){slopePhysical[0]=slopePhysical[1]=slopeProjected[0]=slopeProjected[1]=0;}
};
// Opaque numeric engine: UI translation units never include interval/checker headers.
class Engine {
 void* state;
 Engine(const Engine&);Engine& operator=(const Engine&);
public:
 Engine();~Engine();void invalidate();bool ready()const;
 Frame frame(const Snapshot&,double factor,bool reverse)const;
 View calculate(const Snapshot&,double factor,bool reverse,const Options&,bool improve);
};
}
#endif

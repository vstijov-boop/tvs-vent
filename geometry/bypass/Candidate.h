#ifndef TVS_BYPASS_CANDIDATE_H
#define TVS_BYPASS_CANDIDATE_H
#include "../OffsetGeometry.h"
#include "OutwardInterval.h"
#include <vector>
#include <queue>
#include <string>
namespace tvsBypass {
using bounded::I;using bounded::V;using bounded::Box;
enum Role {FirstElbow,FirstSlope,SecondElbow,Bridge,ThirdElbow,SecondSlope,FourthElbow,IncomingNeighbor,OutgoingNeighbor};
enum Status {Feasible,Infeasible,NumericalBoundary,InvalidInput,Unsupported};
enum Relation {PositiveDistance,Touching,Overlapping,Unresolved};
struct OptionalBound {bool active;double mm;OptionalBound():active(false),mm(0){}};
struct Policy {
 double distanceToleranceMm,jointToleranceMm;unsigned maxCells,maxDepth,supportSlices;
 Policy():distanceToleranceMm(.001),jointToleranceMm(.000001),maxCells(30000),maxDepth(40),supportSlices(24){}
};
struct Input {
 double width,height,ri,angleDegrees,z0,obstacleS,obstacleZ,obstacleWidth,obstacleHeight;
 double insulation,obstacleInsulation,clearance,D,C,s0,windowLeft,windowRight;
 double obstacleTmin,obstacleTmax,minimumSlope,minimumBridge,minimumProjection;
 bool above;OptionalBound floor,ceiling;Policy policy;
 Input():width(400),height(200),ri(100),angleDegrees(45),z0(-400),obstacleS(0),
 obstacleZ(-400),obstacleWidth(250),obstacleHeight(250),insulation(0),
 obstacleInsulation(0),clearance(50),D(400),C(400),s0(-800),windowLeft(-1200),
 windowRight(1200),obstacleTmin(-1000),obstacleTmax(1000),minimumSlope(100),
 minimumBridge(100),minimumProjection(.1),above(false){}
};
struct Region {I left,right,bottom,top;
 Region(I a=I(),I b=I(),I c=I(),I d=I()):left(a),right(b),bottom(c),top(d){}
 Box enclosure()const{return Box(I(left.lo,right.hi),I(bottom.lo,top.hi));}
};
struct Body {
 enum Kind {Straight,Sector};Kind kind;Role role;
 V first,last,normal,center;I half,inner,outer;double theta;int sign,side;
 Body():kind(Straight),role(Bridge),half(0),inner(0),outer(0),theta(0),sign(1),side(1){}
 V at(I u,I v)const{
  if(kind==Straight)return first+(last-first)*u+normal*(half*(I(2)*v-I(1)));
  I a=I(theta)*u,r=inner+(outer-inner)*v;
  return center+V(I(side)*r*bounded::sine(a),I(-sign*side)*r*bounded::cosine(a));
 }
 Box enclosure(I u,I v)const{
  if(kind==Sector)return bounded::pointBox(at(u,v));
  Box b=bounded::pointBox(at(I(u.lo),I(v.lo)));
  b=bounded::boxHull(b,bounded::pointBox(at(I(u.lo),I(v.hi))));
  b=bounded::boxHull(b,bounded::pointBox(at(I(u.hi),I(v.lo))));
  return bounded::boxHull(b,bounded::pointBox(at(I(u.hi),I(v.hi))));
 }
 I projection(V axis,I u)const{
  if(kind==Straight)return bounded::dot(first,axis)+bounded::dot(last-first,axis)*I(0,1)
      +bounded::dot(normal,axis)*half*I(-1,1);
  I a=I(theta)*u;
  I f=axis.s*bounded::sine(a)-I(sign)*axis.z*bounded::cosine(a);
  I derivative=axis.s*bounded::cosine(a)+I(sign)*axis.z*bounded::sine(a);
  I f0=axis.s*bounded::sine(I(a.lo))-I(sign)*axis.z*bounded::cosine(I(a.lo));
  I f1=axis.s*bounded::sine(I(a.hi))-I(sign)*axis.z*bounded::cosine(I(a.hi));
  if(derivative.lo>=0)f=I(f0.lo,f1.hi);else if(derivative.hi<=0)f=I(f1.lo,f0.hi);
  return bounded::dot(center,axis)+I(side)*(inner+(outer-inner)*I(0,1))*f;
 }
 I support(V axis,unsigned slices)const{
  I total=projection(axis,I(0,1));
  if(kind==Straight)return total;
  for(unsigned j=0;j<slices;++j){
   // Adjacent closed dyadic/rational double ranges cover [0,1]; outward division.
   I a=I(static_cast<double>(j))/I(static_cast<double>(slices));
   I b=I(static_cast<double>(j+1))/I(static_cast<double>(slices));
   I p=projection(axis,I(std::max(0.0,a.lo),std::min(1.0,b.hi)));
   if(j==0)total=p;else total=bounded::hull(total,p);
  }
  return total;
 }
 bool strictContains(V p)const{
  if(kind==Sector){
   I x=I(side)*(p.s-center.s),y=I(-sign*side)*(p.z-center.z);
   I a(theta),r2=bounded::square(x)+bounded::square(y);
   return x.lo>0&&y.lo>0 &&
    (x*bounded::cosine(a)-y*bounded::sine(a)).hi<0 &&
    (r2-bounded::square(inner)).lo>0 && (bounded::square(outer)-r2).lo>0;
  }
  V vertices[]={at(I(0),I(0)),at(I(1),I(0)),at(I(1),I(1)),at(I(0),I(1))};
  bool plus=true,minus=true;
  for(int j=0;j<4;++j){V a=vertices[(j+1)%4]-vertices[j],b=p-vertices[j];
   I c=a.s*b.z-a.z*b.s;plus=plus&&c.lo>0;minus=minus&&c.hi<0;}
  return plus||minus;
 }
};
inline Body straight(V a,V b,V n,I half,Role role){
 Body p;p.first=a;p.last=b;p.normal=n;p.half=half;p.role=role;return p;
}
inline Body sector(V c,I ri,I ro,double theta,int sign,int side,Role role){
 Body p;p.kind=Body::Sector;p.center=c;p.inner=ri;p.outer=ro;p.theta=theta;p.sign=sign;p.side=side;p.role=role;return p;
}
inline I regionProjection(const Region& o,V a){
 return o.left*a.s+o.bottom*a.z+I(0,1)*(o.right-o.left)*a.s+I(0,1)*(o.top-o.bottom)*a.z;
}
inline bool regionInterior(const Region& o,V p){
 return p.s.lo>o.left.hi&&p.s.hi<o.right.lo&&p.z.lo>o.bottom.hi&&p.z.hi<o.top.lo;
}
inline V corner(const Region& o,int j){return V(j&1?o.right:o.left,j&2?o.top:o.bottom);}
inline V regionWitness(const Region& o,V a){
 // A fixed binary64 point in the certified inner box is certainly in O.
 double x=std::max(o.left.hi,std::min(o.right.lo,(a.s.lo+a.s.hi)/2));
 double z=std::max(o.bottom.hi,std::min(o.top.lo,(a.z.lo+a.z.hi)/2));
 return V(I(x),I(z));
}
inline std::vector<V> axes(double theta){
 std::vector<V> a;a.push_back(V(I(1),I(0)));a.push_back(V(I(0),I(1)));
 I t(theta);a.push_back(V(bounded::cosine(t),bounded::sine(t)));
 a.push_back(V(bounded::cosine(t),-bounded::sine(t)));return a;
}
inline bool separated(const Body& a,const Body& b,const Policy& p,double guard=0){
 std::vector<V> ax=axes(std::max(a.theta,b.theta));
 for(size_t j=0;j<ax.size();++j){
  I x=a.support(ax[j],p.supportSlices),y=b.support(ax[j],p.supportSlices);
  I margin=I(guard)*bounded::root(bounded::square(ax[j].s)+bounded::square(ax[j].z));
  if((I(y.lo)-I(x.hi)).lo>=margin.hi || (I(x.lo)-I(y.hi)).lo>=margin.hi)return true;
 }
 return false;
}
inline bool separated(const Body& a,const Region& b,const Policy& p){
 Body box=straight(V(b.left,b.bottom),V(b.right,b.bottom),V(I(0),I(1)),I(0),Bridge);
 // Use exact projection of the four rectangle corners, not a fake zero-height body.
 std::vector<V> ax=axes(a.theta);
 for(size_t j=0;j<ax.size();++j){
  I x=a.support(ax[j],p.supportSlices),y=bounded::dot(corner(b,0),ax[j]);
  for(int k=1;k<4;++k)y=bounded::hull(y,bounded::dot(corner(b,k),ax[j]));
  if(x.hi<=y.lo||y.hi<=x.lo)return true;
 }
 (void)box;return false;
}
inline Relation pairRelation(const Body& a,const Body& b,const Policy& p){
 if(separated(a,b,p))return PositiveDistance;
 for(int u=0;u<3;++u)for(int v=0;v<3;++v)
  if(b.strictContains(a.at(I(u*.5),I(v*.5)))||a.strictContains(b.at(I(u*.5),I(v*.5))))return Overlapping;
 return Unresolved;
}
struct Distance {
 I mm;Relation relation;bool converged;Role witnessMember,lowerMember;
 V onBody,onObstacle;double u,v;unsigned cells;
 Distance():mm(0,std::numeric_limits<double>::infinity()),relation(Unresolved),converged(false),
 witnessMember(Bridge),lowerMember(Bridge),u(0),v(0),cells(0){}
};
struct Patch {size_t body;double u0,u1,v0,v1,lower;unsigned depth;};
struct ComparePatch {bool operator()(const Patch& a,const Patch& b)const{return a.lower>b.lower;}};
inline bool containedWitness(const Body& a,const Region& o,V& where){
 for(int j=0;j<5;++j){
  V p=j==4?V((o.left+o.right)/I(2),(o.bottom+o.top)/I(2)):corner(o,j);
  if(a.strictContains(p)){where=p;return true;}
 }
 V mid=a.at(I(.5),I(.5));if(regionInterior(o,mid)){where=mid;return true;}return false;
}
inline Distance measure(const std::vector<Body>& bodies,const Region& o,const Policy& policy){
 Distance out;if(bodies.empty())return out;
 std::priority_queue<Patch,std::vector<Patch>,ComparePatch> queue;
 double upper=std::numeric_limits<double>::infinity();bool allSeparated=true;
 for(size_t j=0;j<bodies.size();++j){V w;
  if(containedWitness(bodies[j],o,w)){out.mm=I(0);out.relation=Overlapping;out.converged=true;out.witnessMember=bodies[j].role;out.onBody=out.onObstacle=w;return out;}
  allSeparated=allSeparated&&separated(bodies[j],o,policy);
  Patch p={j,0,1,0,1,0,0};queue.push(p);
 }
 const Box ob=o.enclosure();
 while(!queue.empty()&&out.cells<policy.maxCells){
  Patch p=queue.top();queue.pop();const Body& b=bodies[p.body];++out.cells;
  Box enclosure=b.enclosure(I(p.u0,p.u1),I(p.v0,p.v1));
  double lower=bounded::boxDistance(enclosure,ob).lo;
  if(lower>upper)continue;
  double us[]={p.u0,(p.u0+p.u1)/2,p.u1},vs[]={p.v0,(p.v0+p.v1)/2,p.v1};
  for(int a=0;a<3;++a)for(int k=0;k<3;++k){
   V q=b.at(I(us[a]),I(vs[k]));
   if(us[a]>0&&us[a]<1&&vs[k]>0&&vs[k]<1&&regionInterior(o,q)){
    out.mm=I(0);out.relation=Overlapping;out.converged=true;out.witnessMember=b.role;out.onBody=out.onObstacle=q;return out;}
   V r=regionWitness(o,q);double v=bounded::distance(q,r).hi;
   if(v<upper){upper=v;out.onBody=q;out.onObstacle=r;out.witnessMember=b.role;out.u=us[a];out.v=vs[k];}
  }
  double other=queue.empty()?upper:queue.top().lower;
  double globalLower=std::min(lower,other);
  if(upper-globalLower<=policy.distanceToleranceMm){
   out.mm=I(std::max(0.0,globalLower),upper);out.lowerMember=lower<=other?b.role:bodies[queue.top().body].role;
   out.converged=true;break;
  }
  if(p.depth>=policy.maxDepth){
   p.lower=lower;queue.push(p);break;
  }
  double su=(p.u1-p.u0)*(b.kind==Body::Sector?b.outer.hi*b.theta:
      std::hypot(b.last.s.hi-b.first.s.lo,b.last.z.hi-b.first.z.lo));
  double sv=(p.v1-p.v0)*(b.kind==Body::Sector?b.outer.hi-b.inner.lo:2*b.half.hi);
  Patch a=p,c=p;a.depth=c.depth=p.depth+1;
  if(su>=sv){double m=(p.u0+p.u1)/2;a.u1=m;c.u0=m;}else{double m=(p.v0+p.v1)/2;a.v1=m;c.v0=m;}
  a.lower=bounded::boxDistance(b.enclosure(I(a.u0,a.u1),I(a.v0,a.v1)),ob).lo;
  c.lower=bounded::boxDistance(b.enclosure(I(c.u0,c.u1),I(c.v0,c.v1)),ob).lo;
  queue.push(a);queue.push(c);
 }
 if(!out.converged){double low=queue.empty()?upper:queue.top().lower;
  out.mm=I(std::max(0.0,low),upper);if(!queue.empty())out.lowerMember=bodies[queue.top().body].role;}
 if(out.mm.lo>0)out.relation=PositiveDistance;
 else if(allSeparated&&out.mm.hi==0)out.relation=Touching;
 return out;
}
struct Result {
 Status status;std::string reason;bool geometryReady,minimumConfirmed;double jointDeviationUpperMm;
 tvsOffset::Result offsets[2];std::vector<Body> members;std::vector<V> axis;
 I run,passElevation,slopeLength,bridgeLength,physicalRise;Distance obstacle;
 Role selfFirst,selfSecond;
 Result():status(InvalidInput),geometryReady(false),minimumConfirmed(false),jointDeviationUpperMm(0),
 selfFirst(FirstElbow),selfSecond(FirstElbow){}
};
inline V translate(const tvsOffset::Point& q,I s,I z){return V(s+I(q.s),z+I(q.z));}
inline Body offsetSector(const tvsOffset::Result& r,int j,I s,I z,I ri,I ro,int sign,double theta,Role role){
 return sector(translate(r.centers[j],s,z),ri,ro,theta,sign,j==0?1:-1,role);
}
inline void port(const Body& b,bool exit,V out[2]){
 if(b.kind==Body::Straight){out[0]=b.at(I(exit?1:0),I(0));out[1]=b.at(I(exit?1:0),I(1));}
 else {double u=(b.side==1)==exit?1:0;out[0]=b.at(I(u),I(0));out[1]=b.at(I(u),I(1));}
}
inline double agreement(const std::vector<Body>& bodies){
 double error=0;
 for(size_t j=0;j+1<bodies.size();++j){V a[2],b[2];port(bodies[j],true,a);port(bodies[j+1],false,b);
  double direct=std::max(bounded::distance(a[0],b[0]).hi,bounded::distance(a[1],b[1]).hi);
  double reverse=std::max(bounded::distance(a[0],b[1]).hi,bounded::distance(a[1],b[0]).hi);
  error=std::max(error,std::min(direct,reverse));}
 return error;
}
inline bool scalar(double x){return std::isfinite(x)&&std::fabs(x)<=1e9;}
inline Result check(const Input& in){
 Result out;
 if(!bounded::supported()){out.status=Unsupported;out.reason="IEEE binary64/FE_TONEAREST required";return out;}
 try{
 const double fields[]={in.width,in.height,in.ri,in.angleDegrees,in.z0,in.obstacleS,in.obstacleZ,
 in.obstacleWidth,in.obstacleHeight,in.insulation,in.obstacleInsulation,in.clearance,in.D,in.C,in.s0,
 in.windowLeft,in.windowRight,in.obstacleTmin,in.obstacleTmax,in.minimumSlope,in.minimumBridge,in.minimumProjection,
 in.policy.distanceToleranceMm,in.policy.jointToleranceMm};
 for(size_t j=0;j<sizeof(fields)/sizeof(fields[0]);++j)if(!scalar(fields[j])){out.reason="nonfinite/out-of-domain input";return out;}
 if(in.width<=0||in.height<=0||in.obstacleWidth<=0||in.obstacleHeight<=0||in.ri<0||in.insulation<0||
 in.obstacleInsulation<0||in.clearance<0||in.D<=0||in.C<=0||in.minimumSlope<0||in.minimumBridge<0||
 in.minimumProjection<0||in.windowLeft>in.windowRight||in.obstacleTmin>=in.obstacleTmax||
 in.policy.distanceToleranceMm<=0||in.policy.jointToleranceMm<=0||!in.policy.maxCells||!in.policy.maxDepth||
 !in.policy.supportSlices||in.policy.supportSlices>4096){out.reason="invalid dimension, policy or limit";return out;}
 if((in.floor.active&&!scalar(in.floor.mm))||(in.ceiling.active&&!scalar(in.ceiling.mm))||
 (in.floor.active&&in.ceiling.active&&in.floor.mm>in.ceiling.mm)){out.reason="invalid active height limit";return out;}
 if((in.angleDegrees!=30&&in.angleDegrees!=45&&in.angleDegrees!=60)||in.ri<=in.insulation){
  out.status=Unsupported;out.reason="MVP: 30/45/60 degrees and Ri > insulation >= 0";return out;}
 I transverse=I(in.width)/I(2)+I(in.insulation)+I(in.clearance);
 if(in.obstacleTmin>-transverse.hi||in.obstacleTmax<transverse.hi){
  out.status=Unsupported;out.reason="full transverse overlap including end margin not proven";return out;}
 double theta=tvsOffset::pi()*in.angleDegrees/180;int sign=in.above?1:-1;
 tvsOffset::Policy base;base.minimumProjection=in.minimumProjection;
 tvsOffset::Input first(sign*in.D,in.width,in.height,in.ri,theta,in.minimumSlope);
 tvsOffset::Input second(-sign*in.D,in.width,in.height,in.ri,theta,in.minimumSlope);
 out.offsets[0]=tvsOffset::solve(first,base);out.offsets[1]=tvsOffset::solve(second,base);
 if(!out.offsets[0].hasGeometry()||!out.offsets[1].hasGeometry()){
  out.status=Infeasible;out.reason="existing solve rejected offset/minimum; strict policy unchanged";return out;}
 if(out.offsets[0].projectedLength<=0){out.status=Unsupported;out.reason="zero projected slope";return out;}
 I s(in.s0),z(in.z0),h=I(in.height)/I(2)+I(in.insulation),ri=I(in.ri)-I(in.insulation),ro=I(in.ri)+I(in.height)+I(in.insulation);
 I x(out.offsets[0].totalRun),s2=s+x+I(in.C),z2=z+I(sign)*I(in.D);
 I t(theta);V n(-I(sign)*bounded::sine(t),bounded::cosine(t)),n2(I(sign)*bounded::sine(t),bounded::cosine(t));
 out.run=x+I(in.C)+I(out.offsets[1].totalRun);out.passElevation=z2;out.slopeLength=I(out.offsets[0].straightLength);
 out.bridgeLength=I(in.C);out.physicalRise=I(out.offsets[0].axis[2].z)-I(out.offsets[0].axis[1].z);
 out.members.push_back(offsetSector(out.offsets[0],0,s,z,ri,ro,sign,theta,FirstElbow));
 out.members.push_back(straight(translate(out.offsets[0].axis[1],s,z),translate(out.offsets[0].axis[2],s,z),n,h,FirstSlope));
 out.members.push_back(offsetSector(out.offsets[0],1,s,z,ri,ro,sign,theta,SecondElbow));
 V q3=translate(out.offsets[0].axis[3],s,z),q4(s2,z2);
 out.members.push_back(straight(q3,q4,V(I(0),I(1)),h,Bridge));
 out.members.push_back(offsetSector(out.offsets[1],0,s2,z2,ri,ro,-sign,theta,ThirdElbow));
 out.members.push_back(straight(translate(out.offsets[1].axis[1],s2,z2),translate(out.offsets[1].axis[2],s2,z2),n2,h,SecondSlope));
 out.members.push_back(offsetSector(out.offsets[1],1,s2,z2,ri,ro,-sign,theta,FourthElbow));
 for(int j=0;j<4;++j)out.axis.push_back(translate(out.offsets[0].axis[j],s,z));
 for(int j=0;j<4;++j)out.axis.push_back(translate(out.offsets[1].axis[j],s2,z2));
 out.geometryReady=true;
 out.jointDeviationUpperMm=agreement(out.members);
 if(out.jointDeviationUpperMm>in.policy.jointToleranceMm){out.status=NumericalBoundary;out.reason="joint agreement not proven within explicit tolerance";return out;}
 if(in.C<in.minimumBridge){out.status=Infeasible;out.reason="minimum horizontal bridge";return out;}
 I end=s+out.run;bool uncertain=s.lo<in.windowLeft||end.hi>in.windowRight;
 if(s.hi<in.windowLeft||end.lo>in.windowRight){out.status=Infeasible;out.reason="available window";return out;}
 // Body bounds include the computed seam discrepancy as a conservative assembly guard.
 double guard=out.jointDeviationUpperMm;
 V tangent[]={V(bounded::cosine(t),I(sign)*bounded::sine(t)),V(bounded::cosine(t),I(sign)*bounded::sine(t)),
 V(I(1),I(0)),V(I(1),I(0)),V(bounded::cosine(t),-I(sign)*bounded::sine(t)),V(bounded::cosine(t),-I(sign)*bounded::sine(t))};
 for(int j=0;j<6;++j){
  I a=out.members[j].support(tangent[j],in.policy.supportSlices);
  I b=out.members[j+1].support(tangent[j],in.policy.supportSlices);
  double overlap=std::max(0.0,(I(a.hi)-I(b.lo)).hi);
  if(overlap>in.policy.jointToleranceMm){out.status=NumericalBoundary;out.reason="adjacent joint half-plane agreement unresolved";return out;}
  guard=std::max(guard,overlap);
 }
 out.jointDeviationUpperMm=guard;
 for(size_t j=0;j<out.members.size();++j){Box b=out.members[j].enclosure(I(0,1),I(0,1));
  if(in.floor.active){I bottom=I(b.z.lo)-I(guard);
   if(bottom.hi<in.floor.mm){ // A lower enclosure alone is not a penetration witness: inspect actual points.
    bool violation=false;for(int u=0;u<3;++u)for(int v=0;v<3;++v)
     if(out.members[j].at(I(u*.5),I(v*.5)).z.hi<in.floor.mm)violation=true;
    if(violation){out.status=Infeasible;out.reason="floor outside-envelope violation";return out;}}
   if(bottom.lo<in.floor.mm)uncertain=true;}
  if(in.ceiling.active){I top=I(b.z.hi)+I(guard);
   if(top.lo>in.ceiling.mm){bool violation=false;for(int u=0;u<3;++u)for(int v=0;v<3;++v)
     if(out.members[j].at(I(u*.5),I(v*.5)).z.lo>in.ceiling.mm)violation=true;
    if(violation){out.status=Infeasible;out.reason="ceiling outside-envelope violation";return out;}}
   if(top.hi>in.ceiling.mm)uncertain=true;}
 }
 // All nonadjacent physical profile pieces must be separated. Adjacent pairs share a joint.
 for(size_t a=0;a<7;++a)for(size_t b=a+2;b<7;++b)
  if(!separated(out.members[a],out.members[b],in.policy,2*guard)){
   out.selfFirst=out.members[a].role;out.selfSecond=out.members[b].role;
   Relation relation=pairRelation(out.members[a],out.members[b],in.policy);
   out.status=relation==Overlapping?Infeasible:NumericalBoundary;
   out.reason=relation==Overlapping?"nonadjacent solid overlap":"nonadjacent separation unresolved";return out;}
 std::vector<Body> checked=out.members;
 if(in.windowLeft<in.s0)checked.push_back(straight(V(I(in.windowLeft),z),out.axis[0],V(I(0),I(1)),h,IncomingNeighbor));
 if(end.hi<in.windowRight)checked.push_back(straight(out.axis[7],V(I(in.windowRight),z),V(I(0),I(1)),h,OutgoingNeighbor));
 I oh=I(in.obstacleHeight)/I(2)+I(in.obstacleInsulation),ow=I(in.obstacleWidth)/I(2)+I(in.obstacleInsulation);
 Region obstacle(I(in.obstacleS)-ow,I(in.obstacleS)+ow,I(in.obstacleZ)-oh,I(in.obstacleZ)+oh);
 out.obstacle=measure(checked,obstacle,in.policy);
 if(out.obstacle.relation==Overlapping){out.status=Infeasible;out.reason="solid overlap/containment";return out;}
 if((I(out.obstacle.mm.hi)+I(guard)).hi<in.clearance){out.status=Infeasible;out.reason="clearance violated";return out;}
 I guaranteed=I(out.obstacle.mm.lo)-I(guard);
 if(!uncertain&&guaranteed.lo>=in.clearance &&
    (out.obstacle.relation==PositiveDistance||out.obstacle.relation==Touching)){
  out.status=Feasible;out.reason="constraints proven for bounded model; minimum reach NOT confirmed";return out;}
 out.status=NumericalBoundary;out.reason="boundary, work limit or assembly guard unresolved";return out;
 }catch(const std::exception& e){out.status=NumericalBoundary;out.reason=e.what();return out;}
}
}
#endif

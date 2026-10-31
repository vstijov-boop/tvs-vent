#include "../geometry/bypass/Candidate.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <limits>
using namespace tvsBypass;
static int checks=0;
static void expect(bool ok,const char* message){++checks;if(!ok){std::cerr<<"FAIL "<<message<<"\n";std::exit(1);}}
static void contains(I a,double x,const char* what){expect(a.lo<=x+1e-10&&a.hi>=x-1e-10,what);}
static Input reference(){Input p;p.D=289.64466094067262;p.C=179.28932188134524;p.s0=-544.9747468305833;return p;}
static void report(const char* label,const Result& r){
 std::cout<<std::setprecision(15)<<label<<" status="<<r.status<<" "<<r.reason
 <<" distance=["<<r.obstacle.mm.lo<<","<<r.obstacle.mm.hi<<"] guard="<<r.jointDeviationUpperMm<<" cells="<<r.obstacle.cells<<std::endl;
}
int main(){
 expect(bounded::supported(),"supported strict IEEE runtime");
 I a=bounded::root(I(2));long double root2=1.414213562373095048801688724209698L;
 expect(static_cast<long double>(a.lo)<=root2&&static_cast<long double>(a.hi)>=root2,"independent sqrt oracle");
 I tiny=I(1e-250)*I(1e-250);expect(tiny.lo<=0&&tiny.hi>0,"underflow not called exact zero");
 expect(bounded::sine(I(0)).point()&&bounded::cosine(I(0)).lo==1,"exact zero trig");
 I ss=bounded::sine(I(tvsOffset::pi()/6)),cc=bounded::cosine(I(tvsOffset::pi()/3));
 contains(ss,.5,"sin30 independent .5");contains(cc,.5,"cos60 independent .5");
 int mode=std::fegetround();std::fesetround(FE_DOWNWARD);expect(check(reference()).status==Unsupported,"candidate rejects wrong rounding");expect(!bounded::supported(),"wrong rounding rejected");std::fesetround(mode);
 Input p=reference();p.clearance=49.9;Result r=check(p);report("reference below with explicit .1 margin",r);
 expect(r.geometryReady&&r.members.size()==7&&r.axis.size()==8,"seven shared-result members/eight ports");
 expect(r.status==Feasible,"reference positive margin feasible");expect(!r.minimumConfirmed,"no optimizer claim");
 contains(r.run,1089.9494936611665,"fixed run oracle");
 contains(r.slopeLength,243.93398282201787,"independent fixed slope");
 contains(r.physicalRise,-172.48737341529164,"slope rise not full D");
 contains(r.obstacle.mm,50,"reference critical distance oracle");
 p.above=true;r=check(p);report("mirrored above",r);expect(r.status==Feasible,"above feasible");
 contains(r.passElevation,-110.35533905932738,"above absolute elevation");contains(r.obstacle.mm,50,"above gap");
 p=reference();r=check(p);report("exact requested 50 boundary",r);
 expect(r.status==NumericalBoundary||r.status==Feasible,"do not falsely reject unresolved numerical reference");
 expect(r.obstacle.mm.hi-r.obstacle.mm.lo<=.0011,"reference distance enclosure resolution");
 p=reference();p.insulation=p.obstacleInsulation=25;p.D=332.32233047033631;p.C=264.64466094067262;p.s0=-630.3300858899107;p.clearance=49.9;
 r=check(p);report("insulated",r);expect(r.status==Feasible,"insulated margin feasible");contains(r.obstacle.mm,50,"insulated critical gap");
 contains(r.slopeLength,304.28932188134524,"insulated fixed slope oracle");
 p=reference();p.D=400;p.C=400;p.s0=-765.685424949238;r=check(p);report("nonoptimal wide candidate",r);
 expect(r.status==Feasible&&!r.minimumConfirmed,"nonoptimal candidate accepted");contains(r.obstacle.mm,175,"independent flat 175 gap");
 p=reference();p.windowLeft=-500;p.windowRight=500;r=check(p);expect(r.status==Infeasible&&r.geometryReady,"short window");
 p=reference();p.s0+=10;r=check(p);report("slope too close to corner",r);
 expect(r.status==Infeasible,"asymmetric shift clearance fail");
 contains(r.obstacle.mm,42.928932188134525,"independent 50-10/sqrt2 slope oracle");
 p=reference();p.D=400;p.C=400;p.s0=-715.685424949238;p.windowLeft=-1000;p.windowRight=900;
 r=check(p);expect(r.status==Feasible,"asymmetric feasible position");
 p=reference();p.minimumBridge=200;expect(check(p).status==Infeasible,"active bridge minimum");
 p=reference();p.minimumSlope=300;expect(check(p).status==Infeasible,"active slope minimum via original solve");
 p=reference();p.minimumProjection=200;expect(check(p).status==Infeasible,"active projection minimum");
 p=reference();p.clearance=49.9;p.floor.active=true;p.floor.mm=-780;expect(check(p).status==Infeasible,"floor full outer body");
 p=reference();p.clearance=49.9;p.ceiling.active=true;p.ceiling.mm=-350;expect(check(p).status==Infeasible,"ceiling includes entry elbows");
 p=reference();p.clearance=49.9;p.floor.mm=p.ceiling.mm=std::numeric_limits<double>::quiet_NaN();
 expect(check(p).status==Feasible,"inactive height fields really inactive");
 p=reference();p.floor.active=p.ceiling.active=true;p.floor.mm=0;p.ceiling.mm=-1;expect(check(p).status==InvalidInput,"contradictory height bounds");
 p=reference();p.angleDegrees=90;expect(check(p).status==Unsupported,"90 not approximated");
 p=reference();p.angleDegrees=37;expect(check(p).status==Unsupported,"fixed-angle MVP");
 p=reference();p.ri=0;expect(check(p).status==Unsupported,"Ri zero physical model");
 p=reference();p.insulation=100;expect(check(p).status==Unsupported,"insulation closes inner radius");
 p=reference();p.obstacleTmin=-100;expect(check(p).status==Unsupported,"partial transverse coverage");
 p=reference();p.s0=std::numeric_limits<double>::quiet_NaN();expect(check(p).status==InvalidInput,"nonfinite input");
 p=reference();p.insulation=-1;expect(check(p).status==InvalidInput,"negative insulation");
 p=reference();p.C=-1;expect(check(p).status==InvalidInput,"negative bridge");
 Policy policy;
 std::vector<Body> b;
 b.push_back(sector(V(),I(100),I(300),tvsOffset::pi()/3,1,1,FirstElbow));
 Region o(I(29),I(30),I(-40),I(-39));
 Distance d=measure(b,o,policy);std::cout<<"interior arc oracle ["<<d.mm.lo<<","<<d.mm.hi<<"] u="<<d.u<<" v="<<d.v<<std::endl;
 expect(d.relation==PositiveDistance,"arc disjoint");contains(d.mm,50,"independent 3-4-5 inner-circle gap");
 expect(d.u>.5&&d.u<.75,"minimum in interior of arc, not endpoint");
 expect(std::fabs((d.onBody.s.lo+d.onBody.s.hi)/2-60)<1,"interior arc witness x near 60");
 expect(std::fabs((d.onBody.z.lo+d.onBody.z.hi)/2+80)<1,"interior arc witness z near -80");
 I angle(tvsOffset::pi()/4);
 b.clear();b.push_back(straight(V(I(0),I(0)),V(I(100),I(100)),V(-bounded::sine(angle),bounded::cosine(angle)),I(5),FirstSlope));
 d=measure(b,Region(I(50),I(60),I(70),I(80)),policy);
 contains(d.mm,2.0710678118654752,"independent 5sqrt2-5 gap");expect(d.relation==PositiveDistance&&d.mm.hi<3,"positive distance but insufficient clearance");
 Body big=straight(V(I(-10),I(0)),V(I(10),I(0)),V(I(0),I(1)),I(10),FirstElbow);
 b.clear();b.push_back(big);d=measure(b,Region(I(-1),I(1),I(-1),I(1)),policy);
 expect(d.relation==Overlapping&&d.mm.hi==0,"obstacle contained: not positive boundary gap");
 b[0]=straight(V(I(-1),I(0)),V(I(1),I(0)),V(I(0),I(1)),I(1),FirstElbow);
 d=measure(b,Region(I(-10),I(10),I(-10),I(10)),policy);expect(d.relation==Overlapping,"body contained in obstacle");
 b[0]=straight(V(I(-10),I(0)),V(I(10),I(0)),V(I(0),I(1)),I(1),FirstElbow);
 d=measure(b,Region(I(-1),I(1),I(-10),I(10)),policy);expect(d.relation==Overlapping,"crossing solids");
 b[0]=straight(V(I(-10),I(0)),V(I(0),I(0)),V(I(0),I(1)),I(10),Bridge);
 d=measure(b,Region(I(0),I(10),I(-1),I(1)),policy);expect(d.relation==Touching&&d.mm.point()&&d.mm.lo==0,"c0 exact touch separated interiors");
 b[0].last.s=I(1);d=measure(b,Region(I(0),I(10),I(-1),I(1)),policy);
 expect(d.relation==Overlapping,"c0 overlap is not allowed touch");
 expect(pairRelation(big,b[0],policy)==Overlapping,"nonadjacent body overlap proof");
 p=reference();p.clearance=49.9;p.policy.maxCells=1;r=check(p);expect(r.status==NumericalBoundary,"work limit not false infeasible");
 
 Body touchA=straight(V(I(-10),I(0)),V(I(0),I(0)),V(I(0),I(1)),I(1),FirstElbow);
 Body touchB=straight(V(I(0),I(0)),V(I(10),I(0)),V(I(0),I(1)),I(1),FourthElbow);
 expect(pairRelation(touchA,touchB,policy)==Touching,"pair boundary touch is not positive distance");
 p=reference();p.D=225;p.C=400;p.s0=-590.685424949238;p.clearance=0;r=check(p);report("candidate zero-gap boundary",r);
 expect(r.obstacle.relation==Touching&&r.status==NumericalBoundary,"c0 touch is distinct; assembly guard stays unresolved");
 p.D=224.5;r=check(p);expect(r.status==Infeasible&&r.obstacle.relation==Overlapping,"candidate c0 actual penetration rejected");
 p=reference();p.clearance=49.9;p.s0+=1000000;p.windowLeft+=1000000;p.windowRight+=1000000;p.obstacleS+=1000000;p.z0+=1000000;p.obstacleZ+=1000000;r=check(p);
 expect(r.status==Feasible,"physical translation invariance with bounded rounding");
 p=reference();p.angleDegrees=30;p.D=281.69872981077805;p.C=200;p.s0=-695.0961894323342;p.clearance=49.9;r=check(p);
 report("fixed 30",r);expect(r.status==Feasible,"fixed 30 candidate");contains(r.obstacle.mm,50,"30 independent contact gap");
 p=reference();p.angleDegrees=60;p.D=300;p.C=163.39745962155614;p.s0=-485.8439182435161;p.clearance=49.9;r=check(p);
 report("fixed 60",r);expect(r.status==Feasible,"fixed 60 candidate");contains(r.obstacle.mm,50,"60 independent contact gap");
 p=reference();p.D=187.86796564403573;p.C=400;p.s0=-500;
 r=check(p);expect(r.status==NumericalBoundary,"strict Lmin decimal boundary stays unresolved, not repaired");
 p=reference();p.policy.distanceToleranceMm=-1;expect(check(p).status==InvalidInput,"invalid numerical policy");
 std::cout<<"ALL PASSED "<<checks<<" checks. Candidate validation only, no optimizer/CAD."<<std::endl;
}

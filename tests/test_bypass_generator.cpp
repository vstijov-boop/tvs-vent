#include "../geometry/bypass/Generator.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <limits>
using namespace tvsBypassSearch;
static int tests=0,solutions=0;
static void expect(bool yes,const char* message){
 ++tests;if(!yes){std::cerr<<"FAIL GENERATOR "<<message<<std::endl;std::exit(1);}
}
static void near(double value,double expected,const char* name){expect(std::fabs(value-expected)<.000001,name);}
static Result run(const char* name,const tvsBypass::Input& in,const Policy& p,bool mustFind){
 Result r=generate(in,p);
 expect(!r.minimumConfirmed,"never claims a minimum");
 expect(r.attempts<=p.maxCheckerCalls&&r.processedCells<=p.maxTotalCells&&r.proposals<=p.maxProposals,
 "work budgets respected");
 expect(r.seedSolveCalls<=p.maxSeedSolves,"seed solve budget respected");
 expect(r.elapsedMs>=0,"measured duration");
 if(mustFind)expect(r.hasCandidate&&r.status==Found,"expected verified feasible candidate");
 if(r.hasCandidate){
  ++solutions;
  expect(r.status==Found&&r.checked.status==tvsBypass::Feasible,"accepted implies Feasible");
  expect(r.candidate.clearance==in.clearance,"required clearance is unchanged");
  expect(r.clearance.lo>=in.clearance+p.designReserveMm,"explicit reserve independently checked");
  tvsBypass::Result again=tvsBypass::check(r.candidate);
  expect(again.status==tvsBypass::Feasible,"independent repeated Candidate.check passes");
  expect(r.checked.geometryReady&&again.members.size()==7,"whole seven-member result retained");
  expect(r.s0>=in.windowLeft&&r.s0+r.W.hi<=in.windowRight,"within given window");
  expect(r.candidate.D==r.D&&r.candidate.C==r.C&&r.candidate.s0==r.s0,"reported parameters agree");
 }
 std::cout<<std::setprecision(17)<<"REFERENCE {\"case\":\""<<name<<"\",\"found\":"<<(r.hasCandidate?"true":"false")
 <<",\"status\":"<<static_cast<int>(r.status)<<",\"D\":"<<r.D<<",\"C\":"<<r.C<<",\"s0\":"<<r.s0
 <<",\"Wlo\":"<<r.W.lo<<",\"Whi\":"<<r.W.hi<<",\"passLo\":"<<r.passElevation.lo
 <<",\"passHi\":"<<r.passElevation.hi;
 if(r.hasCandidate)std::cout<<",\"gapLo\":"<<r.clearance.lo<<",\"gapHi\":"<<r.clearance.hi;
 std::cout<<",\"calls\":"<<r.attempts<<",\"cells\":"<<r.processedCells<<",\"seedSolves\":"<<r.seedSolveCalls
 <<",\"proposals\":"<<r.proposals<<",\"improvements\":"<<r.improvements
 <<",\"elapsedMs\":"<<r.elapsedMs<<",\"reason\":\""<<r.termination<<"\",\"minimumConfirmed\":false}"<<std::endl;
 return r;
}
int main(){
 tvsBypass::Input base(100,100,.1);base.windowLeft=-700;base.windowRight=700;
 Policy p(5,10,25);
 Result reference=run("below45",base,p,true);
 // Independently fixed numeric expectations: R=200, D=290, C=360; W=580+400*sqrt(2)+? 
 // W=2*(290+400*(sqrt(2)-1))+360 = 1271.370849898476.
 near(reference.D,290,"known chosen D");near(reference.C,360,"known chosen C");
 near(reference.W.lo,1271.370849898476,"independently specified 45 degree run");
 near(reference.passElevation.lo,-690,"known pass elevation");
 expect(reference.clearance.lo>64.99&&reference.clearance.hi<65.01,"known 65 mm clearance");
 tvsBypass::Input q=base;q.above=true;
 Result mirror=run("above45",q,p,true);near(mirror.passElevation.lo,-110,"above reference elevation");
 near(mirror.W.lo,reference.W.lo,"mirrored run agrees");
 for(int j=0;j<3;++j){q=base;q.angleDegrees=30+15*j;q.windowLeft=-1100;q.windowRight=1100;
  run(j==0?"angle30":j==1?"angle45":"angle60",q,p,true);}
 q=base;q.insulation=25;q.obstacleInsulation=25;q.windowLeft=-900;q.windowRight=900;
 run("insulated",q,p,true);
 q=base;q.windowLeft=-600;q.windowRight=600;run("narrow-feasible",q,p,true);
 q=base;q.windowLeft=-580;q.windowRight=820;
 Result asymmetric=run("asymmetric",q,p,true);
 expect(std::fabs(asymmetric.s0+asymmetric.W.lo/2)>1,"asymmetric placement is not forced to obstacle center");
 q=base;q.floor.active=true;q.floor.mm=-850;run("active-floor",q,p,true);
 q=base;q.above=true;q.ceiling.active=true;q.ceiling.mm=100;run("active-ceiling",q,p,true);
 q=base;q.floor.active=true;q.floor.mm=-650; // Required bridge axis <=-675, lower face <=-775: impossible.
 Result blocked=run("floor-proven-impossible",q,p,false);expect(!blocked.hasCandidate,"known impossible floor rejected");
 q=base;q.windowLeft=-500;q.windowRight=500;
 // Even allowing C=100, D>=275 gives W >=100+700*sqrt(2)=1089.94949 >1000.
 blocked=run("window-proven-impossible",q,p,false);expect(!blocked.hasCandidate,"known impossible window rejected");
 expect(blocked.status==NoCandidateFoundWithinBudget||blocked.status==Unresolved,"search failure not labeled global infeasibility");
 q=base;q.minimumSlope=300;q.minimumBridge=420;q.windowLeft=-1100;q.windowRight=1100;
 Result minima=run("active-minima",q,p,true);
 expect(minima.checked.slopeLength.lo>=300&&minima.C>=420,"active physical minima honored");
 q=base;q.floor.active=true;q.floor.mm=-775;
 Policy boundary(0,10,25);boundary.depthLevels=1;
 Result edge=run("numerical-boundary",q,boundary,false);
 expect(!edge.hasCandidate&&edge.status==Unresolved,"exact floor/clearance boundary is not accepted");
 Policy limit=p;limit.maxCheckerCalls=0;
 Result zero=run("zero-calls",base,limit,false);expect(!zero.hasCandidate&&zero.attempts==0,"zero calls enforced");
 limit=p;limit.maxTotalCells=0;zero=run("zero-cells",base,limit,false);expect(zero.attempts==0,"zero cells enforced");
 limit=p;limit.maxElapsedMs=0;zero=run("zero-time",base,limit,false);expect(zero.attempts==0,"zero time enforced");
 limit=p;limit.maxProposals=0;zero=run("zero-proposals",base,limit,false);expect(zero.attempts==0,"zero proposals enforced");
 limit=p;limit.maxSeedSolves=0;zero=run("zero-seeds",base,limit,false);expect(zero.seedSolveCalls==0,"zero seeds enforced");
 limit=p;limit.maxTotalCells=1;zero=run("one-cell",base,limit,false);
 expect(!zero.hasCandidate&&zero.status==Unresolved,"cell limit unresolved rather than feasible");
 limit=p;limit.stopAtFirstFeasible=false;limit.maxCheckerCalls=1;
 Result retained=run("retained-at-call-limit",base,limit,true);
 expect(retained.attempts==1&&retained.termination=="checker-call budget","best retained on work termination");
 limit=p;limit.stopAtFirstFeasible=false;limit.maxCheckerCalls=12;
 Result improved=run("bounded-improvement",base,limit,true);
 expect(improved.W.hi<=reference.W.hi,"improvement never discards best for a larger run");
 q=base;q.D=q.C=q.s0=std::numeric_limits<double>::quiet_NaN();q.floor.mm=q.ceiling.mm=q.D;
 run("ignored-candidate-and-inactive-bounds",q,p,true);
 q=base;q.clearance=-1;expect(generate(q,p).status==InvalidInput,"negative clearance invalid");
 q=base;q.floor.active=true;q.floor.mm=std::numeric_limits<double>::quiet_NaN();
 expect(generate(q,p).status==InvalidInput,"active NaN bound invalid");
 q=base;q.angleDegrees=40;expect(generate(q,p).status==Unsupported,"unsupported angle rejected");
 q=base;q.ri=q.insulation;expect(generate(q,p).status==Unsupported,"unsupported radius rejected");
 limit=p;limit.depthStepMm=0;expect(generate(base,limit).status==InvalidInput,"zero step invalid");
 {
  tvsBypass::bounded::Environment outer;std::feclearexcept(FE_ALL_EXCEPT);std::feraiseexcept(FE_DIVBYZERO|FE_INEXACT);
  int flags=std::fetestexcept(FE_ALL_EXCEPT);Result saved=generate(base,p);
  expect(saved.hasCandidate&&std::fetestexcept(FE_ALL_EXCEPT)==flags,"generator restores floating exception flags");
  expect(std::fesetround(FE_UPWARD)==0,"set unsupported generator rounding");
  Result unsupported=generate(base,p);
  expect(unsupported.status==Unsupported&&std::fegetround()==FE_UPWARD,"generator rejects without normalizing host rounding");
 }
 std::cout<<"PASS GENERATOR "<<tests<<" checks, "<<solutions<<" independently rechecked solutions"<<std::endl;
}

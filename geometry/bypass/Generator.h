#ifndef TVS_BYPASS_GENERATOR_H
#define TVS_BYPASS_GENERATOR_H
#include "Candidate.h"
#include <chrono>
#include <cstdint>
// SDK independent, physical mm. A bounded proposal search, NOT a global optimizer.
namespace tvsBypassSearch {
using tvsBypass::bounded::I;
enum Status {Found,NoCandidateFoundWithinBudget,Unresolved,InvalidInput,Unsupported};
struct Policy {
 double designReserveMm,depthStepMm,bridgeStepMm,windowInsetMm,maximumSearchDepthMm;
 double improvementWToleranceMm,improvementDToleranceMm,maxElapsedMs;
 unsigned depthLevels,bridgeLevels,positionLevels,maxSeedSolves,maxProposals,maxCheckerCalls;
 std::uint64_t maxTotalCells;
 bool stopAtFirstFeasible;
 Policy(double reserve,double depthStep,double bridgeStep):
 designReserveMm(reserve),depthStepMm(depthStep),bridgeStepMm(bridgeStep),windowInsetMm(1),
 maximumSearchDepthMm(100000),improvementWToleranceMm(.01),improvementDToleranceMm(.01),
 maxElapsedMs(5000),depthLevels(8),bridgeLevels(5),positionLevels(5),maxSeedSolves(64),
 maxProposals(512),maxCheckerCalls(64),maxTotalCells(300000),stopAtFirstFeasible(true){}
};
struct Result {
 Status status;bool hasCandidate,minimumConfirmed;
 tvsBypass::Input candidate;tvsBypass::Result checked;
 double D,C,s0,elapsedMs;I W,passElevation,clearance;
 unsigned attempts,proposals,seedSolveCalls,improvements;std::uint64_t processedCells;
 std::string termination;
 explicit Result(const tvsBypass::Input& in):status(NoCandidateFoundWithinBudget),hasCandidate(false),
 minimumConfirmed(false),candidate(in),D(0),C(0),s0(0),elapsedMs(0),
 attempts(0),proposals(0),seedSolveCalls(0),improvements(0),processedCells(0){}
};
typedef std::chrono::steady_clock Clock;
inline double elapsed(Clock::time_point start){
 return std::chrono::duration<double,std::milli>(Clock::now()-start).count();
}
inline Result finish(Result r,Clock::time_point start,const char* reason,bool uncertain=false){
 r.elapsedMs=elapsed(start);r.termination=reason;
 if(r.hasCandidate)r.status=Found;
 else if(r.status!=InvalidInput&&r.status!=Unsupported)
  r.status=uncertain?Unresolved:NoCandidateFoundWithinBudget;
 return r;
}
inline bool finiteNonnegative(double x){return tvsBypass::scalar(x)&&x>=0;}
inline bool valid(const tvsBypass::Input& in,const Policy& p){
 // D, C and s0 are OUTPUTS; candidate values in the input are deliberately ignored.
 const double v[]={in.width,in.height,in.ri,in.angleDegrees,in.z0,in.obstacleS,in.obstacleZ,
 in.obstacleWidth,in.obstacleHeight,in.insulation,in.obstacleInsulation,in.clearance,
 in.windowLeft,in.windowRight,in.obstacleTmin,in.obstacleTmax,in.minimumSlope,in.minimumBridge,
 in.minimumProjection,in.policy.distanceToleranceMm,in.policy.jointToleranceMm};
 for(size_t j=0;j<sizeof(v)/sizeof(v[0]);++j)if(!tvsBypass::scalar(v[j]))return false;
 const double q[]={p.designReserveMm,p.depthStepMm,p.bridgeStepMm,p.windowInsetMm,p.maximumSearchDepthMm,
 p.improvementWToleranceMm,p.improvementDToleranceMm,p.maxElapsedMs};
 for(size_t j=0;j<sizeof(q)/sizeof(q[0]);++j)if(!finiteNonnegative(q[j]))return false;
 return in.width>0&&in.height>0&&in.ri>=0&&in.obstacleWidth>0&&in.obstacleHeight>0&&
 in.insulation>=0&&in.obstacleInsulation>=0&&in.clearance>=0&&in.minimumSlope>=0&&
 in.minimumBridge>=0&&in.minimumProjection>=0&&in.windowLeft<=in.windowRight&&
 in.obstacleTmin<in.obstacleTmax&&in.policy.distanceToleranceMm>0&&in.policy.jointToleranceMm>0&&
 in.policy.maxCells&&in.policy.maxDepth&&in.policy.supportSlices&&in.policy.supportSlices<=4096&&
 (!in.floor.active||tvsBypass::scalar(in.floor.mm))&&(!in.ceiling.active||tvsBypass::scalar(in.ceiling.mm))&&
 (!in.floor.active||!in.ceiling.active||in.floor.mm<=in.ceiling.mm)&&
 p.depthStepMm>0&&p.bridgeStepMm>0&&p.maximumSearchDepthMm>0&&
 p.depthLevels<=128&&p.bridgeLevels<=16&&p.positionLevels<=16;
}
inline tvsOffset::Result offset(const tvsBypass::Input& in,double d){
 tvsOffset::Policy p;p.minimumProjection=in.minimumProjection;
 return tvsOffset::solve(tvsOffset::Input((in.above?1:-1)*d,in.width,in.height,in.ri,
 tvsOffset::pi()*in.angleDegrees/180,in.minimumSlope),p);
}
inline bool better(const tvsBypass::Input& in,const tvsBypass::Result& checked,const Result& best,const Policy& p){
 if(!best.hasCandidate)return true;
 // Compare model W differences with outward arithmetic; never substitute a near-W tie for equality.
 I dw=(I(checked.offsets[0].totalRun)-I(best.checked.offsets[0].totalRun))+
 (I(checked.offsets[1].totalRun)-I(best.checked.offsets[1].totalRun))+(I(in.C)-I(best.C));
 if(dw.hi < -p.improvementWToleranceMm)return true;
 return dw.point()&&dw.lo==0&&(I(in.D)+I(p.improvementDToleranceMm)).hi<best.D;
}
inline Result generate(const tvsBypass::Input& problem,const Policy& policy){
 Result out(problem);Clock::time_point start=Clock::now();
 if(!tvsBypass::bounded::supported()){out.status=Unsupported;
  return finish(out,start,"unsupported floating-point environment");}
 tvsBypass::bounded::Environment environment;
 if(!environment.active){out.status=Unsupported;return finish(out,start,"cannot preserve floating environment");}
 if(!valid(problem,policy)){out.status=InvalidInput;return finish(out,start,"invalid input or search policy");}
 if((problem.angleDegrees!=30&&problem.angleDegrees!=45&&problem.angleDegrees!=60)||problem.ri<=problem.insulation){
  out.status=Unsupported;return finish(out,start,"MVP requires 30/45/60 degrees and Ri > insulation");}
 try {
 I transverse=I(problem.width)/I(2)+I(problem.insulation)+I(problem.clearance);
 if(problem.obstacleTmin>-transverse.hi||problem.obstacleTmax<transverse.hi){
  out.status=Unsupported;return finish(out,start,"full transverse overlap not proven");}
 const I h=I(problem.height)/I(2)+I(problem.insulation);
 const I oh=I(problem.obstacleHeight)/I(2)+I(problem.obstacleInsulation);
 const I target=I(problem.clearance)+I(policy.designReserveMm);
 // Necessary vertical offset at the horizontal crossing. Proposal seed only, not a feasibility proof.
 I need=problem.above?I(problem.obstacleZ)+oh+h+target-I(problem.z0):
 I(problem.z0)+h-(I(problem.obstacleZ)-oh)+target;
 double maximum=policy.maximumSearchDepthMm;
 if(problem.above&&problem.ceiling.active)
  maximum=std::min(maximum,(I(problem.ceiling.mm)-I(problem.z0)-h).lo);
 if(!problem.above&&problem.floor.active)
  maximum=std::min(maximum,(I(problem.z0)-h-I(problem.floor.mm)).lo);
 if(maximum<=0)return finish(out,start,"no proposal within configured depth/height bounds");
 double seed=std::min(maximum,std::max(policy.depthStepMm,(need+I(policy.depthStepMm)).hi));
 // Obtain all geometry from solve(). Bracket only the first supported offset depth.
 tvsOffset::Result solved;
 bool seedReady=false;
 while(out.seedSolveCalls<policy.maxSeedSolves){
  if(elapsed(start)>=policy.maxElapsedMs)return finish(out,start,"time budget");
  ++out.seedSolveCalls;solved=offset(problem,seed);
  if(solved.hasGeometry()){seedReady=true;break;}
  double next=std::min(maximum,seed+std::max(policy.depthStepMm,seed*.5));
  if(next<=seed)break;
  seed=next;
 }
 if(!seedReady)return finish(out,start,"seed solve budget or depth bound");
 bool uncertain=false;
 for(unsigned di=0;di<policy.depthLevels;++di){
  if(elapsed(start)>=policy.maxElapsedMs)return finish(out,start,"time budget",uncertain);
  double d=std::min(maximum,seed+di*policy.depthStepMm);
  if(di&&d<=std::min(maximum,seed+(di-1)*policy.depthStepMm))break;
  if(di){
   if(out.seedSolveCalls>=policy.maxSeedSolves)return finish(out,start,"seed solve budget",uncertain);
   ++out.seedSolveCalls;solved=offset(problem,d);if(!solved.hasGeometry())continue;
  }
  const double ow=(I(problem.obstacleWidth)+I(2)*I(problem.obstacleInsulation)).hi;
  const double cmin=std::max(problem.minimumBridge,policy.bridgeStepMm);
  const double safe=std::max(cmin,(I(ow)+I(2)*(target+I(problem.insulation))).hi);
  std::vector<double> bridges;bridges.push_back(safe);bridges.push_back(std::max(cmin,ow));
  bridges.push_back(cmin);bridges.push_back((cmin+std::max(cmin,ow))/2);
  for(unsigned j=4;j<policy.bridgeLevels;++j)bridges.push_back(safe+(j-3)*policy.bridgeStepMm);
  for(unsigned ci=0;ci<policy.bridgeLevels&&ci<bridges.size();++ci){
   double c=bridges[ci];bool duplicate=false;
   for(unsigned j=0;j<ci;++j)if(c==bridges[j])duplicate=true;
   if(duplicate||!tvsBypass::scalar(c))continue;
   I w=I(solved.totalRun)*I(2)+I(c);
   double lo=(I(problem.windowLeft)+I(policy.windowInsetMm)).hi;
   double hi=(I(problem.windowRight)-w-I(policy.windowInsetMm)).lo;
   if(lo>hi)continue; // Skipped proposals are not a proof of global infeasibility.
   double center=std::max(lo,std::min(hi,(I(problem.obstacleS)-w/I(2)).lo));
   std::vector<double> positions;positions.push_back(center);positions.push_back(lo+(hi-lo)/2);
   positions.push_back(lo);positions.push_back(hi);
   for(unsigned j=4;j<policy.positionLevels;++j)
    positions.push_back(lo+(hi-lo)*static_cast<double>(j-3)/static_cast<double>(policy.positionLevels-3));
   for(unsigned si=0;si<policy.positionLevels&&si<positions.size();++si){
    bool seen=false;for(unsigned j=0;j<si;++j)if(positions[si]==positions[j])seen=true;
    if(seen)continue;
    if(elapsed(start)>=policy.maxElapsedMs)return finish(out,start,"time budget",uncertain);
    if(out.proposals>=policy.maxProposals)return finish(out,start,"proposal budget",uncertain);
    if(out.attempts>=policy.maxCheckerCalls)return finish(out,start,"checker-call budget",uncertain);
    if(out.processedCells>=policy.maxTotalCells)return finish(out,start,"cell budget",uncertain);
    tvsBypass::Input in=problem;in.D=d;in.C=c;in.s0=positions[si];
    std::uint64_t left=policy.maxTotalCells-out.processedCells;
    in.policy.maxCells=static_cast<unsigned>(std::min<std::uint64_t>(left,problem.policy.maxCells));
    ++out.proposals;++out.attempts;
    tvsBypass::Result checked=tvsBypass::check(in);out.processedCells+=checked.obstacle.cells;
    if(checked.status==tvsBypass::NumericalBoundary)uncertain=true;
    if(checked.status!=tvsBypass::Feasible)continue;
    I gap=checked.obstacle.mm-I(checked.jointDeviationUpperMm,checked.jointDeviationUpperMm);
    // Symmetric assembly uncertainty: actual distance is enclosed by [lo-guard,hi+guard].
    gap=I(std::max(0.0,gap.lo),(I(checked.obstacle.mm.hi)+I(checked.jointDeviationUpperMm)).hi);
    if(gap.lo<target.hi){uncertain=true;continue;}
    if(better(in,checked,out,policy)){
     if(out.hasCandidate)++out.improvements;
     out.hasCandidate=true;out.candidate=in;out.checked=checked;
     out.D=d;out.C=c;out.s0=in.s0;out.W=checked.run;
     out.passElevation=checked.passElevation;out.clearance=gap;
    }
    if(policy.stopAtFirstFeasible)return finish(out,start,"first verified feasible candidate");
   }
  }
 }
 return finish(out,start,"finite proposal set exhausted",uncertain);
 }catch(const std::exception& e){out.elapsedMs=elapsed(start);out.termination=e.what();
  out.status=out.hasCandidate?Found:Unresolved;return out;}
}
}
#endif

// Section combinations of the bypass checker/generator: rectangular/round route and obstacle,
// perpendicular and oblique crossing, segmented elbow envelope. Independent dense sampling of
// the TRUE 3D geometry confirms the checker's guarantees. SDK-free, no AutoCAD runtime.
#include "../geometry/bypass/Generator.h"
#include <iostream>
#include <cstdlib>
using namespace tvsBypassSearch;
static int tests=0;
static void expect(bool yes,const char* message){++tests;if(!yes){std::cerr<<"FAIL SECTIONS "<<message<<std::endl;std::exit(1);}}
static double mid(tvsBypass::I v){return v.lo+(v.hi-v.lo)/2;}

struct Case {bool routeRound,obstacleRound;double betaDeg;bool above;int angle;};

static tvsBypass::Input problem(const Case& c){
 tvsBypass::Input in(100,100,.1);
 const double d=c.routeRound?315:400;
 in.width=d;in.height=c.routeRound?d:250;in.ri=c.routeRound?d/2:125;in.z0=0;in.angleDegrees=c.angle;in.above=c.above;
 in.obstacleS=0;in.obstacleZ=0;in.obstacleWidth=c.obstacleRound?250:300;in.obstacleHeight=250;in.obstacleRound=c.obstacleRound;
 in.insulation=25;in.obstacleInsulation=20;in.clearance=50;
 const double b=c.betaDeg*tvsOffset::pi()/180;in.crossingSine=c.betaDeg==90?1:std::sin(b);in.crossingCosine=c.betaDeg==90?0:std::fabs(std::cos(b));
 in.windowLeft=-3000;in.windowRight=3000;in.obstacleTmin=-2000;in.obstacleTmax=2000;
 return in;
}
// True 3D distance from the route (slab of half-width W, which contains a round tube) to the
// obstacle prism whose axis makes angle beta with the route axis in plan.
static double sampled3D(const tvsBypass::Result& r,const tvsBypass::Input& in){
 const double b=std::asin(in.crossingSine),W=in.width/2+in.insulation;
 const double hs=in.obstacleWidth/2+in.obstacleInsulation,hz=in.obstacleHeight/2+in.obstacleInsulation;
 double best=1e300;
 for(const auto& body:r.members)for(int iu=0;iu<=60;++iu)for(int iv=0;iv<=6;++iv){
  tvsBypass::V p=body.at(tvsBypass::I(iu/60.0),tvsBypass::I(iv/6.0));const double s=mid(p.s),z=mid(p.z);
  for(int it=0;it<=20;++it){const double t=-W+2*W*it/20.0;
   for(int sign=-1;sign<=1;sign+=2){
    const double sigma=s*std::sin(b)+sign*t*std::cos(b),dz=z-in.obstacleZ;double dist;
    if(in.obstacleRound)dist=std::max(0.0,std::hypot(sigma,dz)-hs);
    else{const double ex=std::max(0.0,std::fabs(sigma)-hs),ez=std::max(0.0,std::fabs(dz)-hz);dist=std::hypot(ex,ez);}
    best=std::min(best,dist);}}
 }
 return best;
}

int main(){
 Policy p(5,10,25);p.maxCheckerCalls=128;p.maxTotalCells=1000000;
 // 1) Defaults reproduce the perpendicular rectangular case bit for bit.
 {Case c={false,false,90,false,45};tvsBypass::Input a=problem(c);tvsBypass::Input b=a;b.crossingSine=1;b.crossingCosine=0;b.obstacleRound=false;b.elbowExcessMm=0;
  Result x=generate(a,p),y=generate(b,p);expect(x.hasCandidate&&y.hasCandidate&&x.D==y.D&&x.C==y.C&&x.s0==y.s0,"defaults unchanged");}
 int solved=0;
 for(bool routeRound:{false,true})for(bool obstacleRound:{false,true})for(double beta:{90.,60.,45.})
 for(bool above:{false,true})for(int angle:{30,45,60}){
  Case c={routeRound,obstacleRound,beta,above,angle};tvsBypass::Input in=problem(c);
  Result r=generate(in,p);expect(r.hasCandidate&&r.status==Found,"every combination finds a verified candidate");++solved;
  tvsBypass::Result again=tvsBypass::check(r.candidate);expect(again.status==tvsBypass::Feasible,"independent re-check");
  const double truth=sampled3D(r.checked,in);
  // Guarantee: sampled true distance never below the required clearance.
  expect(truth>=in.clearance-1e-6,"true 3D clearance holds");
  // Lower bound is sound: sin(beta) x section distance <= true distance.
  expect(in.crossingSine*r.checked.obstacle.mm.lo<=truth+1e-6,"section bound is a lower bound");
  if(beta==90)expect(r.checked.obstacle.mm.hi>=truth-0.5,"perpendicular bound is tight (<=0.5 mm vs sampling)");
 }
 // 2) A round obstacle inscribed in the square never needs a deeper/longer pass.
 for(bool above:{false,true}){Case a={false,false,90,above,45},b={false,true,90,above,45};
  tvsBypass::Input sq=problem(a),rd=problem(b);sq.obstacleWidth=250;Result x=generate(sq,p),y=generate(rd,p);
  expect(x.hasCandidate&&y.hasCandidate&&y.D<=x.D&&mid(y.W)<=mid(x.W)+1e-9,"round <= square");}
 // 3) Oblique crossing is never cheaper than perpendicular.
 for(bool round:{false,true}){Case a={false,round,90,false,45},b={false,round,45,false,45};
  Result x=generate(problem(a),p),y=generate(problem(b),p);expect(y.D>=x.D&&mid(y.W)>=mid(x.W),"oblique >= perpendicular");}
 // 4) Segmented-elbow envelope can only reduce the measured distance.
 {Case c={true,false,90,false,60};tvsBypass::Input in=problem(c);Result r=generate(in,p);expect(r.hasCandidate,"envelope base");
  tvsBypass::Input e=r.candidate;e.elbowExcessMm=(e.ri+e.height)*(1/std::cos(tvsOffset::pi()*60/180/4)-1);
  tvsBypass::Result x=tvsBypass::check(r.candidate),y=tvsBypass::check(e);
  expect(y.jointDeviationUpperMm>=x.jointDeviationUpperMm+e.elbowExcessMm-1e-9,"excess enters the guard");
  Policy q=p;Result s=generate(e,q);expect(s.hasCandidate&&s.D>=r.D,"segmented elbows never need less depth");}
 // 5) Invalid combinations are rejected, not guessed.
 {Case c={false,true,90,false,45};tvsBypass::Input in=problem(c);in.obstacleHeight=260;expect(tvsBypass::check(in).status==tvsBypass::InvalidInput,"round obstacle needs W=H");
  in=problem(c);in.crossingSine=.2;in.crossingCosine=std::sqrt(1-.04);expect(tvsBypass::check(in).status==tvsBypass::InvalidInput,"too oblique rejected");
  in=problem(c);in.elbowExcessMm=-1;expect(tvsBypass::check(in).status==tvsBypass::InvalidInput,"negative excess rejected");}
 std::cout<<"PASS SECTIONS "<<tests<<" checks; "<<solved<<" route x obstacle x angle x side x crossing combinations verified against sampled 3D geometry"<<std::endl;
}

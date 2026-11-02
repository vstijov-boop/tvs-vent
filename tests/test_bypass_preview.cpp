#include "../geometry/bypass/SideDetail.h"
#include <iostream>
#include <cstdlib>
#include <limits>
#include <cfenv>
using namespace tvsBypassPreview;
static int checks=0;
static void expect(bool b,const char* m){++checks;if(!b){std::cerr<<"FAIL "<<m<<std::endl;std::exit(1);}}
static void near(double a,double b,const char* m){expect(std::fabs(a-b)<1e-7,m);}
static Snapshot example(double unit,double angle){
 Snapshot s;double c=std::cos(angle),n=std::sin(angle);
 auto p=[=](double x,double y){return Point((123+x*c-y*n)/unit,(-87+x*n+y*c)/unit,37/unit);};
 s.route.first=p(-1500,0);s.route.last=p(1500,0);s.route.a=400/unit;s.route.b=200/unit;s.route.elevation=-400/unit;
 s.obstacle.first=p(0,-1200);s.obstacle.last=p(0,1200);s.obstacle.a=250/unit;s.obstacle.b=250/unit;s.obstacle.elevation=-400/unit;
 s.route.rectangular=s.route.rigid=s.route.ordinary=true;s.obstacle.rectangular=s.obstacle.rigid=s.obstacle.ordinary=true;return s;
}
int main(){
 for(double unit:{1.,10.,1000.})for(double angle:{0.,37.,90.,217.})for(bool reverse:{false,true}){
  Snapshot s=example(unit,angle*3.14159265358979323846/180);Frame f=adapt(s,unit,reverse);
  expect(f.valid,"valid perpendicular snapshot");near(f.origin.x,123/unit,"intersection x");near(f.origin.y,-87/unit,"intersection y");near(f.origin.z,37/unit,"drawing Z independent");
  near(f.width,400,"A no second A/2 offset");near(f.height,200,"B independent");near(f.z0,-400,"physical axis no B/2 offset");near(f.obstacleZ,-400,"obstacle physical axis");
  near(f.left,-1500,"left window from axis crossing");near(f.right,1500,"right window");near(f.tmin,-1200,"full transverse min");near(f.tmax,1200,"full transverse max");
  Point w=f.world(0,0);near(w.x,123/unit,"world s zero x");near(w.y,-87/unit,"world s zero y");near(w.z,37/unit,"world drawing plane");
  Point end=f.world(100,50);double heading=angle*3.14159265358979323846/180+(reverse?3.14159265358979323846:0);
  near(end.x,(123+100*std::cos(heading)-50*std::sin(heading))/unit,"nonzero transverse projection x");
  near(end.y,(-87+100*std::sin(heading)+50*std::cos(heading))/unit,"nonzero transverse projection y");
 }
 Snapshot s=example(1,0);s.route.first.x=-700;s.route.last.x=900;Frame f=adapt(s,1,false);near(f.left,-823,"asymmetric left");near(f.right,777,"asymmetric right");
 s=example(1,0);s.obstacle.last.x+=10;expect(adapt(s,1,false).valid&&adapt(s,1,false).crossingSine()<1,"slight skew is an oblique crossing");
 // Oblique crossing: origin at the plan axis intersection, sin/cos of the plan angle.
 for(double beta:{60.,45.,30.})for(bool reverse:{false,true}){const double b=beta*3.14159265358979323846/180;
  Snapshot q=example(1,0);q.obstacle.first=Point(123+300-1200*std::cos(b),-87-1200*std::sin(b),37);q.obstacle.last=Point(123+300+1200*std::cos(b),-87+1200*std::sin(b),37);
  Frame g=adapt(q,1,reverse);expect(g.valid,"oblique valid");near(g.origin.x,123+300,"oblique intersection x");near(g.origin.y,-87,"oblique intersection y");
  near(g.crossingSine(),std::sin(b),"crossing sine");near(g.crossingCosine(),std::fabs(std::cos(b)),"crossing cosine");}
 s=example(1,0);s.obstacle.first=Point(123-1200,-87-100,37);s.obstacle.last=Point(123+1200,-87+100,37);expect(!adapt(s,1,false).valid,"below 14.5 deg rejected");
 // Round obstacle: TVS round duct has B = 0; diameter in both section sizes.
 s=example(1,0);s.obstacle.rectangular=false;s.obstacle.b=0;{Frame g=adapt(s,1,false);expect(g.valid&&g.obstacleRound&&g.obstacleWidth==250&&g.obstacleHeight==250,"round obstacle frame");}
 s=example(1,0);s.obstacle.rigid=false;expect(!adapt(s,1,false).valid,"flexible obstacle rejected");
 s=example(1,0);s.obstacle.last.z+=1;expect(!adapt(s,1,false).valid,"vertical obstacle rejected");
 s=example(1,0);s.route.rectangular=false;expect(!adapt(s,1,false).valid,"round route must have B = 0");
 s=example(1,0);s.route.rectangular=false;s.route.b=0;s.route.a=315;{Frame g=adapt(s,1,false);expect(g.valid&&g.routeRound&&g.width==315&&g.height==315,"round route frame: diameter in both");}
 s=example(1,0);s.route.ordinary=false;expect(!adapt(s,1,false).valid,"marked slope rejected");
 s=example(1,0);expect(!adapt(s,0,false).valid,"unknown units not zero");s.route.a=std::numeric_limits<double>::quiet_NaN();expect(!adapt(s,1,false).valid,"invalid source");
 Revision rev;expect(!rev.current(),"no initial result");rev.accept();expect(rev.current(),"current result");rev.invalidate();expect(!rev.current(),"input change invalidates");
 Engine engine;Options o;o.confirmed=true;o.left=-1100;o.right=1100;
 s=example(1,0);View v=engine.calculate(s,1,false,o,false);expect(v.ready&&engine.ready(),"real strict numeric bridge executes");expect(v.profile.size()>7&&v.plan.size()>7,"same result rendered in both projections");
 near(v.D,290,"bridge D oracle");near(v.W,1271.370849898476,"bridge W oracle");
 View shorter=engine.calculate(s,1,false,o,true);expect(shorter.ready&&shorter.W<=v.W,"improvement retains best");
 o.calls=1;View limited=engine.calculate(s,1,false,o,true);expect(limited.ready,"changed budget is a new request");
 // Same signature, deterministic first proposal is longer than the prior improved result: retain best.
 o.calls=64;View improved=engine.calculate(s,1,false,o,true);View repeated=engine.calculate(s,1,false,o,true);expect(repeated.ready&&repeated.W<=improved.W,"repeated improvement preserves best for unchanged inputs");
 engine.invalidate();expect(!engine.ready(),"explicit UI invalidation clears engine");
 o.useFloor=true;o.floor=-650;v=engine.calculate(s,1,false,o,true);expect(!v.ready&&!engine.ready(),"changed inputs cannot retain old feasible result");
 o.useFloor=false;o.confirmed=false;v=engine.calculate(s,1,false,o,false);expect(!v.ready,"unconfirmed manual inputs rejected");
 o.confirmed=true;s.obstacle.first.y=-50;s.obstacle.last.y=50;v=engine.calculate(s,1,false,o,false);expect(!v.ready,"partial transverse overlap rejected by checker");
 {
  std::fenv_t saved;std::fegetenv(&saved);std::feclearexcept(FE_ALL_EXCEPT);std::feraiseexcept(FE_DIVBYZERO|FE_INEXACT);
  int flags=std::fetestexcept(FE_ALL_EXCEPT);s=example(1,0);o=Options();o.confirmed=true;o.left=-1100;o.right=1100;
  v=engine.calculate(s,1,false,o,false);expect(v.ready&&std::fetestexcept(FE_ALL_EXCEPT)==flags,"bridge restores host floating flags");
  Frame f2=engine.frame(s,1,false);expect(f2.valid&&std::fetestexcept(FE_ALL_EXCEPT)==flags,"frame adapter restores flags");
  std::fesetround(FE_UPWARD);flags=std::fetestexcept(FE_ALL_EXCEPT);v=engine.calculate(s,1,false,o,false);
  expect(!v.ready&&std::fegetround()==FE_UPWARD&&std::fetestexcept(FE_ALL_EXCEPT)==flags,"bridge rejects without normalizing rounding");
  std::fesetenv(&saved);
 }
 // Independent regression from the user's actual screenshot, not formulas repeated from solve().
 s=example(1,0);s.route.b=250;s.route.elevation=s.obstacle.elevation=0;o=Options();o.confirmed=true;o.left=-2812.95376;o.right=1987.82064;
 v=engine.calculate(s,1,false,o,false);expect(v.ready,"screenshot case feasible");
 near(v.D,315,"screenshot D");near(v.C,360,"screenshot C");near(v.W,1362.792206135785,"screenshot W");near(v.zPass,-315,"screenshot elevation");
 near(v.slopePhysical[0],259.081169079632,"screenshot physical slope");near(v.slopeProjected[0],183.198051533946,"screenshot projected slope");
 expect(v.axis.size()==8,"checked section axes available for dimensions");near(v.axis.front().z,0,"initial dimension");near(v.axis.back().z,0,"final dimension");
 auto notes=captions(v,o);expect(notes.size()==12,"annotation rail has twelve non-overlay cells");
 expect(notes[3].text.find("125.0 / -125.0")!=std::string::npos,"obstacle top/bottom labels");
 expect(notes[5].text.find("-190.0 / -440.0")!=std::string::npos,"pass top/bottom labels");
 expect(summary(v,o).find("315.0")!=std::string::npos,"human summary one decimal");
 expect(summary(v,o).find("[64.99, 65.01]")!=std::string::npos,"display interval rounded OUTWARD");
 expect(memberName(3)=="srednji komad","member names not raw codes");
 auto contours=planLines(v,false,false),all=planLines(v,true,true);expect(all.size()>contours.size(),"separate contour/all-edges modes");
 for(const auto& l:contours)expect(l.kind==PhysicalOutline,"contours hide symbolic and diagnostic fitting boundaries");
 View empty;expect(captions(empty,o).empty(),"invalid result has no stale dimensions");
 View coincident;Curve diagnostic,physical;diagnostic.kind=DiagnosticLine;diagnostic.points={Vertex(0,0),Vertex(10,0)};
 physical.kind=PhysicalOutline;physical.points={Vertex(10,0),Vertex(0,0)};coincident.plan={diagnostic,physical};
 auto unique=planLines(coincident,true,true);expect(unique.size()==1&&unique[0].kind==PhysicalOutline,"physical contour precedes coincident diagnostic");
 // Display-only axes and geometry labels are checked for both sides, headings, angles and insulation.
 for(bool above:{false,true})for(double heading:{0.,.6457718232379019})for(int angle:{30,45,60})for(double iso:{0.,25.}){
  s=example(1,heading);o=Options();o.confirmed=true;o.above=above;o.angle=angle;o.insulation=o.obstacleInsulation=iso;o.left=-1600;o.right=1600;
  v=engine.calculate(s,1,false,o,false);expect(v.ready,"presentation matrix confirmed candidate");
  near(v.axis.front().z,-400,"initial axis from checked Result");near(v.axis.back().z,-400,"final axis from checked Result");
  expect((v.zPass>-400)==above,"above/below section orientation");
  expect(v.witnessBody.s==v.witnessBody.s&&v.witnessObstacle.z==v.witnessObstacle.z,"critical points finite");
  expect(captions(v,o).size()==12&&!planLines(v,true,true).empty(),"both presentations available");
 }

 // Side-detail units and native circular bulges are checked against the checked numerical bodies.
 expect(nextDetailLabel({"ob-01","OB-03"})=="OB-02","next free case-insensitive label");
 expect(!detailLabelValid("x y")&&!detailLabelValid("a/b")&&!detailLabelValid(std::string(33,'X')),"unsafe labels rejected");
 for(double unit:{1.,10.,1000.})for(bool above:{false,true})for(int angle:{30,45,60})for(double iso:{0.,25.})for(double datum:{-400.,12000.}){
  s=example(unit,.6457718232379019);s.route.elevation=s.obstacle.elevation=datum/unit;
  o=Options();o.confirmed=true;o.above=above;o.angle=angle;o.insulation=o.obstacleInsulation=iso;o.left=-1600;o.right=1600;
  v=engine.calculate(s,unit,false,o,false);expect(v.ready,"detail matrix candidate");
  for(int pair=0;pair<2;++pair){const auto& dto=v.offsets[pair];expect(dto.ready(),"checked native offset DTO exported");
   const auto p=v.frame.world(v.axis[pair?4:0].s,0);near(dto.incomingLast.x,p.x,"accepted candidate start X");near(dto.incomingLast.y,p.y,"accepted candidate start Y");
   near(dto.middle.physicalLengthMm,v.slopePhysical[pair],"native physical length equals checked candidate");
   near(dto.middle.lastElevationMm-dto.middle.firstElevationMm,pair?(above?-1:1)*std::fabs(v.offsets[0].middle.lastElevationMm-v.offsets[0].middle.firstElevationMm):(above?1:-1)*std::fabs(dto.middle.lastElevationMm-dto.middle.firstElevationMm),"independent signed local rise");}
  for(bool detailed:{false,true}){
   SideDetail d=sideDetail(v,o,"OB-01",detailed,3.5,4.);
   near(d.factor,unit,"DWG factor retained");near(d.minS,0,"detail bounds start at local zero, not absolute s0");near(d.maxS,v.W,"detail bounds end at W");expect(d.dimensions.size()==(detailed?5u:3u),"native dimension plan count");
   near(std::hypot(d.dimensions[0].a.s-d.dimensions[0].b.s,d.dimensions[0].a.z-d.dimensions[0].b.z)/unit*unit,v.W,"W dimension geometry times DIMLFAC");
   near(std::hypot(d.dimensions[1].a.s-d.dimensions[1].b.s,d.dimensions[1].a.z-d.dimensions[1].b.z)/unit*unit,v.C,"H dimension real geometry");
   near(std::fabs(d.dimensions[2].a.z-d.dimensions[2].b.z),v.D,"D dimension real geometry");
   expect(d.notes[3].text.find(displayNumber(datum))!=std::string::npos,"absolute datum in annotation");
   if(detailed)for(int j=0;j<2;++j){const auto& q=d.dimensions[3+j];near(std::hypot(q.a.s-q.b.s,q.a.z-q.b.z),v.slopePhysical[j],"S dimension physical not projected");}
   unsigned arcs=0;for(const auto& p:d.paths){expect(p.points.size()==p.bulges.size(),"outgoing bulge topology");
    for(size_t j=0;j<p.points.size();++j)if(p.bulges[j]!=0){++arcs;double sweep=4*std::atan(p.bulges[j]);near(std::fabs(sweep),angle*3.14159265358979323846/180,"true circular sweep");
     Vertex a=p.points[j],b=p.points[(j+1)%p.points.size()];double chord=std::hypot(a.s-b.s,a.z-b.z);double radius=chord/(2*std::sin(std::fabs(sweep)/2));
     double expected=p.axis?o.ri+v.frame.height/2:(j==0?o.ri-(p.insulation?iso:0):o.ri+v.frame.height+(p.insulation?iso:0));
     near(radius,expected,"circle radius derives from checked body");}
   }
   expect(arcs==20,"four elbows: bare and insulation boundaries plus exact axes");
   for(size_t j=0;j<d.dimensions.size();++j){const auto& q=d.dimensions[j];expect(q.rail.z<d.minZ||q.rail.z>d.maxZ||q.rail.s<d.minS||q.rail.s>d.maxS,"dimension text external to geometry");}
  }
 }
 bool invalid=false;try{sideDetail(View(),Options(),"OB-01",false,3.5,3.5);}catch(const std::runtime_error&){invalid=true;}expect(invalid,"stale result has no insert model");
 invalid=false;try{sideDetail(v,o,"OB-01",false,0,3.5);}catch(const std::runtime_error&){invalid=true;}expect(invalid,"invalid annotation height rejected");
 for(bool expert:{false,true})for(double sx:{.65,.8,1.,1.25,1.5,2.})for(double sy:{.65,.8,1.,1.25,1.5,2.}){
  auto layout=sectionLayout(sx,sy,true,expert);expect(layout.labels.size()==16,"dimension rails have fixed non-overlap slots");
  for(size_t a=0;a<layout.labels.size();++a){auto r=layout.labels[a];
   expect(!overlap(r,layout.plot),"dimension text slot outside physical geometry");
   expect(r.left>=layout.area.left&&r.right<=layout.area.right&&r.top>=layout.area.top&&r.bottom<=layout.area.bottom,"dimension text slot inside section panel");
   for(size_t b=a+1;b<layout.labels.size();++b)expect(!overlap(r,layout.labels[b]),"dimension slots do not overlap each other");
  }
  expect(sectionLayout(sx,sy,false,expert).labels.empty(),"Kote off has no stale annotation slots");
 }
 // As-built rebuild (TVS_BYPASS_DETALJ): recorded D, C, s0 and record policy, minimums
 // not stored (0) -> the same checked geometry, axes and native ports, without a search.
 for(double unit:{1.,10.,1000.})for(bool above:{false,true})for(int angle:{30,45,60}){
  Snapshot b=example(unit,.3);Options q;q.confirmed=true;q.above=above;q.angle=angle;Frame bf=adapt(b,unit,false);q.left=bf.left;q.right=bf.right;
  Engine search;View found=search.calculate(b,unit,false,q,false);expect(found.ready,"search candidate for rebuild");
  Options built;built.ri=q.ri;built.insulation=q.insulation;built.obstacleInsulation=q.obstacleInsulation;built.clearance=q.clearance;built.reserve=q.reserve;
  built.angle=angle;built.above=above;built.confirmed=true;built.minSlope=built.minBridge=built.minProjection=0;built.left=bf.left;built.right=bf.right;
  // The command derives s0 from the inlet cut point (= first incoming port) and D/C from H.
  const auto& cut=found.offsets[0].incomingLast;near(((cut.x-bf.origin.x)*bf.ux+(cut.y-bf.origin.y)*bf.uy)*unit,found.s0,"s0 is the cut point");
  const auto& h0=found.offsets[0].outgoingFirst;const auto& h1=found.offsets[1].incomingLast;
  near(std::hypot(h1.x-h0.x,h1.y-h0.y)*unit,found.C,"C is the H length");near(std::fabs(found.zPass-bf.z0),found.D,"D is |zPass-z0|");
  Engine as;View again=as.rebuild(b,unit,false,built,found.D,found.C,found.s0);
  expect(again.ready&&as.ready(),"rebuild reproduces a checked candidate");
  near(again.W,found.W,"rebuild W");near(again.zPass,found.zPass,"rebuild pass elevation");expect(again.axis.size()==found.axis.size(),"rebuild axes");
  for(size_t k=0;k<found.axis.size();++k){near(again.axis[k].s,found.axis[k].s,"rebuild axis s");near(again.axis[k].z,found.axis[k].z,"rebuild axis z");}
  for(int j=0;j<2;++j){near(again.offsets[j].incomingLast.x,found.offsets[j].incomingLast.x,"rebuild port x");near(again.offsets[j].outgoingFirst.y,found.offsets[j].outgoingFirst.y,"rebuild port y");}
  expect(!again.detailPaths.empty()&&again.detailPaths.size()==found.detailPaths.size(),"rebuild detail paths");
  View off=as.rebuild(b,unit,false,built,found.D,found.C,bf.right+1);expect(!off.ready&&!as.ready(),"rebuild outside the duct is refused");
 }
 // Bridge end to end with a round and an oblique obstacle; detail paths and section curves.
 for(int kind=0;kind<6;++kind){Snapshot q=example(1,0);
  if(kind>=3){q.route.rectangular=false;q.route.b=0;q.route.a=315;} // round route x {rect, round, round oblique}
  if(kind%3==1){q.obstacle.rectangular=false;q.obstacle.b=0;}
  if(kind%3==2){const double b=60*3.14159265358979323846/180;q.obstacle.first=Point(123-1200*std::cos(b),-87-1200*std::sin(b),37);q.obstacle.last=Point(123+1200*std::cos(b),-87+1200*std::sin(b),37);q.obstacle.rectangular=false;q.obstacle.b=0;}
  Options k;k.confirmed=true;Frame g=adapt(q,1,false);k.left=g.left;k.right=g.right;Engine e;View r=e.calculate(q,1,false,k,false);
  expect(r.ready,"round/oblique obstacle candidate");expect(r.detailPaths.size()==21,"detail paths for the detail writer");
  Options b2=k;b2.minSlope=b2.minBridge=b2.minProjection=0;Engine e2;View again=e2.rebuild(q,1,false,b2,r.D,r.C,r.s0);
  expect(again.ready,"as-built rebuild for round/oblique obstacle");near(again.W,r.W,"rebuild W round/oblique");
  SideDetail sd=sideDetail(r,k,"OB-01",true,3.5,3.5);bool circle=false;for(const auto& path:sd.paths)if(path.member==7&&path.points.size()==2&&path.bulges[0]==1)circle=true;
  expect(kind%3==1?circle:!circle,"exact circle only for perpendicular round obstacle");}
 std::cout<<"PASS PREVIEW "<<checks<<" adapter / presentation / layout / revision / real numeric-bridge checks; no SDK runtime"<<std::endl;
}


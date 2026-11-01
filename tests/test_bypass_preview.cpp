#include "../geometry/bypass/PreviewPresentation.h"
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
 s=example(1,0);s.obstacle.last.x+=10;expect(!adapt(s,1,false).valid,"skew unsupported");
 s=example(1,0);s.obstacle.last.z+=1;expect(!adapt(s,1,false).valid,"vertical obstacle rejected");
 s=example(1,0);s.route.rectangular=false;expect(!adapt(s,1,false).valid,"round unsupported");
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
 for(bool expert:{false,true})for(double sx:{.65,.8,1.,1.25,1.5,2.})for(double sy:{.65,.8,1.,1.25,1.5,2.}){
  auto layout=sectionLayout(sx,sy,true,expert);expect(layout.labels.size()==16,"dimension rails have fixed non-overlap slots");
  for(size_t a=0;a<layout.labels.size();++a){auto r=layout.labels[a];
   expect(!overlap(r,layout.plot),"dimension text slot outside physical geometry");
   expect(r.left>=layout.area.left&&r.right<=layout.area.right&&r.top>=layout.area.top&&r.bottom<=layout.area.bottom,"dimension text slot inside section panel");
   for(size_t b=a+1;b<layout.labels.size();++b)expect(!overlap(r,layout.labels[b]),"dimension slots do not overlap each other");
  }
  expect(sectionLayout(sx,sy,false,expert).labels.empty(),"Kote off has no stale annotation slots");
 }
 std::cout<<"PASS PREVIEW "<<checks<<" adapter / presentation / layout / revision / real numeric-bridge checks; no SDK runtime"<<std::endl;
}

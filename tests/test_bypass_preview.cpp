#include "../geometry/bypass/PreviewModel.h"
#include <iostream>
#include <cstdlib>
#include <limits>
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
 o.calls=64;View improved=engine.calculate(s,1,false,o,true);View repeated=engine.calculate(s,1,false,o,false);(void)repeated;
 engine.invalidate();expect(!engine.ready(),"explicit UI invalidation clears engine");
 o.useFloor=true;o.floor=-650;v=engine.calculate(s,1,false,o,true);expect(!v.ready&&!engine.ready(),"changed inputs cannot retain old feasible result");
 o.useFloor=false;o.confirmed=false;v=engine.calculate(s,1,false,o,false);expect(!v.ready,"unconfirmed manual inputs rejected");
 o.confirmed=true;s.obstacle.first.y=-50;s.obstacle.last.y=50;v=engine.calculate(s,1,false,o,false);expect(!v.ready,"partial transverse overlap rejected by checker");
 std::cout<<"PASS PREVIEW "<<checks<<" adapter / revision / real numeric-bridge checks; no SDK runtime"<<std::endl;
}

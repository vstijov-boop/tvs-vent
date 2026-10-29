// Prepared for the next approved build. Do not compile while builds are on hold.
#include <cassert>
#include <cmath>
#define near
#include "../TVS_Ventilation_ARX/MoveGeometry.h"
using namespace tvsMoveGeometry;
static const double pi=3.14159265358979323846;
static Point rotate(const Point& p,double a,double scale){return Point((p.x*std::cos(a)-p.y*std::sin(a)+10000)/scale,(p.x*std::sin(a)+p.y*std::cos(a)-5000)/scale,p.z/scale);}
static void close(const Point& a,const Point& b,double tol){assert(pointsNear(a,b,tol));}
int main(){
 const double scales[]={1,10,1000},angles[]={0,pi/6,pi/2,-pi/4};
 for(int u=0;u<3;++u)for(int r=0;r<4;++r){
  const double scale=scales[u],angle=angles[r],tol=1e-7/scale;
  Point selected[2]={rotate(Point(0,1000),angle,scale),rotate(Point(2000,1000),angle,scale)};
  Point attached[2]={rotate(Point(-300,700),angle,scale),rotate(Point(2300,700),angle,scale)};
  Point fixed[2]={rotate(Point(-300,0),angle,scale),rotate(Point(2300,0),angle,scale)};
  Plan p;assert(plan(selected,attached,fixed,rotate(Point(700,1000),angle,scale),rotate(Point(1400,1400),angle,scale),1/scale,.1/scale,p));
  close(p.selected[0],rotate(Point(0,1400),angle,scale),tol);close(p.selected[1],rotate(Point(2000,1400),angle,scale),tol);
  close(p.attached[0],rotate(Point(-300,1100),angle,scale),tol);
  assert(!plan(selected,attached,fixed,selected[0],rotate(Point(0,100),angle,scale),1/scale,.1/scale,p));
  assert(!plan(selected,attached,fixed,selected[0],selected[1],1/scale,.1/scale,p));
  Point reversed[2]={selected[1],selected[0]},ra[2]={attached[1],attached[0]},rf[2]={fixed[1],fixed[0]};
  assert(plan(reversed,ra,rf,rotate(Point(700,1000),angle,scale),rotate(Point(700,1400),angle,scale),1/scale,.1/scale,p));
  close(p.selected[0],rotate(Point(2000,1400),angle,scale),tol);
 }
 // Opposite neighbors: one extends, the other shortens by the same amount.
 {
  Point ends[2]={Point(0,0),Point(2000,0)},attached[2]={Point(-300,-300),Point(2300,300)};
  Point fixed[2]={Point(-300,-1300),Point(2300,1300)};Plan z;
  assert(plan(ends,attached,fixed,Point(500,0),Point(500,400),1,.1,z));
  assert(std::fabs((z.attached[0]-fixed[0]).length()-1400)<1e-8);
  assert(std::fabs((z.attached[1]-fixed[1]).length()-600)<1e-8);
  assert(!plan(ends,attached,fixed,Point(500,0),Point(500,1000),1,.1,z));
  assert(!plan(ends,attached,fixed,Point(500,0),Point(500,1),1,0,z));
  assert(plan(ends,attached,fixed,Point(500,0),Point(500,400),1,.1,z));
  Plan back;assert(plan(z.selected,z.attached,fixed,Point(500,400),Point(500,0),1,.1,back));
  close(back.selected[0],ends[0],1e-8);close(back.selected[1],ends[1],1e-8);
  close(back.attached[0],attached[0],1e-8);close(back.attached[1],attached[1],1e-8);
 }
 // Oblique neighbors translate by different vectors while the selected axis stays parallel.
 Point selected[2]={Point(0,0),Point(2000,0)},attached[2]={Point(-200,-200),Point(2200,-200)},fixed[2]={Point(-1000,-1000),Point(3000,-1000)};
 Plan p;assert(plan(selected,attached,fixed,Point(0,0),Point(800,100),1,.1,p));
 close(p.selected[0],Point(100,100),1e-8);close(p.selected[1],Point(1900,100),1e-8);
 assert(!plan(selected,attached,fixed,Point(0,0),Point(0,1000),1,.1,p)); // central reversal
 fixed[0].z=20;assert(!plan(selected,attached,fixed,Point(0,0),Point(0,100),1,.1,p));fixed[0].z=0;
 assert(!plan(selected,attached,fixed,Point(0,0),Point(HUGE_VAL,0),1,.1,p));
 // Analytic ports remain tangent under arbitrary orientation and bend angles.
 const double sweeps[]={15*pi/180,pi/4,pi/2,135*pi/180,165*pi/180};
 for(int k=0;k<5;++k)for(int r=0;r<4;++r){
  double a=angles[r],s=sweeps[k],radius=300;Point middle(1100,-200,40),ports[2];
  assert(elbowPorts(middle,Point(std::cos(a),std::sin(a)),s,radius,ports));
  double tangent=radius*std::tan(s/2);
  assert(std::fabs((ports[0]-middle).length()-tangent)<1e-8);
  assert(std::fabs((ports[1]-middle).length()-tangent)<1e-8);
  assert(std::fabs((ports[0]-middle).dot(Point(std::cos(a),std::sin(a))))<1e-8);
 }
 Point badPorts[2];
 assert(!elbowPorts(Point(),Point(1,0),0,300,badPorts));
 assert(!elbowPorts(Point(),Point(1,0),pi,300,badPorts));
 assert(!elbowPorts(Point(),Point(),pi/2,300,badPorts));
 assert(!elbowPorts(Point(),Point(1,0),pi/2,-1,badPorts));
 assert(onSegment(Point(5,0),Point(0,0),Point(10,0),.1));
 assert(!onSegment(Point(15,0),Point(0,0),Point(10,0),.1));
}

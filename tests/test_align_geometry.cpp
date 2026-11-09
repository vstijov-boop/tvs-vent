// TVS_ALIGN plan geometry: g++ -std=c++14 tests/test_align_geometry.cpp && ./a.out
#include <cassert>
#include <cmath>
#include "../TVS_Ventilation_ARX/AlignGeometry.h"
using namespace tvsAlign;
static const double pi=3.14159265358979323846;
static bool close(const Point& a,const Point& b,double tol){return tvsMoveGeometry::pointsNear(a,b,tol);}
// Place a fixture (mm) at a rotation and drawing scale (mm per unit: 1 mm, 10 cm, 1000 m).
static Point place(const Point& p,double a,double scale){
    return Point((p.x*std::cos(a)-p.y*std::sin(a)+12000)/scale,(p.x*std::sin(a)+p.y*std::cos(a)-7000)/scale,p.z/scale);}
static Point dir(const Point& v,double a){return Point(v.x*std::cos(a)-v.y*std::sin(a),v.x*std::sin(a)+v.y*std::cos(a),0);}
int main(){
    const double scales[]={1,10,1000},frames[]={0,pi/6,pi/2,-pi/4,pi};
    for(int u=0;u<3;++u)for(int r=0;r<5;++r){
        const double sc=scales[u],fr=frames[r],tol=1e-7/sc;
        // Reference: horizontal line y = 0 (in the fixture frame).
        Line ref;assert(line(place(Point(0,0),fr,sc),place(Point(1000,0),fr,sc),ref));
        // 1. Parallel pipe edge 300 above, offset 100 on the same side: only moves 200 down, no rotation.
        Plan p;Point anchor=place(Point(500,300,2500),fr,sc);
        assert(plan(anchor,dir(Point(1,0),fr),ref,100/sc,1,1e-9,p));
        assert(!p.rotate);
        assert(close(apply(p,anchor),place(Point(500,100,2500),fr,sc),tol));   // no sliding, z kept
        // Other side (side -1): anchor ends 100 below.
        assert(plan(anchor,dir(Point(1,0),fr),ref,100/sc,-1,1e-9,p));
        assert(close(apply(p,anchor),place(Point(500,-100,2500),fr,sc),tol));
        // Reversed element direction (pointing -x) is still parallel: no 180 deg turn.
        assert(plan(anchor,dir(Point(-1,0),fr),ref,0,1,1e-9,p));
        assert(!p.rotate&&close(apply(p,anchor),place(Point(500,0,2500),fr,sc),tol));
        // 2. Oblique element at 30 deg: rotates -30 deg about the anchor, then lands on the line.
        Point a2=place(Point(200,400,-300),fr,sc),far2=place(Point(200+866.0254037844386,400+500,-300),fr,sc);
        assert(plan(a2,far2-a2,ref,0,1,1e-9,p));
        assert(p.rotate&&std::fabs(p.angle+pi/6)<1e-9);
        Point m0=apply(p,a2),m1=apply(p,far2);
        assert(close(m0,place(Point(200,0,-300),fr,sc),tol));
        assert(std::fabs(distance(ref,m1))<tol);                           // whole edge on the line
        assert(std::fabs(m1.z-far2.z)<tol);                                  // elevation kept
        // 3. Nearly opposite (170 deg) turns by -10 deg, never +170 (no flip).
        Point d3=dir(Point(std::cos(pi*170/180),std::sin(pi*170/180)),fr);
        assert(plan(a2,d3,ref,0,1,1e-9,p));
        assert(std::fabs(std::fabs(p.angle)-pi/18)<1e-9);
        Point t3=turn(p,d3);assert(std::fabs(t3.x*ref.d.y-t3.y*ref.d.x)<1e-9);
        // 4. Vertical element (90 deg) turns by exactly 90 one way, result parallel.
        assert(plan(a2,dir(Point(0,1),fr),ref,50/sc,1,1e-9,p));
        assert(std::fabs(std::fabs(p.angle)-pi/2)<1e-9);
        Point t4=turn(p,dir(Point(0,1),fr));assert(std::fabs(t4.x*ref.d.y-t4.y*ref.d.x)<1e-9);
        assert(std::fabs(distance(ref,apply(p,a2))-50/sc)<tol);
        // 5. Different elevation, same XY: identical plan result.
        Plan hi,lo;Point up=place(Point(500,300,9000),fr,sc),down=place(Point(500,300,-9000),fr,sc);
        assert(plan(up,dir(Point(1,0),fr),ref,0,1,1e-9,hi)&&plan(down,dir(Point(1,0),fr),ref,0,1,1e-9,lo));
        assert(close(hi.delta,lo.delta,tol));
        // 6. Port references: rectangular A != B uses the plan width; axis / picked / other edge.
        Point port=place(Point(1000,600),fr,sc),cursor=place(Point(900,750),fr,sc),e;
        assert(portReference(port,dir(Point(1,0),fr),400/sc,cursor,Axis,e)&&close(e,port,tol));
        assert(portReference(port,dir(Point(1,0),fr),400/sc,cursor,PickedEdge,e)&&close(e,place(Point(1000,800),fr,sc),tol));
        assert(portReference(port,dir(Point(1,0),fr),400/sc,cursor,OtherEdge,e)&&close(e,place(Point(1000,400),fr,sc),tol));
        // Round (width = diameter) behaves the same.
        assert(portReference(port,dir(Point(1,0),fr),250/sc,cursor,PickedEdge,e)&&close(e,place(Point(1000,725),fr,sc),tol));
        // 7. Side from cursor; a point on the line keeps the fallback.
        assert(side(ref,place(Point(0,10),fr,sc),tol,-1)==1&&side(ref,place(Point(0,-10),fr,sc),tol,1)==-1);
        assert(side(ref,place(Point(300,0),fr,sc),tol,-1)==-1);
        // 8. Refusals: degenerate direction / line, negative offset.
        assert(!plan(a2,Point(0,0,1),ref,0,1,1e-9,p));
        Line bad;assert(!line(place(Point(5,5,0),fr,sc),place(Point(5,5,100),fr,sc),bad));
        assert(!plan(a2,dir(Point(1,0),fr),ref,-1,1,1e-9,p));
        assert(!portReference(port,dir(Point(1,0),fr),0,cursor,PickedEdge,e));
    }
    return 0;
}

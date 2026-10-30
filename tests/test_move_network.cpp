// SDK-independent regression tests for connected fitting movement.
#include <cassert>
#include <cmath>
#define near
#include "../TVS_Ventilation_ARX/MoveNetworkGeometry.h"
using namespace tvsMoveNetwork;
static Body body(Kind kind,Point a,Point b,double width){
    Body n;n.kind=kind;n.ports.push_back(a);n.ports.push_back(b);
    n.axes.push_back(b-a);n.axes.push_back(b-a);
    n.sections.push_back(Section(width,width,false,0));n.sections.push_back(n.sections[0]);return n;
}
static Body elbow(Point a,Point b,double width){Body n=body(Rigid,a,b,width);n.axes[0]=Point(1,0);n.axes[1]=Point(0,1);return n;}
static std::vector<Body> fixture(){
    std::vector<Body> n;
    n.push_back(body(Pipe,Point(0,0),Point(1000,0),250));
    n.push_back(elbow(Point(0,0),Point(-100,-100),250));
    Body wye=body(Rigid,Point(1000,0),Point(1400,0),250);
    wye.ports.push_back(Point(1200,200));wye.axes.push_back(Point(0,1));wye.sections.push_back(Section(200,200));wye.junctions.push_back(Point(1200,0));n.push_back(wye);
    n.push_back(body(Pipe,Point(-100,-100),Point(-100,-1000),250));
    n.push_back(body(Pipe,Point(1400,0),Point(2000,0),250));
    Body transition=body(Rigid,Point(2000,0),Point(2300,0),250);transition.sections[1]=Section(400,400);n.push_back(transition);
    n.push_back(body(Pipe,Point(2300,0),Point(3000,0),400));
    n.push_back(elbow(Point(3000,0),Point(3100,-100),400));
    n.push_back(body(Pipe,Point(3100,-100),Point(3100,-1000),400));
    n.push_back(body(Pipe,Point(1200,200),Point(1200,1000),200));
    return n;
}
static Point turn(Point p,double angle,double scale,bool vector){
    return Point((p.x*std::cos(angle)-p.y*std::sin(angle)+(vector?0:10000))/scale,
                 (p.x*std::sin(angle)+p.y*std::cos(angle)+(vector?0:-5000))/scale,p.z/scale);
}
int main(){
    const double scales[]={1,10,1000},angles[]={0,.523598775598299,1.570796326794897,-.785398163397448};
    for(int u=0;u<3;++u)for(int r=0;r<4;++r){
        std::vector<Body> n=fixture();double f=scales[u],a=angles[r];
        for(size_t i=0;i<n.size();++i){
            for(size_t p=0;p<n[i].ports.size();++p){n[i].ports[p]=turn(n[i].ports[p],a,f,false);n[i].axes[p]=turn(n[i].axes[p],a,1,true);n[i].sections[p].a/=f;n[i].sections[p].b/=f;}
            for(size_t p=0;p<n[i].junctions.size();++p)n[i].junctions[p]=turn(n[i].junctions[p],a,f,false);
        }
        Result out;assert(plan(n,0,turn(Point(0,300),a,f,true),1/f,.1/f,out));assert(out.changed==10);
        assert(out.masks[0]==3&&out.masks[2]==7&&out.masks[5]==3);
        assert(out.masks[3]==1&&out.masks[8]==1&&out.masks[9]==1);
        assert(tvsMoveGeometry::pointsNear(out.ports[0][0],turn(Point(0,300),a,f,false),1e-7/f));
        assert(tvsMoveGeometry::pointsNear(out.ports[9][1],n[9].ports[1],1e-7/f));
        assert(std::fabs((out.ports[9][1]-out.ports[9][0]).length()-500/f)<1e-7/f);
        assert(!plan(n,0,turn(Point(0,900),a,f,true),1/f,.1/f,out));assert(out.error==TooShort);
    }
    // Round and mixed-section transitions use each port's own shape/diameter.
    {
        std::vector<Body> mixed=fixture();
        for(size_t i=0;i<mixed.size();++i)for(size_t j=0;j<mixed[i].sections.size();++j){
            mixed[i].sections[j].round=true;mixed[i].sections[j].b=0;
        }
        Result checked;assert(plan(mixed,0,Point(0,-300),1,.1,checked));
        assert(std::fabs((checked.ports[9][1]-checked.ports[9][0]).length()-1100)<1e-8);
        mixed[5].sections[1].round=false;mixed[5].sections[1].b=400;
        for(size_t i=6;i<=8;++i)for(size_t j=0;j<mixed[i].sections.size();++j){
            mixed[i].sections[j].round=false;mixed[i].sections[j].b=400;
        }
        assert(plan(mixed,0,Point(0,300),1,.1,checked));
    }
    Result out;std::vector<Body> n=fixture();
    n[9].sections[0].a=300;assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==SectionMismatch);
    n=fixture();n.push_back(n[1]);assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==Ambiguous);
    n=fixture();n[5].kind=Blocked;assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==Unsupported);
    n=fixture();n.push_back(body(Pipe,Point(500,0),Point(500,1000),250));
    assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==SideBranch);
    n=fixture();Body obstacle;obstacle.ports.push_back(Point(0,300));n.push_back(obstacle);
    assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==NewContact);
    n=fixture();n[9].axes[0]=Point(1,0);assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==NotTangent);
    n=fixture();n[9].ports[1].z=100;assert(!plan(n,0,Point(0,300),1,.1,out));assert(out.error==NotTangent||out.error==NonPlanar);
    n.clear();n.push_back(body(Pipe,Point(0,0),Point(1,0),1));
    n.push_back(body(Rigid,Point(-1,0),Point(0,0),1));
    for(int i=1;i<300;++i)n.push_back(body(Rigid,Point(i,0),Point(i+1,0),1));
    assert(!plan(n,0,Point(0,1),.01,.001,out));assert(out.error==TooMany);
    return 0;
}

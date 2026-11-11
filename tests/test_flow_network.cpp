#include "../geometry/FlowNetwork.h"
#include <cassert>
#include <cstdio>
using namespace tvsFlowNetwork;
static Element el(std::vector<P> ports,double flow=0){Element e;e.ports=ports;e.flow=flow;return e;}
static Element duct(double ax,double ay,double bx,double by,double flow=0){return el({P(ax,ay,0),P(bx,by,0)},flow);}
int main(){
    const double tol=0.1;
    // unit -> duct 0 -> tee 1 (main 1->2, branch 3) -> duct 2 (500), elbow 3 -> duct 4 (300)
    {std::vector<Element> v={duct(0,0,1000,0,99),el({P(1000,0,0),P(1400,0,0),P(1200,200,0)},7),duct(1400,0,3000,0,500),
                             el({P(1200,200,0),P(1400,400,0)}),duct(1400,400,1400,2000,300)};
     Result r=solve(v,0,0,tol);assert(r.error==Ok);
     assert(std::fabs(r.flow[0]-800)<1e-9&&std::fabs(r.flow[1]-800)<1e-9&&std::fabs(r.flow[3]-300)<1e-9);
     assert(r.flow[2]==500&&r.flow[4]==300&&r.terminals.size()==2&&r.total==800&&r.open.empty());
     assert(r.parent[1]==0&&r.parent[4]==3);}
    // root must be a free end
    {std::vector<Element> v={duct(0,0,1000,0),duct(1000,0,2000,0,100)};Result r=solve(v,0,1,tol);assert(r.error==RootJoined);
     r=solve(v,1,1,tol);assert(r.error==Ok&&r.total==0&&r.terminals.size()==1&&r.terminals[0]==0);}
    // tee with a free branch: open port reported, the main still sums
    {std::vector<Element> v={duct(0,0,1000,0),el({P(1000,0,0),P(1400,0,0),P(1200,200,0)}),duct(1400,0,3000,0,400)};
     Result r=solve(v,0,0,tol);assert(r.error==Ok&&r.total==400&&r.open.size()==1&&r.open[0].first==1&&r.open[0].second==2);}
    // loop
    {std::vector<Element> v={duct(0,-100,0,0),el({P(0,0,0),P(1000,0,0),P(0,1000,0)}),duct(1000,0,1000,1000),duct(1000,1000,0,1000)};
     Result r=solve(v,0,0,tol);assert(r.error==Cycle);}
    // three ports at one point: ambiguous
    {std::vector<Element> v={duct(0,0,1000,0),duct(1000,0,2000,0,100),duct(1000,0,1000,800,100)};
     Result r=solve(v,0,0,tol);assert(r.error==Ambiguous&&r.problem==0);}
    // stacked at other levels (3D): not joined
    {std::vector<Element> v={duct(0,0,1000,0),el({P(1000,0,0),P(2000,0,0)},100),el({P(1000,0,3000),P(1000,800,3000)},50)};
     Result r=solve(v,0,0,tol);assert(r.error==Ok&&r.total==100&&!r.reached[2]&&r.flow[2]==50);}
    // 0,05 apart joins, 0,2 apart does not (drawing units)
    {std::vector<Element> v={duct(0,0,1000,0),duct(1000.05,0,2000,0,250)};assert(solve(v,0,0,tol).total==250);
     v[1]=duct(1000.2,0,2000,0,250);Result r=solve(v,0,0,tol);assert(r.total==0&&r.terminals.size()==1);}
    // bad root
    {std::vector<Element> v={duct(0,0,1,0)};assert(solve(v,3,0,tol).error==BadRoot&&solve(v,0,2,tol).error==BadRoot);}
    std::puts("flow network OK");return 0;
}

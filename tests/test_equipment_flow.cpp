// geometry/EquipmentFlow.h: oprema in the network readers (units, level of the ports, terminal flow).
#include "../geometry/EquipmentFlow.h"
#include "../geometry/FlowNetwork.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace tvsEquipmentFlow;
int main(){
    assert(nodeKind()==5);
    assert(unitScale(1,1)==1&&unitScale(1000,1000)==1&&std::fabs(unitScale(1,1000)-0.001)<1e-15&&unitScale(1000,1)==1000);
    assert(unitScale(0,1)==1&&unitScale(1,0)==1);
    // level: duct ends have z = 0 (drawing plane), the kota is the element's level
    std::vector<Candidate> c;
    Candidate a={1000,0,0,2850},b={1000,0,0,0},d={5000,0,7,2850};c.push_back(a);c.push_back(b);c.push_back(d);
    assert(pickLevel(c,1000.05,0,2850,0.1)==0);         // stacked: the one at the record's level
    assert(pickLevel(c,1000,0,100,0.1)==1);
    assert(pickLevel(c,1000.5,0,2850,0.1)==-1);         // not in plan
    assert(pickLevel(c,5000,0,2850,0.1)==2&&c[2].z==7);
    // terminal flow: record first, else the duct, warning when both differ
    bool k=false;
    assert(terminalFlow(350,0,k)==350&&!k);
    assert(terminalFlow(0,420,k)==420&&!k);
    assert(terminalFlow(350,420,k)==350&&k);
    assert(terminalFlow(350,350.2,k)==350&&!k);
    assert(terminalFlow(0,0,k)==0&&!k);
    // koljeno gore / dolje: level of the vertical outlet (90 deg: axis -/+ rc)
    const double pi=3.14159265358979323846;
    assert(std::fabs(outletLevel(2800,false,300,pi/2)-2500)<1e-9&&std::fabs(outletLevel(2800,true,300,pi/2)-3100)<1e-9);
    assert(std::fabs(outletLevel(2800,false,300,pi/4)-(2800-300*(1-std::sqrt(0.5))))<1e-9);
    // resetka na dnu: on the duct axis, inside the duct (not at an end), nearest kota of stacked ducts
    assert(std::fabs(onAxis(0,0,3000,0,2920,0.5,1)-2920.0/3000)<1e-12);
    assert(onAxis(0,0,3000,0,2920,1.5,1)<0&&onAxis(0,0,3000,0,2999.5,0,1)<0&&onAxis(0,0,3000,0,-10,0,1)<0&&onAxis(0,0,0,0,0,0,1)<0);
    {std::vector<Axis> ax;const Axis a0={0,0,3000,0,0},a1={0,0,3000,0,2800},a2={0,500,3000,500,2800};ax.push_back(a0);ax.push_back(a1);ax.push_back(a2);
     double t=-1;assert(pickDuct(ax,1500,0,2790,1,t)==1&&std::fabs(t-0.5)<1e-12);
     assert(pickDuct(ax,1500,0,100,1,t)==0);assert(pickDuct(ax,1500,250,2800,1,t)==-1&&t<0);
     assert(pickDuct(ax,1500,500.4,2800,1,t)==2);}
    assert(tapKind()==6&&tapKind()!=nodeKind()&&tapOffset(0.1)>0.1);
    // the split as FlowCommand.h splitBottom builds it: duct A (unit .. tap), tap junction J
    // (A side, B side, branch down), piece B (tap .. closed end), grille on the branch
    {using tvsFlowNetwork::P;const double tol=0.1,s=tapOffset(tol);
     const P x(2920,0,0),xa(x.x-s,0,0),xb(x.x+s,0,0),xc(x.x,0,-s);
     std::vector<tvsFlowNetwork::Element> e(4);
     e[0].ports={P(0,0,0),xa};e[1].ports={xa,xb,xc};e[2].ports={xb,P(3000,0,0)};e[3].ports={xc};e[3].flow=400;
     std::vector<char> piece={1,0,1,0};
     tvsFlowNetwork::Result r=tvsFlowNetwork::solve(e,0,0,tol);assert(r.error==tvsFlowNetwork::Ok&&r.reached[2]&&r.terminals.size()==2);
     tvsFlowNetwork::dropDeadEnds(r,piece,0);
     assert(!r.reached[2]&&r.terminals.size()==1&&r.terminals[0]==3&&r.order.size()==3&&r.open.empty());   // the closed end left
     assert(r.flow[0]==400&&r.flow[1]==400&&r.parent[3]==1&&r.via[3]==2&&r.entry[1]==0);
     // the unit on the other side: piece A is the closed end
     r=tvsFlowNetwork::solve(e,2,1,tol);tvsFlowNetwork::dropDeadEnds(r,piece,2);assert(!r.reached[0]&&r.reached[2]&&r.flow[2]==400&&r.entry[1]==1);
     // the duct goes on: piece B carries the terminal behind it, nothing is dropped
     e.push_back(tvsFlowNetwork::Element());e[4].ports={P(3000,0,0),P(5000,0,0)};
     e.push_back(tvsFlowNetwork::Element());e[5].ports={P(5000,0,0)};e[5].flow=300;
     piece.push_back(0);piece.push_back(0);
     r=tvsFlowNetwork::solve(e,0,0,tol);tvsFlowNetwork::dropDeadEnds(r,piece,0);
     assert(r.error==tvsFlowNetwork::Ok&&r.reached[2]&&r.flow[0]==700&&r.flow[2]==300&&r.terminals.size()==2);}
    std::puts("equipment flow OK");return 0;
}

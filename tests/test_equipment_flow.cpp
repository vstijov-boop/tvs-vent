// geometry/EquipmentFlow.h: oprema in the network readers (units, level of the ports, terminal flow).
#include "../geometry/EquipmentFlow.h"
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
    std::puts("equipment flow OK");return 0;
}

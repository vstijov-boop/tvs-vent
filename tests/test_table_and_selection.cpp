#include "../TVS_Ventilation_ARX/TableScale.h"
#include "../TVS_Ventilation_ARX/SelectionMass.h"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

static void close(double actual,double expected) {
    assert(std::fabs(actual-expected)<1e-11*(1+std::fabs(expected)));
}
int main(int argc,char **argv) {
    const double units[]={1,10,1000};
    const double scales[]={1,20,50,100,200};
    const bool fixtures=argc==2 && std::string(argv[1])=="--fixtures";
    std::cout<<std::setprecision(17);
    for (int i=0;i<3;++i) for (int j=0;j<5;++j) {
        const tvsTable::Sizing sizing(scales[j],units[i]);
        assert(sizing.valid());
        close(sizing.textHeight()*units[i]/scales[j],2.5);
        close(800*sizing.templateFactor()*units[i]/scales[j],8.0);
        close(2000*sizing.templateFactor()*units[i]/scales[j],20.0);
        close(150*sizing.templateFactor()*units[i]/scales[j],1.5);
        if (fixtures) std::cout<<units[i]<<' '<<scales[j]<<' '<<sizing.textHeight()<<'\n';
    }
    close(tvsTable::Sizing(50,1).textHeight(),125);
    close(tvsTable::Sizing(50,10).textHeight(),12.5);
    close(tvsTable::Sizing(50,1000).textHeight(),.125);
    close(tvsTable::Sizing(1,1).textHeight(),2.5); // Metric paper-space 1:1.
    assert(!tvsTable::Sizing(0,1).valid());
    assert(!tvsTable::Sizing(-50,1).valid());
    assert(!tvsTable::Sizing(50,0).valid());
    assert(!tvsTable::Sizing(std::numeric_limits<double>::infinity(),1).valid());
    assert(!tvsTable::Sizing(std::numeric_limits<double>::quiet_NaN(),1).valid());

    tvsSelection::Total<int> total;
    total.add(1,true,16.956); total.add(2,true,5.652);
    total.add(1,true,16.956); // Duplicate selection entry is not a second duct.
    total.add(3,false,1000); // Text/annotation, not native sheet metal.
    total.add(4,true,3.107349293665664);
    total.add(5,true,0); // A supported zero-length duct is still supported.
    close(total.mass,25.715349293665664);
    assert(total.count==4 && total.objects.size()==5);
    assert(total.contains(3) && !total.contains(99));
    total.add(6,true,-1); assert(total.count==4);
    total.add(7,true,std::numeric_limits<double>::quiet_NaN()); assert(total.count==4);
    tvsSelection::CacheKey<int> key;
    const int a[]={10,20,30},b[]={10,30,20},c[]={10,20};
    assert(!key.matches(a,3,1));
    key.set(a,3,1); assert(key.matches(a,3,1));
    assert(!key.matches(a,3,2)); // Geometry changed while the selection stayed the same.
    assert(!key.matches(b,3,1)); assert(!key.matches(c,2,1));
    key.clear(); assert(!key.matches(a,3,1));
    if (!fixtures) std::cout<<"Table scaling and selection totals passed\n";
}

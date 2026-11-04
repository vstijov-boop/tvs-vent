#include "../geometry/OffsetRecordRules.h"
#include "../geometry/SlopeProjection.h"
#include "../geometry/OffsetAdapter.h"
#include "../TVS_Ventilation_DBX/SheetMetal.h"
#include <iostream>
#include <limits>
#include <cstdlib>
static int checks=0;
static void check(bool v){++checks;if(!v){std::cerr<<"FAILED check "<<checks<<"\n";std::exit(1);}}
static void eq(double a,double b,double tolerance=1e-7){check(std::fabs(a-b)<=tolerance);}
int main(){
    using namespace tvsOffsetRecordRules;
    check(validate(1,0,true,0)==ScalarValid);check(validate(1,1,true,282.842712474619)==ScalarValid);
    check(validate(1,1,true,-282.842712474619)==ScalarValid);check(validate(999,1,true,1)==ScalarUnsupported);
    check(validate(1,-1,true,1)==ScalarCorrupt);check(validate(1,3,true,0)==ScalarCorrupt);
    check(validate(1,0,true,1)==ScalarCorrupt);check(validate(1,1,false,1)==ScalarCorrupt);
    check(validate(1,1,true,std::numeric_limits<double>::quiet_NaN())==ScalarCorrupt);
    double length=0;check(physical(282.842712474619,282.842712474619,length));eq(length,400);
    eq(length/1000,.4);double area=tvsSheetMetal::pipeArea(400,200,length);eq(area,.48);
    eq(tvsSheetMetal::mass(area,.6),2.2608);
    check(physical(282.842712474619,-282.842712474619,length));eq(length,400);
    check(!physical(-1,2,length));check(!physical(1,std::numeric_limits<double>::infinity(),length));
    check(physical(0,0,length));eq(length,0);
    const double f[]={1,10,1000},headings[]={0,.6457718232379019,1.570796326794897,-2.2},angles[]={30,45,60};
    for(int u=0;u<3;++u)for(int h=0;h<4;++h)for(int a=0;a<3;++a)for(int sign=-1;sign<=1;sign+=2){
        tvsOffset::Input input(sign*400,400,200,100,angles[a]*tvsOffset::pi()/180,100);
        tvsOffset::Policy policy;policy.minimumProjection=.1;
        tvsOffset::Result result=tvsOffset::solve(input,policy);check(result.hasGeometry());
        double heading=headings[h],factor=f[u];
        tvsOffset::WorldPoint origin(12000/factor,-5000/factor,37/factor),joints[2][4],ends[2];
        for(int j=0;j<2;++j)check(tvsOffset::place(tvsOffset::Point(result.axis[j+1].s,0,0),origin,heading,factor,ends[j]));
        double rise=result.axis[2].z-result.axis[1].z;
        check(physical(result.projectedLength,rise,length));eq(length,result.straightLength);
        check(tvsSlopeProjection::joints(ends[0],ends[1],400/factor,200/factor,rise/factor,joints));
        for(int j=0;j<2;++j)for(int k=0;k<4;++k){
            tvsOffset::WorldPoint expected;
            check(tvsOffset::place(tvsOffset::Point(result.corners[j+1][k].s,0,result.corners[j+1][k].t),origin,heading,factor,expected));
            eq(joints[j][k].x,expected.x,1e-7/factor);eq(joints[j][k].y,expected.y,1e-7/factor);eq(joints[j][k].z,37/factor);
        }
    }
    std::cout<<"PASS metadata scalar validation, physical quantity/mass oracle and projected joint planes against accepted Result\nALL PASSED: "<<checks<<" checks\n";
    return 0;
}

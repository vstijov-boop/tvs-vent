#include "../geometry/OffsetOperation.h"
#include "../geometry/OffsetRecordRules.h"
#include "../TVS_Ventilation_DBX/SheetMetal.h"
#include <cassert>
#include <iostream>
static int checks=0;
static void near(double a,double b,double tolerance=1e-7){++checks;assert(std::fabs(a-b)<=tolerance);}
static void point(const tvsOffset::WorldPoint& a,const tvsOffset::WorldPoint& b){near(a.x,b.x);near(a.y,b.y);near(a.z,b.z);}
int main(){
    using namespace tvsOffsetOperation;
    const double factors[]={1,10,1000},angles[]={30,45,60},headings[]={0,37,90,217};
    int cases=0;
    for(int end=0;end<2;++end)for(int sign=-1;sign<=1;sign+=2)
    for(int unit=0;unit<3;++unit)for(int angle=0;angle<3;++angle)for(int heading=0;heading<4;++heading){
        Snapshot s;s.factor=factors[unit];s.a=400/s.factor;s.b=200/s.factor;s.elevation=-400/s.factor;s.end=End(end);
        const double h=headings[heading]*tvsOffset::pi()/180,c=std::cos(h),v=std::sin(h);
        const tvsOffset::WorldPoint q(1234/s.factor,-789/s.factor,37/s.factor);
        const tvsOffset::WorldPoint fixed(q.x-6000*c/s.factor,q.y-6000*v/s.factor,q.z);
        s.first=end==0?q:fixed;s.last=end==1?q:fixed;
        Request r;r.elevation=s.elevation+sign*800/s.factor;r.angleDegrees=angles[angle];r.ri=100;
        const Plan p=prepare(s,r);assert(p.status==Ready);++cases;
        point(end==0?p.remainingLast:p.remainingFirst,fixed);
        point(end==0?p.remainingFirst:p.remainingLast,p.dto.incomingLast);
        near(p.exitLast.z,37/s.factor);near(p.acceptedElevation*s.factor,-400+sign*800);
        const double ex=p.exitLast.x-p.exitFirst.x,ey=p.exitLast.y-p.exitFirst.y;
        near(std::sqrt(ex*ex+ey*ey)*s.factor,300);near(ex*c+ey*v,300/s.factor);
        const double dx=p.dto.middle.last.x-p.dto.middle.first.x,dy=p.dto.middle.last.y-p.dto.middle.first.y;
        near(std::sqrt(dx*dx+dy*dy)*s.factor,p.geometry.projectedLength);
        const double rise=p.geometry.axis[2].z-p.geometry.axis[1].z;
        double physical=0;assert(tvsOffsetRecordRules::physical(p.geometry.projectedLength,rise,physical));
        near(physical,p.geometry.straightLength);near(rise,p.dto.middle.lastElevationMm-p.dto.middle.firstElevationMm);
        // Same physical input through either endpoint acquisition must reach the same plan.
        Snapshot other=s;other.end=end==0?Last:First;other.first=s.last;other.last=s.first;
        const Plan equivalent=prepare(other,r);assert(equivalent.status==Ready);
        point(p.dto.incomingLast,equivalent.dto.incomingLast);point(p.dto.middle.first,equivalent.dto.middle.first);
        point(p.dto.middle.last,equivalent.dto.middle.last);point(p.exitFirst,equivalent.exitFirst);
        near(p.geometry.straightLength,equivalent.geometry.straightLength);
        const tvsOffset::WorldPoint beforeFirst=s.first,beforeLast=s.last;
        Request noChange=r;noChange.elevation=s.elevation;assert(prepare(s,noChange).status==NoChange);
        Request shortHeight=r;shortHeight.elevation=s.elevation+sign*1/s.factor;assert(prepare(s,shortHeight).status!=Ready);
        Request ninety=r;ninety.angleDegrees=90;assert(prepare(s,ninety).status==UnsupportedGeometry);
        Snapshot shortInput=s;
        if(end==0)shortInput.last=tvsOffset::WorldPoint(q.x-c/s.factor,q.y-v/s.factor,q.z);
        else shortInput.first=tvsOffset::WorldPoint(q.x-c/s.factor,q.y-v/s.factor,q.z);
        assert(prepare(shortInput,r).status==TooShort);
        point(s.first,beforeFirst);point(s.last,beforeLast);
    }
    Snapshot oracle;oracle.first=tvsOffset::WorldPoint(-2000,0,37);oracle.last=tvsOffset::WorldPoint(0,0,37);
    oracle.a=400;oracle.b=200;oracle.elevation=-400;oracle.factor=1;
    Request r;r.elevation=0;
    const Plan p=prepare(oracle,r);assert(p.status==Ready);
    near(p.geometry.projectedLength,282.842712474619);near(p.geometry.straightLength,400);
    near(p.geometry.axis[2].z-p.geometry.axis[1].z,282.842712474619);
    near(p.geometry.straightLength/1000,.4);
    near(tvsSheetMetal::pipeArea(400,200,p.geometry.straightLength),.48);
    near(tvsSheetMetal::mass(.48,.6),2.2608);
    Snapshot invalid=oracle;invalid.last.z+=1;assert(prepare(invalid,r).status==Invalid);
    invalid=oracle;invalid.factor=25.4;assert(prepare(invalid,r).status==Invalid);
    invalid=oracle;invalid.end=End(9);assert(prepare(invalid,r).status==Invalid);
    r.ri=0;assert(prepare(oracle,r).status==Invalid);
    std::cout<<"PASS "<<cases<<" shared physical plans: First/Last, rise/fall, 30/45/60, A!=B, mm/cm/m, four WCS headings and independent drawing Z; "<<checks<<" checks\n";
}

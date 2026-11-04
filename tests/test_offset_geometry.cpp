#include "../geometry/OffsetGeometry.h"
#include <iostream>
#include <cstdlib>
#include <limits>
#include <iomanip>
using namespace tvsOffset;
static int checks=0;
static void require(bool b){++checks;if(!b){std::cerr<<"FAILED check "<<checks<<"\n";std::exit(1);}}
static void equal(double a,double b,double tolerance=1e-8){require(std::fabs(a-b)<=tolerance);}
static double distance(const Point& a,const Point& b){double x=a.s-b.s,y=a.z-b.z,t=a.t-b.t;return std::sqrt(x*x+y*y+t*t);}
static void golden(){
    // Fixed independent reference coordinates/lengths from the accepted analysis.
    Result r=solve(Input(400,400,200,100,pi()/4,0));require(r.status==Ok);
    const double points[4][2]={{0,0},{141.4213562373095,58.5786437626905},{424.2640687119285,341.4213562373095},{565.6854249492380,400}};
    for(int i=0;i<4;++i){equal(r.axis[i].s,points[i][0]);equal(r.axis[i].z,points[i][1]);equal(r.axis[i].t,0);}
    equal(r.straightLength,400);equal(r.projectedLength,282.8427124746190);equal(r.arcLength,157.0796326794897);
    equal(r.centers[0].s,0);equal(r.centers[0].z,200);equal(r.centers[1].s,565.6854249492380);equal(r.centers[1].z,200);
    equal(r.corners[1][2].s,70.7106781186548);equal(r.corners[1][2].z,129.2893218813452);equal(r.corners[1][2].t,-200);
    equal(r.corners[1][0].s,212.1320343559643);equal(r.corners[1][0].z,-12.1320343559643);
    equal(r.corners[2][2].s,353.5533905932738);equal(r.corners[2][2].z,412.1320343559643);
    equal(r.corners[2][0].s,494.9747468305833);equal(r.corners[2][0].z,270.7106781186548);
    const double degrees[2]={30,60},lengths[2]={692.8203230275509,230.9401076758503};
    const double projections[2]={600,115.4700538379252},runs[2]={800,461.8802153517006};
    for(int i=0;i<2;++i){r=solve(Input(400,400,200,100,degrees[i]*pi()/180,0));require(r.status==Ok);
        equal(r.straightLength,lengths[i]);equal(r.projectedLength,projections[i]);equal(r.totalRun,runs[i]);}
    // A/B reversal is physically a different elbow: fixed oracle at 45 degrees.
    r=solve(Input(400,200,400,200,pi()/4,0));require(r.status==Ok);
    equal(r.radius,400);equal(r.straightLength,234.3145750507620);equal(r.projectedLength,165.6854249492380);equal(r.totalRun,731.3708498984760);
    std::cout<<"PASS fixed numerical oracles (30/45/60 deg, joints, contours, A != B)\n";
}
static void minimumAndRadius(){
    require(solve(Input(400,400,200,-1,pi()/4,0)).status==InvalidInput);
    Result r=solve(Input(400,400,200,0,pi()/4,0));require(r.status==Ok);equal(r.radius,100);
    equal(r.straightLength,482.8427124746190);equal(r.totalRun,482.8427124746190);
    // Tangent arcs with Lmin=0: physical middle piece may degenerate to zero.
    r=solve(Input(200,400,200,100,pi()/3,0));require(r.status==Ok);equal(r.straightLength,0);
    equal(r.projectedLength,0);equal(distance(r.axis[1],r.axis[2]),0);
    require(solve(Input(200,400,200,100,pi()/3,1)).status==TooShort);
    require(solve(Input(100,400,200,100,pi()/4,0)).status==TooShort);
    require(solve(Input(400,400,200,100,pi()/4,-1)).status==InvalidInput);
    r=solve(Input(400,400,200,100,pi()/4,0));
    // Threshold tests are metamorphic, separate from fixed numerical oracles.
    double boundary=r.straightLength;
    require(solve(Input(400,400,200,100,pi()/4,boundary)).status==Ok);
    require(solve(Input(400,400,200,100,pi()/4,boundary+.0000001)).status==TooShort);
    require(solve(Input(400,400,200,100,pi()/4,boundary-.0000001)).status==Ok);
    Policy p;p.minimumProjection=.1;
    require(solve(Input(200,400,200,100,pi()/3,0),p).status==ProjectionTooSmall);
    std::cout<<"PASS explicit Lmin zero/positive/boundary, negative/zero Ri, projection policy\n";
}
static const char* statusName(Status s){return s==Ok?"Ok":s==TooShort?"TooShort":"Unexpected";}
static void analyticMinimumBoundaries(){
    // Independent analytic heights for R=200; decimal oracles fixed BEFORE solve.
    // Lmin=100: H30=450-200sqrt(3), H45=400-150sqrt(2), H60=200+50sqrt(3).
    // Lmin=0: H30=400-200sqrt(3), H45=400-200sqrt(2), H60=200.
    const double deg[]={30,45,60};
    const double h100[]={103.5898384862245412945107316988255266,
                         187.8679656440357426797466913685452882,
                         286.6025403784438646763723170752936183};
    const double h0[]={53.5898384862245412945107316988255266,
                      117.1572875253809902396622551580603843,200.0};
    for(int i=0;i<3;++i)for(int zero=0;zero<2;++zero){
        double h=zero?h0[i]:h100[i],minimum=zero?0:100,angle=deg[i]*pi()/180;
        Result below=solve(Input(h-.000001,400,200,100,angle,minimum));
        Result above=solve(Input(h+.000001,400,200,100,angle,minimum));
        require(below.status==TooShort);require(above.status==Ok);
        Result boundary=solve(Input(h,400,200,100,angle,minimum));
        // At the decimal-rounded analytic boundary strict binary comparisons can reject.
        // Do not mandate a libm-specific rounding direction or relax production policy.
        require(boundary.status==Ok||boundary.status==TooShort);
        if(boundary.status==Ok)equal(boundary.straightLength,minimum,1e-10);
        // Diagnostic only: never used to choose heights or acceptance expectations.
        Result measured=solve(Input(h,400,200,100,angle,0));
        std::cout<<std::setprecision(17)<<"BOUNDARY "<<deg[i]<<"deg Lmin="<<minimum
                 <<" H="<<h<<" below="<<statusName(below.status)
                 <<" exact-decimal="<<statusName(boundary.status)<<" above="<<statusName(above.status);
        if(measured.status==Ok)std::cout<<" Lcomputed="<<measured.straightLength;
        else std::cout<<" remainder-negative-under-strict-policy";
        std::cout<<"\n";
    }
    std::cout<<"PASS independent analytic Lmin boundaries (30/45/60 deg, Lmin=0/100)\n";
}
static void invariants(){
    const double degrees[]={15,30,45,60,75};
    for(int a=0;a<5;++a){
        Result up=solve(Input(2000,400,200,100,degrees[a]*pi()/180,0));
        Result down=solve(Input(-2000,400,200,100,degrees[a]*pi()/180,0));
        require(up.hasGeometry()&&down.hasGeometry());
        equal(distance(up.axis[1],up.axis[2]),up.straightLength);
        equal(up.axis[2].s-up.axis[1].s,up.projectedLength);
        for(int j=0;j<4;++j){equal(up.axis[j].s,down.axis[j].s);equal(up.axis[j].z,-down.axis[j].z);
            equal(plan(up.axis[j]).u,profile(up.axis[j]).u);
            for(int k=0;k<4;++k){
                equal(plan(up.corners[j][k]).u,profile(up.corners[j][k]).u);
                // z reflection reverses the local vertical sign, preserves transverse side.
                const Point& mirrored=down.corners[j][k^2];
                equal(up.corners[j][k].s,mirrored.s);
                equal(up.corners[j][k].z,-mirrored.z);
                equal(up.corners[j][k].t,mirrored.t);
            }}
        for(int arc=0;arc<2;++arc)for(int end=0;end<2;++end)equal(distance(up.centers[arc],up.axis[2*arc+end]),200);
        // Tangency tested as radial dot straight vector = 0 (independent constraint).
        Point v(up.axis[2].s-up.axis[1].s,up.axis[2].z-up.axis[1].z);
        equal(((up.axis[1].s-up.centers[0].s)*v.s+(up.axis[1].z-up.centers[0].z)*v.z)/up.straightLength,0);
        equal(((up.axis[2].s-up.centers[1].s)*v.s+(up.axis[2].z-up.centers[1].z)*v.z)/up.straightLength,0);
    }
    std::cout<<"PASS rising/falling, ALL joint corners mapped k XOR 2, shared projections and tangency (15..75 deg)\n";
}
static void placement(){
    Result r=solve(Input(400,400,200,100,pi()/4,0));const double units[]={1,10,1000};
    for(int u=0;u<3;++u)for(int k=0;k<4;++k){
        WorldPoint origin(10000,-5000,-400/units[u]),end;
        require(place(r.axis[3],origin,k*pi()/2,units[u],end));
        const double xs[]={565.685424949238,0,-565.685424949238,0},ys[]={0,565.685424949238,0,-565.685424949238};
        equal(end.x,origin.x+xs[k]/units[u]);equal(end.y,origin.y+ys[k]/units[u]);equal(end.z,0);
    }
    // Independent 3-4-5 rotation oracle, arbitrary non-cardinal heading 53.1301 deg.
    // (s,t)=(120,35) -> (x,y)=(44,117); z=-40, all local components nonzero.
    for(int u=0;u<3;++u){
        WorldPoint shifted;
        require(place(Point(120,-40,35),WorldPoint(1000,-500,120),std::atan2(4.0,3.0),units[u],shifted));
        equal(shifted.x,1000+44/units[u]);equal(shifted.y,-500+117/units[u]);equal(shifted.z,120-40/units[u]);
    }
    WorldPoint out(1,2,3);require(!place(r.axis[3],WorldPoint(),0,0,out));equal(out.x,1);
    // Absolute elevation is not input to solve: it affects placement only.
    require(place(r.axis[3],WorldPoint(0,0,1e12),0,1,out));equal(out.z,1e12+400,.001);
    std::cout<<"PASS placement: mm/cm/m, four headings + nonzero s/t/z arbitrary-heading oracle, absolute elevation separated\n";
}
static void validation(){
    require(solve(Input(0,400,200,100,pi()/4,0)).status==NoChange);
    require(solve(Input(.0005,400,200,100,pi()/4,0)).status==NoChange);
    require(solve(Input(-.0005,400,200,100,pi()/4,0)).status==NoChange);
    require(solve(Input(400,400,200,100,pi()/2,0)).status==Unsupported90);
    require(solve(Input(400,400,200,100,pi()/2-5e-9,0)).status==Unsupported90);
    require(solve(Input(400,400,200,100,pi()/2+5e-9,0)).status==Unsupported90);
    require(solve(Input(400,400,200,100,pi()/2+.001,0)).status==InvalidInput);
    require(solve(Input(400,400,200,100,0,0)).status==InvalidInput);
    require(solve(Input(400,400,200,100,pi()/3600,0)).status==InvalidInput);
    require(solve(Input(400,0,200,100,pi()/4,0)).status==InvalidInput);
    require(solve(Input(400,400,0,100,pi()/4,0)).status==InvalidInput);
    require(solve(Input(std::numeric_limits<double>::infinity(),400,200,100,pi()/4,0)).status==InvalidInput);
    require(solve(Input(400,400,200,std::numeric_limits<double>::quiet_NaN(),pi()/4,0)).status==InvalidInput);
    Policy p;p.absoluteTolerance=0;require(solve(Input(400,400,200,100,pi()/4,0),p).status==InvalidInput);
    Result large=solve(Input(1e9,400,200,100,pi()/4,0));require(large.status==Ok);
    equal(large.axis[3].z,1e9,.001);
    require(solve(Input(DBL_MAX,400,DBL_MAX,DBL_MAX,pi()/4,0)).status==NumericalFailure);
    Result fail=solve(Input(100,400,200,100,pi()/4,0));require(!fail.hasGeometry());equal(fail.totalRun,0);
    std::cout<<"PASS NoChange, exact 90 rejection, invalid/NaN/infinite/overflow, failure has no geometry\n";
}
int main(){golden();minimumAndRadius();analyticMinimumBoundaries();invariants();placement();validation();std::cout<<"ALL PASSED: "<<checks<<" checks\n";return 0;}

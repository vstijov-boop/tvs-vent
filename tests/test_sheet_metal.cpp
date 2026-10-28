#include "../TVS_Ventilation_DBX/SheetMetal.h"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

static void close(double actual, double expected)
{
    assert(std::fabs(actual-expected) <= 1e-10*(1.0+std::fabs(expected)));
}

static void fixture(int kind, double a, double b, double a2, double b2,
                    double length, double radius, double degrees)
{
    using namespace tvsSheetMetal;
    const double area = kind == 0 ? pipeArea(a,b,length)
        : kind == 1 ? elbowArea(a,b,radius,degrees*pi/180.0)
        : kind == 2 ? transitionArea(a,b,a2,b2,length)
        : wyeArea(a,b,a2,b2,length);
    const double t = thickness(larger(larger(a,b),larger(a2,b2)));
    std::cout << kind << ' ' << a << ' ' << b << ' ' << a2 << ' ' << b2
        << ' ' << length << ' ' << radius << ' ' << degrees << ' '
        << area << ' ' << t << ' ' << mass(area,t) << '\n';
}

int main(int argc, char **argv)
{
    using namespace tvsSheetMetal;
    if (argc == 2 && std::string(argv[1]) == "--fixtures") {
        std::cout << std::setprecision(17);
        const double sides[] = {400,500,500.01,1000,1000.01,1500,1500.01};
        const double angles[] = {15,45,90,135,165};
        for (int i=0;i<7;++i) {
            fixture(0,sides[i],200,0,0,3000,0,0);
            for (int j=0;j<5;++j) fixture(1,sides[i],200,0,0,0,150,angles[j]);
            fixture(2,sides[i],200,300,150,500,0,0);
            fixture(3,sides[i],200,300,150,100,0,0);
        }
        return 0;
    }
    close(pipeArea(400,200,3000),3.6);
    close(mass(3.6,0.6),16.956);
    // Two annular side plates plus inner/outer curved strips, independently summed.
    const double theta = pi/2, ri=150, ro=550, height=200;
    const double developed = (theta*(ro*ro-ri*ri) + theta*(ri+ro)*height)/1e6;
    close(elbowArea(400,200,ri,theta),developed);
    close(developed,0.21*pi);
    close(transitionArea(400,200,300,150,500),0.525);
    close(wyeArea(400,200,300,150,100),0.645);
    close(mass(pipeArea(400,200,3000),0.6)/3.0,
          mass(pipeArea(400,200,1000),0.6));
    close(mass(pipeArea(400,200,1000)+pipeArea(400,200,2000),0.6),16.956);
    close(elbowArea(400,200,ri,pi/4)*2,elbowArea(400,200,ri,pi/2));
    close(elbowArea(400,200,ri,-pi/2),elbowArea(400,200,ri,pi/2));
    close(elbowArea(200,400,ri,pi/2),0.15*pi); // Vertical bend plane differs.
    close(pipeArea(400,0,3000),1.2*pi);
    close(elbowArea(400,0,150,pi/2),0.07*pi*pi);
    close(transitionArea(400,0,200,0,500),pi*300*std::sqrt(260000.0)/1e6);
    assert(wyeArea(400,0,200,0,100)>0);
    const double sides[] = {500,500.001,1000,1000.001,1500,1500.001};
    const double expected[] = {.6,.8,.8,1.,1.,1.2};
    for (int i=0;i<6;++i) close(thickness(sides[i]),expected[i]);
    close(thickness(0),0); close(thickness(-1),0);
    assert(!nonnegative(std::numeric_limits<double>::infinity()));
    assert(!nonnegative(std::numeric_limits<double>::quiet_NaN()));
    // Read identical physical dimensions from mm/cm/m models.
    const double factors[] = {1,10,1000};
    for (int i=0;i<3;++i) {
        const double f=factors[i];
        close(pipeArea((400/f)*f,(200/f)*f,(3000/f)*f),3.6);
        close(elbowArea((400/f)*f,(200/f)*f,(150/f)*f,theta),developed);
        close(transitionArea((400/f)*f,(200/f)*f,(300/f)*f,(150/f)*f,(500/f)*f),.525);
        close(wyeArea((400/f)*f,(200/f)*f,(300/f)*f,(150/f)*f,(100/f)*f),.645);
    }
    std::cout << "Sheet metal calculations passed\n";
}

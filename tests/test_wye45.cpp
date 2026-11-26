// Gacasta racva Y45 (Soko str. 36): geometry/TeeShape.h (Pants + lateral) and geometry/SokoPants.h.
#include "../geometry/TeeShape.h"
#include "../geometry/SokoPants.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
static int fails=0;
static void near(double a,double b,double tol,const char* what){if(std::fabs(a-b)>tol){std::printf("FAIL %s: %.3f != %.3f\n",what,a,b);++fails;}}
static void check(bool c,const char* what){if(!c){std::printf("FAIL %s\n",what);++fails;}}
static tvsTee::Tee wye(double A,double w1,double w2,double s,double L2){
    tvsTee::Tee t;t.type=tvsTee::Pants;t.round=true;t.A=A;t.w1=w1;t.w2=w2;t.margin=s;t.l=L2;t.lateral=45;return t;}
int main(){
    // the user's block KTP_htany_200: inlet straight 40, branch axis through (81.4, 0), crotch 222.8
    near(tvsTee::wyeAxisY(200,200,40,45),81.42,0.01,"axis O");
    tvsTee::P k;check(tvsTee::wyeCrotch(200,200,200,40,45,k),"crotch");near(k.x,0,1e-9,"crotch x");near(k.y,222.84,0.01,"crotch y");
    near(tvsTee::wyeMinL2(200,200,200,40,45,1),100,1e-6,"min L2 = w/2");
    // block: face 140 from O (130 + ring 10)
    tvsTee::Tee b=wye(200,200,200,40,140);check(tvsTee::valid(b),"block valid");
    tvsTee::P p[3],d[3];tvsTee::wyePorts(b,p,d);
    near(p[1].x,140/std::sqrt(2.0),1e-6,"port 1 x");near(p[1].y,81.42+140/std::sqrt(2.0),0.01,"port 1 y");
    near(p[0].x,-p[1].x,1e-9,"port 0 mirrored");near(d[1].x,std::sqrt(0.5),1e-12,"dir 1");near(d[2].y,-1,1e-12,"inlet dir");
    tvsTee::Shape s;check(tvsTee::shape(b,s),"shape");check(s.lines.size()==9&&s.axis.size()==3,"9 lines, 3 axes");
    // Soko 200 / 140, L2 280
    double L2=0;const double s200=tvsSokoPants::spigotMm(200);near(s200,40,0,"spigot 200");
    check(tvsSokoPants::length(200,140,140,tvsTee::wyeMinL2(200,140,140,s200,45,1),L2),"Soko row");near(L2,280,0,"L2 280");
    check(tvsTee::valid(wye(200,140,140,s200,L2)),"200/140 valid");
    check(tvsSokoPants::branchFor(630)==400,"branch for 630");
    // fallback: d3 + spigot(d3)
    check(!tvsSokoPants::length(200,160,125,tvsTee::wyeMinL2(200,160,125,40,45,1),L2),"unequal: fallback");near(L2,200,0,"fallback 160");
    check(tvsTee::valid(wye(200,160,125,40,200)),"unequal valid");
    // every table row is drawable with its L2
    int n=0;const tvsSokoPants::Row* r=tvsSokoPants::rows(n);
    for(int i=0;i<n;++i){double l=0;const double sp=tvsSokoPants::spigotMm(r[i].d1);
        check(tvsSokoPants::length(r[i].d1,r[i].d3,r[i].d3,tvsTee::wyeMinL2(r[i].d1,r[i].d3,r[i].d3,sp,45,1),l),"table row used");
        if(!tvsTee::valid(wye(r[i].d1,r[i].d3,r[i].d3,sp,l))){std::printf("FAIL row %d/%d\n",r[i].d1,r[i].d3);++fails;}}
    // branches far too small for the inlet: crotch inside the inlet straight
    check(!tvsTee::valid(wye(400,100,100,75,300)),"too small");
    // TK-G rectangular untouched
    tvsTee::Tee g;g.type=tvsTee::Pants;g.A=400;g.w1=300;g.w2=300;g.R1=450;g.R2=450;g.l=25;check(tvsTee::valid(g),"TK-G");
    near(tvsTee::area(wye(200,140,140,40,280),0,0),3.14159265*(200*40+2*140*280)/1e6,1e-9,"area");
    if(fails)return 1;
    std::printf("OK\n");return 0;
}

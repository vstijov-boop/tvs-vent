// Stucna with a round branch (geometry/TeeShape.h): cylindrical (SR5-11) and conical (SR5-12).
#include "../geometry/TeeShape.h"
#include <cmath>
#include <cstdio>
using namespace tvsTee;
static int fails=0;
static void yes(bool c,const char* w){if(!c){std::printf("FAIL %s\n",w);++fails;}}
static bool eqd(double a,double b){return std::fabs(a-b)<1e-6;}
int main(){
    // main 400 wide, branch D 160: cylinder L = 0.25 D -> 75 mm minimum
    Tee c;c.type=Tap;c.A=400;c.w1=160;c.l=tapLength(160,75);c.round=true;c.flange=25;
    Shape s;yes(shape(c,s),"cylinder drawn");yes(eqd(c.l,75),"L 75");
    bool top=false,bottom=false,slant=false;
    for(size_t k=0;k<s.lines.size();++k){const Seg& g=s.lines[k];
        if(eqd(g.a.x,200)&&eqd(g.b.x,275)){if(eqd(g.a.y,75)&&eqd(g.b.y,75))bottom=true;if(eqd(g.a.y,235)&&eqd(g.b.y,235))top=true;}
        if(eqd(g.a.x,200)&&eqd(g.a.y,0)&&eqd(g.b.x,275))slant=true;}
    yes(top&&bottom&&!slant,"cylinder: two parallel walls, no 45 deg entry");
    // cone: L = 100, base = D + 2 L tan 15
    Tee k=c;k.l=coneLengthMm();k.cone=coneBase(160,k.l);Shape t;yes(shape(k,t),"cone drawn");
    yes(eqd(k.cone,160+200*std::tan(15*3.14159265358979323846/180)),"cone base 15 deg");
    const double yc=100+80;bool base=false;
    for(size_t j=0;j<t.lines.size();++j){const Seg& g=t.lines[j];if(eqd(g.a.x,200)&&eqd(g.a.y,yc-k.cone/2)&&eqd(g.b.x,300)&&eqd(g.b.y,100))base=true;}
    yes(base,"cone wall from the base on the main to the branch face");
    yes(area(c,200,0)>0&&area(k,200,0)>area(c,200,0)-1,"areas");
    Tee bad=k;bad.round=false;yes(!valid(bad),"cone only for a round branch");
    if(fails)return 1;std::printf("OK\n");return 0;
}

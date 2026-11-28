// plan 26: XK second branch (geometry/TeeShape.h lateralSecond)
#include "../geometry/TeeShape.h"
#include <cstdio>
using namespace tvsTee;
static int bad=0;
static void yes(bool c,const char* w){if(!c){++bad;std::printf("FAIL %s\n",w);}}
static bool near(double a,double b){return std::fabs(a-b)<1e-6;}
int main(){
    Tee t;t.type=Tap;t.A=315;t.C=315;t.w1=200;t.l=50;t.round=true;t.lateral=45;t.margin=25;
    Shape s;yes(shape(t,s),"kosa racva shape");const size_t lines=s.lines.size(),axes=s.axis.size();
    P f,d;yes(lateralSecond(t,200,s,&f,&d),"XK same size");
    double before,after;tapRun(t,before,after);const double th=45*pi()/180,reach=lateralReach(t.A,200,45,t.l);
    yes(near(f.x,-std::sin(th)*reach)&&near(f.y,before+std::cos(th)*reach),"second face = mirror of the first");
    yes(near(d.x,-std::sin(th))&&near(d.y,std::cos(th)),"second dir leans downstream on -x");
    yes(s.lines.size()==lines+6&&s.axis.size()==axes+1,"walls split + branch lines + axis");
    bool split=false;for(size_t k=0;k<s.lines.size();++k){const Seg& g=s.lines[k];if(near(g.a.x,-t.A/2)&&near(g.b.x,-t.A/2)&&near(g.a.y,0)&&!near(g.b.y,before+after))split=true;}
    yes(split,"-x main wall opened at the footprint");
    Shape s2;shape(t,s2);yes(!lateralSecond(t,250,s2),"wider second branch refused");
    Shape s3;shape(t,s3);yes(lateralSecond(t,125,s3,&f,&d)&&f.x<0,"narrower second branch");
    Tee k;k.type=Tap;k.A=400;k.w1=200;k.l=75;Shape s4;shape(k,s4);yes(!lateralSecond(k,150,s4),"stucna (not lateral) refused");
    if(!bad)std::printf("OK\n");
    return bad;
}

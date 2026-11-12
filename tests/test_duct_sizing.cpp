#include "../geometry/DuctSizing.h"
#include <cassert>
#include <cstdio>
using namespace tvsDuctSizing;
static bool near1(double a,double b,double t=1e-6){return std::fabs(a-b)<=t;}
int main(){
    // 400x200, 2880 m3/h: 0.8 m3/s / 0.08 m2 = 10 m/s
    assert(near1(velocity(2880,400,200,false),10));
    assert(near1(velocity(-2880,400,200,false),10));
    assert(near1(velocity(1000,200,0,true),1000/3600.0/(3.14159265358979323846*0.01)));
    // width only, height 200, 5 m/s: area >= 0.16 m2 -> 800
    {Size s=widthOnly(2880,200,5);assert(s.ok&&s.a==800&&s.b==200&&s.v<=5);}
    // exactly on the limit is accepted
    {Size s=widthOnly(1800,200,5);assert(s.ok&&s.a==500);}
    // width and height: 0.16 m2, height >= 200, ratio <= 4: 400x400 (0.16)
    {Size s=widthHeight(2880,200,5);assert(s.ok&&s.a*s.b>=160000-1e-6&&s.a>=s.b&&s.a<=4*s.b);assert(near1(s.a*s.b,160000));}
    // height may not drop below the current one
    {Size s=widthHeight(360,300,5);assert(s.ok&&s.b>=300&&s.a==300);}
    // round: 1000 m3/h, 4 m/s -> area >= 0.0694 -> d >= 297 -> 300
    {Size s=diameter(1000,4);assert(s.ok&&s.a==300);}
    // impossible / bad input
    {assert(!widthOnly(1e7,100,2).ok);assert(!widthOnly(100,0,5).ok);assert(!diameter(100,0).ok);}
    // dionice: 0 root -> 1 -> 2 (tee) -> {3 -> 4, 5}; 6 unreached
    {std::vector<int> parent={-1,0,1,2,3,2,-1};std::vector<char> reached={1,1,1,1,1,1,0};std::vector<int> g;
     const int n=dionice(parent,reached,g);
     assert(n==3&&g[0]==g[1]&&g[1]==g[2]&&g[3]==g[4]&&g[3]!=g[0]&&g[5]!=g[3]&&g[5]!=g[0]&&g[6]==-1);}
    // order independent: a child listed before its parent
    {std::vector<int> parent={2,-1,1,0};std::vector<char> reached={1,1,1,1};std::vector<int> g;
     assert(dionice(parent,reached,g)==1&&g[0]==g[1]&&g[2]==g[3]&&g[0]==g[3]);}
    // manual size
    {Size s;assert(parse(L"300x150",false,s)&&s.a==300&&s.b==150);
     assert(parse(L" 300 X 150 ",false,s)&&s.a==300&&s.b==150);
     assert(parse(L"300*150",false,s)&&parse(L"300\u00D7150",false,s)&&s.b==150);
     assert(parse(L"d200",false,s)&&s.a==200&&s.b==0);assert(parse(L"\u00D8250",false,s)&&s.a==250);
     assert(parse(L"200",true,s)&&s.a==200&&s.b==0);
     assert(!parse(L"200",false,s));assert(!parse(L"",false,s));assert(!parse(L"abc",false,s));
     assert(!parse(L"d200x100",false,s));assert(!parse(L"300x",false,s));assert(!parse(L"5x150",false,s));
     assert(parse(L"312,5x150",false,s)&&s.a==312.5);}
    // friction: 400x200 at 10 m/s (Dh 266.7 mm, Re ~1.77e5, k 0.15): about 4.2 Pa/m
    {const double r=friction(2880,400,200,false);assert(r>3.9&&r<4.5);
     const double d=friction(1000,250,0,true);assert(d>1.3&&d<1.8);   // d250, 5.66 m/s: ~1.5 Pa/m
     assert(friction(1000,500,250,false)<friction(1000,400,200,false));assert(friction(0,400,200,false)==0);}
    // candidates: height kept, 2 too fast first, then the acceptable ones, widths ascending
    {std::vector<Size> c;candidates(2880,false,200,0,5,c);
     assert(c.size()==6&&c[0].a==700&&c[1].a==750&&c[2].a==800&&c[2].v<=5&&c[1].v>5&&c[5].a==950);
     for(size_t i=0;i<c.size();++i)assert(c[i].b==200&&c[i].r>0);}
    // ratio <= 4: areas ascending, width >= height, width <= 4 height
    {std::vector<Size> c;candidates(2880,false,200,1,5,c);assert(c.size()==6);
     for(size_t i=0;i<c.size();++i){assert(c[i].a>=c[i].b&&c[i].a<=4*c[i].b);if(i)assert(c[i].a*c[i].b>=c[i-1].a*c[i-1].b);}
     assert(c[1].v>5&&c[2].v<=5&&c[2].a==400&&c[2].b==400);}
    // round
    {std::vector<Size> c;candidates(1000,true,0,0,4,c);assert(c[2].a==300&&c[1].v>4&&c[2].b==0);}
    // nothing fast enough: the largest ones
    {std::vector<Size> c;candidates(1e8,false,100,0,2,c);assert(c.size()==2&&c[1].a==3000);}
    std::puts("duct sizing OK");return 0;
}

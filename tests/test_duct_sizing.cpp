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
    std::puts("duct sizing OK");return 0;
}

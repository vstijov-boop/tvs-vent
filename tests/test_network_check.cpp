#include "../geometry/NetworkCheck.h"
#include <cassert>
#include <cstdio>
using namespace tvsNetworkCheck;
static Item duct(double ax,double ay,double bx,double by,double a,double b,double z){Item it;it.kind=1;it.x={ax,bx};it.y={ay,by};
    Section s;s.a=a;s.b=b;s.elevation=z;s.known=true;it.s={s,s};return it;}
static int count(const std::vector<Issue>& v,int t){int n=0;for(auto& e:v)if(e.type==t)++n;return n;}
int main(){
    std::vector<Issue> r;
    // clean joint
    {std::vector<Item> v={duct(0,0,1000,0,400,200,2850),duct(1000,0,2000,0,400,200,2850)};check(v,1,r);assert(r.empty());}
    // other size, no prelaz
    {std::vector<Item> v={duct(0,0,1000,0,400,200,2850),duct(1000,0,2000,0,300,200,2850)};check(v,1,r);assert(count(r,Size)==1&&r.size()==1);}
    // other level (the user's case: duct at 0 on a T komad at 2850)
    {std::vector<Item> v={duct(0,0,1000,0,200,200,2850),duct(1000,0,1000,1000,200,200,0)};check(v,1,r);assert(count(r,Level)==1);assert(std::fabs(r[0].value+2850)<1e-6);}
    // end that misses by 2 mm
    {std::vector<Item> v={duct(0,0,1000,0,400,200,0),duct(1002,0,2000,0,400,200,0)};check(v,1,r);assert(count(r,Miss)==1&&std::fabs(r[0].value-2)<1e-6);}
    // duct end on the middle of another duct
    {std::vector<Item> v={duct(0,0,2000,0,400,200,0),duct(1000,200,1000,1000,200,200,0)};check(v,1,r);assert(count(r,Middle)==1);}
    // elbow 89,954 deg -> angle issue; 90 exact -> none
    {Item e;e.kind=2;e.x={0,1};e.y={0,1};e.angle=89.954;std::vector<Item> v={e};check(v,1,r);assert(count(r,Angle)==1);
     v[0].angle=90;check(v,1,r);assert(r.empty());}
    // 5 mm duct
    {std::vector<Item> v={duct(0,0,5,0,400,200,0)};check(v,1,r);assert(count(r,Short)==1);}
    // drawing in cm (f = 10): 0,2 cm = 2 mm miss
    {std::vector<Item> v={duct(0,0,100,0,40,20,0),duct(100.2,0,200,0,40,20,0)};check(v,10,r);assert(count(r,Miss)==1&&std::fabs(r[0].value-2)<1e-6);}
    // stacked: three ports at one point -> no one-to-one level issue
    {std::vector<Item> v={duct(0,0,1000,0,200,200,0),duct(1000,0,2000,0,200,200,0),duct(1000,0,1000,800,200,200,3000)};check(v,1,r);assert(count(r,Level)==0);}
    // oprema (kind 5, known = false): ports on duct ends of its size and level -> fine
    {auto eq=[](double ax,double bx,double a,double b,double z){Item it;it.kind=5;it.x={ax,bx};it.y={0,0};Section s;s.a=a;s.b=b;s.elevation=z;s.known=false;it.s={s,s};return it;};
     std::vector<Item> v={duct(0,0,1000,0,400,200,2850),eq(1000,1400,400,200,2850),duct(1400,0,2000,0,400,200,2850)};check(v,1,r);assert(r.empty());
     v[1].s[1].b=250;check(v,1,r);assert(count(r,Equipment)==1&&r[0].b==2&&r[0].value==0&&r[0].a==1);          // other size
     v[1].s[1].b=200;v[2].x[0]=1403;check(v,1,r);assert(count(r,Equipment)==1&&std::fabs(r[0].value-3)<1e-6&&count(r,Miss)==0);   // 3 mm gap
     v[2].x[0]=1400;v[1].s[0].elevation=2700;check(v,1,r);assert(count(r,Equipment)==1&&r[0].x==1000);          // other level
     v[1].s[0].elevation=2850;v.erase(v.begin()+2);v.erase(v.begin());v.push_back(duct(0,0,3000,0,400,200,2850));check(v,1,r);assert(count(r,Equipment)==2);}   // duct under the block
    std::puts("network check OK");return 0;
}

#include "../geometry/DionicaSegment.h"
#include <cassert>
#include <cstdio>
using namespace tvsDionicaSegment;
using tvsFlowNetwork::P;
static Item it(int kind,P a,P b,double w=200,double h=150,int group=0){Item i;i.kind=kind;i.ports={a,b};i.a=w;i.b=h;i.group=group;return i;}
static bool has(const std::vector<int>& v,int x){for(int y:v)if(y==x)return true;return false;}
int main(){
    const double t=0.1;
    // 0 duct - 1 elbow - 2 duct - 3 duct (other size) ; 4 tee-ish other at the start of 0
    {std::vector<Item> v={it(Duct,P(0,0,0),P(1000,0,0)),it(Elbow,P(1000,0,0),P(1200,200,0)),it(Duct,P(1200,200,0),P(1200,1200,0)),
                          it(Duct,P(1200,1200,0),P(1200,2000,0),300,150),it(Other,P(-300,0,0),P(0,0,0))};
     v[4].ports.push_back(P(-150,150,0));
     Result r=find(v,2,t);assert(r.ok&&r.members.size()==3&&has(r.members,0)&&has(r.members,1)&&has(r.members,2));
     assert(r.ends.size()==2);
     for(const End& e:r.ends){if(e.element==0){assert(e.port==0&&e.other==4&&e.otherPort==1);}else{assert(e.element==2&&e.port==1&&e.other==3&&e.otherPort==0);}}}
    // elbow at the end is left out: 0 duct - 1 elbow - 2 duct of another size
    {std::vector<Item> v={it(Duct,P(0,0,0),P(1000,0,0)),it(Elbow,P(1000,0,0),P(1200,200,0)),it(Duct,P(1200,200,0),P(1200,1200,0),250,150)};
     Result r=find(v,0,t);assert(r.ok&&r.members.size()==1&&r.members[0]==0);
     assert(r.ends.size()==2&&(r.ends[0].other==1||r.ends[1].other==1));}
    // other dionica (group) stops the part; free end
    {std::vector<Item> v={it(Duct,P(0,0,0),P(1000,0,0)),it(Duct,P(1000,0,0),P(2000,0,0),200,150,1)};
     Result r=find(v,0,t);assert(r.ok&&r.members.size()==1);
     int freeEnds=0;for(const End& e:r.ends)if(e.other<0)++freeEnds;assert(freeEnds==1);}
    // two elbows at the end in a row both out
    {std::vector<Item> v={it(Duct,P(0,0,0),P(1000,0,0)),it(Elbow,P(1000,0,0),P(1100,100,0)),it(Elbow,P(1100,100,0),P(1100,300,0))};
     Result r=find(v,0,t);assert(r.ok&&r.members.size()==1);}
    // ambiguous joint
    {std::vector<Item> v={it(Duct,P(0,0,0),P(1000,0,0)),it(Duct,P(1000,0,0),P(2000,0,0)),it(Duct,P(1000,0,0),P(1000,900,0))};
     Result r=find(v,0,t);assert(!r.ok&&r.ambiguous);}
    // start must be a duct
    {std::vector<Item> v={it(Elbow,P(0,0,0),P(10,10,0))};assert(!find(v,0,t).ok);assert(!find(v,5,t).ok);}
    std::puts("dionica segment OK");return 0;
}

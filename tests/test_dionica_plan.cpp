// tests for geometry/DionicaPlan.h (TVS_PROTOK Primijeni). Units: mm.
#include "../geometry/DionicaPlan.h"
#include <cstdio>
#include <cstdlib>
using namespace tvsDionicaPlan;
static int fails=0;
#define CHECK(x) do{if(!(x)){std::printf("FAIL line %d: %s\n",__LINE__,#x);++fails;}}while(0)
static Item duct(double x0,double y0,double x1,double y1,Section s,int g){Item d;d.kind=Duct;d.plain=true;d.group=g;d.ports.push_back(P(x0,y0,0));d.ports.push_back(P(x1,y1,0));d.s.assign(2,s);return d;}
static Item prelaz(double x0,double y0,double x1,double y1,Section a,Section b,int g,bool aligned=false){Item d=duct(x0,y0,x1,y1,a,g);d.kind=Prelaz;d.plain=false;d.s[1]=b;d.aligned=aligned;return d;}
// plain T komad, main along +x through (bx,by), branch +y
static Item tee(double bx,double by,double Apr,double Bpr,double Aotv,double Botv,double lengthPl,int g,int type=Plain){
    Item t;t.kind=Tee;t.group=g;t.tee=type;t.base=P(bx,by,0);t.u=P(1,0,0);t.v=P(0,1,0);t.lengthPl=lengthPl;t.tapMin=75;
    t.ports.assign(3,P());t.s.push_back(Section(Apr,Bpr));t.s.push_back(Section(Apr,Bpr));t.s.push_back(Section(Aotv,Botv));
    P p[3];teePorts(t,Apr,Aotv,p);for(int k=0;k<3;++k)t.ports[k]=p[k];return t;}
// 90 deg plan elbow, middle (mx,my), legs towards -x and +y, R = k x A
static Item elbow(double mx,double my,Section s,double k,int g){Item e;e.kind=Elbow;e.plain=true;e.group=g;e.middle=P(mx,my,0);e.sweep=1.5707963267948966;e.rule=Variable;e.k=k;
    const double r=k*s.a+s.a/2;e.ports.push_back(P(mx-r,my,0));e.ports.push_back(P(mx,my+r,0));e.s.assign(2,s);return e;}
static const Edit* find(const Plan& p,int op,int item=-1){for(size_t i=0;i<p.edits.size();++i)if(p.edits[i].op==op&&(item<0||p.edits[i].item==item))return &p.edits[i];return 0;}
static Request request(int worst,Section s,int scope=Whole){Request r;r.worst=worst;r.size=s;r.scope=scope;return r;}
int main(){
    const Section o(400,300),big(600,300),nw(500,300),narrow(300,300);
    {   // 1. user's case: upstream T (branch outlet) - duct - downstream plain T (main inlet), ducts behind it
        std::vector<Item> v;
        v.push_back(tee(0,0,600,300,400,300,100,0));                  // 0 upstream: branch port at (0,400)
        v.push_back(duct(0,400,0,3000,o,1));                          // 1 dionica
        Item t=tee(0,3300,400,300,300,300,100,1);t.u=P(0,1,0);t.v=P(1,0,0);P p[3];teePorts(t,400,300,p);for(int k=0;k<3;++k)t.ports[k]=p[k];   // main along +y: inlet (0,3050)
        v[1].ports[1]=p[0];v.push_back(t);                            // 2 downstream T
        v.push_back(duct(p[1].x,p[1].y,0,6000,o,2));                  // 3 behind the prolaz
        v.push_back(duct(p[2].x,p[2].y,3000,3300,narrow,3));           // 4 branch duct
        const Plan r=plan(v,request(1,nw));
        CHECK(r.ok);CHECK(r.resized.size()==1&&r.resized[0]==1);
        CHECK(find(r,TeeMain,2)&&same(find(r,TeeMain,2)->s,nw));     // the T komad inlet takes the size
        const Edit* behind=find(r,PrelazInsert,3);CHECK(behind&&same(behind->from,nw)&&same(behind->s,o)&&behind->other==2&&behind->otherPort==1);   // prelaz behind the prolaz
        const Edit* branch=find(r,DuctEnd,4);CHECK(branch&&std::fabs(branch->to.x-(p[2].x+50))<1e-9);   // branch follows dA/2
        const Edit* up=find(r,PrelazInsert,1);CHECK(up&&up->other==0&&up->otherPort==2&&same(up->from,o)&&same(up->s,nw));   // prelaz on the upstream outlet, T komad 0 untouched
        CHECK(!find(r,TeeMain,0)&&!find(r,TeeBranch,0));
        CHECK(r.edits.front().op==TeeMain&&r.edits.back().op==PrelazInsert);   // order
    }
    {   // 2. TK-O downstream: its dialog, no prelaz in front; nothing else of the T komad planned
        std::vector<Item> v;v.push_back(duct(0,0,3000,0,o,0));Item t=tee(3250,0,400,300,300,300,100,0,Branch);t.ports[0]=P(3000,0,0);v.push_back(t);
        v.push_back(duct(t.ports[1].x,0,6000,0,o,1));
        const Plan r=plan(v,request(0,nw));
        CHECK(r.ok&&find(r,TeeDialog,1)&&find(r,TeeDialog,1)->port==0&&!find(r,PrelazInsert)&&!find(r,TeeMain));
    }
    {   // 3. elbow between two ducts: new radius, both duct ends follow along their axes
        std::vector<Item> v;v.push_back(duct(-3000,0,-600,0,o,0));v.push_back(elbow(0,0,o,1.0,0));v.push_back(duct(0,600,0,3000,o,0));
        const Plan r=plan(v,request(0,nw));
        CHECK(r.ok&&r.resized.size()==3&&find(r,ElbowSize,1));
        const Edit* a=find(r,DuctEnd,0);const Edit* b=find(r,DuctEnd,2);
        CHECK(a&&a->port==1&&std::fabs(a->to.x+750)<1e-6&&b&&b->port==0&&std::fabs(b->to.y-750)<1e-6);
    }
    {   // 4. elbow directly on the downstream T komad: elbow and T komad keep the size, prelaz in the duct in front of the elbow
        std::vector<Item> v;v.push_back(duct(-3000,0,-600,0,o,0));v.push_back(elbow(0,0,o,1.0,0));
        Item t=tee(0,850,400,300,300,300,100,0);t.u=P(0,1,0);t.v=P(1,0,0);P p[3];teePorts(t,400,300,p);for(int k=0;k<3;++k)t.ports[k]=p[k];v.push_back(t);
        CHECK(std::fabs(p[0].y-600)<1e-9);
        const Plan r=plan(v,request(0,nw));
        CHECK(r.ok&&r.resized.size()==1&&!find(r,TeeMain)&&!find(r,ElbowSize)&&r.has(ElbowKept));
        const Edit* x=find(r,PrelazInsert,0);CHECK(x&&x->port==1&&same(x->from,o)&&x->other<0);
    }
    {   // 5. prelaz at the upstream end takes the size; when no step is left it is removed and the duct grows
        std::vector<Item> v;v.push_back(duct(-2000,0,0,0,big,0));v.push_back(prelaz(0,0,300,0,big,o,1));v.push_back(duct(300,0,3000,0,o,1));
        Plan r=plan(v,request(2,nw));
        CHECK(r.ok&&find(r,PrelazEnd,1)&&find(r,PrelazEnd,1)->port==1&&!find(r,PrelazInsert)&&!find(r,PrelazRemove));
        r=plan(v,request(2,big));
        const Edit* x=find(r,PrelazRemove,1);CHECK(r.ok&&x&&x->other==2&&x->otherPort==0&&std::fabs(x->to.x)<1e-9&&!find(r,PrelazEnd));
    }
    {   // 6. prelaz aligned to one side: left alone, centred prelaz in front of it (warning)
        std::vector<Item> v;v.push_back(prelaz(0,0,300,0,big,o,1,true));v.push_back(duct(300,0,3000,0,o,1));
        const Plan r=plan(v,request(1,nw));
        CHECK(r.ok&&!find(r,PrelazEnd)&&find(r,PrelazInsert,1)&&r.has(AlignedPrelaz));
    }
    {   // 7. the prolaz has an elbow instead of a duct: the T komad stays, prelaz in front of it (warning)
        std::vector<Item> v;v.push_back(duct(0,0,3000,0,o,0));Item t=tee(3250,0,400,300,300,300,100,0);v.push_back(t);
        Item e=elbow(t.ports[1].x+600,0,o,1.0,1);v.push_back(e);
        const Plan r=plan(v,request(0,nw));
        CHECK(r.ok&&!find(r,TeeMain)&&r.has(TeeKept));const Edit* x=find(r,PrelazInsert,0);CHECK(x&&x->other==1&&x->otherPort==0&&same(x->from,o));
    }
    {   // 8. short stub on the upstream T komad: it keeps the size (with the elbow behind it), the prelaz goes into the first duct with room
        std::vector<Item> v;v.push_back(tee(0,-850,600,300,400,300,100,0));     // 0 upstream, branch port (0,-450)
        v.push_back(duct(0,-450,0,-250,o,1));v[1].ports[0]=v[0].ports[2];           // 1 stub 200 mm
        Item e;e.kind=Elbow;e.plain=true;e.group=1;e.middle=P(0,350,0);e.sweep=1.5707963267948966;e.rule=Variable;e.k=1.0;e.ports.push_back(P(0,-250,0));e.ports.push_back(P(600,350,0));e.s.assign(2,o);v.push_back(e);   // 2
        v.push_back(duct(600,350,4000,350,o,1));                                  // 3
        const Plan r=plan(v,request(3,nw));
        CHECK(r.ok&&r.resized.size()==1&&r.resized[0]==3&&r.has(DuctKept)&&r.has(ElbowKept)&&r.has(PartLeft));
        const Edit* x=find(r,PrelazInsert,3);CHECK(x&&x->port==0&&x->other<0&&same(x->from,o));
        CHECK(!plan(v,request(1,nw)).ok);    // the stub itself cannot take a prelaz
    }
    {   // 9. change of shape: the T komad stays (prelaz on its port)
        std::vector<Item> v;v.push_back(duct(0,0,3000,0,o,0));Item t=tee(3250,0,400,300,300,300,100,0);v.push_back(t);v.push_back(duct(t.ports[1].x,0,6000,0,o,1));
        const Plan r=plan(v,request(0,Section(400,0,true)));
        CHECK(r.ok&&!find(r,TeeMain)&&find(r,PrelazInsert,0)&&find(r,PrelazInsert,0)->other==1);
    }
    {   // 10. two sizes in one dionica: whole = the prelaz between them goes; same size only = its end takes the size
        std::vector<Item> v;v.push_back(duct(0,0,2000,0,big,0));v.push_back(prelaz(2000,0,2300,0,big,o,0));v.push_back(duct(2300,0,5000,0,o,0));
        Plan r=plan(v,request(2,nw));
        CHECK(r.ok&&r.resized.size()==2&&find(r,PrelazRemove,1)&&find(r,PrelazRemove,1)->other==0&&std::fabs(find(r,PrelazRemove,1)->to.x-2300)<1e-9&&r.count(DuctSize)==2&&!r.has(PartLeft));
        r=plan(v,request(2,nw,SameSize));
        CHECK(r.ok&&r.resized.size()==1&&find(r,PrelazEnd,1)&&r.has(PartLeft));
    }
    {   // 11. the dionica arrives on the branch of a plain T komad: the branch takes the size, the main ducts follow
        std::vector<Item> v;Item t=tee(0,0,600,300,400,300,100,0);v.push_back(duct(0,3000,0,400,o,0));v.push_back(t);
        v.push_back(duct(-3000,0,t.ports[0].x,0,big,1));v.push_back(duct(t.ports[1].x,0,3000,0,big,2));
        const Plan r=plan(v,request(0,nw));
        CHECK(r.ok&&find(r,TeeBranch,1)&&!find(r,PrelazInsert));
        const Edit* a=find(r,DuctEnd,2);const Edit* b=find(r,DuctEnd,3);CHECK(a&&std::fabs(a->to.x+350)<1e-9&&b&&std::fabs(b->to.x-350)<1e-9);
    }
    {   // 12. refusals: more than two ends in a point, same size, not a duct
        std::vector<Item> v;v.push_back(duct(0,0,3000,0,o,0));v.push_back(duct(3000,0,6000,0,o,0));v.push_back(duct(3000,0,3000,3000,o,0));
        Plan r=plan(v,request(0,nw));CHECK(!r.ok&&r.has(Ambiguous));
        v.pop_back();r=plan(v,request(0,o));CHECK(!r.ok&&r.has(SameAlready));
        r=plan(v,request(0,nw));CHECK(r.ok&&r.resized.size()==2&&r.count(DuctEnd)==0&&r.edits.size()==2);   // free ends: sizes only
    }
    std::printf(fails?"FAILED %d\n":"OK\n",fails);return fails?1:0;
}

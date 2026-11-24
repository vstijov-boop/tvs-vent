#include "../geometry/Equipment.h"
#include <cassert>
#include <cstdio>
using namespace tvsEquipment;
int main(){
    Record r;r.kind=FireDamper;r.tag=L"PPK-01;a=b\\";r.note=L"Klapna 400x250";r.ports=2;r.mmPerUnit=1000;
    r.a=400;r.b=250;r.lengthMm=500;r.model=Nominal;r.dpNominal=30;r.qNominal=1800;
    r.port[0].x=1.25;r.port[0].y=-3.5;r.port[0].z=2.7;r.port[0].dx=-1;r.port[0].dy=0;
    r.port[1].x=1.75;r.port[1].y=-3.5;r.port[1].z=2.7;r.port[1].dx=1;r.port[1].dy=0;
    Record q;assert(parse(format(r),q));
    assert(q.kind==FireDamper&&q.tag==r.tag&&q.note==r.note&&q.ports==2&&q.mmPerUnit==1000);
    assert(q.a==400&&q.b==250&&q.lengthMm==500&&q.model==Nominal&&q.dpNominal==30&&q.qNominal==1800);
    assert(q.port[0].x==1.25&&q.port[0].y==-3.5&&q.port[0].z==2.7&&q.port[0].dx==-1&&q.port[1].x==1.75&&q.port[1].dx==1);
    // loss models
    assert(std::fabs(loss(q,900,0)-7.5)<1e-12);            // 30 * (0.5)^2
    Record z;z.model=Zeta;z.zeta=1.5;assert(std::fabs(loss(z,0,20)-30)<1e-12);
    Record f;f.model=FixedDp;f.dp=25;assert(loss(f,123,4)==25);
    Record n;assert(loss(n,1000,50)==0);
    Record neg;neg.model=FixedDp;neg.dp=-5;assert(loss(neg,1,1)==0);
    // terminal round trip
    Record t;t.kind=Diffuser;t.ports=1;t.round=true;t.a=250;t.qTerminal=350;t.model=FixedDp;t.dp=18;
    Record u;assert(parse(format(t),u));assert(u.terminal()&&u.round&&u.a==250&&u.qTerminal==350&&u.dp==18);
    assert(terminalKind(Grille)&&!terminalKind(Vav));
    // not a record / unknown version / bad kind
    Record w;assert(!parse(L"",w));assert(!parse(L"v=2;",w));assert(parse(L"v=1;kind=99;model=7;",w)&&w.kind==Other&&w.model==NoLoss);
    // bottom grille (mount, neck)
    Record g;g.kind=Grille;g.ports=1;g.mount=Bottom;g.neckA=425;g.neckB=125;g.a=400;g.b=250;
    Record h;assert(parse(format(g),h));assert(h.mount==Bottom&&h.neckA==425&&h.neckB==125&&!h.neckRound);
    Record e;assert(parse(format(t),e)&&e.mount==AtEnd&&e.neckA==0);   // old records: at the end
    // curve (Q^2 interpolation, extrapolation by Q^2)
    {Record c;c.model=Curve;c.curve.push_back(std::make_pair(500.0,20.0));c.curve.push_back(std::make_pair(1000.0,80.0));c.curve.push_back(std::make_pair(1500.0,200.0));
     assert(std::fabs(loss(c,500,0)-20)<1e-9&&std::fabs(loss(c,1000,0)-80)<1e-9&&std::fabs(loss(c,1500,0)-200)<1e-9);
     assert(std::fabs(loss(c,750,0)-(20+60*(750.0*750-250000)/(1000000-250000)))<1e-9);
     assert(std::fabs(loss(c,250,0)-5)<1e-9&&std::fabs(loss(c,3000,0)-800)<1e-9);
     assert(curveOutside(c,250)&&curveOutside(c,3000)&&!curveOutside(c,500)&&!curveOutside(c,1200));
     Record d;assert(parse(format(c),d)&&d.model==Curve&&d.curve.size()==3&&d.curve[2].first==1500&&d.curve[2].second==200);
     Record one;one.model=Curve;one.curve.push_back(std::make_pair(750.0,50.0));assert(std::fabs(loss(one,1500,0)-200)<1e-9);}
    // anchor: moved / copied after linking
    {Record a;a.anchored=true;a.ax=100;a.ay=200;a.arot=0.5;Record b;assert(parse(format(a),b)&&b.anchored&&b.ax==100&&b.ay==200&&b.arot==0.5);
     assert(!detached(b,100,200,0.5,1e-4)&&detached(b,100.01,200,0.5,1e-4)&&detached(b,100,200,0.6,1e-4)&&!detached(b,100,200,0.5+6.283185307179586,1e-4));
     Record old;assert(!detached(old,5,5,1,1e-4));}
    // twin terminal (linijski difuzor): 2 ports on the box, Q/2 each, loss at the total flow on both
    {Record l;l.kind=Diffuser;l.ports=2;l.twin=true;l.qTerminal=600;l.model=Nominal;l.dpNominal=20;l.qNominal=600;
     l.port[0].x=1;l.port[1].x=2;assert(l.terminal());
     Record m;assert(parse(format(l),m)&&m.twin&&m.ports==2&&m.terminal()&&m.port[1].x==2);
     Record h=half(m,1);assert(h.ports==1&&h.half==1&&h.port[0].x==2&&h.qTerminal==300&&h.terminal());
     assert(std::fabs(loss(h,300,0)-20)<1e-9&&std::fabs(loss(m,600,0)-20)<1e-9);   // not halved
     Record one;one.ports=1;one.twin=true;Record o2;assert(parse(format(one),o2)&&!o2.twin);   // twin only with 2 ports
     Record inl;assert(!inl.terminal());}
    std::puts("equipment OK");
    return 0;
}

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
    std::puts("equipment OK");
    return 0;
}

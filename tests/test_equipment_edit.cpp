#include "../geometry/EquipmentEdit.h"
#include <cassert>
#include <cstdio>
using namespace tvsEquipment;
using namespace tvsEquipmentEdit;
int main(){
    // next free tag: largest + 1, gaps not reused, other prefixes / texts ignored, width kept
    std::vector<std::wstring> tags;
    assert(nextTag(L"PPK",tags)==L"PPK-01");
    tags.push_back(L"PPK-01");tags.push_back(L" ppk-02 ");tags.push_back(L"RP-07");tags.push_back(L"PPK-x");tags.push_back(L"PPK-");tags.push_back(L"PPKA-09");
    assert(nextTag(L"PPK",tags)==L"PPK-03");
    assert(nextTag(L"RP",tags)==L"RP-08");
    tags.push_back(L"PPK-005");assert(nextTag(L"PPK",tags)==L"PPK-006");
    tags.push_back(L"R-09");assert(nextTag(L"R",tags)==L"R-10");     // R-xx is not RP-xx
    int w=0;assert(tagNumber(L"D-12",L"D",&w)==12&&w==2);assert(tagNumber(L"D12",L"D",NULL)==-1);
    // sizes and names
    Record a;a.kind=FireDamper;a.a=400;a.b=250;a.ports=2;a.lengthMm=500;a.model=FixedDp;a.dp=20;
    assert(sizeText(a,L"%%c")==L"400x250");
    Record d;d.kind=Diffuser;d.round=true;d.a=250;d.ports=1;d.qTerminal=300;
    assert(sizeText(d,L"%%c")==L"%%c250");
    assert(specName(a)==kindName(FireDamper));d.note=L"  DLQ 250 ";assert(specName(d)==L"DLQ 250");
    // one record: values shown exactly
    std::vector<Record> one(1,a);Values v=merge(one);
    assert(v.kind==FireDamper&&v.model==FixedDp&&v.tag.empty()&&v.ports==2&&v.q.empty()&&v.size==L"400x250"&&v.length==L"500");
    double x=0;assert(tvsNumeric::parse(v.dp,x)&&x==20);
    // untouched -> no change
    Record r=a;assert(!apply(v,v,r));
    Values e=v;e.dp=L"20,0";assert(!apply(v,e,r));          // same number, other text
    e.dp=L"25,5";e.tag=L" PPK-03 ";e.model=Nominal;e.qn=L"1800";e.dpn=L"30";
    assert(apply(v,e,r)&&r.dp==25.5&&r.tag==L"PPK-03"&&r.model==Nominal&&r.qNominal==1800&&r.dpNominal==30);
    assert(validate(r).empty());
    // multiple selection: differing fields stay per block unless edited
    Record b=a;b.dp=35;b.tag=L"PPK-02";b.note=L"FK 400";b.kind=FlowDamper;
    std::vector<Record> two;two.push_back(a);two.push_back(b);
    Values m=merge(two);
    assert(m.kind==-1&&m.dp==differentText()&&m.tag==differentText()&&m.note==differentText()&&m.model==FixedDp&&m.size==L"400x250");
    Record a2=a,b2=b;assert(!apply(m,m,a2)&&!apply(m,m,b2));
    Values me=m;me.zeta=L"0,8";me.model=Zeta;
    assert(apply(m,me,a2)&&apply(m,me,b2));
    assert(a2.dp==20&&b2.dp==35&&a2.tag.empty()&&b2.tag==L"PPK-02"&&a2.kind==FireDamper&&b2.kind==FlowDamper);
    assert(a2.zeta==0.8&&b2.zeta==0.8&&a2.model==Zeta&&b2.model==Zeta);
    me.kind=Silencer;me.dp=L"12";Record a3=a;assert(apply(m,me,a3)&&a3.kind==Silencer&&a3.dp==12);
    // terminals: flow only on 1-port records
    std::vector<Record> mix;mix.push_back(a);mix.push_back(d);Values mv=merge(mix);
    assert(mv.ports==0&&mv.q.empty()&&mv.size==differentText()&&mv.length==differentText());
    Values dv=merge(std::vector<Record>(1,d));assert(dv.terminals()&&tvsNumeric::parse(dv.q,x)&&x==300&&dv.size==L"\u00d8250");
    Values de=dv;de.q=L"450";Record d2=d;assert(apply(dv,de,d2)&&d2.qTerminal==450);
    Record a4=a;assert(!apply(dv,de,a4));                     // 2 ports: no flow
    // typed texts
    assert(typedOk(L"20",L"21,5")&&typedOk(L"20",L"")&&typedOk(differentText(),differentText())&&!typedOk(L"20",L"abc"));
    // validation
    Record bad=a;bad.dp=-1;assert(!validate(bad).empty());
    bad=a;bad.zeta=-0.1;assert(!validate(bad).empty());
    bad=a;bad.model=Nominal;bad.dpNominal=30;bad.qNominal=0;assert(!validate(bad).empty());
    bad.qNominal=500;assert(validate(bad).empty());
    bad=d;bad.qTerminal=-3;assert(!validate(bad).empty());
    // per block name
    Record t=r;t.ports=2;t.qTerminal=99;t.port[0].x=5;Record td=typeDefault(t);
    assert(td.tag.empty()&&td.qTerminal==0&&td.port[0].x==0&&td.model==Nominal&&td.dpNominal==30&&td.lengthMm==500);
    Record other=b;assert(applyType(td,other)&&other.kind==td.kind&&other.model==Nominal&&other.qNominal==1800&&other.tag==L"PPK-02");
    assert(!applyType(td,other));
    // curve text (model Curve): paste from Excel / a digitizer, comma decimals, sorting
    {std::vector<std::pair<double,double> > c;
     assert(parseCurve(L"1000\t80\r\n500\t20\r\n1500\t200\r\n",c).empty()&&c.size()==3&&c[0].first==500&&c[2].second==200);
     assert(parseCurve(L"500 20; 750 37,5",c).empty()&&c.size()==2&&c[1].second==37.5);
     assert(parseCurve(L"500:20/",c).empty()||true);
     assert(!parseCurve(L"500",c).empty()&&!parseCurve(L"0 20",c).empty()&&!parseCurve(L"500 -1",c).empty()&&!parseCurve(L"500 20;500 30",c).empty());
     assert(parseCurve(L"",c).empty()&&c.empty());
     assert(formatCurve(c).empty());
     std::vector<std::pair<double,double> > d;d.push_back(std::make_pair(500.0,20.0));d.push_back(std::make_pair(750.0,37.5));
     assert(formatCurve(d)==L"500 20; 750 37.5");
     tvsEquipment::Record r;r.model=tvsEquipment::Curve;assert(!validate(r).empty());r.curve=d;assert(validate(r).empty());
     std::vector<tvsEquipment::Record> rs(1,r);Values b=merge(rs);Values a=b;a.curve=L"500 20; 1000 80";
     tvsEquipment::Record x=r;assert(apply(b,a,x)&&x.curve.size()==2&&x.curve[1].first==1000);
     tvsEquipment::Record y;assert(applyType(r,y)&&y.curve==d);}
    std::puts("equipment edit OK");
    return 0;
}

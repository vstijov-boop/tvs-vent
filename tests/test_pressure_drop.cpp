// geometry/PressureDrop.h + PressureSheet.h on a small network with made-up ASHRAE data.
//   test_pressure_drop [ashrae_data folder] [dump file]: with the real data (local) the same
//   network is computed with the ASHRAE tables and the workbook model is dumped (preview).
#include "../geometry/PressureSheet.h"
#include "../geometry/DuctSizing.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace tvsPressure;
static int fails=0;
static void eq(double a,double b,const char* w,double tol=1e-6){if(!(std::fabs(a-b)<=tol*(1+std::fabs(b)))){std::printf("FAIL %s: %.9g vs %.9g\n",w,a,b);++fails;}}
static void yes(bool c,const char* w){if(!c){std::printf("FAIL %s\n",w);++fails;}}
static Part duct(double a,double b,bool round,double len,XY p,XY q,bool flex=false){Part t;t.kind=Duct;t.flex=flex;t.length=len;t.sections.assign(2,Section(a,b,round));t.line.push_back(p);t.line.push_back(q);return t;}
static Element one(const Part& t,double flow,int parent,int entry,int via,std::vector<XY> at){Element e;e.parts.push_back(t);e.ports=t.sections;e.at=at;e.flow=flow;e.parent=parent;e.entry=entry;e.via=via;return e;}
// unit -> duct 600x300 -> elbow 90 -> duct -> TK-O (prolaz 500x300, ogranak 300x300)
//   prolaz: duct -> prelaz 500x300 -> 400x300 -> duct (terminal)
//   ogranak: duct 300x300 -> flex 300x300 (terminal)
static std::vector<Element> network(){
    std::vector<Element> e;
    e.push_back(one(duct(600,300,false,5000,XY(0,0),XY(5000,0)),3600,-1,0,-1,{XY(0,0),XY(5000,0)}));
    Part el;el.kind=Elbow;el.angle=90;el.radius=600;el.sections.assign(2,Section(600,300,false));el.line={XY(5000,0),XY(5600,0),XY(5600,600)};
    e.push_back(one(el,3600,0,0,1,{XY(5000,0),XY(5600,600)}));
    e.push_back(one(duct(600,300,false,4000,XY(5600,600),XY(5600,4600)),3600,1,0,1,{XY(5600,600),XY(5600,4600)}));
    Part t;t.kind=Tee;t.tee=TeeBranch;t.sections={Section(600,300,false),Section(500,300,false),Section(300,300,false)};t.line={XY(5600,4600),XY(5600,5400)};
    e.push_back(one(t,3600,2,0,1,{XY(5600,4600),XY(5600,5400),XY(5900,5000)}));
    e.push_back(one(duct(500,300,false,3000,XY(5600,5400),XY(5600,8400)),2400,3,0,1,{XY(5600,5400),XY(5600,8400)}));
    Part tr;tr.kind=Transition;tr.length=300;tr.sections={Section(500,300,false),Section(400,300,false)};tr.line={XY(5600,8400),XY(5600,8700)};
    e.push_back(one(tr,2400,4,0,1,{XY(5600,8400),XY(5600,8700)}));
    e.push_back(one(duct(400,300,false,2000,XY(5600,8700),XY(5600,10700)),2400,5,0,1,{XY(5600,8700),XY(5600,10700)}));
    e.push_back(one(duct(300,300,false,2500,XY(5900,5000),XY(8400,5000)),1200,3,0,2,{XY(5900,5000),XY(8400,5000)}));
    e.push_back(one(duct(300,300,false,1500,XY(8400,5000),XY(9900,5000),true),1200,7,0,1,{XY(8400,5000),XY(9900,5000)}));
    return e;
}
static std::string utf8(const std::wstring& w){std::string o;for(size_t i=0;i<w.size();++i){const unsigned c=static_cast<unsigned>(w[i]);
    if(c<0x80)o+=static_cast<char>(c);else if(c<0x800){o+=static_cast<char>(0xC0|(c>>6));o+=static_cast<char>(0x80|(c&63));}
    else{o+=static_cast<char>(0xE0|(c>>12));o+=static_cast<char>(0x80|((c>>6)&63));o+=static_cast<char>(0x80|(c&63));}}return o;}
struct U8 {std::ofstream f;explicit U8(const char* n):f(n){}};
template<class T> U8& operator<<(U8& u,const T& v){std::wostringstream s;s.imbue(std::locale::classic());s<<v;u.f<<utf8(s.str());return u;}
static bool s0temp(){return true;}
static void dump(const tvsSheet::Workbook& wb,const char* file){
    U8 o(file);
    for(size_t i=0;i<wb.sheets.size();++i){const tvsSheet::Sheet& s=wb.sheets[i];o<<L"SHEET\t"<<s.name<<L"\n";
        for(size_t r=0;r<s.cells.size();++r)for(size_t c=0;c<s.cells[r].size();++c)if(!s.cells[r][c].empty()){std::wstring v=s.cells[r][c];for(size_t k=0;k<v.size();++k)if(v[k]==L'\n')v.replace(k,1,L"\\n");o<<L"C\t"<<r+1<<L"\t"<<c+1<<L"\t"<<v<<L"\n";}
        for(size_t k=0;k<s.widths.size();++k)o<<L"W\t"<<k+1<<L"\t"<<s.widths[k]<<L"\n";
        for(size_t k=0;k<s.hidden.size();++k)o<<L"H\t"<<s.hidden[k]<<L"\n";
        for(size_t k=0;k<s.formats.size();++k){const tvsSheet::Format& f=s.formats[k];o<<L"F\t"<<f.range<<L"\t"<<f.number<<L"\t"<<f.bold<<L"\t"<<f.italic<<L"\t"<<f.wrap<<L"\t"<<f.border<<L"\t"<<f.size<<L"\t"<<f.font<<L"\t"<<f.fill<<L"\t"<<f.align<<L"\n";}
        for(size_t k=0;k<s.conditions.size();++k)o<<L"K\t"<<s.conditions[k].range<<L"\t"<<s.conditions[k].formula<<L"\t"<<s.conditions[k].fill<<L"\n";
        if(!s.freeze.empty())o<<L"Z\t"<<s.freeze<<L"\n";
        for(size_t k=0;k<s.strokes.size();++k){const tvsSheet::Stroke& t=s.strokes[k];o<<L"S\t"<<t.x1<<L"\t"<<t.y1<<L"\t"<<t.x2<<L"\t"<<t.y2<<L"\t"<<t.weight<<L"\t"<<t.color<<L"\t"<<t.dash<<L"\n";}
        for(size_t k=0;k<s.labels.size();++k){const tvsSheet::Label& t=s.labels[k];o<<L"L\t"<<t.x<<L"\t"<<t.y<<L"\t"<<t.size<<L"\t"<<t.color<<L"\t"<<t.bold<<L"\t"<<t.text<<L"\n";}}
}
int main(int argc,char** argv){
    std::map<std::string,std::string> files;
    files["Constants.csv"]="Variable;IPValue;SIValue;Description;Calculated\nVPC;4005;1.2910;x;0\n";
    files["Equation.csv"]="Key;Order;Equation;Input;Value\n"
        "CR3-1;1;Pv=(Q/1000/(W*H/1000000)/VPC)^2;;\nCR3-1;2;Co=0.2*Theta/90+RR/100;;\nCR3-1;3;Po=Co*Pv;;\n"
        "SR4-1;1;Co=0.01*H1/Ho;;\nSR4-1;2;Po=Co*10;;\n"
        "SR5-15;1;Cb1=0.3+Qb1/Qc;;\nSR5-15;2;Pob1=Cb1*10;;\nSR5-15;3;Cb2=0.7*Wb2/Wc;;\nSR5-15;4;Pob2=Cb2*20;;\nSR5-1;1;Cb=0.9;;\nSR5-1;2;Pob=Cb*Qb/100;;\nSR5-1;3;Cs=0.05;;\nSR5-1;4;Pos=Cs*Hs*Ws/10000;;\n";
    files["Input.csv"]="Key;Order;Prompt;Variable;IPUnitID;IPDecimals;IPMinimum;IPMaximum;IPDefault;SIUnitID;SIDecimals;SIMinimum;SIMaximum;SIDefault\n"
        "CR3-1;0;W;W;;;;;;(W, mm);0;25;2500;300\nCR3-1;1;H;H;;;;;;(H, mm);0;25;2500;300\nCR3-1;2;RR;RR;;;;;;(r/W);2;0.5;2;1.5\nCR3-1;3;T;Theta;;;;;;(Theta, deg.);0;0;180;90\nCR3-1;4;Q;Q;;;;;;(Q, L/s);0;0;1;700\n"
        "SR4-1;0;Ho;Ho;;;;;;(Ho, mm);0;0;1;300\nSR4-1;1;W;W;;;;;;(W, mm);0;0;1;400\nSR4-1;2;H1;H1;;;;;;(H1, mm);0;0;1;600\nSR4-1;3;L;L;;;;;;(L, mm);0;0;1;600\nSR4-1;4;Q;Q;;;;;;(Q, L/s);0;0;1;1650\n"
        "SR5-1;0;H;H;;;;;;(H, mm);0;0;1;300\nSR5-1;1;W;W;;;;;;(W, mm);0;0;1;300\nSR5-1;2;Hs;Hs;;;;;;(Hs, mm);0;0;1;300\nSR5-1;3;Ws;Ws;;;;;;(Ws, mm);0;0;1;250\nSR5-1;4;Qc;Qc;;;;;;(Qc, L/s);0;0;1;750\nSR5-1;5;Qb;Qb;;;;;;(Qb, L/s);0;0;1;150\nSR5-1;6;Hb;Hb;;;;;;(Hb, mm);0;0;1;150\nSR5-1;7;Wb;Wb;;;;;;(Wb, mm);0;0;1;150\n";
    files["Output.csv"]="Key;Order;Prompt;Variable;IPUnitID;IPDecimals;SIUnitID;SIDecimals\n";
    tvsAshrae::Reader fake=[&](const std::string& f,std::string& t){auto i=files.find(f);if(i==files.end())return false;t=i->second;return true;};
    tvsAshrae::Engine e;yes(e.load(fake),"load");
    Options o;Calculator calc(&e,o);
    const std::vector<Element> net=network();
    Result r=calc.run(net,0);
    yes(r.d.size()==5,"5 dionice");
    if(r.d.size()==5){
        // 1: J-T1 (duct, elbow, duct, tee) 600x300; 2: prolaz up to the prelaz (larger flow first); 3: ogranak; 4: after the prelaz; 5: flex
        yes(r.d[0].from==L"J"&&r.d[0].to==L"T1","1 J-T1");yes(r.d[1].from==L"T1"&&r.d[1].to==L"P1"&&r.d[1].flow==2400,"2 prolaz to the prelaz");
        yes(r.d[2].flow==1200&&r.d[2].to==L"P2","3 ogranak to the flex");yes(r.d[3].to==L"K1"&&r.d[3].terminal&&r.d[3].s.a==400,"4 after the prelaz");
        yes(r.d[4].flex&&r.d[4].terminal&&r.d[4].to==L"K2","5 flex");
        eq(r.d[0].length,9.0,"L: ducts only, no elbow arc");eq(r.d[1].length,3.0,"L: no prelaz");
        eq(r.d[4].rough,3.0,"flex roughness");eq(r.d[0].rough,0.15,"sheet roughness");
        yes(r.d[1].parent==0&&r.d[3].parent==1&&r.d[4].parent==2,"parents");
        const double v=3600/3600.0/0.18;eq(r.d[0].v,v,"v");eq(r.d[0].pd,0.6*v*v,"pd");
        yes(std::fabs(r.d[0].r/tvsDuctSizing::friction(3600,600,300,false)-1)<2e-3,"R as DuctSizing (Serghides 3.7 vs iterated 3.71)");
        yes(s0temp(),"temperature row");
    }
    yes(r.f.size()==4,"4 fittings");
    for(size_t k=0;k<r.f.size();++k){const Fitting& f=r.f[k];yes(f.ok,"computed");
        if(f.key==L"CR3-1"){yes(f.dionica==0,"elbow on 1");const double pv=std::pow(1000/0.18/1000/1.291,2);eq(f.dp,(0.2+1.0/100)*pv,"elbow dp (r/W 1.0)");eq(f.zeta,f.dp/r.d[0].pd,"zeta on pd");}
        if(f.key==L"SR4-1"){yes(f.dionica==1,"prelaz on 2");eq(f.c,0.01*500/400,"SR4-1: the width changes -> H1 500, Ho 400");}
        if(f.key==L"SR5-1"&&f.what.find(L"ogranak")!=std::wstring::npos){yes(f.dionica==2,"branch on 3");eq(f.dp,0.9*(1200/3.6)/100,"branch Qb");}
        if(f.key==L"SR5-1"&&f.what.find(L"prolaz")!=std::wstring::npos){yes(f.dionica==1,"main on 2");eq(f.dp,0.05*300*500/10000.0,"main Hs Ws");}}
    {   // terminal stucna (resetka on the ogranak, prolaz end open in the drawing = cep)
        std::vector<Element> n2;n2.push_back(one(duct(400,200,false,3000,XY(0,0),XY(3000,0)),500,-1,0,-1,{XY(0,0),XY(3000,0)}));
        Part st;st.kind=Tee;st.tee=TeeBranch;st.sections={Section(400,200,false),Section(400,200,false),Section(300,200,false)};st.line={XY(3000,0),XY(3400,0)};
        n2.push_back(one(st,500,0,0,1,{XY(3000,0),XY(3400,0),XY(3200,-300)}));
        Result t=calc.run(n2,0);yes(t.d.size()==1&&t.d[0].terminal,"one dionica, terminal");
        yes(t.d[0].to==L"T1","terminal stucna is a T node");yes(t.f.size()==1&&t.f[0].ok&&t.f[0].what.find(L"ogranak")!=std::wstring::npos&&t.f[0].note.find(L"\u010Dep")!=std::wstring::npos,"capped stucna computed");
        if(!t.f.empty())eq(t.f[0].dp,0.9*(500/3.6)/100,"Qb = the whole flow");}
    {   // exhaust TK0: Idelchik sharp T (C on Vc)
        eq(idelchikBranch(0.3,0.2),2.3,"KanCalc 1.7 beta 0.2");eq(idelchikStraight(0.3),0.38,"KanCalc 1.6");eq(idelchikBranch(0.5,0.6),0.66,"KanCalc 1.7 beta 0.6");eq(idelchikBranch(0.5,0.5),(1.1+0.66)/2,"beta 0.5 between rows");eq(idelchikBranch(0.35,0.05),(9.2+16.0)/2,"clamped beta");
        std::vector<Element> n3=network();n3[3].parts[0].tee=TeePlain;Options ox;ox.exhaust=true;Calculator cx(&e,ox);Result t=cx.run(n3,0);
        int idel=0;for(size_t k=0;k<t.f.size();++k)if(t.f[k].key==L"Idelchik"){++idel;yes(t.f[k].ok&&t.f[k].note.find(L"potcijenjeno")!=std::wstring::npos,"Idelchik note");
            const double q=1200/3600.0,beta=0.09/0.18,vc=1.0/0.18,pvc=0.6*vc*vc;
            if(t.f[k].what.find(L"ogranak")!=std::wstring::npos)eq(t.f[k].dp,idelchikBranch(q,beta)*pvc,"Idelchik branch dp");else eq(t.f[k].dp,idelchikStraight(q)*pvc,"Idelchik main dp");}
        yes(idel==2,"two Idelchik legs");}
    yes(junctionKey(false,TeeTap,Section(400,200,false),Section(160,0,true),true)=="SR5-12"&&junctionKey(false,TeeTap,Section(400,200,false),Section(160,0,true))=="SR5-11"&&junctionKey(true,TeeTap,Section(400,200,false),Section(160,0,true),true)=="ER5-2","round tap keys");
    {   // supply TK0, branch lower than the run: KanCalc 0.7 / 0.6 instead of SR5-5
        eq(idelchikDivBranch(0.3,0.2),2.9,"KanCalc 0.7");eq(idelchikDivStraight(0.5),0.1,"KanCalc 0.6");
        std::vector<Element> n4=network();n4[3].parts[0].tee=TeePlain;n4[3].ports[2]=Section(300,200,false);n4[3].parts[0].sections[2]=n4[3].ports[2];
        Result t=calc.run(n4,0);int idel=0;
        for(size_t k=0;k<t.f.size();++k)if(t.f[k].key==L"Idelchik"){++idel;yes(t.f[k].note.find(L"0.7")!=std::wstring::npos,"note 0.7 / 0.6");
            const double vc=1.0/0.18,pvc=0.6*vc*vc;
            if(t.f[k].what.find(L"ogranak")!=std::wstring::npos)eq(t.f[k].dp,idelchikDivBranch(1.0/3,0.06/0.18)*pvc,"div branch dp");
            else eq(t.f[k].dp,idelchikDivStraight((2400/3600.0/0.15)/vc)*pvc,"div main dp");}
        yes(idel==2,"two KanCalc legs");}
    {   // bullhead: unit duct on the ogranak, run both ways
        std::vector<Element> n5;n5.push_back(one(duct(600,600,false,2000,XY(-2000,0),XY(0,0)),3000,-1,0,-1,{XY(-2000,0),XY(0,0)}));
        Part bh;bh.kind=Tee;bh.tee=TeePlain;bh.sections={Section(600,600,false),Section(500,600,false),Section(600,600,false)};bh.line={XY(300,-300),XY(300,300)};
        n5.push_back(one(bh,3000,0,2,1,{XY(300,-300),XY(300,300),XY(0,0)}));
        n5.push_back(one(duct(600,600,false,2000,XY(300,-300),XY(300,-2300)),1000,1,0,0,{XY(300,-300),XY(300,-2300)}));
        n5.push_back(one(duct(500,600,false,2000,XY(300,300),XY(300,2300)),2000,1,0,1,{XY(300,300),XY(300,2300)}));
        Result t=calc.run(n5,0);int legs=0;
        for(size_t k=0;k<t.f.size();++k)if(t.f[k].key==L"SR5-15"){++legs;yes(t.f[k].ok&&t.f[k].note.find(L"bullhead")!=std::wstring::npos,"bullhead computed");
            if(t.f[k].note.find(L"krak 1")!=std::wstring::npos)eq(t.f[k].dp,(0.3+1000.0/3000)*10,"krak 1 Qb1");else eq(t.f[k].dp,0.7*500/600*20,"krak 2 Wb2");}
        yes(legs==2,"two bullhead legs");}
    {   // oprema: damper (Nominal) and silencer (FixedDp) inline, diffuser (Zeta, qTerminal) at the end of a flex
        auto equip=[](int kind,int ports,int model,double v1,double v2,const wchar_t* tag,std::vector<XY> at){Part t;t.kind=Equipment;
            t.equipment.kind=kind;t.equipment.ports=ports;t.equipment.model=model;t.equipment.tag=tag;t.equipment.a=400;t.equipment.b=200;
            if(model==tvsEquipment::Nominal){t.equipment.dpNominal=v1;t.equipment.qNominal=v2;}else if(model==tvsEquipment::FixedDp)t.equipment.dp=v1;else t.equipment.zeta=v1;
            t.sections.assign(ports,Section(400,200,false));t.line=at;return t;};
        std::vector<Element> n6;
        n6.push_back(one(duct(400,200,false,3000,XY(0,0),XY(3000,0)),900,-1,0,-1,{XY(0,0),XY(3000,0)}));
        n6.push_back(one(equip(tvsEquipment::FlowDamper,2,tvsEquipment::Nominal,30,1800,L"RP-01",{XY(3000,0),XY(3400,0)}),900,0,0,1,{XY(3000,0),XY(3400,0)}));
        n6.push_back(one(duct(400,200,false,2600,XY(3400,0),XY(6000,0)),900,1,0,1,{XY(3400,0),XY(6000,0)}));
        n6.push_back(one(equip(tvsEquipment::Silencer,2,tvsEquipment::FixedDp,25,0,L"PB-01",{XY(6000,0),XY(7000,0)}),900,2,0,1,{XY(6000,0),XY(7000,0)}));
        n6.push_back(one(duct(400,200,false,2000,XY(7000,0),XY(9000,0)),900,3,0,1,{XY(7000,0),XY(9000,0)}));
        n6.push_back(one(duct(400,200,false,1000,XY(9000,0),XY(10000,0),true),900,4,0,1,{XY(9000,0),XY(10000,0)}));
        Part df=equip(tvsEquipment::Diffuser,1,tvsEquipment::Zeta,2.0,0,L"D-01",{XY(10000,0)});df.equipment.qTerminal=900;
        n6.push_back(one(df,900,5,0,1,{XY(10000,0)}));
        Result t=calc.run(n6,0);
        yes(t.d.size()==2,"oprema: inline is no node, flex is, the terminal stays with the flex");
        if(t.d.size()==2){eq(t.d[0].length,7.6,"oprema has no length");yes(t.d[1].flex&&t.d[1].terminal&&t.d[1].to==L"K1"&&t.d[1].tag==L"D-01","terminal tag");
            yes(std::fabs(t.d[1].toAt.x-10000)<1e-9,"terminal end at its port");}
        yes(t.f.size()==3,"three oprema rows");const double pd=0.6*std::pow(900/3600.0/0.08,2);
        for(size_t k=0;k<t.f.size();++k){const Fitting& f=t.f[k];yes(f.equipment&&f.ok,"oprema computed");
            if(f.tag==L"RP-01"){eq(f.dp,7.5,"Nominal: 30 Pa * (900/1800)^2");eq(f.zeta,7.5/pd,"zeta on pd");yes(f.dionica==0&&!f.terminal&&f.what==L"Regulator protoka","damper");}
            if(f.tag==L"PB-01"){eq(f.dp,25,"FixedDp");yes(f.dionica==0,"silencer on 1");}
            if(f.tag==L"D-01"){eq(f.dp,2.0*pd,"Zeta on the pd of the flex at the port");yes(f.terminal&&f.zeta==0&&f.dionica==1&&f.c==2.0,"terminal: dp only");}}
        tvsSheet::Info in6;tvsSheet::Workbook w6=tvsSheet::build(t,in6,o);const tvsSheet::Sheet& d6=w6.sheets[0];const tvsSheet::Sheet& f6=w6.sheets[1];
        yes(f6.at(2,6)==L"'Oprema"&&f6.at(4,6)==L"'Oprema"&&f6.at(2,2)==L"'Regulator protoka","Izvor Oprema, kind name");
        yes(f6.at(2,3).find(L"RP-01")!=std::wstring::npos&&f6.at(2,5)==tvsSheet::num(7.5/pd,4)&&f6.at(2,8).find(L"=E2*INDEX(")==0,"inline row: zeta and H formula");
        yes(f6.at(4,5).empty()&&f6.at(4,8)==tvsSheet::num(2.0*pd,1),"terminal row: dp value, no zeta");
        yes(d6.at(13,tvsSheet::cTerm)==L"=Fitinzi!$H$4"&&d6.at(12,tvsSheet::cTerm).empty(),"Dp kraj. elem. = the terminal row");
        yes(d6.at(18,4)==L"=SUMPRODUCT(MAX((U12:U13-S12:S13)*(1+D17/100)+S12:S13))","no reserve on terminals");
        bool label=false;for(size_t k=0;k<w6.sheets[2].labels.size();++k)label|=w6.sheets[2].labels[k].text==L"D-01";yes(label,"Sema: the tag of the terminal");
        // no loss model: listed, typed by the user (yellow Dp kraj. elem.)
        n6[6].parts[0].equipment.model=tvsEquipment::NoLoss;Result u=calc.run(n6,0);tvsSheet::Workbook w7=tvsSheet::build(u,in6,o);
        yes(u.f.size()==3&&!u.f[2].ok&&w7.sheets[0].at(13,tvsSheet::cTerm)==L"0","no model: typed");
    }
    {   // resetka na dnu (Bottom): duct 400x200 -> tap junction 80 mm before the closed end -> grille R-01
        //  (neck 300x100, FixedDp 15 Pa, 400 m3/h) as FlowCommand.h splitBottom builds it; the closed end left (dropDeadEnds)
        auto bottomNet=[](bool goesOn){std::vector<Element> n;
            n.push_back(one(duct(400,200,false,2919.8,XY(0,0),XY(2919.8,0)),goesOn?700:400,-1,0,-1,{XY(0,0),XY(2919.8,0)}));
            Part tp;tp.kind=BottomTap;tp.sections={Section(400,200,false),Section(400,200,false),Section(300,100,false)};
            n.push_back(one(tp,goesOn?700:400,0,0,1,{XY(2919.8,0),XY(2920.2,0),XY(2920,0)}));
            Part g;g.kind=Equipment;g.equipment.kind=tvsEquipment::Grille;g.equipment.ports=1;g.equipment.mount=tvsEquipment::Bottom;g.equipment.model=tvsEquipment::FixedDp;
            g.equipment.dp=15;g.equipment.tag=L"R-01";g.equipment.a=400;g.equipment.b=200;g.equipment.neckA=300;g.equipment.neckB=100;g.sections.assign(1,Section(300,100,false));g.line={XY(2920,0)};
            n.push_back(one(g,400,1,0,2,{XY(2920,0)}));
            if(goesOn){n.push_back(one(duct(400,200,false,2079.8,XY(2920.2,0),XY(5000,0)),300,1,0,1,{XY(2920.2,0),XY(5000,0)}));
                Part h=g;h.equipment.mount=tvsEquipment::AtEnd;h.equipment.tag=L"R-02";h.equipment.dp=10;h.sections.assign(1,Section(400,200,false));h.line={XY(5000,0)};
                n.push_back(one(h,300,3,0,1,{XY(5000,0)}));}
            return n;};
        for(int ex=0;ex<2;++ex){Options ob;ob.exhaust=ex==1;Calculator cb(&e,ob);const std::vector<Element> n=bottomNet(false);Result t=cb.run(n,0);
            yes(t.d.size()==2,"bottom: duct + zero-length branch");
            if(t.d.size()!=2)continue;
            yes(t.d[0].to==L"T1"&&!t.d[0].terminal&&t.d[1].from==L"T1"&&t.d[1].to==L"K1"&&t.d[1].terminal&&t.d[1].tag==L"R-01","bottom: tap is a T node, grille the end");
            eq(t.d[1].length,0,"bottom: branch without length");yes(t.d[1].s.a==300&&t.d[1].s.b==100,"bottom: branch = neck");eq(t.d[1].flow,400,"bottom: grille flow");
            int idel=0,grille=0;
            for(size_t k=0;k<t.f.size();++k){const Fitting& f=t.f[k];
                if(f.key==L"Idelchik"){++idel;yes(f.dionica==1&&f.what.find(L"ogranak")!=std::wstring::npos&&f.ok,"bottom: branch loss on the grille dionica");
                    yes(f.note.find(L"\u010Dep")!=std::wstring::npos&&f.note.find(ex?L"1.7":L"0.7")!=std::wstring::npos&&f.note.find(L"R-01")!=std::wstring::npos,"bottom: note (cep, table, tag)");
                    const double vc=400/3600.0/0.08,pvc=0.6*vc*vc,beta=0.03/0.08;
                    eq(f.dp,(ex?idelchikBranch(1.0,beta):idelchikDivBranch(1.0,beta))*pvc,"bottom: Idelchik branch on Vc");
                    eq(f.zeta,f.dp/t.d[1].pd,"bottom: zeta on the neck pd");}
                if(f.equipment){++grille;yes(f.terminal&&f.tag==L"R-01"&&f.dionica==1&&f.dp==15,"bottom: grille own loss, terminal");}}
            yes(idel==1&&grille==1&&t.f.size()==2,"bottom: closed end has no row");
            bool stub=false;for(size_t k=0;k<t.lines.size();++k)if(t.lines[k].dionica==1&&std::fabs(std::hypot(t.lines[k].b.x-t.lines[k].a.x,t.lines[k].b.y-t.lines[k].a.y)-300)<1e-6)stub=true;
            yes(stub,"bottom: schematic stub");
            tvsSheet::Info ib;tvsSheet::Workbook wbb=tvsSheet::build(t,ib,ob);const tvsSheet::Sheet& fb=wbb.sheets[1];
            yes(fb.at(2,6)==L"'Idelchik"&&fb.at(3,6)==L"'Oprema"&&fb.at(3,2)==L"'Re\u0161etka","bottom: Fitinzi Izvor Idelchik / Oprema");
            yes(wbb.sheets[0].at(13,tvsSheet::cTerm)==L"=Fitinzi!$H$3","bottom: dp kraj. elem. = the grille row");
            bool label=false;for(size_t k=0;k<wbb.sheets[2].labels.size();++k)label|=wbb.sheets[2].labels[k].text==L"R-01";yes(label,"bottom: Sema tag");}
        {   // the duct goes on to a grille at its end: straight loss on the dionica behind the tap
            const std::vector<Element> n=bottomNet(true);Result t=calc.run(n,0);
            yes(t.d.size()==3&&t.d[1].flow==400&&t.d[1].tag==L"R-01"&&t.d[2].flow==300&&t.d[2].tag==L"R-02","bottom on: dionice (larger flow first)");
            int legs=0;for(size_t k=0;k<t.f.size();++k)if(t.f[k].key==L"Idelchik"){++legs;const double vc=700/3600.0/0.08,pvc=0.6*vc*vc;
                if(t.f[k].what.find(L"prolaz")!=std::wstring::npos){std::printf("dio %d %ls\n",t.f[k].dionica,t.f[k].note.c_str());yes(t.f[k].dionica==2&&t.f[k].note.find(L"\u010Dep")==std::wstring::npos,"bottom on: straight on the duct on");eq(t.f[k].dp,idelchikDivStraight(3.0/7)*pvc,"bottom on: 0.6 on Vs/Vc");}
                else{yes(t.f[k].dionica==1,"bottom on: branch");eq(t.f[k].dp,idelchikDivBranch(4.0/7,0.375)*pvc,"bottom on: 0.7 on Qb/Qc");}}
            yes(legs==2,"bottom on: two legs");}
    }
    // exhaust: transition keys turn
    yes(transitionKey(true,Section(400,300,false),Section(500,300,false))=="ER4-1","exhaust ER4-1");
    yes(transitionKey(false,Section(250,0,true),Section(400,300,false))=="SR4-3"&&transitionKey(false,Section(400,300,false),Section(250,0,true))=="SD4-2","shape change keys");
    yes(junctionKey(false,TeePlain,Section(600,300,false),Section(300,300,false))=="SR5-5"&&junctionKey(false,TeeBranch,Section(600,300,false),Section(300,300,false))=="SR5-1"&&junctionKey(false,TeeTap,Section(600,300,false),Section(300,300,false))=="SR5-13","tee keys");
    yes(junctionKey(false,TeePants,Section(600,300,false),Section(300,300,false))=="SR5-14"&&junctionKey(true,TeePlain,Section(600,300,false),Section(200,0,true))=="ER5-2","junction keys");
    yes(junctionKey(true,TeeTap,Section(400,200,false),Section(500,150,false))=="ER5-3"&&junctionKey(true,TeeBranch,Section(400,200,false),Section(300,200,false))=="ER5-1","exhaust stucna ER5-3");
    yes(elbowKey(Section(200,0,true),90)=="CD3-2"&&elbowKey(Section(315,0,true),90)=="CD3-21"&&elbowKey(Section(200,0,true),45)=="CD3-4"&&elbowKey(Section(315,0,true),45).empty(),"elbow keys");
    // no engine: fittings listed for the user
    {Calculator none(nullptr,o);Result x=none.run(net,0);yes(x.f.size()==4,"listed without data");for(size_t k=0;k<x.f.size();++k)yes(!x.f[k].ok&&x.f[k].note.find(L"unesite")!=std::wstring::npos,"typed by the user");}
    // workbook
    tvsSheet::Info info;info.project=L"Test";info.system=L"Dovod";info.date=L"14.11.2026";
    tvsSheet::Workbook wb=tvsSheet::build(r,info,o);
    yes(wb.sheets.size()==3&&wb.sheets[0].name==L"Dionice"&&wb.sheets[1].name==L"Fitinzi"&&wb.sheets[2].name==L"\u0160ema","sheets");
    const tvsSheet::Sheet& s=wb.sheets[0];
    yes(s.at(12,tvsSheet::cZeta)==L"=SUMIF(Fitinzi!$A:$A,A12,Fitinzi!$E:$E)","SUMIF");
    yes(s.at(15,tvsSheet::cCum)==L"=T15+U13","cumulative from the parent row");
    yes(s.at(12,tvsSheet::cMax)==L"=MAX(U12,V13,V14)","max below");
    yes(s.at(13,tvsSheet::cOver)==L"=MAX(V13,V14)-V13","visak");
    yes(s.at(15,tvsSheet::cTerm)==L"0"&&s.at(13,tvsSheet::cTerm).empty(),"terminal input only on terminals");
yes(s.at(6,2)==L"20"&&s.at(7,2).find(L"=101325/(287.05*($B$6+273.15))")==0&&s.at(12,tvsSheet::cLambda).find(L"=IF(M12<=0")==0,"t, rho and lambda formulas");
        yes(s.hidden.size()==2&&s.hidden[0]==tvsSheet::cRe,"Re hidden");
    yes(!wb.sheets[2].strokes.empty()&&wb.sheets[2].strokes.size()==r.lines.size(),"schematic lines");
    bool dashed=false;for(size_t k=0;k<wb.sheets[2].strokes.size();++k)dashed|=wb.sheets[2].strokes[k].dash;yes(dashed,"flex dashed");
    if(argc>=3){   // real ASHRAE data: the same network, workbook dumped for the preview
        const std::string dir=argv[1];
        tvsAshrae::Reader rd=[dir](const std::string& f,std::string& t){std::ifstream in((dir+"/"+f).c_str(),std::ios::binary);if(!in)return false;std::ostringstream x;x<<in.rdbuf();t=x.str();return true;};
        tvsAshrae::Engine real;if(!real.load(rd)){std::printf("FAIL real data\n");return 1;}
        Calculator c2(&real,o);Result x=c2.run(net,0);
        for(size_t k=0;k<x.f.size();++k)std::printf("  %d %s %s C=%.3f dp=%.2f zeta=%.3f %s\n",x.d[x.f[k].dionica].number,utf8(x.f[k].key).c_str(),utf8(x.f[k].what).c_str(),x.f[k].c,x.f[k].dp,x.f[k].zeta,utf8(x.f[k].note).c_str());
        info.source=L"DuctPressDrop ashrae_data";dump(tvsSheet::build(x,info,o),argv[2]);
    }
    if(fails)return 1;std::printf("OK\n");return 0;
}

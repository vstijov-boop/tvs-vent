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
        "SR5-1;1;Cb=0.9;;\nSR5-1;2;Pob=Cb*Qb/100;;\nSR5-1;3;Cs=0.05;;\nSR5-1;4;Pos=Cs*Hs*Ws/10000;;\n";
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
        yes(std::fabs(r.d[0].r-tvsDuctSizing::friction(3600,600,300,false))<1e-9,"R as DuctSizing");
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
    yes(s.at(11,tvsSheet::cZeta)==L"=SUMIF(Fitinzi!$A:$A,A11,Fitinzi!$E:$E)","SUMIF");
    yes(s.at(14,tvsSheet::cCum)==L"=T14+U12","cumulative from the parent row");
    yes(s.at(11,tvsSheet::cMax)==L"=MAX(U11,V12,V13)","max below");
    yes(s.at(12,tvsSheet::cOver)==L"=MAX(V12,V13)-V12","visak");
    yes(s.at(14,tvsSheet::cTerm)==L"0"&&s.at(12,tvsSheet::cTerm).empty(),"terminal input only on terminals");
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

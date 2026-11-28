#pragma once
// Pad pritiska po dionicama (TVS_PAD): the flow network of TVS_PROTOK (geometry/FlowNetwork.h)
// split into dionice, friction per dionica and the fittings with ASHRAE coefficients
// (geometry/AshraeEngine.h). Air in through a T komad's ogranak and out both run ends = bullhead
// (SR5-15 / ER5-5, SD5-19 / ED5-4). A free T komad port at the end of the network counts as capped (cep):
// a T komad / stucna with nothing on it is a terminal, the air leaves through its ogranak. Pure C++14, sizes in mm, flows in m3/h. tests/test_pressure_drop.cpp.
//   Dionica: a chain of elements with the same flow, section and duct kind; a new one starts at
//   the unit, after a T komad (also one with a capped port), after a prelaz (it ends the dionica before), at a
//   change between flex and rigid duct and where the section changes without a prelaz.
//   Numbered from the unit, level by level (larger flow first). Nodes: J the unit, T T komad,
//   P a change of section / kind, K a terminal end.
//   L: the drawn duct lengths only (rigid and flex; fittings are in their C, user 2026-11-14). R: Darcy-Weisbach, lambda Colebrook-White,
//   air rho 1.2 kg/m3, nu 15.1e-6 m2/s; k = 0.15 mm sheet metal, 3.0 mm flex (ASHRAE, extended).
//   Fittings (ASHRAE DFDB, user's choice 2026-11-14): elbow rectangular CR3-1 (W in the bend
//   plane = SizeA, r/W of the axis, angle); round 90 deg CD3-2 (D <= 250) / CD3-21, 45 deg CD3-4
//   (D <= 250), other round angles: no ASHRAE piece (zeta typed by the user). Prelaz: supply
//   SR4-1 (one side kept) / SR4-2 / SD4-1 / SR4-3 round->rect / SD4-2 rect->round; exhaust ER4-1 /
//   ER4-2 / ED4-1 / ED4-2 / ER4-3. T komad (common = the port towards the unit; branch = port 2):
//   supply TK0 SR5-5 (branch of another height: KanCalc 0.7 / 0.6, below), round stucna SR5-11 / conical SR5-12, TK-O SR5-1, stucna SR5-13 (user 2026-11-14), round branch SR5-11, TK-G SR5-14, round main SD5-9;
//   exhaust ER5-1 (TK-O), TK0 and stucna outside Ab/Ac 0.4..0.6 Idelchik (sharp T), stucna ER5-3, round branch ER5-2, TK-G ER5-4, round main ED5-3. The branch loss goes to the
//   dionica of the branch, the straight (main) loss to the dionica of the prolaz. Every loss is
//   given as zeta on the pd of its dionica (zeta = dp / pd), so Z = sum(zeta) * pd in the sheet.
//   Oprema (linked block, geometry/Equipment.h): inline (2 ports) is a local loss inside the dionica
//   like an elbow (no new dionica, no length), tvsEquipment::loss at the flow of the dionica and the
//   pd of the duct at its inlet port; terminal (1 port) ends the dionica, its loss is the "dp kraj.
//   elem." of that dionica (no zeta, no reserve) and its tag labels the end in the schematic.
//   Resetka na dnu (tvsEquipment::Bottom): the readers split the duct at the tap point; the tap
//   junction (part BottomTap, ports 0 / 1 the duct run, 2 the neck) is a node like a T komad, the
//   grille is a zero-length branch dionica (section = neck). Loss as the KanCalc / Idelchik sharp T:
//   supply diverging 0.7 / 0.6, exhaust converging 1.7 / 1.6 (branch = neck, Qb = the grille's flow,
//   straight = the duct on), C on Vc; the closed end beyond the last grille is the capped prolaz
//   (cep, no row). The grille's own loss is its terminal "dp kraj. elem."; in the schematic the
//   branch is a short stub across the duct (tapStub) with the grille's tag.
#include "AshraeEngine.h"
#include "AshraeKeys.h"   // Section, TeeType, elbowKey / transitionKey / junctionKey
#include "Equipment.h"
#include <cmath>
#include <cwchar>
#include <map>
#include <string>
#include <vector>
namespace tvsPressure {
struct XY {double x,y;XY():x(0),y(0){}XY(double a,double b):x(a),y(b){}};
enum PartKind {Other=0,Duct=1,Elbow=2,Transition=3,Tee=4,Equipment=5,BottomTap=6};   // BottomTap: resetka na dnu (see above)
struct Part {
    int kind;bool flex;
    double length;        // mm: duct, prelaz; elbow: the axis arc (computed when 0); BottomTap: the closed duct end beyond it (DPD export, 0 none)
    double angle,radius;  // elbow: deflection (deg), axis radius (mm)
    int tee;              // TeeType
    bool cone;            // stucna with a round conical branch (SR5-12)
    bool lateral;         // kosa stucna 45, round on round (SD5-1 / ED5-2)
    std::vector<Section> sections;   // per port (a single part: the element's ports)
    std::vector<XY> line;            // plan polyline for the schematic, mm
    tvsEquipment::Record equipment;  // kind Equipment: the record of the linked block
    Part():kind(Other),flex(false),length(0),angle(0),radius(0),tee(TeePlain),cone(false),lateral(false){}
};
struct Element {
    std::vector<Part> parts;          // one, or the members of an etaz / obilazak
    std::vector<Section> ports;       // section per port of the element
    std::vector<XY> at;               // port positions (plan, mm)
    double flow;int parent,entry,via; // FlowNetwork: parent, own port to the unit, parent's port
    Element():flow(0),parent(-1),entry(-1),via(-1){}
};
struct Options {bool exhaust;double rough,flexRough,rho,nu,temperature;Options():exhaust(false),rough(0.15),flexRough(3.0),rho(1.2),nu(15.1e-6),temperature(20){}};   // temperature: the Excel default
struct Fitting {
    int dionica,element;
    std::wstring what,note,dim,key;   // Element, opis / napomena, dimenzija, ASHRAE key ("" none)
    double c,dp,zeta;                 // ASHRAE C (its own reference), dp Pa, zeta on the dionica pd
    bool ok;                          // computed (else the user types zeta)
    bool equipment,terminal;          // oprema; terminal: dp goes to "dp kraj. elem." (zeta 0)
    std::wstring tag;                 // oprema: its tag
    Fitting():dionica(-1),element(-1),c(0),dp(0),zeta(0),ok(false),equipment(false),terminal(false){}
};
struct Dionica {
    int number,parent;std::vector<int> children,elements;
    std::wstring from,to,tag;XY fromAt,toAt;   // tag: the terminal oprema at the end (schematic label)
    double flow;Section s;bool flex,terminal;
    double length,rough,dh,v,pd,re,lambda,r;   // length m, rough mm, dh mm, r Pa/m
    Dionica():number(0),parent(-1),flow(0),flex(false),terminal(false),length(0),rough(0),dh(0),v(0),pd(0),re(0),lambda(0),r(0){}
};
struct Line {XY a,b;int dionica;bool flex;};
struct Result {std::vector<Dionica> d;std::vector<Fitting> f;std::vector<Line> lines;std::vector<std::wstring> warnings;};
inline bool same(const Section& x,const Section& y){return x.round==y.round&&std::fabs(x.a-y.a)<0.5&&(x.round||std::fabs(x.b-y.b)<0.5);}
inline std::wstring dimText(const Section& s){wchar_t t[48];if(s.round)swprintf(t,48,L"\u00D8%.0f",s.a);else swprintf(t,48,L"%.0fx%.0f",s.a,s.b);return t;}
inline double area(const Section& s){const double pi=3.14159265358979323846;return s.round?pi*s.a*s.a/4e6:s.a*s.b/1e6;}
inline double hydraulic(const Section& s){return s.round?s.a:(s.a+s.b>0?2*s.a*s.b/(s.a+s.b):0);}
// lambda: laminar 64/Re, else Colebrook-White (Serghides explicit; within 1e-4 of the iterated tvsDuctSizing::friction)
inline double lambdaOf(double re,double roughMm,double dhMm){
    if(!(re>0)||!(dhMm>0))return 0;if(re<2300)return 64/re;
    // Colebrook-White by Serghides (1984), the same expression as the sheet's lambda formula
    const double e=roughMm/(3.7*dhMm);
    const double A=-2*std::log10(e+12/re),B=-2*std::log10(e+2.51*A/re),C=-2*std::log10(e+2.51*B/re);
    const double x=A-(B-A)*(B-A)/(C-2*B+A);return 1/(x*x);
}
inline void hydraulics(Dionica& d,const Options& o){
    d.rough=d.flex?o.flexRough:o.rough;d.dh=hydraulic(d.s);const double s=area(d.s);
    d.v=s>0?std::fabs(d.flow)/3600.0/s:0;d.pd=o.rho*d.v*d.v/2;d.re=d.v*d.dh/1000.0/o.nu;
    d.lambda=lambdaOf(d.re,d.rough,d.dh);d.r=d.dh>0?d.lambda/(d.dh/1000.0)*d.pd:0;
}
// ---- ASHRAE inputs (as DuctPressDrop services/ashrae/seeding.py) ----
inline double dimOf(const Section& s,char d){if(d=='D')return s.round?s.a:0;if(d=='W')return s.round?0:s.a;if(d=='H')return s.round?0:s.b;return 0;}
inline std::string lower(std::string s){for(size_t i=0;i<s.size();++i)if(s[i]>='A'&&s[i]<='Z')s[i]=static_cast<char>(s[i]-'A'+'a');return s;}
// elbow / prelaz: suffix "" "1" "c" "i" "s" "in" "a" = upstream, "o" "0" "2" "out" = downstream; a
// side without that dimension (other shape) gives it from the other side
inline std::map<std::string,double> seed(const std::vector<tvsAshrae::Param>& ps,const Section& up,const Section* down,double flowLs,double lengthMm){
    std::map<std::string,double> in;
    for(size_t i=0;i<ps.size();++i){const tvsAshrae::Param& p=ps[i];double v=p.def;
        if(tvsAshrae::isFlow(p)){if(flowLs>0)v=flowLs;in[p.var]=v;continue;}
        const char first=p.var.empty()?0:static_cast<char>(p.var[0]>='a'&&p.var[0]<='z'?p.var[0]-'a'+'A':p.var[0]);
        const std::string suf=lower(p.var.substr(1));
        if(first=='L'&&suf.empty()&&lengthMm>0){in[p.var]=lengthMm;continue;}
        if(first=='D'||first=='W'||first=='H'){
            const bool next=suf=="o"||suf=="0"||suf=="2"||suf=="out",here=suf.empty()||suf=="1"||suf=="c"||suf=="i"||suf=="s"||suf=="in"||suf=="a";
            if(next||here){const Section* side=next&&down?down:&up;const Section* other=side==&up?down:&up;
                double x=dimOf(*side,first);if(!(x>0)&&other)x=dimOf(*other,first);if(x>0)v=x;}}
        in[p.var]=v;}
    return in;
}
// junction: suffix c = common, b = branch, s = straight, none = common (fill_junction_inputs)
inline std::map<std::string,double> seedJunction(const std::vector<tvsAshrae::Param>& ps,const Section& c,const Section& b,const Section& s,double qc,double qb,double qs){
    std::map<std::string,double> in;
    for(size_t i=0;i<ps.size();++i){const tvsAshrae::Param& p=ps[i];double v=p.def;
        const char role=p.var.size()>1?static_cast<char>(lower(p.var.substr(1,1))[0]):'c';
        const Section& sec=role=='b'?b:role=='s'?s:c;const double q=role=='b'?qb:role=='s'?qs:qc;
        if(tvsAshrae::isFlow(p))v=q>0?q/3.6:0;   // a closed leg is Q = 0, not the default
        else{const char first=p.var[0];const double x=dimOf(sec,first);if(x>0)v=x;}
        in[p.var]=v;}
    return in;
}
// Idelchik, converging sharp 90 deg T, As = Ac (Handbook of Hydraulic Resistance, ch. 7; C on Vc),
// tabulated as in the user's KanCalc base (hanKanCalc.mdb, LOCalc 1.7 branch / 1.6 straight; user
// 2026-11-15: the table, not the approximation K[1 + (q/beta)^2 - 2(1-q)^2]). q = Qb/Qc, beta = Ab/Ac;
// bilinear, clamped to the table (beta 0.1..1.0, q 0..1).
inline double idelchikLerp(const double* x,int n,double v,int& i,double& t){
    if(v<=x[0]){i=0;t=0;return 0;}if(v>=x[n-1]){i=n-2;t=1;return 0;}
    i=0;while(i<n-2&&v>x[i+1])++i;t=(v-x[i])/(x[i+1]-x[i]);return 0;
}
inline double idelchikBranch(double q,double beta){
    static const double B[7]={0.1,0.2,0.3,0.4,0.6,0.8,1.0};
    static const double Q[11]={0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};
    static const double C[7][11]={
        {-1.0,0.4,3.8,9.2,16.0,26.0,37.0,43.0,65.0,82.0,101.0},
        {-1.0,-0.37,0.72,2.3,4.3,6.8,9.7,13.0,17.0,21.0,26.0},
        {-1.0,-0.51,0.17,1.0,2.1,3.2,4.7,6.3,7.9,9.7,12.0},
        {-0.9,-0.46,-0.02,0.44,0.94,1.1,1.6,2.1,2.7,3.4,4.0},
        {-0.9,-0.5,-0.14,0.21,0.54,0.66,0.92,1.2,1.5,1.8,2.1},
        {-0.9,-0.51,-0.18,0.11,0.4,0.49,0.69,0.88,1.1,1.2,1.4},
        {-0.9,-0.52,-0.24,-0.08,0.32,0.42,0.57,0.72,0.86,0.99,1.1}};
    int i,j;double u,v;idelchikLerp(B,7,beta,i,u);idelchikLerp(Q,11,q,j,v);
    return C[i][j]*(1-u)*(1-v)+C[i][j+1]*(1-u)*v+C[i+1][j]*u*(1-v)+C[i+1][j+1]*u*v;
}
inline double idelchikStraight(double q){
    static const double Q[11]={0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0};
    static const double C[11]={0.0,0.16,0.27,0.38,0.46,0.53,0.57,0.59,0.6,0.59,0.55};
    int j;double v;idelchikLerp(Q,11,q,j,v);return C[j]*(1-v)+C[j+1]*v;
}
// Idelchik, diverging sharp 90 deg T (KanCalc LOCalc 0.7 branch: beta = Ab/Ac 0.1..0.8, q = Qb/Qc
// 0.1..0.9; 0.6 straight: Vs/Vc 0..1); C on Vc, bilinear, clamped.
inline double idelchikDivBranch(double q,double beta){
    static const double B[8]={0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8};
    static const double Q[9]={0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9};
    static const double C[8][9]={
        {2.1,4.8,8.9,14.0,14.0,14.0,14.0,14.0,14.0},{1.3,1.9,2.9,4.1,6.2,8.5,11.0,11.0,11.0},
        {1.1,1.4,1.8,2.3,3.2,4.3,5.5,6.9,8.5},{0.99,1.1,1.3,1.5,1.7,2.0,2.4,3.0,3.6},
        {0.97,1.0,1.1,1.2,1.4,1.5,1.8,2.1,2.5},{0.96,0.97,1.0,1.1,1.1,1.2,1.4,1.7,2.0},
        {0.95,0.94,0.95,0.98,1.0,1.1,1.2,1.4,1.9},{0.95,0.92,0.92,0.93,0.94,0.95,1.1,1.2,1.4}};
    int i,j;double u,v;idelchikLerp(B,8,beta,i,u);idelchikLerp(Q,9,q,j,v);
    return C[i][j]*(1-u)*(1-v)+C[i][j+1]*(1-u)*v+C[i+1][j]*u*(1-v)+C[i+1][j+1]*u*v;
}
inline double idelchikDivStraight(double vsvc){
    static const double X[9]={0,0.1,0.2,0.3,0.4,0.5,0.6,0.8,1.0};
    static const double C[9]={0.4,0.32,0.26,0.2,0.14,0.1,0.06,0.02,0.0};
    int j;double v;idelchikLerp(X,9,vsvc,j,v);return C[j]*(1-v)+C[j+1]*v;
}
inline std::wstring wide(const std::string& s){return std::wstring(s.begin(),s.end());}
inline std::wstring notes(const std::vector<std::string>& w){   // engine warnings are UTF-8
    std::wstring o;
    for(size_t k=0;k<w.size();++k){if(!o.empty())o+=L"; ";const std::string& s=w[k];
        for(size_t i=0;i<s.size();){const unsigned char c=static_cast<unsigned char>(s[i]);unsigned cp=c;int n=0;
            if(c>=0xF0){cp=c&7;n=3;}else if(c>=0xE0){cp=c&15;n=2;}else if(c>=0xC0){cp=c&31;n=1;}
            ++i;for(int j=0;j<n&&i<s.size();++j,++i)cp=(cp<<6)|(static_cast<unsigned char>(s[i])&63);
            o+=static_cast<wchar_t>(cp<0x10000?cp:0xFFFD);}}
    return o;
}
class Calculator {
public:
    Calculator(tvsAshrae::Engine* engine,const Options& o):eng_(engine),o_(o){}
    // elements: the reached ones carry parent / entry / via; root: the element at the unit
    Result run(const std::vector<Element>& e,int root){
        Result r;const int n=static_cast<int>(e.size());if(root<0||root>=n)return r;
        std::vector<std::vector<int> > kids(n);
        for(int i=0;i<n;++i)if(e[i].parent>=0&&e[i].parent<n)kids[e[i].parent].push_back(i);
        for(int i=0;i<n;++i){std::vector<int>& k=kids[i];   // larger flow first, then index
            for(size_t a=1;a<k.size();++a){const int x=k[a];size_t j=a;while(j>0&&(e[k[j-1]].flow<e[x].flow-1e-9)){k[j]=k[j-1];--j;}k[j]=x;}}
        // dionice (temporary ids), walking from the unit
        std::vector<int> of(n,-1);std::vector<Dionica> d;std::vector<int> queue(1,root);
        d.push_back(Dionica());d[0].elements.push_back(root);of[root]=0;
        for(size_t q=0;q<queue.size();++q){const int p=queue[q];
            for(size_t k=0;k<kids[p].size();++k){const int c=kids[p][k];queue.push_back(c);
                if(startsNew(e,kids,p,c)){Dionica x;x.parent=of[p];x.elements.push_back(c);of[c]=static_cast<int>(d.size());d[of[p]].children.push_back(of[c]);d.push_back(x);}
                else{of[c]=of[p];d[of[p]].elements.push_back(c);}}}
        // numbers: level by level, larger flow first (children are already in that order)
        std::vector<int> number(d.size(),0),walk(1,0);
        for(size_t q=0;q<walk.size();++q)for(size_t k=0;k<d[walk[q]].children.size();++k)walk.push_back(d[walk[q]].children[k]);
        for(size_t q=0;q<walk.size();++q)number[walk[q]]=static_cast<int>(q);
        r.d.resize(d.size());
        for(size_t q=0;q<d.size();++q){Dionica x=d[q];x.number=number[q]+1;x.parent=x.parent>=0?number[x.parent]:-1;
            for(size_t k=0;k<x.children.size();++k)x.children[k]=number[x.children[k]];r.d[number[q]]=x;}
        for(int i=0;i<n;++i)if(of[i]>=0)of[i]=number[of[i]];
        for(size_t q=0;q<r.d.size();++q)describe(e,kids,r.d[q]);
        nodes(e,kids,r);
        for(size_t q=0;q<r.d.size();++q)fittings(e,kids,of,r,static_cast<int>(q));
        for(int i=0;i<n;++i){if(of[i]<0)continue;for(size_t p=0;p<e[i].parts.size();++p){const Part& t=e[i].parts[p];
            for(size_t k=1;k<t.line.size();++k){Line l;l.a=t.line[k-1];l.b=t.line[k];l.dionica=of[i];l.flex=t.flex;r.lines.push_back(l);}}}
        for(size_t q=0;q<r.d.size();++q)if(bottomBranch(e,r.d[q])){Line l;l.a=r.d[q].fromAt;l.b=r.d[q].toAt;l.dionica=static_cast<int>(q);l.flex=false;r.lines.push_back(l);}
        return r;
    }
    static double tapStub(){return 300;}   // mm: schematic stub of a resetka na dnu (plan, across the duct)
private:
    tvsAshrae::Engine* eng_;Options o_;
    static bool flexOf(const Element& x){return !x.parts.empty()&&x.parts[0].flex;}
    static bool isTransition(const Element& x){return x.parts.size()==1&&x.parts[0].kind==Transition;}
    static bool isTee(const Element& x){return x.parts.size()==1&&x.parts[0].kind==Tee;}
    static bool isEquipment(const Element& x){return x.parts.size()==1&&x.parts[0].kind==Equipment;}
    static bool isTap(const Element& x){return x.parts.size()==1&&x.parts[0].kind==BottomTap&&x.ports.size()==3&&x.at.size()==3;}
    // the branch dionica of a resetka na dnu: the grille alone, right behind its tap junction
    static bool bottomBranch(const std::vector<Element>& e,const Dionica& d){
        if(d.elements.size()!=1)return false;const Element& x=e[d.elements[0]];
        return isEquipment(x)&&x.parent>=0&&x.parent<static_cast<int>(e.size())&&isTap(e[x.parent])&&x.via==2;
    }
    static const Section& port(const Element& x,int p){static const Section none;return p>=0&&p<static_cast<int>(x.ports.size())?x.ports[p]:none;}
    static bool startsNew(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,int p,int c){
        if(kids[p].size()>1||isTee(e[p])||isTap(e[p])||isTransition(e[p]))return true;   // a T komad is always a node, also with one free port (cep); so is a resetka na dnu
        if(isEquipment(e[c]))return false;   // oprema: a local loss inside the dionica (like an elbow)
        // after oprema: compare with the element before it (flex / section of the duct it sits in)
        int up=p,via=e[c].via;const int n=static_cast<int>(e.size());
        for(int guard=0;guard<n&&isEquipment(e[up])&&e[up].parent>=0&&e[up].parent<n;++guard){via=e[up].via;up=e[up].parent;}
        if(flexOf(e[up])!=flexOf(e[c]))return true;
        const Section& a=port(e[up],via);const Section& b=port(e[c],e[c].entry);
        return a.a>0&&b.a>0&&!same(a,b);
    }
    void describe(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,Dionica& d)const{
        const Element& first=e[d.elements[0]];d.flow=first.flow;d.flex=flexOf(first);
        bool found=false;double mm=0;
        for(size_t k=0;k<d.elements.size();++k)for(size_t p=0;p<e[d.elements[k]].parts.size();++p){const Part& t=e[d.elements[k]].parts[p];
            if(t.kind==Duct){mm+=t.length;if(!found&&!t.sections.empty()){d.s=t.sections[0];found=true;}}
}
        if(!found)d.s=port(first,first.entry);
        d.length=mm/1000.0;d.terminal=kids[d.elements.back()].empty();
        hydraulics(d,o_);
    }
    void nodes(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,Result& r)const{
        int t=0,p=0,k=0;
        for(size_t q=0;q<r.d.size();++q){Dionica& d=r.d[q];const int last=d.elements.back();const Element& x=e[last];wchar_t name[16];
            if(d.children.empty()&&!isTee(x)&&!isTap(x)){swprintf(name,16,L"K%d",++k);for(size_t j=0;j<x.at.size();++j)if(static_cast<int>(j)!=x.entry){d.toAt=x.at[j];break;}
                if(x.at.size()==1)d.toAt=x.at[0];   // terminal oprema: its one port
                if(isEquipment(x))d.tag=x.parts[0].equipment.tag;
                if(bottomBranch(e,d)){const Element& t=e[x.parent];   // stub across the duct from the tap point
                    double ux=t.at[1].x-t.at[0].x,uy=t.at[1].y-t.at[0].y;const double l=std::hypot(ux,uy);if(l>0){ux/=l;uy/=l;}else{ux=1;uy=0;}
                    XY c;for(size_t j=0;j<t.at.size();++j){c.x+=t.at[j].x/t.at.size();c.y+=t.at[j].y/t.at.size();}
                    d.toAt=XY(c.x+uy*tapStub(),c.y-ux*tapStub());}}
            else if(kids[last].size()>1||isTee(x)||isTap(x)){swprintf(name,16,L"T%d",++t);XY c;for(size_t j=0;j<x.at.size();++j){c.x+=x.at[j].x/x.at.size();c.y+=x.at[j].y/x.at.size();}d.toAt=c;}
            else{swprintf(name,16,L"P%d",++p);const Element& y=e[kids[last][0]];if(y.entry>=0&&y.entry<static_cast<int>(y.at.size()))d.toAt=y.at[y.entry];}
            d.to=name;}
        for(size_t q=0;q<r.d.size();++q){Dionica& d=r.d[q];
            if(d.parent<0){d.from=L"J";const Element& x=e[d.elements[0]];if(x.entry>=0&&x.entry<static_cast<int>(x.at.size()))d.fromAt=x.at[x.entry];}
            else{d.from=r.d[d.parent].to;d.fromAt=r.d[d.parent].toAt;}}
    }
    void add(Result& r,int dionica,int element,const std::wstring& what,const std::wstring& note,const Section& s,const std::string& key,std::map<std::string,double> in,bool main,double pvOwn,bool capped=false){
        Fitting f;f.dionica=dionica;f.element=element;f.what=what;f.note=note;f.dim=dimText(s);f.key=wide(key);
        if(key.empty()){f.note+=note.empty()?L"nema ASHRAE komada \u2013 unesite \u03B6":L"; nema ASHRAE komada \u2013 unesite \u03B6";r.f.push_back(f);return;}
        if(!eng_||!eng_->loaded()||!eng_->has(key)){f.note+=(f.note.empty()?L"":L"; ")+std::wstring(L"ASHRAE podaci nisu u\u010Ditani \u2013 unesite \u03B6");r.f.push_back(f);return;}
        std::vector<std::string> w;tvsAshrae::Engine::Loss l=eng_->loss(key,in,main,&w);
        if(capped){   // the closed leg has Q = 0: below the first table column (0.01), not a real warning
            std::vector<std::string> keep;for(size_t k=0;k<w.size();++k)if(w[k].find("=0 van")==std::string::npos)keep.push_back(w[k]);w.swap(keep);
            if(!std::isfinite(l.c)||std::fabs(l.c)>20){   // some tables (ED5-3) are unusable at Qb/Qc = 1: take 0.9
                double top=0;const std::vector<tvsAshrae::Param>& ps=eng_->params(key);
                for(size_t k=0;k<ps.size();++k)if(tvsAshrae::isFlow(ps[k])&&in.count(ps[k].var)&&in[ps[k].var]>top)top=in[ps[k].var];
                for(size_t k=0;k<ps.size();++k)if(tvsAshrae::isFlow(ps[k])&&in.count(ps[k].var)&&!(in[ps[k].var]>0))in[ps[k].var]=top/9;
                w.clear();l=eng_->loss(key,in,main,&w);f.note+=L"; ASHRAE tabela na Qb/Qc = 1 nije upotrebljiva \u2013 uzeto Qb/Qc = 0,9";}}
        if(key=="ER5-2"||key=="ER5-3"){   // tables for one geometry only: Ab / Ac = 0.5
            const double ac=(in.count("H")?in.at("H"):0)*(in.count("W")?in.at("W"):0);
            const double ab=in.count("Db")?3.14159265358979323846*in.at("Db")*in.at("Db")/4:(in.count("Hb")?in.at("Hb"):0)*(in.count("Wb")?in.at("Wb"):0);
            if(ac>0&&ab>0&&std::fabs(ab/ac/0.5-1)>0.25){wchar_t t[120];swprintf(t,120,L" va\u017Ei za Ab/Ac = 0,5, ovdje %.2f \u2013 C nije pouzdan",ab/ac);f.note+=(f.note.empty()?L"":L"; ")+wide(key)+std::wstring(t);}}
        f.c=l.c;f.dp=pvOwn>0?l.c*pvOwn:l.dp;f.ok=std::isfinite(f.dp);
        const double pd=r.d[dionica].pd;f.zeta=pd>0&&f.ok?f.dp/pd:0;
        const std::wstring extra=notes(w);if(!extra.empty())f.note+=(f.note.empty()?L"":L"; ")+extra;
        r.f.push_back(f);
    }
    void fittings(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,const std::vector<int>& of,Result& r,int q){
        const Dionica& d=r.d[q];
        for(size_t k=0;k<d.elements.size();++k){const int i=d.elements[k];const Element& x=e[i];
            for(size_t p=0;p<x.parts.size();++p){const Part& t=x.parts[p];
                if(t.kind==Elbow&&!t.sections.empty()&&t.angle>=1){const Section& s=t.sections[0];wchar_t note[64];
                    const double rc=t.radius,rr=s.a>0?rc/s.a:0;
                    if(s.round)swprintf(note,64,L"%.0f\u00B0, r/D = %.2f",t.angle,rr);else swprintf(note,64,L"%.0f\u00B0, r/W = %.2f",t.angle,rr);for(wchar_t* c=note;*c;++c)if(*c==L'.')*c=L',';
                    const std::string key=elbowKey(s,t.angle);std::map<std::string,double> in;
                    if(!key.empty()&&eng_&&eng_->has(key)){in=seed(eng_->params(key),s,nullptr,x.flow/3.6,0);if(in.count("RR"))in["RR"]=rr;if(in.count("Theta"))in["Theta"]=t.angle;}
                    add(r,q,i,t.flex?L"Fleks koljeno":L"Koljeno",note,s,key,in,false,0);}
                else if(t.kind==Transition&&x.ports.size()==2&&x.parts.size()==1){
                    const int up=o_.exhaust?1-x.entry:x.entry;const Section& a=port(x,up);const Section& b=port(x,1-up);
                    const std::string key=transitionKey(o_.exhaust,a,b);std::map<std::string,double> in;
                    // SR4-1 / ER4-1: the changing side is H
                    Section a2=a,b2=b;if((key=="SR4-1"||key=="ER4-1")&&std::fabs(a.b-b.b)<0.5&&std::fabs(a.a-b.a)>=0.5){a2=Section(a.b,a.a,false);b2=Section(b.b,b.a,false);}
                    if(eng_&&eng_->has(key))in=seed(eng_->params(key),a2,&b2,x.flow/3.6,t.length);
                    add(r,q,i,L"Prelaz",dimText(a)+L" \u2192 "+dimText(b),a,key,in,false,0);}
                else if(t.kind==Tee&&x.parts.size()==1)junction(e,kids,of,r,i);
                else if(t.kind==Equipment&&x.parts.size()==1)equipment(e,r,q,i);
                else if(t.kind==BottomTap&&isTap(x))bottomTap(e,kids,of,r,i);
                else if(t.kind==Other)r.warnings.push_back(L"element bez prora\u010Duna (dionica "+std::to_wstring(d.number)+L")");}}
    }
    // Oprema: tvsEquipment::loss at the flow of the element and the pd of the duct at its inlet (the
    // parent's port; else the record section). Inline: zeta = dp / pd of the dionica; terminal: dp
    // only (the sheet puts it in "dp kraj. elem."). No loss model: listed, the user types it.
    void equipment(const std::vector<Element>& e,Result& r,int q,int i){
        const Element& x=e[i];const tvsEquipment::Record& rec=x.parts[0].equipment;
        Section s=port(x,x.entry);
        if(x.parent>=0&&x.parent<static_cast<int>(e.size())){const Section& up=port(e[x.parent],x.via);if(up.a>0)s=up;}
        if(!(s.a>0)&&rec.a>0)s=Section(rec.a,rec.round?0:rec.b,rec.round);
        const double ar=area(s),v=ar>0?std::fabs(x.flow)/3600.0/ar:0,pv=o_.rho*v*v/2;
        Fitting f;f.dionica=q;f.element=i;f.equipment=true;f.terminal=rec.terminal();f.tag=rec.tag;
        f.what=tvsEquipment::kindName(rec.kind);f.dim=dimText(s);f.note=rec.tag;
        auto more=[&](const std::wstring& t){if(!f.note.empty())f.note+=L", ";f.note+=t;};
        if(!rec.note.empty())more(rec.note);
        wchar_t t[160];
        switch(rec.model){
        case tvsEquipment::FixedDp:swprintf(t,160,L"\u0394p = %.1f Pa (zadato)",rec.dp);break;
        case tvsEquipment::Zeta:swprintf(t,160,L"\u03B6 = %.2f na pd priklju\u010Dka (%.1f Pa)",rec.zeta,pv);break;
        case tvsEquipment::Nominal:swprintf(t,160,L"\u0394p = %.1f Pa pri %.0f m\u00B3/h \u00B7 (Q/Qn)\u00B2",rec.dpNominal,rec.qNominal);break;
        case tvsEquipment::Curve:
            if(rec.curve.empty())t[0]=0;
            else swprintf(t,160,tvsEquipment::curveOutside(rec,std::fabs(x.flow))?L"kriva Q\u2013\u0394p (%d ta\u010D.), Q van opsega %.0f\u2013%.0f m\u00B3/h \u2013 produ\u017Eeno po Q\u00B2":L"kriva Q\u2013\u0394p (%d ta\u010D., %.0f\u2013%.0f m\u00B3/h)",
                     static_cast<int>(rec.curve.size()),rec.curve.front().first,rec.curve.back().first);
            break;
        default:t[0]=0;break;}
        for(wchar_t* c=t;*c;++c)if(*c==L'.')*c=L',';
        if(t[0])more(t);else more(f.terminal?L"model gubitka nije zadat \u2013 unesite \u0394p kraj. elem.":L"model gubitka nije zadat \u2013 unesite \u03B6");
        if(rec.model==tvsEquipment::Zeta)f.c=rec.zeta;
        f.dp=tvsEquipment::loss(rec,x.flow,pv);
        const double pd=r.d[q].pd;
        if(rec.model<tvsEquipment::FixedDp||rec.model>tvsEquipment::Curve||(rec.model==tvsEquipment::Curve&&rec.curve.empty()))f.ok=false;
        else if(f.terminal)f.ok=true;
        else if(pd>0){f.ok=true;f.zeta=f.dp/pd;}
        else more(L"Q = 0 \u2013 unesite \u03B6");
        if(f.terminal&&f.ok)more(L"u \u0394p kraj. elem. (bez rezerve)");
        r.f.push_back(f);
    }
    // Resetka na dnu: the tap junction (ports 0 / 1 the duct, 2 the neck); common = the run port
    // towards the unit. KanCalc / Idelchik sharp T on Vc of the common duct: supply diverging
    // (0.7 branch on Qb/Qc and Ab/Ac, 0.6 straight on Vs/Vc), exhaust converging (1.7 / 1.6). The
    // branch loss goes to the grille's dionica, the straight loss to the dionica of the duct on; no
    // duct on (closed end) = capped prolaz, no row.
    void bottomTap(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,const std::vector<int>& of,Result& r,int i){
        const Element& x=e[i];
        double q[3]={0,0,0};int child[3]={-1,-1,-1};
        for(size_t k=0;k<kids[i].size();++k){const int c=kids[i][k];if(e[c].via>=0&&e[c].via<3){q[e[c].via]=e[c].flow;child[e[c].via]=c;}}
        if(x.entry!=0&&x.entry!=1){   // the unit behind the grille
            for(size_t k=0;k<kids[i].size();++k){const int c=kids[i][k];
                add(r,of[c],i,L"Re\u0161etka na dnu",L"ulaz kroz re\u0161etku \u2013 nestandardno",port(x,e[c].via),std::string(),std::map<std::string,double>(),false,0);}
            return;}
        const int straight=1-x.entry,branch=2;
        const Section& common=port(x,x.entry);const Section& br=port(x,branch);
        const double ac=area(common),beta=ac>0?area(br)/ac:0;
        const double vc=ac>0?x.flow/3600.0/ac:0,pvc=o_.rho*vc*vc/2,q2=x.flow>0?q[branch]/x.flow:0;
        const double as=area(port(x,straight)),vs=as>0?q[straight]/3600.0/as:0,vsvc=vc>0?vs/vc:0;
        const std::wstring capped=child[straight]<0?L", kraj kanala zatvoren (\u010Dep)":L"";
        std::wstring tag;if(child[branch]>=0&&isEquipment(e[child[branch]]))tag=e[child[branch]].parts[0].equipment.tag;
        const int legs[2]={branch,straight};
        for(int k=0;k<2;++k){const int leg=legs[k],c=child[leg];if(c<0)continue;const bool main=leg==straight;
            const double cc=o_.exhaust?(main?idelchikStraight(q2):idelchikBranch(q2,beta)):(main?idelchikDivStraight(vsvc):idelchikDivBranch(q2,beta));
            Fitting f;f.dionica=of[c];f.element=i;f.what=main?L"Re\u0161etka na dnu \u2013 prolaz":L"Re\u0161etka na dnu \u2013 ogranak";f.dim=dimText(port(x,leg));
            f.key=L"Idelchik";f.c=cc;f.dp=cc*pvc;f.ok=std::isfinite(f.dp);const double pd=r.d[f.dionica].pd;f.zeta=pd>0&&f.ok?f.dp/pd:0;
            wchar_t t2[96];swprintf(t2,96,L", Ab/Ac = %.2f, Qb/Qc = %.2f",beta,q2);for(wchar_t* z=t2;*z;++z)if(*z==L'.')*z=L',';
            f.note=(tag.empty()?std::wstring():tag+L", ")+L"odvajanje u dnu kanala, "+(main?L"prolaz":L"ogranak = priklju\u010Dak re\u0161etke")+capped+t2
                +(o_.exhaust?L"; Idelchik o\u0161tar T, sabiranje (tabela kao KanCalc 1.7 / 1.6), C na Vc":L"; Idelchik o\u0161tar T, razdvajanje (tabela kao KanCalc 0.7 / 0.6), C na Vc");
            const double top=o_.exhaust?1.0:0.8;
            if(beta<0.1||beta>top)f.note+=o_.exhaust?L"; Ab/Ac van tabele 0,1\u20131,0 \u2013 uzeta granica":L"; Ab/Ac van tabele 0,1\u20130,8 \u2013 uzeta granica";
            if(!o_.exhaust&&!main&&(q2<0.1||q2>0.9))f.note+=L"; Qb/Qc van tabele 0,1\u20130,9 \u2013 uzeta granica";
            if(main&&ac>0&&std::fabs(as/ac-1)>0.05)f.note+=L"; As \u2260 Ac \u2013 izvan formule, ra\u010Dunato kao As = Ac";
            r.f.push_back(f);}
    }
    // Bullhead (ASHRAE SR5-15 / ER5-5 rectangular, SD5-19 / ED5-4 round): the common duct on the
    // T komad's ogranak, the two run ends are branches b1 (port 0) and b2 (port 1); C on each Vb.
    void bullhead(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,const std::vector<int>& of,Result& r,int i,const std::wstring& type){
        const Element& x=e[i];const Section& c=port(x,2);const Section& b1=port(x,0);const Section& b2=port(x,1);
        double q[2]={0,0};int child[2]={-1,-1};
        for(size_t k=0;k<kids[i].size();++k){const int ch=kids[i][k];if(e[ch].via==0||e[ch].via==1){q[e[ch].via]=e[ch].flow;child[e[ch].via]=ch;}}
        const bool round=c.round&&b1.round&&b2.round,rect=!c.round&&!b1.round&&!b2.round;
        const std::string key=round?(o_.exhaust?"ED5-4":"SD5-19"):rect?(o_.exhaust?"ER5-5":"SR5-15"):std::string();
        std::map<std::string,double> in;
        if(rect){in["H"]=c.b;in["Wc"]=c.a;in["Wb1"]=b1.a;in["Wb2"]=b2.a;}
        else if(round){in["Dc"]=c.a;in["Db1"]=b1.a;in["Db2"]=b2.a;}
        in["Qc"]=x.flow/3.6;in["Qb1"]=q[0]/3.6;
        std::vector<std::string> w;std::map<std::string,double> cv;const bool have=!key.empty()&&eng_&&eng_->loaded()&&eng_->has(key);
        if(have)cv=eng_->calculate(key,in,&w);
        std::vector<std::string> keep;for(size_t k=0;k<w.size();++k)if(w[k].find("=0 van")==std::string::npos)keep.push_back(w[k]);
        for(int leg=0;leg<2;++leg){const int ch=child[leg];if(ch<0)continue;
            Fitting f;f.dionica=of[ch];f.element=i;f.what=L"T komad \u2013 ogranak";f.dim=dimText(port(x,leg));f.key=wide(key);
            f.note=type+L", ulaz kroz ogranak (bullhead), "+(leg==0?L"krak 1":L"krak 2");
            if(rect&&(std::fabs(b1.b-c.b)>=0.5||std::fabs(b2.b-c.b)>=0.5))f.note+=L"; ASHRAE pretpostavlja istu visinu svih krakova";
            if(key.empty())f.note+=L"; nema ASHRAE komada \u2013 unesite \u03B6";
            else if(!have)f.note+=L"; ASHRAE podaci nisu u\u010Ditani \u2013 unesite \u03B6";
            else{const char* cn=leg==0?"Cb1":"Cb2";const char* pn=leg==0?"Pob1":"Pob2";
                auto ic=cv.find(cn),ip=cv.find(pn);
                if(ic!=cv.end()&&ip!=cv.end()&&std::isfinite(ip->second)){f.c=ic->second;f.dp=ip->second;f.ok=true;const double pd=r.d[f.dionica].pd;f.zeta=pd>0?f.dp/pd:0;}
                const std::wstring extra=notes(keep);if(!extra.empty())f.note+=L"; "+extra;}
            r.f.push_back(f);}
    }
    // Krstasti komad / XK (plan 26, 2026-11-28; TVS port 3 = the second branch): ASHRAE has no cross,
    // so each branch is a T komad of its own (common Qc, its Qb, straight Qc - Qb), the prolaz a T
    // komad with both branches together (Qb = Qb1 + Qb2, the first branch's section). Air in
    // through a branch: not standard (rows without C).
    void crossJunction(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,const std::vector<int>& of,Result& r,int i){
        const Element& x=e[i];const Part& t=x.parts[0];
        const std::wstring type=t.lateral?L"XK":L"krstasti komad";
        if(x.entry>=2){for(size_t k=0;k<kids[i].size();++k){const int c=kids[i][k];
                add(r,of[c],i,L"T komad",type+L", ulaz kroz ogranak \u2013 nestandardno",port(x,e[c].via),std::string(),std::map<std::string,double>(),false,0);}
            return;}
        const Section& common=port(x,x.entry);const int straight=x.entry==0?1:0;
        double q[4]={0,0,0,0};int child[4]={-1,-1,-1,-1};
        for(size_t k=0;k<kids[i].size();++k){const int c=kids[i][k];if(e[c].via>=0&&e[c].via<4){q[e[c].via]=e[c].flow;child[e[c].via]=c;}}
        const std::string key=junctionKey(o_.exhaust,t.tee,common,port(x,2),t.cone,t.lateral);
        const bool have=eng_&&!key.empty()&&eng_->has(key);
        for(int leg=0;leg<4;++leg){const int c=child[leg];if(c<0||leg==x.entry)continue;
            const bool main=leg==straight;std::map<std::string,double> in;
            std::wstring note=type+(main?L", prolaz":L", ogranak")+L"; krstasti: ASHRAE nema krsta \u2013 ra\u010Dunato kao T komad po ogranku";
            if(have){
                if(main)in=seedJunction(eng_->params(key),common,port(x,2),port(x,straight),x.flow,q[2]+q[3],q[straight]);
                else in=seedJunction(eng_->params(key),common,port(x,leg),port(x,straight),x.flow,q[leg],x.flow-q[leg]);}
            if(child[straight]<0&&!main)note+=L", prolaz zatvoren (\u010Dep)";
            add(r,of[c],i,main?L"T komad \u2013 prolaz":L"T komad \u2013 ogranak",note,port(x,leg),key,in,main,0);}
    }
    void junction(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,const std::vector<int>& of,Result& r,int i){
        const Element& x=e[i];const Part& t=x.parts[0];if(x.ports.size()<3)return;
        const bool pants=t.tee==TeePants;
        if(x.ports.size()==4&&!pants){crossJunction(e,kids,of,r,i);return;}
        static const wchar_t* names[]={L"TK0",L"TK-O",L"TK-G",L"\u0161tucna"};const std::wstring type=pants&&t.lateral?std::wstring(L"Y45"):t.tee>=0&&t.tee<=3?names[t.tee]:L"T";
        if(!pants&&x.entry==2){bullhead(e,kids,of,r,i,type);return;}   // air in through the ogranak, out both ways
        if(pants?x.entry!=2:x.entry==2){
            for(size_t k=0;k<kids[i].size();++k){const int c=kids[i][k];
                add(r,of[c],i,L"T komad",type+L", ulaz kroz "+(pants?L"krak":L"ogranak")+L" \u2013 nestandardno",port(x,e[c].via),std::string(),std::map<std::string,double>(),false,0);}
            return;}
        const Section& common=port(x,x.entry);
        int branchPort=2,straightPort=x.entry==0?1:0;if(pants){branchPort=0;straightPort=1;}
        double q[3]={0,0,0};int child[3]={-1,-1,-1};
        for(size_t k=0;k<kids[i].size();++k){const int c=kids[i][k];if(e[c].via>=0&&e[c].via<3){q[e[c].via]=e[c].flow;child[e[c].via]=c;}}
        // a free port at the end of the network is closed (cep); a T komad / stucna with nothing on it
        // at all is a terminal: the air leaves through the ogranak (resetka), the prolaz is capped
        std::wstring capped;
        if(!pants&&kids[i].empty()){q[branchPort]=x.flow;child[branchPort]=i;capped=L", kraj mre\u017Ee: izlaz kroz ogranak, prolaz zatvoren (\u010Dep)";}
        else if(!pants&&child[straightPort]<0&&child[branchPort]>=0)capped=L", prolaz zatvoren (\u010Dep)";
        else if(!pants&&child[branchPort]<0&&child[straightPort]>=0)capped=L", ogranak zatvoren (\u010Dep)";
        const std::string key=junctionKey(o_.exhaust,t.tee,common,port(x,branchPort),t.cone,t.lateral);
        // exhaust, rectangular, TK0 / stucna outside ER5-3 (Ab/Ac 0.4..0.6): Idelchik, sharp T (user 2026-11-15)
        {const Section& br=port(x,branchPort);const double ac=area(common),beta=ac>0?area(br)/ac:0;
         // supply TK0 whose branch height differs from the run: SR5-5 assumes Hb = H -> KanCalc 0.7 / 0.6
         const bool divIdel=!o_.exhaust&&!pants&&t.tee==TeePlain&&!common.round&&!br.round&&beta>0&&std::fabs(br.b-common.b)>=0.5;
         if(divIdel||(o_.exhaust&&!pants&&!common.round&&!br.round&&beta>0&&(t.tee==TeePlain||(t.tee==TeeTap&&(beta<0.4||beta>0.6))))){
            const double vc=ac>0?x.flow/3600.0/ac:0,pvc=o_.rho*vc*vc/2,q2=x.flow>0?q[branchPort]/x.flow:0;
            const double as=area(port(x,straightPort)),vs=as>0?q[straightPort]/3600.0/as:0,vsvc=vc>0?vs/vc:0;
            for(int leg=0;leg<3;++leg){const int c=child[leg];if(c<0)continue;const bool main=leg==straightPort;
                const double cc=divIdel?(main?idelchikDivStraight(vsvc):idelchikDivBranch(q2,beta)):(main?idelchikStraight(q2):idelchikBranch(q2,beta));
                Fitting f;f.dionica=c==i?of[i]:of[c];f.element=i;f.what=main?L"T komad \u2013 prolaz":L"T komad \u2013 ogranak";f.dim=dimText(port(x,leg));
                f.key=L"Idelchik";f.c=cc;f.dp=cc*pvc;f.ok=std::isfinite(f.dp);const double pd=r.d[f.dionica].pd;f.zeta=pd>0&&f.ok?f.dp/pd:0;
                wchar_t t2[96];swprintf(t2,96,L", Ab/Ac = %.2f, Qb/Qc = %.2f",beta,q2);for(wchar_t* z=t2;*z;++z)if(*z==L'.')*z=L',';
                if(divIdel)f.note=type+(main?L", prolaz":L", ogranak")+capped+t2+L"; ogranak druge visine \u2013 Idelchik o\u0161tar T, razdvajanje (tabela kao KanCalc 0.7 / 0.6), C na Vc";
                else f.note=type+(main?L", prolaz":L", ogranak")+capped+t2+L"; Idelchik o\u0161tar T (As = Ac, tabela kao KanCalc 1.7 / 1.6), C na Vc \u2013 za pravougaoni spoj vjerovatno potcijenjeno (SMACNA pri Ab/Ac 0,5 daje 30\u201350 % vi\u0161e)";
                if(beta<0.1||beta>(divIdel?0.8:1.0))f.note+=divIdel?L"; Ab/Ac van tabele 0,1\u20130,8 \u2013 uzeta granica":L"; Ab/Ac van tabele 0,1\u20131,0 \u2013 uzeta granica";
                if(ac>0&&std::fabs(as/ac-1)>0.05)f.note+=L"; As \u2260 Ac \u2013 izvan formule, ra\u010Dunato kao As = Ac";
                r.f.push_back(f);}
            return;}}
        for(int leg=0;leg<3;++leg){const int c=child[leg];if(c<0)continue;
            const bool main=!pants&&leg==straightPort;
            std::map<std::string,double> in;double pv=0;std::wstring note=type+(main?L", prolaz":L", ogranak")+capped;
            if(eng_&&!key.empty()&&eng_->has(key)){
                if(pants){   // dovetail: C of this branch's Ab / Ac, on this branch's own pd (ASHRAE: equal split)
                    if(t.lateral)in=seedJunction(eng_->params(key),common,port(x,leg),port(x,1-leg),x.flow,q[leg],q[1-leg]);   // Y45: the other leg is the "prolaz"
                    else in=seedJunction(eng_->params(key),common,port(x,leg),port(x,leg),x.flow,q[leg],q[leg]);
                    const double s=area(port(x,leg)),v=s>0?q[leg]/3600.0/s:0;pv=o_.rho*v*v/2;
                    if(t.lateral)note+=L", ra\u010Dva Y45: krak kao ogranak ra\u010Dve 45\u00B0";
                    else if(x.flow>0&&std::fabs(q[leg]/x.flow-0.5)>0.1)note+=L", ASHRAE pretpostavlja jednaku podjelu protoka";}
                else in=seedJunction(eng_->params(key),common,port(x,branchPort),port(x,straightPort),x.flow,q[branchPort],q[straightPort]);}
            add(r,c==i?of[i]:of[c],i,main?L"T komad \u2013 prolaz":L"T komad \u2013 ogranak",note,port(x,leg),key,in,main,pv,!capped.empty());}
    }
};
}

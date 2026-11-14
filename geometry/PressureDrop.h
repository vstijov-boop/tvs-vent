#pragma once
// Pad pritiska po dionicama (TVS_PAD): the flow network of TVS_PROTOK (geometry/FlowNetwork.h)
// split into dionice, friction per dionica and the fittings with ASHRAE coefficients
// (geometry/AshraeEngine.h). A free T komad port at the end of the network counts as capped (cep):
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
//   supply TK0 SR5-5, TK-O SR5-1, stucna SR5-13 (user 2026-11-14), round branch SR5-11, TK-G SR5-14, round main SD5-9;
//   exhaust ER5-1, round branch ER5-2, TK-G ER5-4, round main ED5-3. The branch loss goes to the
//   dionica of the branch, the straight (main) loss to the dionica of the prolaz. Every loss is
//   given as zeta on the pd of its dionica (zeta = dp / pd), so Z = sum(zeta) * pd in the sheet.
#include "AshraeEngine.h"
#include <cmath>
#include <cwchar>
#include <map>
#include <string>
#include <vector>
namespace tvsPressure {
struct Section {double a,b;bool round;Section():a(0),b(0),round(false){}Section(double x,double y,bool r):a(x),b(y),round(r){}};
struct XY {double x,y;XY():x(0),y(0){}XY(double a,double b):x(a),y(b){}};
enum PartKind {Other=0,Duct=1,Elbow=2,Transition=3,Tee=4};
enum TeeType {TeePlain=0,TeeBranch=1,TeePants=2,TeeTap=3};   // TK0, TK-O, TK-G, stucna (TeeData.h)
struct Part {
    int kind;bool flex;
    double length;        // mm: duct, prelaz; elbow: the axis arc (computed when 0)
    double angle,radius;  // elbow: deflection (deg), axis radius (mm)
    int tee;              // TeeType
    std::vector<Section> sections;   // per port (a single part: the element's ports)
    std::vector<XY> line;            // plan polyline for the schematic, mm
    Part():kind(Other),flex(false),length(0),angle(0),radius(0),tee(TeePlain){}
};
struct Element {
    std::vector<Part> parts;          // one, or the members of an etaz / obilazak
    std::vector<Section> ports;       // section per port of the element
    std::vector<XY> at;               // port positions (plan, mm)
    double flow;int parent,entry,via; // FlowNetwork: parent, own port to the unit, parent's port
    Element():flow(0),parent(-1),entry(-1),via(-1){}
};
struct Options {bool exhaust;double rough,flexRough,rho,nu;Options():exhaust(false),rough(0.15),flexRough(3.0),rho(1.2),nu(15.1e-6){}};
struct Fitting {
    int dionica,element;
    std::wstring what,note,dim,key;   // Element, opis / napomena, dimenzija, ASHRAE key ("" none)
    double c,dp,zeta;                 // ASHRAE C (its own reference), dp Pa, zeta on the dionica pd
    bool ok;                          // computed (else the user types zeta)
    Fitting():dionica(-1),element(-1),c(0),dp(0),zeta(0),ok(false){}
};
struct Dionica {
    int number,parent;std::vector<int> children,elements;
    std::wstring from,to;XY fromAt,toAt;
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
// lambda: laminar 64/Re, else Colebrook-White (as tvsDuctSizing::friction)
inline double lambdaOf(double re,double roughMm,double dhMm){
    if(!(re>0)||!(dhMm>0))return 0;if(re<2300)return 64/re;
    const double k=roughMm/1000.0,dh=dhMm/1000.0;double x=1/std::sqrt(0.02);
    for(int i=0;i<50;++i){const double n=-2*std::log10(k/(3.71*dh)+2.51*x/re);if(std::fabs(n-x)<1e-12){x=n;break;}x=n;}
    return 1/(x*x);
}
inline void hydraulics(Dionica& d,const Options& o){
    d.rough=d.flex?o.flexRough:o.rough;d.dh=hydraulic(d.s);const double s=area(d.s);
    d.v=s>0?std::fabs(d.flow)/3600.0/s:0;d.pd=o.rho*d.v*d.v/2;d.re=d.v*d.dh/1000.0/o.nu;
    d.lambda=lambdaOf(d.re,d.rough,d.dh);d.r=d.dh>0?d.lambda/(d.dh/1000.0)*d.pd:0;
}
// ---- ASHRAE keys (the user's mapping) ----
inline std::string elbowKey(const Section& s,double angle){
    if(!s.round)return "CR3-1";
    if(std::fabs(angle-90)<=7.5)return s.a<=250?"CD3-2":"CD3-21";
    if(std::fabs(angle-45)<=7.5&&s.a<=250)return "CD3-4";
    return std::string();
}
inline std::string transitionKey(bool exhaust,const Section& up,const Section& down){
    if(up.round&&down.round)return exhaust?"ED4-1":"SD4-1";
    if(up.round)return exhaust?"ED4-2":"SR4-3";     // round -> rectangular
    if(down.round)return exhaust?"ER4-3":"SD4-2";   // rectangular -> round
    const bool oneSide=std::fabs(up.a-down.a)<0.5||std::fabs(up.b-down.b)<0.5;
    return exhaust?(oneSide?"ER4-1":"ER4-2"):(oneSide?"SR4-1":"SR4-2");
}
inline std::string junctionKey(bool exhaust,int tee,const Section& common,const Section& branch){
    if(common.round)return branch.round?(exhaust?"ED5-3":"SD5-9"):std::string();
    if(tee==TeePants)return exhaust?"ER5-4":"SR5-14";
    if(branch.round)return exhaust?"ER5-2":"SR5-11";
    if(exhaust)return "ER5-1";
    return tee==TeePlain?"SR5-5":tee==TeeBranch?"SR5-1":"SR5-13";   // TK0 plain tee, TK-O smooth radius, stucna 45 deg entry
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
        if(tvsAshrae::isFlow(p)){if(q>0)v=q/3.6;}
        else{const char first=p.var[0];const double x=dimOf(sec,first);if(x>0)v=x;}
        in[p.var]=v;}
    return in;
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
        return r;
    }
private:
    tvsAshrae::Engine* eng_;Options o_;
    static bool flexOf(const Element& x){return !x.parts.empty()&&x.parts[0].flex;}
    static bool isTransition(const Element& x){return x.parts.size()==1&&x.parts[0].kind==Transition;}
    static bool isTee(const Element& x){return x.parts.size()==1&&x.parts[0].kind==Tee;}
    static const Section& port(const Element& x,int p){static const Section none;return p>=0&&p<static_cast<int>(x.ports.size())?x.ports[p]:none;}
    static bool startsNew(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,int p,int c){
        if(kids[p].size()>1||isTee(e[p])||isTransition(e[p])||flexOf(e[p])!=flexOf(e[c]))return true;   // a T komad is always a node, also with one free port (cep)
        const Section& a=port(e[p],e[c].via);const Section& b=port(e[c],e[c].entry);
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
            if(d.children.empty()&&!isTee(x)){swprintf(name,16,L"K%d",++k);for(size_t j=0;j<x.at.size();++j)if(static_cast<int>(j)!=x.entry){d.toAt=x.at[j];break;}}
            else if(kids[last].size()>1||isTee(x)){swprintf(name,16,L"T%d",++t);XY c;for(size_t j=0;j<x.at.size();++j){c.x+=x.at[j].x/x.at.size();c.y+=x.at[j].y/x.at.size();}d.toAt=c;}
            else{swprintf(name,16,L"P%d",++p);const Element& y=e[kids[last][0]];if(y.entry>=0&&y.entry<static_cast<int>(y.at.size()))d.toAt=y.at[y.entry];}
            d.to=name;}
        for(size_t q=0;q<r.d.size();++q){Dionica& d=r.d[q];
            if(d.parent<0){d.from=L"J";const Element& x=e[d.elements[0]];if(x.entry>=0&&x.entry<static_cast<int>(x.at.size()))d.fromAt=x.at[x.entry];}
            else{d.from=r.d[d.parent].to;d.fromAt=r.d[d.parent].toAt;}}
    }
    void add(Result& r,int dionica,int element,const std::wstring& what,const std::wstring& note,const Section& s,const std::string& key,const std::map<std::string,double>& in,bool main,double pvOwn){
        Fitting f;f.dionica=dionica;f.element=element;f.what=what;f.note=note;f.dim=dimText(s);f.key=wide(key);
        if(key.empty()){f.note+=note.empty()?L"nema ASHRAE komada \u2013 unesite \u03B6":L"; nema ASHRAE komada \u2013 unesite \u03B6";r.f.push_back(f);return;}
        if(!eng_||!eng_->loaded()||!eng_->has(key)){f.note+=(f.note.empty()?L"":L"; ")+std::wstring(L"ASHRAE podaci nisu u\u010Ditani \u2013 unesite \u03B6");r.f.push_back(f);return;}
        std::vector<std::string> w;const tvsAshrae::Engine::Loss l=eng_->loss(key,in,main,&w);
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
                else if(t.kind==Other)r.warnings.push_back(L"element bez prora\u010Duna (dionica "+std::to_wstring(d.number)+L")");}}
    }
    void junction(const std::vector<Element>& e,const std::vector<std::vector<int> >& kids,const std::vector<int>& of,Result& r,int i){
        const Element& x=e[i];const Part& t=x.parts[0];if(x.ports.size()<3)return;
        const bool pants=t.tee==TeePants;
        static const wchar_t* names[]={L"TK0",L"TK-O",L"TK-G",L"\u0161tucna"};const std::wstring type=t.tee>=0&&t.tee<=3?names[t.tee]:L"T";
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
        const std::string key=junctionKey(o_.exhaust,t.tee,common,port(x,branchPort));
        for(int leg=0;leg<3;++leg){const int c=child[leg];if(c<0)continue;
            const bool main=!pants&&leg==straightPort;
            std::map<std::string,double> in;double pv=0;std::wstring note=type+(main?L", prolaz":L", ogranak")+capped;
            if(eng_&&!key.empty()&&eng_->has(key)){
                if(pants){   // dovetail: C of this branch's Ab / Ac, on this branch's own pd (ASHRAE: equal split)
                    in=seedJunction(eng_->params(key),common,port(x,leg),port(x,leg),x.flow,q[leg],q[leg]);
                    const double s=area(port(x,leg)),v=s>0?q[leg]/3600.0/s:0;pv=o_.rho*v*v/2;
                    if(x.flow>0&&std::fabs(q[leg]/x.flow-0.5)>0.1)note+=L", ASHRAE pretpostavlja jednaku podjelu protoka";}
                else in=seedJunction(eng_->params(key),common,port(x,branchPort),port(x,straightPort),x.flow,q[branchPort],q[straightPort]);}
            add(r,c==i?of[i]:of[c],i,main?L"T komad \u2013 prolaz":L"T komad \u2013 ogranak",note,port(x,leg),key,in,main,pv);}
    }
};
}

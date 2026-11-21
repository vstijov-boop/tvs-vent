#pragma once
// TVS_DPD_IZVOZ: the network of TVS_PAD (geometry/PressureDrop.h) as a DuctPressDrop system file
// (JSON, the flat "Uvezi sistem" format of DuctPressDrop services/system_import.py, the same as
// its Revit / Dynamo export). Pure C++14, ASCII; tests/test_dpd_export.cpp, test_dpd_export.py.
//   One DuctPressDrop dionica = one straight piece of a TVS dionica: a TVS dionica with plan
//   elbows is split at the elbow vertices (nodes KO1, KO2, ...), because DuctPressDrop keeps one
//   direction vector per dionica and derives the elbows / tees / transitions from the geometry
//   itself (so no fittings are written). Nodes: the TVS names (J unit, T T komad, P change of
//   section / kind, K terminal end); the node of a T komad is on the main axis (the branch port
//   projected onto it), so the run stays straight and the branch is perpendicular.
//   Units as DuctPressDrop: Q m3/h, W / H / D mm, L and smjer m (E = +x, N = +y, U = +z). Plan
//   only: dz = 0 (TVS ports do not carry a reliable elevation). OD -> DO away from the unit (also
//   for exhaust; DuctPressDrop reverses the air flow of "extract" itself). L = |smjer| (the axis
//   between the nodes, fittings included): DuctPressDrop always sets L from smjer.
//   Numbers with '.', whatever the C locale; strings are UTF-8 and escaped for JSON.
#include "PressureDrop.h"
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
namespace tvsDpd {
struct Segment {
    std::string id,opis,from,to;
    bool round,flex;
    double w,h,d,q;        // mm, m3/h
    double dx,dy,dz;       // m, from -> to
    Segment():round(false),flex(false),w(0),h(0),d(0),q(0),dx(0),dy(0),dz(0){}
    double length()const{return std::sqrt(dx*dx+dy*dy+dz*dz);}
};
struct System {
    std::string name,project,engineer,source;   // UTF-8
    bool exhaust;double temperature;std::string safety;   // sig_factor_pct (DuctPressDrop keeps it as text)
    std::vector<Segment> segments;
    System():exhaust(false),temperature(20.0),safety("10"){}
};
// fixed point with '.', trailing zeros removed ("-0" -> "0")
inline std::string number(double v,int decimals){
    if(!std::isfinite(v))v=0;
    char buf[64];std::snprintf(buf,sizeof(buf),"%.*f",decimals,v);
    std::string s(buf);for(size_t i=0;i<s.size();++i)if(s[i]==',')s[i]='.';   // a locale with a decimal comma
    if(s.find('.')!=std::string::npos){while(!s.empty()&&s[s.size()-1]=='0')s.erase(s.size()-1);if(!s.empty()&&s[s.size()-1]=='.')s.erase(s.size()-1);}
    if(s=="-0"||s.empty())s="0";
    return s;
}
// JSON string body (UTF-8 passes through)
inline std::string escape(const std::string& s){
    std::string o;o.reserve(s.size()+8);
    for(size_t i=0;i<s.size();++i){const unsigned char c=static_cast<unsigned char>(s[i]);
        if(c=='"')o+="\\\"";else if(c=='\\')o+="\\\\";else if(c=='\n')o+="\\n";else if(c=='\r')o+="\\r";else if(c=='\t')o+="\\t";
        else if(c<0x20){char b[8];std::snprintf(b,sizeof(b),"\\u%04x",static_cast<unsigned>(c));o+=b;}
        else o+=static_cast<char>(c);}
    return o;
}
inline std::string quoted(const std::string& s){return "\""+escape(s)+"\"";}
// wide (UTF-16 on Windows, UTF-32 elsewhere) -> UTF-8
inline std::string utf8(const std::wstring& w){
    std::string o;
    for(size_t i=0;i<w.size();++i){unsigned long c=static_cast<unsigned long>(w[i]);
        if(c>=0xD800&&c<=0xDBFF&&i+1<w.size()){const unsigned long lo=static_cast<unsigned long>(w[i+1]);
            if(lo>=0xDC00&&lo<=0xDFFF){c=0x10000+((c-0xD800)<<10)+(lo-0xDC00);++i;}}
        if(c>=0xD800&&c<=0xDFFF)c=0xFFFD;
        if(c<0x80)o+=static_cast<char>(c);
        else if(c<0x800){o+=static_cast<char>(0xC0|(c>>6));o+=static_cast<char>(0x80|(c&63));}
        else if(c<0x10000){o+=static_cast<char>(0xE0|(c>>12));o+=static_cast<char>(0x80|((c>>6)&63));o+=static_cast<char>(0x80|(c&63));}
        else{o+=static_cast<char>(0xF0|(c>>18));o+=static_cast<char>(0x80|((c>>12)&63));o+=static_cast<char>(0x80|((c>>6)&63));o+=static_cast<char>(0x80|(c&63));}}
    return o;
}
// material names of DuctPressDrop MATERIAL_PRESETS (JSON escapes keep this header ASCII)
inline const char* sheetMaterial(){return "Pocinkovani \\u010deli\\u010dni lim";}
inline const char* flexMaterial(){return "Fleksibilni izolovani";}
inline std::string json(const System& s){
    std::string o="{\n";
    o+="  \"naziv\": "+quoted(s.name)+",\n";
    o+="  \"projekat\": "+quoted(s.project)+",\n";
    o+="  \"inzenjer\": "+quoted(s.engineer)+",\n";
    o+="  \"izvor\": "+quoted(s.source)+",\n";
    o+="  \"temp_vazduha\": "+number(s.temperature,2)+",\n";
    o+="  \"sig_factor_pct\": "+quoted(s.safety)+",\n";
    o+="  \"dionice\": [";
    const char* kind=s.exhaust?"extract":"supply";
    for(size_t i=0;i<s.segments.size();++i){const Segment& x=s.segments[i];
        o+=i?",\n    {":"\n    {";
        o+="\"id\": "+quoted(x.id)+", \"opis\": "+quoted(x.opis)+", \"cvor_od\": "+quoted(x.from)+", \"cvor_do\": "+quoted(x.to)+", ";
        o+="\"tip\": \""+std::string(x.round?"K":"P")+"\", \"sistem\": \""+kind+"\", ";
        o+="\"Q\": "+number(x.q,1)+", \"W\": "+number(x.round?0:x.w,1)+", \"H\": "+number(x.round?0:x.h,1)+", \"D\": "+number(x.round?x.d:0,1)+", ";
        o+="\"L\": "+number(x.length(),4)+", \"smjer\": ["+number(x.dx,4)+", "+number(x.dy,4)+", "+number(x.dz,4)+"], ";
        o+="\"materijal\": \""+std::string(x.flex?flexMaterial():sheetMaterial())+"\", \"fitinzi\": []}";}
    o+=s.segments.empty()?"]\n}\n":"\n  ]\n}\n";
    return o;
}
// ---- geometry: tvsPressure dionice -> straight DuctPressDrop pieces ---------------------------
typedef tvsPressure::XY XY;
inline double dist(const XY& a,const XY& b){return std::hypot(a.x-b.x,a.y-b.y);}
inline bool isTee(const tvsPressure::Element& x){return x.parts.size()==1&&x.parts[0].kind==tvsPressure::Tee&&x.at.size()==3;}
// T komad node: the branch port (2) projected onto the run axis (ports 0 - 1)
inline XY teeCentre(const tvsPressure::Element& x){
    const XY& a=x.at[0];const XY& b=x.at[1];const XY& c=x.at[2];
    const double ux=b.x-a.x,uy=b.y-a.y,l2=ux*ux+uy*uy;
    if(!(l2>0))return XY((a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3);
    const double t=((c.x-a.x)*ux+(c.y-a.y)*uy)/l2;return XY(a.x+ux*t,a.y+uy*t);
}
inline XY entryPoint(const tvsPressure::Element& x){return x.entry>=0&&x.entry<static_cast<int>(x.at.size())?x.at[x.entry]:(x.at.empty()?XY():x.at[0]);}
inline XY exitPoint(const tvsPressure::Element& x){for(size_t j=0;j<x.at.size();++j)if(static_cast<int>(j)!=x.entry)return x.at[j];return entryPoint(x);}
// turning angle (deg) at b on a -> b -> c
inline double turn(const XY& a,const XY& b,const XY& c){
    const double ux=b.x-a.x,uy=b.y-a.y,vx=c.x-b.x,vy=c.y-b.y,lu=std::hypot(ux,uy),lv=std::hypot(vx,vy);
    if(!(lu>0)||!(lv>0))return 0;
    double k=(ux*vx+uy*vy)/(lu*lv);if(k>1)k=1;if(k<-1)k=-1;
    return std::acos(k)*180.0/3.14159265358979323846;
}
struct Options {
    double minLength;   // mm: a shorter piece is dropped and its end node joined to its start
    double minTurn;     // deg: a smaller bend is not a vertex (straight on)
    Options():minLength(1.0),minTurn(1.0){}
};
// plan polyline of dionica q in mm: its start node, the plan elbow vertices, its end node
inline std::vector<XY> path(const std::vector<tvsPressure::Element>& e,const tvsPressure::Result& r,int q,const std::vector<XY>& ends,const Options& o){
    const tvsPressure::Dionica& d=r.d[q];std::vector<XY> p;
    if(d.parent>=0&&d.parent<static_cast<int>(ends.size()))p.push_back(ends[d.parent]);
    else if(!d.elements.empty())p.push_back(entryPoint(e[d.elements[0]]));
    for(size_t k=0;k<d.elements.size();++k){const tvsPressure::Element& x=e[d.elements[k]];
        if(x.parts.size()!=1)continue;const tvsPressure::Part& t=x.parts[0];
        if(t.kind==tvsPressure::Elbow&&t.angle>=o.minTurn&&t.line.size()==3)p.push_back(t.line[1]);}
    p.push_back(ends[q]);
    // without repeated points (the end node is kept) and without straight-on vertices
    std::vector<XY> a;
    for(size_t k=0;k<p.size();++k){
        if(a.empty()||dist(a.back(),p[k])>=o.minLength){a.push_back(p[k]);continue;}
        if(k+1==p.size()){if(a.size()>1)a.back()=p[k];else a.push_back(p[k]);}}
    std::vector<XY> b;
    for(size_t k=0;k<a.size();++k){
        if(k>0&&k+1<a.size()&&turn(b.back(),a[k],a[k+1])<o.minTurn)continue;
        b.push_back(a[k]);}
    return b;
}
// end node point of every dionica (mm, plan)
inline std::vector<XY> ends(const std::vector<tvsPressure::Element>& e,const tvsPressure::Result& r){
    std::vector<XY> out(r.d.size());
    for(size_t q=0;q<r.d.size();++q){const tvsPressure::Dionica& d=r.d[q];if(d.elements.empty())continue;
        const tvsPressure::Element& x=e[d.elements.back()];
        if(isTee(x))out[q]=teeCentre(x);
        else if(d.children.size()==1&&d.children[0]>=0&&d.children[0]<static_cast<int>(r.d.size())&&!r.d[d.children[0]].elements.empty())
            out[q]=entryPoint(e[r.d[d.children[0]].elements[0]]);
        else if(d.children.empty())out[q]=exitPoint(x);
        else out[q]=d.toAt;}
    return out;
}
inline std::string idOf(bool exhaust,int n){char b[24];std::snprintf(b,sizeof(b),"%s-%02d",exhaust?"O":"D",n);return b;}
// all pieces, in the order of the TVS dionice (from the unit)
inline std::vector<Segment> segments(const std::vector<tvsPressure::Element>& e,const tvsPressure::Result& r,bool exhaust,
                                     std::vector<std::wstring>* warnings=nullptr,const Options& o=Options()){
    std::vector<Segment> out;const std::vector<XY> end=ends(e,r);
    std::map<std::string,std::string> alias;int corner=0,zero=0;
    auto node=[&](const std::string& n){std::map<std::string,std::string>::const_iterator i=alias.find(n);return i==alias.end()?n:i->second;};
    for(size_t q=0;q<r.d.size();++q){const tvsPressure::Dionica& d=r.d[q];
        const std::string from=node(utf8(d.from)),to=utf8(d.to);
        const std::vector<XY> p=path(e,r,static_cast<int>(q),end,o);
        double total=0;for(size_t k=1;k<p.size();++k)total+=dist(p[k-1],p[k]);
        if(p.size()<2||total<o.minLength){alias[to]=from;
            if(warnings)warnings->push_back(L"dionica "+std::to_wstring(d.number)+L" ("+d.from+L" \u2013 "+d.to+L") nema du\u017Einu: \u010Dvor "+d.to+L" spojen sa prethodnim");
            continue;}
        if(!(d.flow>0))++zero;
        const size_t pieces=p.size()-1;std::string at=from;
        for(size_t k=1;k<p.size();++k){Segment s;
            s.from=at;s.to=k+1<p.size()?"KO"+std::to_string(++corner):to;at=s.to;
            s.id=idOf(exhaust,static_cast<int>(out.size())+1);
            s.opis="TVS dionica "+std::to_string(d.number);
            if(pieces>1)s.opis+=" ("+std::to_string(k)+"/"+std::to_string(pieces)+")";
            s.round=d.s.round;s.flex=d.flex;s.q=d.flow;
            if(s.round)s.d=d.s.a;else{s.w=d.s.a;s.h=d.s.b;}
            s.dx=(p[k].x-p[k-1].x)/1000.0;s.dy=(p[k].y-p[k-1].y)/1000.0;s.dz=0;
            out.push_back(s);}
    }
    if(zero&&warnings)warnings->push_back(L"dionica bez protoka (Q = 0): "+std::to_wstring(zero));
    return out;
}
}

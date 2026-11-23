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
//   Oprema (linked blocks, geometry/Equipment.h): a fitting of the DuctPressDrop piece it lies on,
//   with Fiting.oprema = {tip (oprema.py palette key), pos 0..1 from cvor_od, oznaka = tag} and an
//   explicit "terminal"; terminal at the free end (pos 1, OD -> DO points away from the unit), inline
//   at its centre. FixedDp -> fn dp_fix (dp), Zeta -> konst / manual (zeta), Nominal -> dp_fix
//   computed at the exported Q (DuctPressDrop keeps it fixed: warning), no model -> dp_fix 0.
//   Names are DuctPressDrop hvac_data FITING_META keys (JSON escapes keep this header ASCII).
//   Resetka na dnu (tvsEquipment::Bottom): its tap junction is a node like a T komad (on the duct
//   axis); the grille is a short vertical branch piece from it: D or W x H = the neck, smjer
//   (0, 0, -tapDrop) m (DuctPressDrop takes dz), the terminal at its free end. The closed duct end
//   beyond the last grille is exported as it is drawn (Part::length of the tap, along the duct, Q = 0,
//   node Z1, Z2 ...), so DuctPressDrop derives a T komad there (closed leg) and not an elbow.
#include "PressureDrop.h"
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
namespace tvsDpd {
struct Equip {
    std::string naziv,fn,tip,oznaka;   // naziv: JSON string body (escaped); oznaka UTF-8
    double value,pos;bool terminal;    // dp_fix: Pa, konst / manual: zeta
    Equip():value(0),pos(0.5),terminal(false){}
};
struct Segment {
    std::string id,opis,from,to;
    bool round,flex;
    double w,h,d,q;        // mm, m3/h
    double dx,dy,dz;       // m, from -> to
    std::vector<Equip> equipment;
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
// ---- oprema -> DuctPressDrop fitting (FITING_META name, fn, oprema.py palette key) ----
inline const char* equipmentTip(int kind){
    switch(kind){
    case tvsEquipment::FlowDamper:return "zaluzina";case tvsEquipment::FireDamper:return "pp_klapna";
    case tvsEquipment::Cav:case tvsEquipment::Vav:return "vav";case tvsEquipment::Silencer:return "prigusivac";
    case tvsEquipment::Grille:return "resetka";case tvsEquipment::Diffuser:return "difuzor";
    case tvsEquipment::Hood:return "kapa";case tvsEquipment::Louvre:return "usisna_zaluzina";
    default:return "oprema";}   // not a palette type: no symbol in DuctPressDrop, the loss counts
}
// fixed dp (fn dp_fix)
inline const char* equipmentDpName(int kind,bool exhaust){
    switch(kind){
    case tvsEquipment::FireDamper:return "Po\\u017earna klapna (otvorena)";
    case tvsEquipment::Silencer:return "Prigu\\u0161iva\\u010d buke (fiksni \\u0394P)";
    case tvsEquipment::Cav:case tvsEquipment::Vav:return "VAV kutija (terminal)";
    case tvsEquipment::Grille:return exhaust?"Re\\u0161etka \\u2013 povrat (fiksni \\u0394P)":"Re\\u0161etka \\u2013 dovod (fiksni \\u0394P)";
    case tvsEquipment::Diffuser:return "Difuzor kvadratni (fiksni \\u0394P)";
    case tvsEquipment::Louvre:return exhaust?"\\u017daluzina \\u2013 odvod (fiksni \\u0394P)":"\\u017daluzina \\u2013 dovod (fiksni \\u0394P)";
    default:return "Ru\\u010dni unos \\u0394P [Pa]";}
}
// zeta: terminals with a catalogue entry (fn konst, param = zeta), else manual zeta
inline const char* equipmentZetaName(int kind,bool exhaust,const char*& fn){
    fn="konst";
    switch(kind){
    case tvsEquipment::Grille:return exhaust?"Re\\u0161etka \\u2013 povrat (tipi\\u010dna)":"Re\\u0161etka \\u2013 dovod (tipi\\u010dna)";
    case tvsEquipment::Diffuser:return "Difuzor kvadratni (tipi\\u010dan)";
    case tvsEquipment::Louvre:return exhaust?"\\u017daluzina \\u2013 odvod (tipi\\u010dna)":"\\u017daluzina \\u2013 dovod (tipi\\u010dna)";
    default:fn="manual";return "Ru\\u010dni unos \\u03b6";}
}
// q: the flow through it (m3/h). warning: set for Nominal / no model (UTF-16 text)
inline Equip equipment(const tvsEquipment::Record& r,double q,bool exhaust,const std::string& oznaka,std::wstring* warning=nullptr){
    Equip x;x.tip=equipmentTip(r.kind);x.oznaka=oznaka;x.terminal=r.terminal();
    if(r.model==tvsEquipment::Zeta){const char* fn=nullptr;x.naziv=equipmentZetaName(r.kind,exhaust,fn);x.fn=fn;x.value=r.zeta>0?r.zeta:0;return x;}
    x.naziv=equipmentDpName(r.kind,exhaust);x.fn="dp_fix";x.value=tvsEquipment::loss(r,q,0);
    if(warning){wchar_t t[200];t[0]=0;
        if(r.model==tvsEquipment::Nominal)std::swprintf(t,200,L": \u0394p %.1f Pa izra\u010dunat pri Q = %.0f m\u00b3/h (katalo\u0161ka ta\u010dka %.1f Pa / %.0f m\u00b3/h) \u2013 u DuctPressDrop-u fiksan",x.value,q,r.dpNominal,r.qNominal);
        else if(r.model!=tvsEquipment::FixedDp)std::swprintf(t,200,L": bez modela gubitka \u2013 izvezeno \u0394p = 0");
        if(t[0])*warning=t;}
    return x;
}
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
        o+="\"materijal\": \""+std::string(x.flex?flexMaterial():sheetMaterial())+"\", \"fitinzi\": [";
        for(size_t k=0;k<x.equipment.size();++k){const Equip& q=x.equipment[k];
            o+=k?", {":"{";
            o+="\"naziv\": \""+q.naziv+"\", \"fn\": \""+q.fn+"\", \"kolicina\": 1, \"param_vals\": ["+number(q.value,3)+"], ";
            o+="\"terminal\": "+std::string(q.terminal?"true":"false")+", \"oprema\": {\"tip\": \""+q.tip+"\", \"pos\": "+number(q.pos,4)+", \"oznaka\": "+quoted(q.oznaka)+"}}";}
        o+="]}";}
    o+=s.segments.empty()?"]\n}\n":"\n  ]\n}\n";
    return o;
}
// ---- geometry: tvsPressure dionice -> straight DuctPressDrop pieces ---------------------------
typedef tvsPressure::XY XY;
inline double dist(const XY& a,const XY& b){return std::hypot(a.x-b.x,a.y-b.y);}
// parameter (0..1) of the point c projected on a -> b, and its distance from the segment
inline double along(const XY& a,const XY& b,const XY& c,double& off){
    const double ux=b.x-a.x,uy=b.y-a.y,l2=ux*ux+uy*uy;double t=l2>0?((c.x-a.x)*ux+(c.y-a.y)*uy)/l2:0;
    if(t<0)t=0;if(t>1)t=1;off=dist(XY(a.x+ux*t,a.y+uy*t),c);return t;
}
inline bool isTee(const tvsPressure::Element& x){return x.parts.size()==1&&(x.parts[0].kind==tvsPressure::Tee||x.parts[0].kind==tvsPressure::BottomTap)&&x.at.size()==3;}
// the branch dionica of a resetka na dnu: the grille alone behind its tap junction (port 2)
inline bool bottomBranch(const std::vector<tvsPressure::Element>& e,const tvsPressure::Dionica& d){
    if(d.elements.size()!=1)return false;const tvsPressure::Element& x=e[d.elements[0]];
    return x.parts.size()==1&&x.parts[0].kind==tvsPressure::Equipment&&x.via==2&&x.parent>=0&&x.parent<static_cast<int>(e.size())
        &&e[x.parent].parts.size()==1&&e[x.parent].parts[0].kind==tvsPressure::BottomTap;
}
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
    double tapDrop;     // m: length of the vertical branch piece of a resetka na dnu
    Options():minLength(1.0),minTurn(1.0),tapDrop(0.10){}
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
    std::map<std::string,std::string> alias;int corner=0,zero=0,closed=0;std::map<std::wstring,int> tags;
    auto tagOf=[&](const tvsEquipment::Record& r){   // the tag, or prefix-nn for an untagged one
        if(!r.tag.empty())return utf8(r.tag);const std::wstring p=tvsEquipment::tagPrefix(r.kind);char b[16];std::snprintf(b,sizeof(b),"-%02d",++tags[p]);return utf8(p)+b;};
    auto node=[&](const std::string& n){std::map<std::string,std::string>::const_iterator i=alias.find(n);return i==alias.end()?n:i->second;};
    for(size_t q=0;q<r.d.size();++q){const tvsPressure::Dionica& d=r.d[q];
        const std::string from=node(utf8(d.from)),to=utf8(d.to);
        if(bottomBranch(e,d)){   // resetka na dnu: vertical branch piece, the grille at its free end
            const tvsPressure::Element& x=e[d.elements[0]];const tvsEquipment::Record& rec=x.parts[0].equipment;
            Segment s;s.from=from;s.to=to;s.id=idOf(exhaust,static_cast<int>(out.size())+1);
            s.opis="TVS dionica "+std::to_string(d.number)+" (resetka na dnu)";
            s.round=d.s.round;s.q=d.flow;if(s.round)s.d=d.s.a;else{s.w=d.s.a;s.h=d.s.b;}
            s.dz=-o.tapDrop;if(!(d.flow>0))++zero;
            std::wstring w;Equip q=equipment(rec,x.flow,exhaust,tagOf(rec),warnings?&w:nullptr);q.pos=1;s.equipment.push_back(q);
            if(warnings&&!w.empty())warnings->push_back(L"oprema "+(rec.tag.empty()?std::wstring(tvsEquipment::kindName(rec.kind)):rec.tag)+w);
            out.push_back(s);continue;}
        const std::vector<XY> p=path(e,r,static_cast<int>(q),end,o);
        double total=0;for(size_t k=1;k<p.size();++k)total+=dist(p[k-1],p[k]);
        if(p.size()<2||total<o.minLength){alias[to]=from;
            if(warnings)warnings->push_back(L"dionica "+std::to_wstring(d.number)+L" ("+d.from+L" \u2013 "+d.to+L") nema du\u017Einu: \u010Dvor "+d.to+L" spojen sa prethodnim");
            for(size_t k=0;k<d.elements.size();++k){const tvsPressure::Element& x=e[d.elements[k]];
                if(warnings&&x.parts.size()==1&&x.parts[0].kind==tvsPressure::Equipment)warnings->push_back(L"oprema "+x.parts[0].equipment.tag+L" nije izvezena (dionica bez du\u017Eine)");}
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
        // oprema of the dionica: on the piece nearest to its centre; a terminal at the free end
        for(size_t k=0;k<d.elements.size();++k){const tvsPressure::Element& x=e[d.elements[k]];
            if(x.parts.size()!=1||x.parts[0].kind!=tvsPressure::Equipment||x.at.empty())continue;const tvsEquipment::Record& rec=x.parts[0].equipment;
            XY c;for(size_t j=0;j<x.at.size();++j){c.x+=x.at[j].x/x.at.size();c.y+=x.at[j].y/x.at.size();}
            size_t best=out.size()-1;double pos=1,off=-1;
            if(!rec.terminal())for(size_t j=1;j<p.size();++j){double dj=0;const double t=along(p[j-1],p[j],c,dj);if(off<0||dj<off){off=dj;best=out.size()-pieces+j-1;pos=t;}}
            std::wstring w;Equip q=equipment(rec,x.flow,exhaust,tagOf(rec),warnings?&w:nullptr);q.pos=pos;out[best].equipment.push_back(q);
            if(warnings&&!w.empty())warnings->push_back(L"oprema "+(rec.tag.empty()?std::wstring(tvsEquipment::kindName(rec.kind)):rec.tag)+w);}
        // resetka na dnu with the closed duct end beyond it (no element on the straight port)
        {const int last=d.elements.back();const tvsPressure::Element& x=e[last];
         if(x.parts.size()==1&&x.parts[0].kind==tvsPressure::BottomTap&&x.at.size()==3&&x.ports.size()==3&&x.parts[0].length>=o.minLength&&(x.entry==0||x.entry==1)){
            bool on=false;for(size_t k=0;k<e.size();++k)if(e[k].parent==last&&e[k].via==1-x.entry)on=true;
            const XY& a=x.at[x.entry];const XY& b=x.at[1-x.entry];const double l=dist(a,b);
            if(!on&&l>0){Segment c;c.from=to;c.to="Z"+std::to_string(++closed);c.id=idOf(exhaust,static_cast<int>(out.size())+1);
                c.opis="TVS dionica "+std::to_string(d.number)+" - zatvoren kraj (resetka na dnu)";
                const tvsPressure::Section& s=x.ports[1-x.entry];c.round=s.round;if(s.round)c.d=s.a;else{c.w=s.a;c.h=s.b;}c.q=0;
                c.dx=(b.x-a.x)/l*x.parts[0].length/1000.0;c.dy=(b.y-a.y)/l*x.parts[0].length/1000.0;out.push_back(c);
                if(warnings)warnings->push_back(L"zatvoren kraj kanala iza re\u0161etke na dnu (dionica "+std::to_wstring(d.number)+L"): izvezen bez protoka, DuctPressDrop ga vidi kao T komad sa zatvorenim krakom");}}}
    }
    if(zero&&warnings)warnings->push_back(L"dionica bez protoka (Q = 0): "+std::to_wstring(zero));
    return out;
}
}

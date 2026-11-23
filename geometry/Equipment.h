#pragma once
// Oprema na kanalskoj mrezi (regulatori protoka, PP klapne, CAV / VAV, prigusivaci, resetke,
// difuzori, kape ...): an ordinary AutoCAD block that TVS_OPREMA links into the duct network.
// The block keeps its graphics; TVS stores one Record in its extension dictionary
// (TVS_Ventilation_ARX/EquipmentStore.h). Pure C++ (no AutoCAD), ASCII; tests/test_equipment.cpp.
//
// Contract shared by the link command, the network readers (TVS_PAD, TVS_PROTOK, TVS_DPD_IZVOZ,
// TVS_DIM_PRATI, TVS_PROVJERA), the properties dialog and the specification:
// - ports: 1 (terminal: resetka, difuzor, kapa ... at a duct end) or 2 (inline, between two
//   duct pieces). Port points are in drawing units, WCS, on the duct axis, z = axis level.
//   `dir` is the unit plan direction pointing OUT of the block (towards the duct).
// - mmPerUnit: drawing units of the stored points (mm per unit, 1 / 10 / 1000) - the record
//   stays right when the block is copied into a drawing with other units (review proposal 2).
// - sizes a, b in mm (b = 0 when round; a = D), flows in m3/h, pressures in Pa.
// - loss model: fixed dp, zeta (on the velocity pressure of the duct at the port), or a
//   catalogue point dp_n at q_n scaled with (q / q_n)^2; terminals (1 port) get no safety factor.
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
namespace tvsEquipment {

enum Kind {Other=0,FlowDamper=1,FireDamper=2,Cav=3,Vav=4,Silencer=5,Grille=6,Diffuser=7,Hood=8,Louvre=9};
enum Model {NoLoss=0,FixedDp=1,Zeta=2,Nominal=3};
// How a terminal (1 port) sits on the network:
//  AtEnd  - port on a free end (duct, flex, stub of a tee branch, vertical outlet of an elbow);
//  Bottom - grille in the bottom face of a duct, drawn over the duct: NO duct end, port[0] is the
//           tap point on the duct axis (z = axis level); the duct is not cut. The network treats it
//           as a zero-length branch with the neck size (na x nb / round) - tee loss by the KanCalc /
//           Idelchik tables (branch = neck, straight = duct) plus the grille's own loss
//           (user decision 2026-11-23).
enum Mount {AtEnd=0,Bottom=1};

inline int kindCount(){return 10;}
// Names shown in the dialogs / specification (BCS, \u escapes keep this file ASCII).
inline const wchar_t* kindName(int k){
    switch(k){
    case FlowDamper:return L"Regulator protoka";
    case FireDamper:return L"Protivpo\u017earna klapna";
    case Cav:return L"CAV regulator";
    case Vav:return L"VAV regulator";
    case Silencer:return L"Prigu\u0161iva\u010d buke";
    case Grille:return L"Re\u0161etka";
    case Diffuser:return L"Difuzor";
    case Hood:return L"Krovna kapa";
    case Louvre:return L"Protivki\u0161na \u017ealuzina";
    default:return L"Ostala oprema";
    }
}
// Tag prefix for new tags (RP-01, PPK-01 ...).
inline const wchar_t* tagPrefix(int k){
    switch(k){
    case FlowDamper:return L"RP";case FireDamper:return L"PPK";case Cav:return L"CAV";case Vav:return L"VAV";
    case Silencer:return L"PB";case Grille:return L"R";case Diffuser:return L"D";case Hood:return L"KK";
    case Louvre:return L"PZ";default:return L"OP";
    }
}
// Kinds that normally sit at a free end (1 port). A record may still differ (ports decides).
inline bool terminalKind(int k){return k==Grille||k==Diffuser||k==Hood||k==Louvre;}

struct Port {double x,y,z,dx,dy;Port():x(0),y(0),z(0),dx(1),dy(0){}};

struct Record {
    int version;          // 1
    int kind;             // Kind
    std::wstring tag;     // oznaka, e.g. PPK-01 (may be empty)
    std::wstring note;    // opis / tip iz kataloga (may be empty)
    int ports;            // 1 or 2
    Port port[2];
    double mmPerUnit;     // units of port[] (mm per drawing unit)
    bool round;
    double a,b;           // mm; round: a = D, b = 0
    double lengthMm;      // length of the block along the duct axis (2 ports; 0 for terminals)
    int model;            // Model
    double dp;            // Pa (FixedDp)
    double zeta;          // - (Zeta)
    double dpNominal;     // Pa at qNominal (Nominal)
    double qNominal;      // m3/h (Nominal)
    double qTerminal;     // m3/h, terminals: design flow (0 = not set, TVS_PROTOK)
    int mount;            // Mount (terminals)
    double neckA,neckB;   // mm, grille connection (Bottom); 0 = take the duct size
    bool neckRound;
    Record():version(1),kind(Other),ports(2),mmPerUnit(1),round(false),a(0),b(0),lengthMm(0),
             model(NoLoss),dp(0),zeta(0),dpNominal(0),qNominal(0),qTerminal(0),mount(AtEnd),neckA(0),neckB(0),neckRound(false){}
    bool terminal()const{return ports==1;}
};

// Pressure loss [Pa] at flow q [m3/h]; pv = velocity pressure [Pa] of the duct at the port
// (only the Zeta model needs it). Never negative.
inline double loss(const Record& r,double q,double pv){
    double v=0;
    switch(r.model){
    case FixedDp:v=r.dp;break;
    case Zeta:v=r.zeta*pv;break;
    case Nominal:v=r.qNominal>0?r.dpNominal*(q/r.qNominal)*(q/r.qNominal):r.dpNominal;break;
    default:v=0;
    }
    return v>0&&std::isfinite(v)?v:0;
}

// ---- text form (one string in the XRecord, also used for the per-block defaults) ----
// "k=v;k=v;..." with '.' decimals; ';' '=' '\' inside text are escaped with '\'.
inline std::wstring escape(const std::wstring& s){
    std::wstring o;for(size_t i=0;i<s.size();++i){if(s[i]==L';'||s[i]==L'='||s[i]==L'\\')o+=L'\\';o+=s[i];}return o;
}
inline std::wstring num(double v){
    if(!std::isfinite(v))v=0;
    if(v==0)v=0.0;
    wchar_t buf[64];std::swprintf(buf,sizeof(buf)/sizeof(buf[0]),L"%.17g",v);
    std::wstring s(buf);for(size_t i=0;i<s.size();++i)if(s[i]==L',')s[i]=L'.';return s;
}
inline std::wstring format(const Record& r){
    std::wstring s;
    auto put=[&](const wchar_t* k,const std::wstring& v){s+=k;s+=L'=';s+=v;s+=L';';};
    put(L"v",num(r.version));put(L"kind",num(r.kind));put(L"tag",escape(r.tag));put(L"note",escape(r.note));
    put(L"ports",num(r.ports));put(L"mpu",num(r.mmPerUnit));put(L"round",num(r.round?1:0));
    put(L"a",num(r.a));put(L"b",num(r.b));put(L"len",num(r.lengthMm));
    put(L"model",num(r.model));put(L"dp",num(r.dp));put(L"zeta",num(r.zeta));
    put(L"dpn",num(r.dpNominal));put(L"qn",num(r.qNominal));put(L"q",num(r.qTerminal));
    if(r.mount!=AtEnd)put(L"mount",num(r.mount));
    if(r.neckA>0){put(L"na",num(r.neckA));put(L"nb",num(r.neckB));put(L"nr",num(r.neckRound?1:0));}
    for(int k=0;k<2&&k<r.ports;++k){const wchar_t* p=k==0?L"p0":L"p1";const Port& t=r.port[k];
        put(p,num(t.x)+L","+num(t.y)+L","+num(t.z)+L","+num(t.dx)+L","+num(t.dy));}
    return s;
}
inline bool number(const std::wstring& t,double& v){
    if(t.empty())return false;
    wchar_t* end=NULL;v=std::wcstod(t.c_str(),&end);return end&&*end==0&&std::isfinite(v);
}
// False when the text is not a record of a known version (r then holds what was read).
inline bool parse(const std::wstring& s,Record& r){
    r=Record();bool any=false;size_t i=0;
    while(i<s.size()){
        std::wstring key,val;bool inVal=false;
        for(;i<s.size();++i){wchar_t c=s[i];
            if(c==L'\\'&&i+1<s.size()){(inVal?val:key)+=s[++i];continue;}
            if(c==L'='&&!inVal){inVal=true;continue;}
            if(c==L';'){++i;break;}
            (inVal?val:key)+=c;}
        if(key.empty())continue;
        double v=0;const bool n=number(val,v);
        if(key==L"v"&&n){r.version=static_cast<int>(v);any=true;}
        else if(key==L"kind"&&n)r.kind=static_cast<int>(v);
        else if(key==L"tag")r.tag=val;
        else if(key==L"note")r.note=val;
        else if(key==L"ports"&&n)r.ports=v==1?1:2;
        else if(key==L"mpu"&&n&&v>0)r.mmPerUnit=v;
        else if(key==L"round"&&n)r.round=v!=0;
        else if(key==L"a"&&n)r.a=v;
        else if(key==L"b"&&n)r.b=v;
        else if(key==L"len"&&n)r.lengthMm=v;
        else if(key==L"model"&&n)r.model=static_cast<int>(v);
        else if(key==L"dp"&&n)r.dp=v;
        else if(key==L"zeta"&&n)r.zeta=v;
        else if(key==L"dpn"&&n)r.dpNominal=v;
        else if(key==L"qn"&&n)r.qNominal=v;
        else if(key==L"q"&&n)r.qTerminal=v;
        else if(key==L"mount"&&n)r.mount=v==Bottom?Bottom:AtEnd;
        else if(key==L"na"&&n)r.neckA=v;
        else if(key==L"nb"&&n)r.neckB=v;
        else if(key==L"nr"&&n)r.neckRound=v!=0;
        else if(key==L"p0"||key==L"p1"){
            double c[5]={0,0,0,1,0};int k=0;std::wstring part;
            for(size_t j=0;j<=val.size()&&k<5;++j){if(j==val.size()||val[j]==L','){double x=0;if(number(part,x))c[k]=x;++k;part.clear();}else part+=val[j];}
            Port& p=r.port[key==L"p0"?0:1];p.x=c[0];p.y=c[1];p.z=c[2];p.dx=c[3];p.dy=c[4];
        }
    }
    if(r.kind<0||r.kind>=kindCount())r.kind=Other;
    if(r.model<NoLoss||r.model>Nominal)r.model=NoLoss;
    return any&&r.version==1;
}
}

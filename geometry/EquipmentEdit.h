#pragma once
// TVS_OPREMA_SVOJSTVA (svojstva povezanih blokova opreme) and the specification of linked blocks:
// pure logic over geometry/Equipment.h records. Pure C++ (no AutoCAD / MFC), ASCII;
// tests/test_equipment_edit.cpp. The dialog is TVS_Ventilation_ARX/EquipmentDialog.h.
//
// - nextTag: the next free tag of a prefix in the drawing = largest number used + 1
//   (PPK-01, PPK-02 -> PPK-03; a gap is not reused), at least 2 digits, as wide as the widest.
// - Values: what the dialog shows for one or more records. A field whose records differ shows
//   differentText() (kind / model: -1) and stays unchanged unless the user edits it - the same
//   rule as the Svojstva dialog (numbers compared as numbers, tvsNumeric::differs).
// - apply: the edited fields onto one record; validate: the record after the edit.
// - typeDefault / applyType: "Primijeni na sve blokove ovog naziva" (per block name defaults).
#include "Equipment.h"
#include "NumericField.h"
#include <cwctype>
#include <utility>
#include <string>
#include <vector>
namespace tvsEquipmentEdit {

inline const wchar_t* differentText(){return L"Razli\u010dito";}

inline std::wstring trim(const std::wstring& s){
    std::wstring::size_type a=0,b=s.size();
    while(a<b&&std::iswspace(s[a]))++a;
    while(b>a&&std::iswspace(s[b-1]))--b;
    return s.substr(a,b-a);
}

// ---- tags ----------------------------------------------------------------------------------
// Number of `tag` when it is <prefix>-<digits> (prefix without case, spaces around ignored),
// else -1. `width` (or NULL) = count of digits.
inline int tagNumber(const std::wstring& tag,const std::wstring& prefix,int* width){
    const std::wstring t=trim(tag);
    if(prefix.empty()||t.size()<prefix.size()+2)return -1;
    for(std::wstring::size_type k=0;k<prefix.size();++k)
        if(std::towupper(t[k])!=std::towupper(prefix[k]))return -1;
    if(t[prefix.size()]!=L'-')return -1;
    int n=0,digits=0;
    for(std::wstring::size_type k=prefix.size()+1;k<t.size();++k){
        if(t[k]<L'0'||t[k]>L'9')return -1;
        if(n>99999)return -1;
        n=n*10+static_cast<int>(t[k]-L'0');++digits;
    }
    if(width)*width=digits;
    return n;
}
inline std::wstring nextTag(const std::wstring& prefix,const std::vector<std::wstring>& tags){
    int top=0,width=2;
    for(size_t k=0;k<tags.size();++k){int w=0;const int n=tagNumber(tags[k],prefix,&w);
        if(n<0)continue;
        if(n>top)top=n;
        if(w>width)width=w;
    }
    wchar_t buf[32];std::swprintf(buf,sizeof(buf)/sizeof(buf[0]),L"%0*d",width,top+1);
    return prefix+L"-"+buf;
}

// ---- texts ---------------------------------------------------------------------------------
// Size in mm: "400x250", round: <diameter sign>"250" (sign: L"%%c" in AutoCAD text, U+2300 in Excel).
inline std::wstring sizeText(const tvsEquipment::Record& r,const std::wstring& diameterSign){
    if(!(r.a>0))return std::wstring();
    if(r.round||!(r.b>0))return diameterSign+tvsNumeric::formatG(r.a,6);
    return tvsNumeric::formatG(r.a,6)+L"x"+tvsNumeric::formatG(r.b,6);
}
// Name in the specification: the note (opis / tip) when set, else the kind name.
inline std::wstring specName(const tvsEquipment::Record& r){
    const std::wstring n=trim(r.note);
    return n.empty()?std::wstring(tvsEquipment::kindName(r.kind)):n;
}

// ---- curve text (model Curve): one point per line or ';', "Q dp" (space, tab, ':' or '/'),
// decimal comma or point - also what Excel or a plot digitizer pastes. Points sorted by Q.
inline std::wstring formatCurve(const std::vector<std::pair<double,double> >& c){
    std::wstring s;for(size_t i=0;i<c.size();++i){if(i)s+=L"; ";s+=tvsNumeric::formatG(c[i].first,10)+L" "+tvsNumeric::formatG(c[i].second,10);}
    return s;
}
// Empty error = ok. Every point: Q > 0, dp >= 0, Q not repeated; at least one point.
inline std::wstring parseCurve(const std::wstring& text,std::vector<std::pair<double,double> >& out){
    out.clear();std::wstring line;
    for(size_t i=0;i<=text.size();++i){
        const wchar_t c=i<text.size()?text[i]:L'\n';
        if(c==L'\n'||c==L'\r'||c==L';'){
            std::vector<std::wstring> w;std::wstring cur;
            for(size_t j=0;j<=line.size();++j){const wchar_t d=j<line.size()?line[j]:L' ';
                if(d==L' '||d==L'\t'||d==L':'||d==L'/'){if(!cur.empty())w.push_back(cur);cur.clear();}else cur+=d;}
            line.clear();
            if(w.empty())continue;
            double q=0,p=0;
            if(w.size()!=2||!tvsNumeric::parse(w[0],q)||!tvsNumeric::parse(w[1],p))return L"Kriva: ta\u010dka mora biti \u201eQ \u0394p\u201c (npr. 750 50).";
            if(!(q>0))return L"Kriva: Q mora biti ve\u0107i od 0.";
            if(p<0)return L"Kriva: \u0394p ne smije biti negativan.";
            out.push_back(std::make_pair(q,p));
            if(out.size()>20)return L"Kriva: najvi\u0161e 20 ta\u010daka.";
        }else line+=c;
    }
    for(size_t i=1;i<out.size();++i)for(size_t k=i;k>0&&out[k].first<out[k-1].first;--k)std::swap(out[k],out[k-1]);
    for(size_t i=1;i<out.size();++i)if(tvsNumeric::same(out[i].first,out[i-1].first))return L"Kriva: isti protok je upisan dva puta.";
    return std::wstring();
}

// ---- dialog values -------------------------------------------------------------------------
struct Values {
    int kind;                         // -1 = different
    std::wstring tag,note;            // differentText() = different
    int model;                        // -1 = different
    std::wstring dp,zeta,dpn,qn,q;    // exact number text, differentText(); q empty = not terminals
    std::wstring curve;               // formatCurve or differentText()
    int ports;                        // 1, 2; 0 = mixed
    std::wstring size,length;         // read-only info (mm) or differentText()
    Values():kind(-1),model(-1),ports(0){}
    bool terminals()const{return ports==1;}
};
inline Values merge(const std::vector<tvsEquipment::Record>& rs){
    Values v;if(rs.empty())return v;
    const tvsEquipment::Record& f=rs[0];
    bool kind=true,tag=true,note=true,model=true,dp=true,zeta=true,dpn=true,qn=true,q=true,ports=true,size=true,length=true,curve=true;
    for(size_t k=1;k<rs.size();++k){const tvsEquipment::Record& r=rs[k];
        kind=kind&&r.kind==f.kind;tag=tag&&trim(r.tag)==trim(f.tag);note=note&&trim(r.note)==trim(f.note);
        model=model&&r.model==f.model;
        dp=dp&&tvsNumeric::same(r.dp,f.dp);zeta=zeta&&tvsNumeric::same(r.zeta,f.zeta);
        dpn=dpn&&tvsNumeric::same(r.dpNominal,f.dpNominal);qn=qn&&tvsNumeric::same(r.qNominal,f.qNominal);
        q=q&&tvsNumeric::same(r.qTerminal,f.qTerminal);ports=ports&&r.ports==f.ports;curve=curve&&r.curve==f.curve;
        size=size&&sizeText(r,L"D")==sizeText(f,L"D");length=length&&tvsNumeric::same(r.lengthMm,f.lengthMm);}
    const std::wstring diff=differentText();
    v.kind=kind?f.kind:-1;v.tag=tag?trim(f.tag):diff;v.note=note?trim(f.note):diff;v.model=model?f.model:-1;
    v.dp=dp?tvsNumeric::exact(f.dp):diff;v.zeta=zeta?tvsNumeric::exact(f.zeta):diff;
    v.dpn=dpn?tvsNumeric::exact(f.dpNominal):diff;v.qn=qn?tvsNumeric::exact(f.qNominal):diff;
    v.ports=ports?f.ports:0;v.curve=curve?formatCurve(f.curve):diff;
    v.q=!v.terminals()?std::wstring():q?tvsNumeric::exact(f.qTerminal):diff;
    v.size=size?sizeText(f,L"\u00d8"):diff;
    v.length=length?tvsNumeric::formatG(f.lengthMm,6):diff;
    return v;
}
// A typed number field is acceptable: a number, untouched, or empty (= no change).
inline bool typedOk(const std::wstring& before,const std::wstring& after){
    double v=0;const std::wstring t=trim(after);
    return t.empty()||t==before||tvsNumeric::parse(t,v);
}
// The fields changed in the dialog (after) onto one record; true = the record changed.
inline bool apply(const Values& before,const Values& after,tvsEquipment::Record& r){
    bool any=false;const std::wstring diff=differentText();
    if(after.kind>=0&&after.kind<tvsEquipment::kindCount()&&after.kind!=before.kind&&r.kind!=after.kind){r.kind=after.kind;any=true;}
    const std::wstring tag=trim(after.tag),note=trim(after.note);
    if(tag!=diff&&tag!=before.tag&&trim(r.tag)!=tag){r.tag=tag;any=true;}
    if(note!=diff&&note!=before.note&&trim(r.note)!=note){r.note=note;any=true;}
    if(after.model>=tvsEquipment::NoLoss&&after.model<=tvsEquipment::Nominal&&after.model!=before.model&&r.model!=after.model){r.model=after.model;any=true;}
    double x=0;
    if(tvsNumeric::differs(before.dp.c_str(),after.dp.c_str(),x)&&!tvsNumeric::same(r.dp,x)){r.dp=x;any=true;}
    if(tvsNumeric::differs(before.zeta.c_str(),after.zeta.c_str(),x)&&!tvsNumeric::same(r.zeta,x)){r.zeta=x;any=true;}
    if(tvsNumeric::differs(before.dpn.c_str(),after.dpn.c_str(),x)&&!tvsNumeric::same(r.dpNominal,x)){r.dpNominal=x;any=true;}
    if(tvsNumeric::differs(before.qn.c_str(),after.qn.c_str(),x)&&!tvsNumeric::same(r.qNominal,x)){r.qNominal=x;any=true;}
    if(r.terminal()&&tvsNumeric::differs(before.q.c_str(),after.q.c_str(),x)&&!tvsNumeric::same(r.qTerminal,x)){r.qTerminal=x;any=true;}
    {const std::wstring t=trim(after.curve);std::vector<std::pair<double,double> > c;
     if(t!=diff&&t!=trim(before.curve)&&parseCurve(t,c).empty()&&c!=r.curve){r.curve=c;any=true;}}
    return any;
}
// Empty = the record is valid, else the message (BCS).
inline std::wstring validate(const tvsEquipment::Record& r){
    if(r.dp<0)return L"\u0394p ne smije biti negativan.";
    if(r.zeta<0)return L"\u03b6 ne smije biti negativan.";
    if(r.dpNominal<0)return L"\u0394p_n ne smije biti negativan.";
    if(r.qNominal<0)return L"Q_n ne smije biti negativan.";
    if(r.model==tvsEquipment::Nominal&&!(r.qNominal>0))return L"Model \u0394p_n pri Q_n: Q_n mora biti ve\u0107i od 0.";
    if(r.model==tvsEquipment::Curve&&r.curve.empty())return L"Model Kriva Q\u2013\u0394p: upi\u0161ite bar jednu ta\u010dku (Q \u0394p).";
    if(r.qTerminal<0)return L"Protok ne smije biti negativan.";
    return std::wstring();
}

// ---- per block name ------------------------------------------------------------------------
// Record stored as the default of a block name: without ports, tag and flow.
inline tvsEquipment::Record typeDefault(const tvsEquipment::Record& r){
    tvsEquipment::Record d=r;d.tag.clear();d.qTerminal=0;d.port[0]=tvsEquipment::Port();d.port[1]=tvsEquipment::Port();
    return d;
}
// Kind, note and the loss model with its values from `from` onto `to`; true = `to` changed.
inline bool applyType(const tvsEquipment::Record& from,tvsEquipment::Record& to){
    const bool same=to.kind==from.kind&&to.note==from.note&&to.model==from.model&&to.dp==from.dp&&to.zeta==from.zeta&&
                    to.dpNominal==from.dpNominal&&to.qNominal==from.qNominal&&to.curve==from.curve;
    if(same)return false;
    to.kind=from.kind;to.note=from.note;to.model=from.model;to.dp=from.dp;to.zeta=from.zeta;
    to.dpNominal=from.dpNominal;to.qNominal=from.qNominal;to.curve=from.curve;
    return true;
}
}

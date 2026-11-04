#pragma once
// TVS_TK_TIP (faza 1) and Racva while drawing (faza 2): turns an existing TVS T-komad into TK-O "sa ogrankom" or TK-G
// "gacasti" (or back to the plain T). Defaults: centre-line radius = branch width (outer
// 1.5 w), collars 25 mm as in the dynamic block "T komad". LengthPl becomes l + R - w so the TVS ports
// match the drawn fitting; connected TVS ducts follow the ports (TeeNeighbours.h, faza 3).
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "TeeNeighbours.h"
#include <algorithm>
#include <cwchar>
namespace tvsTeeCommand {
inline bool askWidth(const wchar_t* prompt,double defaultMm,double& valueMm){
    valueMm=defaultMm;wchar_t text[200];swprintf_s(text,L"%ls <%.0f>: ",prompt,defaultMm);
    acedInitGet(RSG_NOZERO|RSG_NONEG,NULL);double v=0;const int rc=acedGetReal(text,&v);
    if(rc==RTNONE)return true;if(rc!=RTNORM)return false;valueMm=v;return true;
}
// Last type chosen (TVS_TK_TIP and the Racva option while drawing): default of the next prompt.
inline int& lastType(){static int t=tvsTee::Straight;return t;}
inline const wchar_t* typeKeyword(int t){return t==tvsTee::Branch?L"Ogranak":t==tvsTee::Pants?L"Gacasti":L"Ravni";}
// Reverse the run direction (TK-O: inlet A <-> outlet C side; TK-G: left <-> right branch).
// Ports are matched by place, so the duct on each end takes the width of the end now there.
inline void flip(TVS_WYE* w){
    const tvsTeeFit::Ports before=tvsTeeFit::ports(w);
    w->assertWriteEnabled();w->Vectpr=-w->Vectpr;w->recordGraphicsModified();
    tvsTeeFit::Ports after=tvsTeeFit::ports(w);
    if(after.ok){std::swap(after.p[0],after.p[1]);std::swap(after.w[0],after.w[1]);}
    tvsTeeFit::fit(w,before,after);
}
// Type of the T komad `id`: prompt with keywords, Okreni flips and asks again. Connected TVS
// ducts follow the ports (TeeNeighbours.h). Returns false on cancel.
inline bool chooseType(const AcDbObjectId& id){
    AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk){acutPrintf(L"\nIzabrani objekat nije TVS T-komad.");return false;}
    if(w->ThisRoundpr||w->ThisRoundotv){acutPrintf(L"\nTK-O/TK-G su za pravougaone kanale.");return true;}
    const double f=tvsUnits::factor(w.object());
    for(;;){
        tvsTeeData::Record current;tvsTeeData::read(w.object(),current);
        const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
        const int def=current.type?current.type:lastType();
        wchar_t prompt[200];swprintf_s(prompt,L"\nTip T komada [Ogranak (TK-O)/Gacasti (TK-G)/Ravni/oKreni] <%ls>: ",typeKeyword(def));
        acedInitGet(0,L"Ogranak Gacasti Ravni oKreni");ACHAR kw[32]=L"";
        const int rc=acedGetKword(prompt,kw);
        if(rc==RTNONE)wcscpy_s(kw,typeKeyword(def));else if(rc!=RTNORM)return false;
        if(_wcsicmp(kw,L"oKreni")==0||_wcsicmp(kw,L"Okreni")==0){flip(w.object());acutPrintf(L"\nT komad je okrenut.");continue;}
        tvsTeeData::Record r;
        if(_wcsicmp(kw,L"Ravni")==0){
            lastType()=tvsTee::Straight;
            if(current.type==0)return true;
            if(tvsTeeData::write(w.object(),r)==Acad::eOk){acutPrintf(L"\nT-komad je ponovo obican (TK0).");w->recordGraphicsModified();tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));}
            return true;
        }
        const bool pants=_wcsicmp(kw,L"Gacasti")==0;
        r.type=pants?tvsTee::Pants:tvsTee::Branch;r.l=tvsTeeData::collarMm()/f;
        // TK-O: run = TVS run (A in -> C out), branch = TVS branch. TK-G: inlet = TVS branch, outlets = TVS run.
        const double w1=pants?w->SizeApr:w->SizeAotv;
        const double defSecond=current.type==r.type&&current.second>0?current.second:w->SizeApr;
        double secondMm=0;
        if(!askWidth(pants?L"\nSirina lijevog ogranka, mm":L"\nSirina izlaza prolaza C, mm",defSecond*f,secondMm))return false;
        r.second=secondMm/f;r.radius2=pants?tvsTee::defaultRadius(r.second):0;
        if(pants&&!tvsTee::pantsFeasible(w->SizeAotv,w1,r.second,r.l)){
            acutPrintf(L"\nGacasti T komad nije moguc: ogranci su preuski za ulaz %.0f mm (potrebno priblizno lijevi + desni >= ulaz).",w->SizeAotv*f);continue;}
        w->assertWriteEnabled();w->LengthPl=pants?tvsTee::pantsLengthPl(r.l,w1,r.second):tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1);
        const Acad::ErrorStatus es=tvsTeeData::write(w.object(),r);
        if(es!=Acad::eOk){acutPrintf(L"\nTip T komada nije upisan (%ls).",acadErrorStatusText(es));return true;}
        w->recordGraphicsModified();lastType()=r.type;
        tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));
        acutPrintf(L"\n%ls: R = %.0f mm, prikljucak %.0f mm.",
            pants?L"Gacasti T komad TK-G":L"T komad sa ogrankom TK-O",tvsTee::defaultRadius(w1)*f,tvsTeeData::collarMm());
        // offer Okreni once more (inlet side / left-right), Enter keeps it
        acedInitGet(0,L"oKreni");ACHAR k2[32]=L"";
        const int rc2=acedGetKword(L"\n[oKreni] <Gotovo>: ",k2);
        if(rc2==RTNORM){flip(w.object());acutPrintf(L"\nT komad je okrenut.");}
        return true;
    }
}
inline void setType(){
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite TVS T-komad: ",name,pick)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    chooseType(id);
}
// Faza 2, Racva (TVS_DRAW) and TVS_CONNECTW: the type is asked before the T-komad is made, so
// it is created directly as TK-O / TK-G. main = width of the run duct (TVS SizeApr), branch =
// width of the branch duct (TVS SizeAotv), drawing units. false = cancel.
inline bool askType(int& type,double& second,double main,double branch,double f){
    for(;;){
        const int def=lastType();
        wchar_t prompt[200];swprintf_s(prompt,L"\nTip T komada [Ogranak (TK-O)/Gacasti (TK-G)/Ravni] <%ls>: ",typeKeyword(def));
        acedInitGet(0,L"Ogranak Gacasti Ravni");ACHAR kw[32]=L"";
        const int rc=acedGetKword(prompt,kw);
        if(rc==RTNONE)wcscpy_s(kw,typeKeyword(def));else if(rc!=RTNORM)return false;
        type=_wcsicmp(kw,L"Ogranak")==0?tvsTee::Branch:_wcsicmp(kw,L"Gacasti")==0?tvsTee::Pants:tvsTee::Straight;
        second=0;if(type==tvsTee::Straight){lastType()=type;return true;}
        double mm=0;
        if(!askWidth(type==tvsTee::Pants?L"\nSirina lijevog ogranka, mm":L"\nSirina izlaza prolaza C, mm",main*f,mm))return false;
        second=mm/f;
        if(type==tvsTee::Pants&&!tvsTee::pantsFeasible(branch,main,second,tvsTeeData::collarMm()/f)){
            acutPrintf(L"\nGacasti T komad nije moguc: ogranci su preuski za ulaz %.0f mm (potrebno priblizno lijevi + desni >= ulaz).",branch*f);continue;}
        return true;
    }
}
// Applies a type chosen by askType to the T-komad just made; connected ducts follow (faza 3).
inline void applyType(const AcDbObjectId& id,int type,double second){
    if(id.isNull()||type==tvsTee::Straight)return;
    AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk||w->ThisRoundpr||w->ThisRoundotv)return;
    const double f=tvsUnits::factor(w.object());
    const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
    tvsTeeData::Record r;r.type=type;r.l=tvsTeeData::collarMm()/f;r.second=second;
    const bool pants=type==tvsTee::Pants;r.radius2=pants?tvsTee::defaultRadius(second):0;
    const double w1=pants?w->SizeApr:w->SizeAotv;
    w->LengthPl=pants?tvsTee::pantsLengthPl(r.l,w1,second):tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1);
    if(tvsTeeData::write(w.object(),r)!=Acad::eOk)return;
    w->recordGraphicsModified();lastType()=type;
    tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));
    acedInitGet(0,L"oKreni");ACHAR k2[32]=L"";
    if(acedGetKword(L"\n[oKreni] <Gotovo>: ",k2)==RTNORM){flip(w.object());acutPrintf(L"\nT komad je okrenut.");}
}
}

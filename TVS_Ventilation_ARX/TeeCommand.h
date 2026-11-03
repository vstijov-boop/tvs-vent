#pragma once
// TVS_TK_TIP (faza 1): turns an existing TVS T-komad into TK-O "sa ogrankom" or TK-G
// "gacasti" (or back to the plain T). Defaults: centre-line radius = branch width (outer
// 1.5 w), collars 25 mm as in the dynamic block "T komad". LengthPl becomes l + R - w so the TVS ports
// match the drawn fitting; connected TVS ducts follow the ports (TeeNeighbours.h, faza 3).
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "TeeNeighbours.h"
namespace tvsTeeCommand {
inline bool askWidth(const wchar_t* prompt,double defaultMm,double& valueMm){
    valueMm=defaultMm;wchar_t text[200];swprintf_s(text,L"%ls <%.0f>: ",prompt,defaultMm);
    acedInitGet(RSG_NOZERO|RSG_NONEG,NULL);double v=0;const int rc=acedGetReal(text,&v);
    if(rc==RTNONE)return true;if(rc!=RTNORM)return false;valueMm=v;return true;
}
inline void setType(){
    ads_name name;ads_point pick;if(acedEntSel(L"\nIzaberite TVS T-komad: ",name,pick)!=RTNORM)return;
    AcDbObjectId id;if(acdbGetObjectId(id,name)!=Acad::eOk)return;
    AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk){acutPrintf(L"\nIzabrani objekat nije TVS T-komad.");return;}
    if(w->ThisRoundpr||w->ThisRoundotv){acutPrintf(L"\nTK-O/TK-G su za pravougaone kanale.");return;}
    tvsTeeData::Record current;tvsTeeData::read(w.object(),current);
    const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
    acedInitGet(0,L"Ogranak Gacasti Ravni");ACHAR kw[32]=L"";
    const int rc=acedGetKword(L"\nTip T komada [Ogranak (TK-O)/Gacasti (TK-G)/Ravni] <Ogranak>: ",kw);
    if(rc!=RTNORM&&rc!=RTNONE)return;
    const double f=tvsUnits::factor(w.object());tvsTeeData::Record r;
    if(_wcsicmp(kw,L"Ravni")==0){if(tvsTeeData::write(w.object(),r)==Acad::eOk){acutPrintf(L"\nT-komad je ponovo obican (TK0).");w->recordGraphicsModified();tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));}return;}
    const bool pants=_wcsicmp(kw,L"Gacasti")==0;
    r.type=pants?tvsTee::Pants:tvsTee::Branch;r.l=tvsTeeData::collarMm()/f;
    // TK-O: run = TVS run (A in -> C out), branch = TVS branch. TK-G: inlet = TVS branch, outlets = TVS run.
    const double w1=pants?w->SizeApr:w->SizeAotv;
    double secondMm=0;
    if(!askWidth(pants?L"\nSirina lijevog ogranka, mm":L"\nSirina izlaza prolaza C, mm",(pants?w->SizeApr:w->SizeApr)*f,secondMm))return;
    r.second=secondMm/f;r.radius2=pants?tvsTee::defaultRadius(r.second):0;
    if(pants&&!tvsTee::pantsFeasible(w->SizeAotv,w1,r.second,r.l)){
        acutPrintf(L"\nGacasti T komad nije moguc: ogranci su preuski za ulaz %.0f mm (potrebno priblizno lijevi + desni >= ulaz).",w->SizeAotv*f);return;}
    w->assertWriteEnabled();w->LengthPl=pants?tvsTee::pantsLengthPl(r.l,w1,r.second):tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1);
    const Acad::ErrorStatus es=tvsTeeData::write(w.object(),r);
    if(es!=Acad::eOk){acutPrintf(L"\nTip T komada nije upisan (%ls).",acadErrorStatusText(es));return;}
    w->recordGraphicsModified();
    tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));
    acutPrintf(L"\n%ls: R = %.0f mm, prikljucak %.0f mm.",
        pants?L"Gacasti T komad TK-G":L"T komad sa ogrankom TK-O",tvsTee::defaultRadius(w1)*f,tvsTeeData::collarMm());
    (void)current;
}
}

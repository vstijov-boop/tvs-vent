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
// Type prompts: the bracket list holds plain keywords only (AutoCAD offers them for a click and
// for dynamic input; "Ogranak (TK-O)" was sent as "TK-O" and refused). TK-O / TK-G / SR5-3 typed
// are taken too and mean Ogranak / Gacasti / Stucna.
#define TVS_TEE_KEYWORDS L"Ogranak Gacasti Stucna Ravni TK-O TK-G SR5-3"
inline void typeLegend(bool round){acutPrintf(round?L"\nStucna = SR5-3 (45), Ravni = obicni T komad.":L"\nOgranak = TK-O, Gacasti = TK-G, Stucna = SR5-3, Ravni = obicni T komad.");}
inline void typeAlias(ACHAR* kw,size_t n){
    if(_wcsicmp(kw,L"TK-O")==0)wcscpy_s(kw,n,L"Ogranak");
    else if(_wcsicmp(kw,L"TK-G")==0)wcscpy_s(kw,n,L"Gacasti");
    else if(_wcsicmp(kw,L"SR5-3")==0)wcscpy_s(kw,n,L"Stucna");
}
inline const wchar_t* typeKeyword(int t){return t==tvsTee::Branch?L"Ogranak":t==tvsTee::Pants?L"Gacasti":t==tvsTee::Tap?L"Stucna":L"Ravni";}
// Stucna SR5-3: collar L = 0.25 x branch width, at least 75 mm (drawing units).
inline double tapLength(double branch,double f){return tvsTee::tapLength(branch,tvsTeeData::tapMinimumMm()/f);}
// Reverse the run direction (TK-O: inlet A <-> outlet C side; TK-G: left <-> right branch).
// Ports are matched by place, so the duct on each end takes the width of the end now there.
inline void flip(TVS_WYE* w){
    const tvsTeeFit::Ports before=tvsTeeFit::ports(w);
    w->assertWriteEnabled();w->Vectpr=-w->Vectpr;w->recordGraphicsModified();
    tvsTeeFit::Ports after=tvsTeeFit::ports(w);
    if(after.ok){std::swap(after.p[0],after.p[1]);std::swap(after.w[0],after.w[1]);}
    tvsTeeFit::fit(w,before,after);
}
// The T komad as it is now, on screen before a question: closed (so AutoCAD redraws it), graphics
// flushed. While the user answers, nothing of it is open.
inline void showNow(const AcDbObjectId& id){
    {AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()==Acad::eOk)w->recordGraphicsModified();}
    if(actrTransactionManager){actrTransactionManager->queueForGraphicsFlush();actrTransactionManager->flushGraphics();}
    acedUpdateDisplay();
}
inline bool flipById(const AcDbObjectId& id){
    AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk)return false;flip(w.object());return true;
}
// oKreni until Enter, each time with the drawn result on screen (highlighted).
inline void orient(const AcDbObjectId& id,const wchar_t* prompt,const wchar_t* done){
    for(;;){
        showNow(id);tvsMove::StretchHighlight mark;mark.add(id);
        acedInitGet(0,L"oKreni");ACHAR k[32]=L"";
        if(acedGetKword(prompt,k)!=RTNORM)return;
        mark.clear();if(flipById(id))acutPrintf(L"%ls",done);
    }
}
// Type of the T komad `id`: prompt with keywords, Okreni flips and asks again. Connected TVS
// ducts follow the ports (TeeNeighbours.h). The T komad is closed and redrawn before every
// question, so each answer is seen at once. Returns false on cancel.
inline bool chooseType(const AcDbObjectId& id){
    bool round=false;double f=1,sizeApr=0,sizeAotv=0;
    {AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk){acutPrintf(L"\nIzabrani objekat nije TVS T-komad.");return false;}
     if(w->ThisRoundpr||w->SizeBpr<=0){acutPrintf(L"\nTK-O/TK-G i stucna su za pravougaoni glavni kanal.");return true;}
     round=w->ThisRoundotv;   // okrugli ogranak: samo stucna (iste mjere: L = 0.25 x D, min. 75 mm)
     f=tvsUnits::factor(w.object());sizeApr=w->SizeApr;sizeAotv=w->SizeAotv;}
    for(;;){
        tvsTeeData::Record current;
        {AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return false;tvsTeeData::read(w.object(),current);}
        const int def=current.type?current.type:lastType();
        const int shown=round&&def!=tvsTee::Tap&&def!=tvsTee::Straight?tvsTee::Tap:def;
        wchar_t prompt[200];
        typeLegend(round);
        if(round)swprintf_s(prompt,L"\nTip T komada, okrugli ogranak [Stucna/Ravni/oKreni] <%ls>: ",typeKeyword(shown));
        else swprintf_s(prompt,L"\nTip T komada [Ogranak/Gacasti/Stucna/Ravni/oKreni] <%ls>: ",typeKeyword(shown));
        showNow(id);
        int rc;ACHAR kw[32]=L"";
        {tvsMove::StretchHighlight mark;mark.add(id);
         acedInitGet(0,round?L"Stucna Ravni oKreni SR5-3":TVS_TEE_KEYWORDS L" oKreni");rc=acedGetKword(prompt,kw);}
        if(rc==RTNORM)typeAlias(kw,32);
        if(rc==RTNONE)wcscpy_s(kw,typeKeyword(shown));else if(rc!=RTNORM)return false;
        if(_wcsicmp(kw,L"oKreni")==0||_wcsicmp(kw,L"Okreni")==0){if(flipById(id))acutPrintf(L"\nT komad je okrenut.");continue;}
        tvsTeeData::Record r;
        if(_wcsicmp(kw,L"Ravni")==0){
            lastType()=tvsTee::Straight;
            if(current.type==0)return true;
            AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk)return false;
            const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
            if(tvsTeeData::write(w.object(),r)==Acad::eOk){acutPrintf(L"\nT-komad je ponovo obican (TK0).");w->recordGraphicsModified();tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));}
            return true;
        }
        if(_wcsicmp(kw,L"Stucna")==0){
            const double L=tapLength(sizeAotv,f);
            {AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk)return false;
             const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
             r.type=tvsTee::Tap;r.l=L;w->LengthPl=L;
             const Acad::ErrorStatus es=tvsTeeData::write(w.object(),r);
             if(es!=Acad::eOk){acutPrintf(L"\nTip T komada nije upisan (%ls).",acadErrorStatusText(es));return true;}
             w->recordGraphicsModified();lastType()=r.type;
             tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));}
            acutPrintf(round?L"\nStucna 45, okrugli ogranak: L = %.0f mm (0.25 x D %.0f, min. %.0f).":L"\nStucna 45 SR5-3: L = %.0f mm (0.25 x %.0f, min. %.0f).",L*f,sizeAotv*f,tvsTeeData::tapMinimumMm());
            orient(id,L"\nStrana kosog ulaza (uzvodno) [oKreni] <Gotovo>: ",L"\nStucna je okrenuta.");
            return true;
        }
        const bool pants=_wcsicmp(kw,L"Gacasti")==0;
        r.type=pants?tvsTee::Pants:tvsTee::Branch;r.l=tvsTeeData::collarMm()/f;
        // TK-O: run = TVS run (A in -> C out), branch = TVS branch. TK-G: inlet = TVS branch, outlets = TVS run.
        const double w1=pants?sizeApr:sizeAotv;
        const double defSecond=current.type==r.type&&current.second>0?current.second:sizeApr;
        double secondMm=0;
        if(!askWidth(pants?L"\nSirina lijevog ogranka, mm":L"\nSirina izlaza prolaza C, mm",defSecond*f,secondMm))return false;
        r.second=secondMm/f;r.radius2=pants?tvsTee::defaultRadius(r.second):0;
        if(pants&&!tvsTee::pantsFeasible(sizeAotv,w1,r.second,r.l)){
            acutPrintf(L"\nGacasti T komad nije moguc: ogranci su preuski za ulaz %.0f mm (potrebno priblizno lijevi + desni >= ulaz).",sizeAotv*f);continue;}
        {AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk)return false;
         const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
         w->LengthPl=pants?tvsTee::pantsLengthPl(r.l,w1,r.second):tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1);
         const Acad::ErrorStatus es=tvsTeeData::write(w.object(),r);
         if(es!=Acad::eOk){acutPrintf(L"\nTip T komada nije upisan (%ls).",acadErrorStatusText(es));return true;}
         w->recordGraphicsModified();lastType()=r.type;
         tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));}
        acutPrintf(L"\n%ls: R = %.0f mm, prikljucak %.0f mm.",
            pants?L"Gacasti T komad TK-G":L"T komad sa ogrankom TK-O",tvsTee::defaultRadius(w1)*f,tvsTeeData::collarMm());
        orient(id,L"\n[oKreni] <Gotovo>: ",L"\nT komad je okrenut.");   // inlet side / left-right, Enter keeps it
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
inline bool askType(int& type,double& second,double main,double branch,double f,bool roundBranch=false){
    if(roundBranch){   // okrugli ogranak na pravougaonom kanalu: stucna (iste mjere kao SR5-3) ili ravni T
        const int def=lastType()==tvsTee::Straight?tvsTee::Straight:tvsTee::Tap;
        typeLegend(true);
        wchar_t prompt[200];swprintf_s(prompt,L"\nTip T komada, okrugli ogranak [Stucna/Ravni] <%ls>: ",typeKeyword(def));
        acedInitGet(0,L"Stucna Ravni SR5-3");ACHAR kw[32]=L"";
        const int rc=acedGetKword(prompt,kw);
        if(rc==RTNORM)typeAlias(kw,32);
        if(rc==RTNONE)wcscpy_s(kw,typeKeyword(def));else if(rc!=RTNORM)return false;
        type=_wcsicmp(kw,L"Stucna")==0?tvsTee::Tap:tvsTee::Straight;second=0;lastType()=type;return true;
    }
    for(;;){
        const int def=lastType();
        typeLegend(false);
        wchar_t prompt[200];swprintf_s(prompt,L"\nTip T komada [Ogranak/Gacasti/Stucna/Ravni] <%ls>: ",typeKeyword(def));
        acedInitGet(0,TVS_TEE_KEYWORDS);ACHAR kw[32]=L"";
        const int rc=acedGetKword(prompt,kw);
        if(rc==RTNORM)typeAlias(kw,32);
        if(rc==RTNONE)wcscpy_s(kw,typeKeyword(def));else if(rc!=RTNORM)return false;
        type=_wcsicmp(kw,L"Ogranak")==0?tvsTee::Branch:_wcsicmp(kw,L"Gacasti")==0?tvsTee::Pants:_wcsicmp(kw,L"Stucna")==0?tvsTee::Tap:tvsTee::Straight;
        second=0;if(type==tvsTee::Straight||type==tvsTee::Tap){lastType()=type;return true;}
        double mm=0;
        if(!askWidth(type==tvsTee::Pants?L"\nSirina lijevog ogranka, mm":L"\nSirina izlaza prolaza C, mm",main*f,mm))return false;
        second=mm/f;
        if(type==tvsTee::Pants&&!tvsTee::pantsFeasible(branch,main,second,tvsTeeData::collarMm()/f)){
            acutPrintf(L"\nGacasti T komad nije moguc: ogranci su preuski za ulaz %.0f mm (potrebno priblizno lijevi + desni >= ulaz).",branch*f);continue;}
        return true;
    }
}
// Applies a type chosen by askType to the T-komad just made; connected ducts follow (faza 3).
// flow (optional): drawing direction of the main duct = air flow; the stucna's 45 deg entry is
// put on the upstream side (oKreni still offered).
inline void applyType(const AcDbObjectId& id,int type,double second,const AcGeVector3d& flow=AcGeVector3d::kIdentity){
    if(id.isNull()||type==tvsTee::Straight)return;
    const bool pants=type==tvsTee::Pants,tap=type==tvsTee::Tap;bool round=false;double f=1,L=0;
    {AcDbObjectPointer<TVS_WYE> w(id,AcDb::kForWrite);
     if(w.openStatus()!=Acad::eOk||w->ThisRoundpr||(w->ThisRoundotv&&type!=tvsTee::Tap))return;
     f=tvsUnits::factor(w.object());round=w->ThisRoundotv;
     const tvsTeeFit::Ports before=tvsTeeFit::ports(w.object());
     tvsTeeData::Record r;r.type=type;r.l=tvsTeeData::collarMm()/f;r.second=second;r.radius2=pants?tvsTee::defaultRadius(second):0;
     const double w1=pants?w->SizeApr:w->SizeAotv;
     if(tap){r.l=tapLength(w1,f);r.second=0;}
     w->LengthPl=tap?r.l:pants?tvsTee::pantsLengthPl(r.l,w1,second):tvsTee::lengthPl(r.l,tvsTee::defaultRadius(w1),w1);
     if(tvsTeeData::write(w.object(),r)!=Acad::eOk)return;
     w->recordGraphicsModified();lastType()=type;L=r.l;
     tvsTeeFit::fit(w.object(),before,tvsTeeFit::ports(w.object()));
     if(tap&&!flow.isZeroLength()){AcGeVector3d u=w->Vectpr,d=flow;u.z=d.z=0;if(u.dotProduct(d)<0)flip(w.object());}}
    if(tap)acutPrintf(round?L"\nStucna 45, okrugli ogranak: L = %.0f mm.":L"\nStucna 45 SR5-3: L = %.0f mm.",L*f);
    orient(id,tap?L"\nStrana kosog ulaza (uzvodno) [oKreni] <Gotovo>: ":L"\n[oKreni] <Gotovo>: ",L"\nT komad je okrenut.");
}
}

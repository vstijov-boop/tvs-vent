#pragma once
// Krstasti komad / XK (plan 26, user 2026-11-28): TVS_CONNECTW - first the duct, then a T komad
// instead of the second duct. A ravni T komad (TK0) becomes the krstasti komad (Soko "X", e.g.
// 200/100/100), a kosa racva TK (round, 45 deg) the XK (Soko "Spiro XK"): the second branch is
// the mirror of the first across the main axis (TVS_Ventilation_DBX/TeeCross.h), with the size of
// the picked duct; the duct's near end goes to the new port. The duct must lie on the axis of
// that branch (in plan, beyond the port), at the axis elevation of the T komad, and must not be
// wider than the first branch (the run stays as it is). Otherwise a message and nothing changes.
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/TeeCross.h"
#include "../TVS_Ventilation_DBX/TeeOpenEnds.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include <cmath>
namespace tvsTeeCrossCmd {
inline bool refuse(const wchar_t* text){acutPrintf(L"\nKrstasti komad: %ls",text);return false;}
inline bool connect(const AcDbObjectId& pipeId,const AcDbObjectId& teeId){
    AcDbObjectPointer<TVS_WYE> w(teeId,AcDb::kForWrite);
    if(w.openStatus()!=Acad::eOk)return refuse(L"T komad se ne može otvoriti (zaključan sloj?).");
    AcDbObjectPointer<TVS_Pipe> p(pipeId,AcDb::kForWrite);
    if(p.openStatus()!=Acad::eOk)return refuse(L"kanal se ne može otvoriti (zaključan sloj?).");
    TVS_WYE* t=w.object();TVS_Pipe* d=p.object();
    const double f=tvsUnits::factor(t),tol=2.0/f;
    {AcGePoint3d xp;AcGeVector3d xd;double xa,xb;bool xr;
     if(t->crossPort(xp,xd,xa,xb,xr))return refuse(L"T komad već ima drugi ogranak (krstasti komad / XK).");}
    if(d->DuctType==DuctTypeFlex)return refuse(L"fleksibilni kanal se ne priključuje na krstasti komad – nacrtajte krut kanal.");
    if(tvsTeeOpenEnds::read(t))return refuse(L"T komad rešetke na boku kanala ne može biti krstasti.");
    tvsTeeData::Record r;const bool shaped=tvsTeeData::read(t,r);
    const bool lateral=shaped&&r.type==tvsTee::Tap&&r.neck>0&&t->ThisRoundpr&&t->ThisRoundotv;
    if(shaped&&!lateral)return refuse(L"samo ravni T komad (TK0) postaje krstasti komad, a kosa račva TK postaje XK (TK-O, TK-G, štucna i Y45: ne).");
    if(lateral&&!d->ThisRound)return refuse(L"XK ima okrugle ogranke – izaberite okrugli kanal.");
    const double a2=d->SizeA,b2=d->ThisRound?0:d->SizeB;
    if(a2>t->SizeAotv+1e-9){wchar_t s[200];swprintf_s(s,L"drugi ogranak (%.0f mm) ne može biti širi od prvog (%.0f mm) – spojite širi kanal prvi.",a2*f,t->SizeAotv*f);return refuse(s);}
    if(!t->ThisRoundpr&&!d->ThisRound&&b2>t->SizeBpr+1e-9)return refuse(L"visina drugog ogranka je veća od visine kanala.");
    if(std::fabs(d->Elev-t->Elev)>tol){wchar_t s[200];swprintf_s(s,L"kanal nije na koti ose T komada (%.0f / %.0f mm).",d->Elev*f,t->Elev*f);return refuse(s);}
    tvsTeeCross::Record x;x.a=a2;x.b=b2;x.round=d->ThisRound;
    if(tvsTeeCross::write(t,&x)!=Acad::eOk)return refuse(L"zapis krstastog komada nije upisan.");
    AcGePoint3d xp;AcGeVector3d xd;double xa=0,xb=0;bool xr=true;
    if(!t->crossPort(xp,xd,xa,xb,xr)){tvsTeeCross::write(t,NULL);return refuse(L"drugi ogranak se ne može nacrtati za ove dimenzije.");}
    // the duct on the axis of the new branch, beyond its port (plan)
    const AcGePoint3d e0=d->FirstPoint,e1=d->LastPoint;
    auto along=[&](const AcGePoint3d& q){return (q.x-xp.x)*xd.x+(q.y-xp.y)*xd.y;};
    auto off=[&](const AcGePoint3d& q){return std::fabs((q.x-xp.x)*xd.y-(q.y-xp.y)*xd.x);};
    const bool firstNear=along(e0)<along(e1);const AcGePoint3d nearEnd=firstNear?e0:e1,farEnd=firstNear?e1:e0;
    if(off(e0)>tol||off(e1)>tol){tvsTeeCross::write(t,NULL);
        return refuse(lateral?L"kanal nije na osi drugog ogranka (pod 45°, preslikan prvi ogranak preko ose kanala).":L"kanal nije na osi drugog ogranka (naspram prvog, okomito na kanal).");}
    if(!(along(farEnd)>tol)){tvsTeeCross::write(t,NULL);return refuse(L"kanal je sa pogrešne strane T komada.");}
    const double gap=along(nearEnd);
    if(gap<-(t->SizeApr/2+t->LengthPl)){tvsTeeCross::write(t,NULL);return refuse(L"kanal ulazi u T komad – skratite ga ili izaberite drugi.");}
    const AcGePoint3d to(xp.x,xp.y,nearEnd.z);
    const Acad::ErrorStatus es=firstNear?d->put_FirstPoint(to):d->put_Lastpoint(to);
    if(es!=Acad::eOk){tvsTeeCross::write(t,NULL);return refuse(L"kraj kanala se ne može pomjeriti.");}
    d->recordGraphicsModified();t->recordGraphicsModified();
    acutPrintf(lateral?L"\nKosa račva TK -> XK: drugi ogranak Ø%.0f.":L"\nT komad -> krstasti komad: drugi ogranak %.0f.",a2*f);
    if(std::fabs(gap)>tol)acutPrintf(L" Kraj kanala pomjeren na priključak (%.0f mm).",-gap*f);
    return true;
}
}

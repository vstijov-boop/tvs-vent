#pragma once
// Krstasti komad / XK (plan 26, user 2026-11-28): TVS_CONNECTW - first the duct, then a T komad
// instead of the second duct. A ravni T komad (TK0) becomes the krstasti komad (Soko "X", e.g.
// 200/100/100), a kosa racva TK (round, 45 deg) the XK (Soko "Spiro XK"): the second branch is
// the mirror of the first across the main axis (TVS_Ventilation_DBX/TeeCross.h), with the size of
// the picked duct; the duct's near end goes to the new port. The duct must lie on the axis of
// that branch (in plan, beyond the port), at the axis elevation of the T komad, and must not be
// wider than the first branch (the run stays as it is). XK: a duct perpendicular to the main gets a
// koljeno 45 on the second branch (user 28. 11., as the first branch with TVS_CONNECTW Upravni).
// Otherwise a message and nothing changes.
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
    const AcGePoint3d e0=d->FirstPoint,e1=d->LastPoint;
    AcGeVector3d dd(e1.x-e0.x,e1.y-e0.y,0),u=t->Vectpr;u.z=0;
    if(dd.length()<1e-9||u.length()<1e-9){tvsTeeCross::write(t,NULL);return refuse(L"kanal nema dužinu.");}
    dd.normalize();u.normalize();
    // XK and a duct perpendicular to the main: koljeno 45 on the second branch (as TVS_CONNECTW
    // Upravni / kosa racva with its koljeno) - the duct axis through the koljeno exit.
    const bool elbow=lateral&&std::fabs(dd.dotProduct(u))<1e-3;
    // Not on the axis of the koljeno exit (user 28. 11.): a 45 deg duct between the branch and the
    // koljeno takes the difference - the perpendicular duct may lie farther down the main, never closer.
    AcGeVector3d axis=xd;AcGePoint3d start=xp,elbowAt=xp;double T=0,Ld=0;
    if(elbow){axis=xd-u*xd.dotProduct(u);axis.normalize();
        double radius=globalTapRadiusConst;
        if(globalRadiusTypeRound==RadiusTypeRound_RadiusVariable)radius=a2*globalTapRadiusVariableParameter;
        else if(globalRadiusTypeRound==RadiusTypeRound_RadiusSpecific)radius=a2*f<355?a2:a2/2;
        T=(radius+a2/2)*std::tan(3.14159265358979323846/8);
        const double xu=xd.dotProduct(u),c=(e0.x-xp.x)*u.x+(e0.y-xp.y)*u.y;
        if(std::fabs(xu)<1e-9){tvsTeeCross::write(t,NULL);return refuse(L"drugi ogranak nije pod uglom prema kanalu.");}
        Ld=c/xu-T;
        if(Ld<-tol){tvsTeeCross::write(t,NULL);wchar_t s[240];
            swprintf_s(s,L"okomiti kanal je preblizu – njegova osa treba da bude bar %.0f mm dalje niz kanal (drugi ogranak 45° + koljeno 45°).",-Ld*std::fabs(xu)*f);
            return refuse(s);}
        if(Ld<tol)Ld=0;
        elbowAt=xp+xd*Ld;start=elbowAt+xd*T+axis*T;}
    auto along=[&](const AcGePoint3d& q){return (q.x-start.x)*axis.x+(q.y-start.y)*axis.y;};
    auto side=[&](const AcGePoint3d& q){return (q.x-start.x)*axis.y-(q.y-start.y)*axis.x;};
    const bool firstNear=along(e0)<along(e1);const AcGePoint3d nearEnd=firstNear?e0:e1,farEnd=firstNear?e1:e0;
    if(std::fabs(side(e0))>tol||std::fabs(side(e1))>tol){tvsTeeCross::write(t,NULL);
        return refuse(lateral?L"kanal nije ni na osi drugog ogranka (45°) ni okomit na kanal kroz koljeno 45°.":L"kanal nije na osi drugog ogranka (naspram prvog, okomito na kanal).");}
    if(!(along(farEnd)>tol)){tvsTeeCross::write(t,NULL);return refuse(L"kanal je sa pogrešne strane T komada ili je prekratak.");}
    if(!elbow&&along(nearEnd)<-(t->SizeApr/2+t->LengthPl)){tvsTeeCross::write(t,NULL);return refuse(L"kanal ulazi u T komad – skratite ga ili izaberite drugi.");}
    const double gap=along(nearEnd),elev=d->Elev;
    double likeA=d->SizeA,likeB=d->SizeB;const bool likeRound=d->ThisRound,like1D=d->This1D;   // for the 45 deg duct
    const AcDbObjectId layerId=d->layerId();const AcCmColor colour=d->color();const AcDb::LineWeight weight=d->lineWeight();
    const AcDbObjectId linetypeId=d->linetypeId();const double ltScale=d->linetypeScale();const int ductType=d->getDuctType();
    const bool wipe=d->get_Wipeout(),grani=d->get_Grani();const double flow=d->get_Flow();
    w.close();p.close();   // planElbow opens the T komad itself
    if(elbow&&Ld>0){   // the 45 deg duct from the branch to the koljeno
        AcGePoint3d a(xp.x,xp.y,nearEnd.z),b(elbowAt.x,elbowAt.y,nearEnd.z);
        TVS_Pipe* np=TVS_Pipe::add_new(a,b,likeA,likeB,like1D,likeRound);
        const AcDbObjectId nid=np?np->objectId():AcDbObjectId::kNull;
        AcDbObjectPointer<TVS_Pipe> q(nid,AcDb::kForWrite);
        if(q.openStatus()!=Acad::eOk){AcDbObjectPointer<TVS_WYE> back(teeId,AcDb::kForWrite);if(back.openStatus()==Acad::eOk)tvsTeeCross::write(back.object(),NULL);
            return refuse(L"kanal pod 45° nije napravljen – T komad ostaje kako je bio.");}
        q->put_Elevation(elev);q->setDuctType(ductType);q->put_Wipeout(wipe);q->put_Grani(grani);q->put_Flow(flow);
        q->setLayer(layerId);q->setColor(colour);q->setLineWeight(weight);q->setLinetype(linetypeId);q->setLinetypeScale(ltScale);
        q->recordGraphicsModified();}
    if(elbow){
        tvsTeeDialog::Section sec;sec.a=a2;sec.b=0;sec.round=true;AcGePoint3d exit;AcGeVector3d out;
        const double turn=(xd.x*axis.y-xd.y*axis.x)>0?3.14159265358979323846/4:-3.14159265358979323846/4;
        if(!tvsTeeDialog::planElbow(teeId,elbowAt,xd,turn,sec,elev,f,exit,out)){
            AcDbObjectPointer<TVS_WYE> back(teeId,AcDb::kForWrite);if(back.openStatus()==Acad::eOk)tvsTeeCross::write(back.object(),NULL);
            return refuse(L"koljeno 45° nije napravljeno – T komad ostaje kako je bio.");}
        start=exit;}
    {AcDbObjectPointer<TVS_Pipe> q(pipeId,AcDb::kForWrite);
     const AcGePoint3d to(start.x,start.y,nearEnd.z);
     if(q.openStatus()!=Acad::eOk||(firstNear?q->put_FirstPoint(to):q->put_Lastpoint(to))!=Acad::eOk)
        acutPrintf(L"\nKrstasti komad: kraj kanala nije pomjeren (zaključan sloj?) – spojite ga ručno.");
     else q->recordGraphicsModified();}
    acutPrintf(lateral?L"\nKosa račva TK -> XK: drugi ogranak Ø%.0f%ls.":L"\nT komad -> krstasti komad: drugi ogranak %.0f%ls.",a2*f,elbow?L" + koljeno 45°":L"");
    if(Ld>0)acutPrintf(L" Kanal pod 45°: %.0f mm.",Ld*f);
    if(std::fabs(gap)>tol)acutPrintf(L" Kraj kanala pomjeren na priključak (%.0f mm).",-gap*f);
    return true;
}
}

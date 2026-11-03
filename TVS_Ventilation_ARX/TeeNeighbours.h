#pragma once
// T komad faza 3: after TVS_TK_TIP or the Svojstva dialog changed a TK-O / TK-G, the TVS duct
// (TVS_Pipe) connected to each port follows it: its end moves to the drawn face (TK-G narrower
// branch with a fixed collar) and its width becomes the port width (TK-O prolaz C, TK-G left
// branch, back to Dpr for a plain T). At the duct's other end a width step to the next element
// gets a TVS transition (or an existing transition there takes the new width). Neighbours
// other than a duct directly on the T komad port are only reported.
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../TVS_Ventilation_DBX/TVS_TRANS.h"
#include "TVSMoveCommand.h"
#include "dbsymtb.h"
#include "dbobjptr.h"
#include <cmath>
#include <vector>
namespace tvsTeeFit {
struct Ports {bool ok;AcGePoint3d p[3];double w[3];Ports():ok(false){for(int k=0;k<3;++k)w[k]=0;}};
// Ports in the TVS order: run start, run end, branch (TK-G: left branch, first branch, inlet).
inline Ports ports(const TVS_WYE* w){
    Ports r;if(!w)return r;
    if(w->teePorts(r.p,r.w)){r.ok=true;return r;}
    AcGeVector3d u=w->Vectpr,v=w->Vectotv;u.z=v.z=0;if(u.length()<1e-12||v.length()<1e-12)return r;
    u.normalize();v.normalize();const double half=w->LengthPl+w->SizeAotv/2;
    r.p[0]=w->Basepoint-u*half;r.p[1]=w->Basepoint+u*half;r.p[2]=w->Basepoint+v*(w->SizeApr/2+w->LengthPl);
    r.w[0]=r.w[1]=w->SizeApr;r.w[2]=w->SizeAotv;r.ok=true;return r;
}
inline double planDistance(const AcGePoint3d& a,const AcGePoint3d& b){return std::hypot(a.x-b.x,a.y-b.y);}
struct End {AcDbObjectId id;bool first;};
// TVS ducts with an end on `at` (plan, tolerance tol) in the space that owns the T komad.
inline std::vector<End> ductsAt(const TVS_WYE* w,const AcGePoint3d& at,double tol,int& others){
    std::vector<End> out;others=0;
    AcDbObjectPointer<AcDbBlockTableRecord> space(w->ownerId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return out;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return out;
    for(;!it->done();it->step()){
        AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id==w->objectId())continue;
        AcDbEntity* e=NULL;if(acdbOpenAcDbEntity(e,id,AcDb::kForRead)!=Acad::eOk)continue;
        if(TVS_Pipe* p=TVS_Pipe::cast(e)){
            if(planDistance(p->FirstPoint,at)<=tol){End x={id,true};out.push_back(x);}
            else if(planDistance(p->LastPoint,at)<=tol){End x={id,false};out.push_back(x);}
        }else if(TVS_WYE* t=TVS_WYE::cast(e)){
            const Ports q=ports(t);if(q.ok)for(int k=0;k<3;++k)if(planDistance(q.p[k],at)<=tol){++others;break;}
        }
        e->close();
    }
    delete it;return out;
}
// Other end of a duct whose width changed: the element there (any TVS port within tol)
// still has the old width. A transition there takes the new width on that side; otherwise a
// centred transition of transitionMm is inserted by shortening the duct.
// Transition length, mm: TVS_SETTINGS "Prelaz L" (set by the settings dialog; one value for
// all translation units, unlike the static globals of Settings.h). Default 300 mm.
inline double& transitionSetting(){static double mm=300;return mm;}
inline double transitionMm(){return transitionSetting()>0?transitionSetting():300;}
inline void farEnd(const AcDbObjectId& teeId,const AcDbObjectId& ductId,bool nearIsFirst,double tol,double f){
    AcGePoint3d E,N;double width=0,height=0,elev=0;bool oneD=false;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return;
     E=nearIsFirst?d->LastPoint:d->FirstPoint;N=nearIsFirst?d->FirstPoint:d->LastPoint;
     width=d->SizeA;height=d->SizeB;elev=d->Elev;oneD=d->This1D;}
    AcDbObjectId found;int count=0,port=-1;double otherWidth=0,otherHeight=0;bool trans=false,round=false;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return;
     AcDbObjectPointer<AcDbBlockTableRecord> space(d->ownerId(),AcDb::kForRead);d.close();if(space.openStatus()!=Acad::eOk)return;
     AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return;
     for(;!it->done();it->step()){
        AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id==ductId||id==teeId)continue;
        AcDbEntity* e=NULL;if(acdbOpenAcDbEntity(e,id,AcDb::kForRead)!=Acad::eOk)continue;
        tvsMove::Node n;
        if(tvsMove::describe(e,n))for(size_t j=0;j<n.ports.size()&&j<n.sections.size();++j)
            if(planDistance(tvsMove::acad(n.ports[j]),E)<=tol){
                ++count;found=id;port=static_cast<int>(j);otherWidth=n.sections[j].a;otherHeight=n.sections[j].b;
                round=n.sections[j].round;trans=TVS_TRANS::cast(e)!=NULL;break;}
        e->close();
     }
     delete it;}
    if(count==0||std::fabs(otherWidth-width)<=1e-9)return;               // free end or same width
    if(count>1||round){acutPrintf(L"\nDrugi kraj kanala: vise elemenata ili okrugli prikljucak; prilagodite rucno.");return;}
    if(trans){
        AcDbObjectPointer<TVS_TRANS> t(found,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)return;
        const Acad::ErrorStatus es=port==0?t->put_SizeAp1(width):t->put_SizeAp2(width);
        if(es==Acad::eOk){t->recordGraphicsModified();acutPrintf(L"\nDrugi kraj kanala: postojeci prelaz preuzima sirinu %.0f mm.",width*f);}
        return;
    }
    AcGeVector3d dir=E-N;dir.z=0;const double length=dir.length();const double lt=transitionMm()/f;
    if(length<lt+50.0/f){acutPrintf(L"\nDrugi kraj kanala: kanal je prekratak za prelaz %.0f mm; prilagodite rucno.",transitionMm());return;}
    dir.normalize();AcGePoint3d start=E-dir*lt;start.z=E.z;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForWrite);if(d.openStatus()!=Acad::eOk)return;
     const Acad::ErrorStatus es=nearIsFirst?d->put_Lastpoint(start):d->put_FirstPoint(start);
     if(es!=Acad::eOk){acutPrintf(L"\nDrugi kraj kanala: kanal nije skracen (%ls).",acadErrorStatusText(es));return;}
     d->recordGraphicsModified();}
    double a1=width,b1=height,a2=otherWidth,b2=otherHeight>0?otherHeight:height,len=lt;AcGeVector3d v=dir*lt;
    TVS_TRANS* t=TVS_TRANS::add_new(a1,b1,a2,b2,len,v,start,false,false,1,oneD);
    if(t&&!t->objectId().isNull()){
        AcDbObjectPointer<TVS_TRANS> o(t->objectId(),AcDb::kForWrite);
        if(o.openStatus()==Acad::eOk){
            AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()==Acad::eOk)o->setPropertiesFrom(d.object());
            o->put_Elevation(elev);o->recordGraphicsModified();}
        acutPrintf(L"\nDrugi kraj kanala: umetnut prelaz %.0f -> %.0f mm, duzine %.0f mm.",width*f,otherWidth*f,transitionMm());
    }
}
// `before` = ports before the change, `after` = ports of the changed T komad (open for write).
inline void fit(TVS_WYE* w,const Ports& before,const Ports& after){
    if(!w||!before.ok||!after.ok)return;
    const double f=tvsUnits::factor(w),tol=2.0/f;   // 2 mm in plan
    static const wchar_t* names[3]={L"pocetak prolaza",L"kraj prolaza",L"ogranak"};
    int moved=0;
    for(int k=0;k<3;++k){
        const bool shift=planDistance(before.p[k],after.p[k])>1e-9,resize=std::fabs(before.w[k]-after.w[k])>1e-9;
        if(!shift&&!resize)continue;
        int others=0;const std::vector<End> ends=ductsAt(w,before.p[k],tol,others);
        if(ends.size()!=1){
            if(ends.empty()&&others==0)acutPrintf(L"\nT komad, %ls: nema prikljucenog TVS kanala; susjedni element (koljeno, prelaz) prilagodite rucno.",names[k]);
            else if(ends.size()>1)acutPrintf(L"\nT komad, %ls: vise kanala na istom prikljucku; nije mijenjano.",names[k]);
            continue;
        }
        AcDbObjectPointer<TVS_Pipe> p(ends[0].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
        AcGePoint3d to=after.p[k];to.z=(ends[0].first?p->FirstPoint:p->LastPoint).z;
        Acad::ErrorStatus es=Acad::eOk;
        if(shift)es=ends[0].first?p->put_FirstPoint(to):p->put_Lastpoint(to);
        if(es==Acad::eOk&&resize)es=p->put_SizeA(after.w[k]);
        if(es!=Acad::eOk){acutPrintf(L"\nT komad, %ls: kanal nije prilagodjen (%ls).",names[k],acadErrorStatusText(es));continue;}
        p->recordGraphicsModified();++moved;
        if(resize){acutPrintf(L"\nT komad, %ls: kanal je sada sirine %.0f mm.",names[k],after.w[k]*f);
            p.close();farEnd(w->objectId(),ends[0].id,ends[0].first,tol,f);}
    }
    if(moved)acutPrintf(L"\nPrilagodjeno kanala uz T komad: %d.",moved);
}
}

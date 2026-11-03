#pragma once
// T komad faza 3: after TVS_TK_TIP or the Svojstva dialog changed a TK-O / TK-G, the TVS duct
// (TVS_Pipe) connected to each port follows it: its end moves to the drawn face (TK-G narrower
// branch with a fixed collar) and its width becomes the port width (TK-O prolaz C, TK-G left
// branch, back to Dpr for a plain T). Other neighbours (elbow, transition) are only reported.
#include "../TVS_Ventilation_DBX/TVS_WYE.h"
#include "../TVS_Ventilation_DBX/TVS_Pipe.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
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
        if(resize)acutPrintf(L"\nT komad, %ls: kanal je sada sirine %.0f mm; provjerite njegov drugi kraj.",names[k],after.w[k]*f);
    }
    if(moved)acutPrintf(L"\nPrilagodjeno kanala uz T komad: %d.",moved);
}
}

#pragma once
// TVS_KOLJENA_UGAO: elbows in plan whose angle is a little off a standard one (15 / 22,5 / 30 / 45 /
// 60 / 75 / 90 deg, less than 0,5 deg) get the standard angle back. Until 11/2026 an OK in Svojstva
// rounded the angle to two decimals of a radian (90 -> 89,954, 45 -> 45,26), so the elbow ends no
// longer met the ducts within the tolerances of TVS_MOVE / TVS_ALIGN / TVS_DIM_PRATI.
// The duct ends on the elbow ports follow the corrected ports; an elbow with another fitting on a
// moving port is left (listed), members of an etaz / obilazak are refused by the element.
#include "SizeFollow.h"
#include <cmath>
#include <vector>
namespace tvsElbowAngle {
inline double standard(double deg,double maxOff){
    static const double list[]={15,22.5,30,45,60,75,90};double best=-1,off=1e9;
    for(size_t i=0;i<sizeof(list)/sizeof(list[0]);++i){const double d=std::fabs(deg-list[i]);if(d<off){off=d;best=list[i];}}
    return off<=maxOff?best:-1;
}
struct End {AcDbObjectId duct;bool first;};
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db),tol=5.0/f;   // duct end within 5 mm of the old port
    ads_name ss;
    acutPrintf(L"\nIzaberite koljena (Enter = sva u crtežu): ");
    const int rc=acedSSGet(NULL,NULL,NULL,NULL,ss);
    if(rc==RTCAN)return;
    const bool all=rc!=RTNORM;if(all&&acedSSGet(L"X",NULL,NULL,NULL,ss)!=RTNORM)return;
    std::vector<AcDbObjectId> elbows,ducts,others;
    Int32 n=0;acedSSLength(ss,&n);
    for(Int32 i=0;i<n;++i){ads_name e;AcDbObjectId id;if(acedSSName(ss,i,e)!=RTNORM||acdbGetObjectId(id,e)!=Acad::eOk)continue;
        if(id.objectClass()&&id.objectClass()->isDerivedFrom(TVS_TAP::desc()))elbows.push_back(id);}
    acedSSFree(ss);
    {   // every duct end / fitting of the space (the elbows may be picked alone)
        AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForRead);
        AcDbBlockTableRecordIterator* it=NULL;
        if(space.openStatus()==Acad::eOk&&space->newIterator(it)==Acad::eOk){
            for(;!it->done();it->step()){AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||!id.objectClass())continue;
                if(id.objectClass()->isDerivedFrom(TVS_Pipe::desc()))ducts.push_back(id);
                else if(id.objectClass()->isDerivedFrom(TVS_TRANS::desc())||id.objectClass()->isDerivedFrom(TVS_WYE::desc())||id.objectClass()->isDerivedFrom(TVS_TAP::desc()))others.push_back(id);}
            delete it;}
    }
    int fixed=0,blocked=0,refused=0;
    for(size_t k=0;k<elbows.size();++k){
        tvsMove::Point was[2],now[2];double target=-1,before=0;
        {AcDbObjectPointer<TVS_TAP> t(elbows[k],AcDb::kForRead);if(t.openStatus()!=Acad::eOk)continue;
         if(t->Form!=Form_Direct||std::fabs(std::fabs(t->Normvector.z)-1)>1e-9)continue;   // plan elbows only
         before=t->Swectangle*180/M_PI;target=standard(before,0.5);
         if(target<0||std::fabs(target-before)<1e-7)continue;
         if(!tvsSizeFollow::elbowPortsFor(t.object(),t->SizeA,t->ThisRound,was))continue;}
        // what is on the ports: duct ends move with them, another fitting there blocks
        std::vector<End> ends;bool other=false;
        for(size_t j=0;j<others.size()&&!other;++j){if(others[j]==elbows[k])continue;
            AcDbObjectPointer<TVS_Entity> o(others[j],AcDb::kForRead);if(o.openStatus()!=Acad::eOk)continue;
            AcGePoint3d q[3];const int m=tvsPortGrip::points(o.object(),q);
            for(int i=0;i<m;++i)for(int p=0;p<2;++p)if(std::hypot(q[i].x-was[p].x,q[i].y-was[p].y)<=tol)other=true;}
        if(other){++blocked;continue;}
        for(size_t j=0;j<ducts.size();++j){AcDbObjectPointer<TVS_Pipe> d(ducts[j],AcDb::kForRead);if(d.openStatus()!=Acad::eOk)continue;
            for(int p=0;p<2;++p){
                if(std::hypot(d->FirstPoint.x-was[p].x,d->FirstPoint.y-was[p].y)<=tol){End e={ducts[j],true};ends.push_back(e);}
                if(std::hypot(d->LastPoint.x-was[p].x,d->LastPoint.y-was[p].y)<=tol){End e={ducts[j],false};ends.push_back(e);}}}
        {AcDbObjectPointer<TVS_TAP> t(elbows[k],AcDb::kForWrite);
         if(t.openStatus()!=Acad::eOk||t->put_Swectangle(target*M_PI/180)!=Acad::eOk){++refused;continue;}
         if(!tvsSizeFollow::elbowPortsFor(t.object(),t->SizeA,t->ThisRound,now)){t->put_Swectangle(before*M_PI/180);++refused;continue;}
         t->recordGraphicsModified();}
        for(size_t j=0;j<ends.size();++j){AcDbObjectPointer<TVS_Pipe> d(ends[j].duct,AcDb::kForWrite);if(d.openStatus()!=Acad::eOk)continue;
            const AcGePoint3d at=ends[j].first?d->FirstPoint:d->LastPoint;
            int p=std::hypot(at.x-was[0].x,at.y-was[0].y)<=std::hypot(at.x-was[1].x,at.y-was[1].y)?0:1;
            const AcGePoint3d to(now[p].x,now[p].y,at.z);
            if(ends[j].first)d->put_FirstPoint(to);else d->put_Lastpoint(to);d->recordGraphicsModified();}
        ++fixed;
        acutPrintf(L"\nKoljeno %.3f° → %g°.",before,target);
    }
    acutPrintf(L"\nIspravljeno koljena: %d%ls.",fixed,all?L" (cijeli crtež)":L"");
    if(blocked)acutPrintf(L"\nNije ispravljeno (na priključku je prelaz / T komad / koljeno): %d – ugao ispravite u Svojstvima.",blocked);
    if(refused)acutPrintf(L"\nNije ispravljeno (zaključan sloj ili dio etaža / obilaska): %d.",refused);
}
}

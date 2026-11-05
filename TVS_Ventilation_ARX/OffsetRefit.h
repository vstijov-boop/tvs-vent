#pragma once
// Etaz (TVS_OFFSET sklop: koljeno, kosi kanal, koljeno) follows a changed duct elevation.
// When the elevation of an ordinary duct joined to an etaz changes (Properties, Svojstva,
// TVS_MATCH ...), the etaz no longer meets it. TVS_ETAZ_KOTA (run automatically after the change)
// asks "Prilagoditi etaz?" and rebuilds the triple for the new difference with the same angle,
// radius and size: the elbow on the other side stays, the sloped piece and the elbow on the
// changed side follow (horizontal length of the etaz changes), the changed duct's end follows
// its elbow along the axis. Geometry: the same solver/adapter that validates the assembly.
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetService.h"
#include "BypassAssemblyData.h"
#include "../geometry/OffsetAdapter.h"
#include "../geometry/RoundElbow.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "acdocman.h"
#include "dbtrans.h"
#include <map>
#include <set>
#include <vector>
namespace tvsOffsetRefit {
// --- detection: ducts whose Elev changed (database reactor) ---
inline std::vector<AcDbObjectId>& pending(){static std::vector<AcDbObjectId> p;return p;}
inline std::map<AcDbObjectId,double>& opened(){static std::map<AcDbObjectId,double> m;return m;}
inline bool& posted(){static bool b=false;return b;}
inline void post(){
    if(posted()||pending().empty())return;
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(!doc)return;
    posted()=true;acDocManager->sendStringToExecute(doc,L"_TVS_ETAZ_KOTA\n",false,true,false);
}
inline bool plainDuct(const AcDbObject* o){return o&&o->isA()==TVS_Pipe::desc()&&!tvsOffsetData::marked(o);}
class DbReactor : public AcDbDatabaseReactor {
public:
    virtual void objectOpenedForModify(const AcDbDatabase*,const AcDbObject* o){
        if(o->isUndoing()||!plainDuct(o))return;
        opened()[o->objectId()]=TVS_Pipe::cast(o)->Elev;   // value at this open (each open/close pair)
    }
    virtual void objectModified(const AcDbDatabase*,const AcDbObject* o){
        std::map<AcDbObjectId,double>::iterator it=opened().find(o->objectId());if(it==opened().end())return;
        const double was=it->second;opened().erase(it);
        if(o->isUndoing()||!plainDuct(o))return;
        if(std::fabs(TVS_Pipe::cast(o)->Elev-was)<=1e-9)return;
        for(size_t i=0;i<pending().size();++i)if(pending()[i]==o->objectId())return;
        pending().push_back(o->objectId());
        // Properties palette: no command runs; inside a command: commandEnded posts.
        AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;
        if(doc&&doc->isQuiescent())post();
    }
    virtual void goodbye(const AcDbDatabase* db);
};
inline DbReactor& dbReactor(){static DbReactor r;return r;}
inline std::set<AcDbDatabase*>& attached(){static std::set<AcDbDatabase*> s;return s;}
inline void DbReactor::goodbye(const AcDbDatabase* db){attached().erase(const_cast<AcDbDatabase*>(db));}
inline void attach(AcDbDatabase* db){if(db&&attached().insert(db).second)db->addReactor(&dbReactor());}
inline void attachAll(){
    if(!acDocManager)return;AcApDocumentIterator* it=acDocManager->newAcApDocumentIterator();if(!it)return;
    for(;!it->done();it->step())if(it->document())attach(it->document()->database());
    delete it;
}
inline void detachAll(){
    for(std::set<AcDbDatabase*>::iterator i=attached().begin();i!=attached().end();++i)(*i)->removeReactor(&dbReactor());
    attached().clear();
}
// Editor commandEnded: queued changes from a command are settled by TVS_ETAZ_KOTA.
inline void commandEnded(const ACHAR* command){
    if(!command)return;
    if(_wcsicmp(command,L"TVS_ETAZ_KOTA")==0||_wcsicmp(command,L"U")==0||_wcsicmp(command,L"UNDO")==0||_wcsicmp(command,L"REDO")==0||_wcsicmp(command,L"MREDO")==0){
        if(_wcsicmp(command,L"TVS_ETAZ_KOTA")!=0)pending().clear();return;}
    post();
}
// --- geometry ---
struct Assembly {AcDbObjectId a,p,b;AcGePoint3d maA,maB;double elevA,elevB;};
inline AcGePoint3d rectMiddle(const TVS_TAP* tap){
    if(!tap->ThisRound)return tap->MiddlePoint;tvsRoundElbow::P ma,mc;
    tvsRoundElbow::ports(tvsRoundElbow::P(tap->MiddlePoint.x,tap->MiddlePoint.y),tap->Startvector.x,tap->Startvector.y,tap->Swectangle,tap->get_radius()+tap->SizeA/2,ma,mc);
    return AcGePoint3d(mc.x,mc.y,tap->MiddlePoint.z);
}
// Plan of the triple for elevations elevA (first elbow side) / elevB, anchored at the first
// elbow's middle; same inputs as tvsOffsetData::validateCore.
inline bool plan(const TVS_TAP* a,const TVS_Pipe* p,double elevA,double elevB,double f,tvsOffset::Result& g,tvsOffsetAdapter::Result& d){
    const double bend=p->ThisRound?p->SizeA:p->SizeB;
    const tvsOffset::Input in((elevB-elevA)*f,p->SizeA*f,bend*f,a->get_radius()*f,a->Swectangle,0);
    g=tvsOffset::solve(in);if(!g.hasGeometry())return false;
    const AcGeVector3d axis=p->LastPoint-p->FirstPoint;if(axis.length()<=0)return false;
    const AcGePoint3d m=rectMiddle(a);
    tvsOffsetAdapter::Placement place(tvsOffset::WorldPoint(m.x,m.y,m.z),std::atan2(axis.y,axis.x),f,elevA*f);
    d=tvsOffsetAdapter::adapt(in,g,place);return d.ready();
}
inline bool assemblies(AcDbDatabase* db,std::vector<Assembly>& out){
    out.clear();const double f=tvsUnits::factor(db);
    AcDbObjectPointer<AcDbBlockTableRecord> space(tvsOffsetProof::modelId(db),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* raw=NULL;if(space->newIterator(raw)!=Acad::eOk)return false;std::unique_ptr<AcDbBlockTableRecordIterator> it(raw);
    for(;!it->done();it->step()){
        AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk)continue;
        AcDbObjectPointer<TVS_TAP> a(id,AcDb::kForRead);if(a.openStatus()!=Acad::eOk)continue;
        tvsOffsetData::Record r;if(tvsOffsetData::read(a.object(),r)!=tvsOffsetData::Valid||r.role!=tvsOffsetData::FirstElbow)continue;
        if(tvsBypassData::marked(a.object()))continue;   // obilazak: its own parent geometry
        if(tvsOffsetData::validate(a.object())!=tvsOffsetData::Valid)continue;
        AcDbObjectPointer<TVS_Pipe> p(r.members[1],AcDb::kForRead);AcDbObjectPointer<TVS_TAP> b(r.members[2],AcDb::kForRead);
        if(p.openStatus()!=Acad::eOk||b.openStatus()!=Acad::eOk)continue;
        tvsOffset::Result g;tvsOffsetAdapter::Result d;
        if(!plan(a.object(),p.object(),a->get_Elevation(),b->get_Elevation(),f,g,d))continue;
        Assembly s;s.a=id;s.p=r.members[1];s.b=r.members[2];s.elevA=a->get_Elevation();s.elevB=b->get_Elevation();
        s.maA=AcGePoint3d(d.incomingLast.x,d.incomingLast.y,d.incomingLast.z);s.maB=AcGePoint3d(d.outgoingFirst.x,d.outgoingFirst.y,d.outgoingFirst.z);
        out.push_back(s);
    }
    return true;
}
inline bool nearXY(const AcGePoint3d& u,const AcGePoint3d& v,double tol){return std::hypot(u.x-v.x,u.y-v.y)<=tol;}
inline const wchar_t* why(tvsOffset::Status s){
    return s==tvsOffset::TooShort?L"razlika kota je premala za dva koljena":s==tvsOffset::NoChange?L"kote su jednake (etaz nije potreban; obrisite ga rucno)":L"nova geometrija nije moguca";
}
// Rebuild for the new elevation of side `side` (0 = first elbow, 2 = second elbow); the
// duct `duct` end `end` follows. One transaction.
inline bool refit(const Assembly& s,int side,double elevation,const AcDbObjectId& duct,int end,double f){
    AcTransaction* tr=actrTransactionManager->startTransaction();if(!tr)return false;
    bool ok=false;
    {
        tvsOffsetData::WriteScope scope;
        AcDbObject *oa=NULL,*op=NULL,*ob=NULL,*od=NULL;
        if(tr->getObject(oa,s.a,AcDb::kForWrite)==Acad::eOk&&tr->getObject(op,s.p,AcDb::kForWrite)==Acad::eOk
            &&tr->getObject(ob,s.b,AcDb::kForWrite)==Acad::eOk&&tr->getObject(od,duct,AcDb::kForWrite)==Acad::eOk){
            TVS_TAP* a=TVS_TAP::cast(oa);TVS_TAP* b=TVS_TAP::cast(ob);TVS_Pipe* p=TVS_Pipe::cast(op);TVS_Pipe* q=TVS_Pipe::cast(od);
            if(a&&b&&p&&q){
                const double elevA=side==0?elevation:a->get_Elevation(),elevB=side==2?elevation:b->get_Elevation();
                const AcGePoint3d bOld=rectMiddle(b);
                tvsOffset::Result g;tvsOffsetAdapter::Result d;
                if(!plan(a,p,elevA,elevB,f,g,d)){
                    const double bend=p->ThisRound?p->SizeA:p->SizeB;
                    const tvsOffset::Result t=tvsOffset::solve(tvsOffset::Input((elevB-elevA)*f,p->SizeA*f,bend*f,a->get_radius()*f,a->Swectangle,0));
                    acutPrintf(L"\nEtaz nije prilagodjen: %ls.",why(t.status));
                }else{
                    // the elbow on the unchanged side stays where it is
                    AcGeVector3d T(0,0,0);if(side==0)T=AcGeVector3d(bOld.x-d.elbows[1].middle.x,bOld.y-d.elbows[1].middle.y,0);
                    TVS_TAP* taps[2]={a,b};
                    for(int j=0;j<2;++j){const tvsOffsetAdapter::Elbow& e=d.elbows[j];TVS_TAP* tap=taps[j];
                        AcGePoint3d m(e.middle.x,e.middle.y,e.middle.z);m+=T;
                        if(tap->ThisRound){const tvsRoundElbow::P r=tvsRoundElbow::middleFromMc(tvsRoundElbow::P(m.x,m.y),e.startVector.x,e.startVector.y,e.angle,e.innerRadius+e.sizeA/2);m=AcGePoint3d(r.x,r.y,m.z);}
                        tap->assertWriteEnabled();tap->MiddlePoint=m;tap->CenterPoint=m;tap->Startvector=AcGeVector3d(e.startVector.x,e.startVector.y,0);
                        tap->put_Form(e.form==tvsOffsetAdapter::Up?Form_Up:Form_Down);tap->put_Elevation(e.elevation);tap->recordGraphicsModified();}
                    p->assertWriteEnabled();
                    p->FirstPoint=AcGePoint3d(d.middle.first.x,d.middle.first.y,d.middle.first.z)+T;
                    p->LastPoint=AcGePoint3d(d.middle.last.x,d.middle.last.y,d.middle.last.z)+T;
                    p->put_Elevation((d.middle.firstElevationMm+d.middle.lastElevationMm)/2/f);
                    tvsOffsetData::Record r;
                    if(tvsOffsetData::read(p,r)==tvsOffsetData::Valid){r.rise=g.axis[2].z-g.axis[1].z;tvsOffsetData::write(p,r);}
                    p->recordGraphicsModified();
                    // the changed duct's end follows its elbow (outer port), along the duct axis
                    const tvsOffset::WorldPoint& w=side==0?d.incomingLast:d.outgoingFirst;
                    AcGePoint3d& at=end==0?q->FirstPoint:q->LastPoint;q->assertWriteEnabled();
                    at=AcGePoint3d(w.x+T.x,w.y+T.y,at.z);q->recordGraphicsModified();
                    acutPrintf(L"\nEtaz prilagodjen: razlika kota %.0f mm, kosi dio %.0f mm, duzina u osnovi %.0f mm.",
                        std::fabs(elevB-elevA)*f,g.straightLength,g.totalRun);
                    ok=true;
                }
            }
        }
    }
    if(ok)actrTransactionManager->endTransaction();else actrTransactionManager->abortTransaction();
    return ok;
}
// One etaz joined to `duct` with a different elevation: ask and rebuild. true = rebuilt.
inline bool step(AcDbDatabase* db,const AcDbObjectId& duct,std::vector<Assembly>& all,std::set<std::pair<int,int> >& declined,double f,double tol){
    AcGePoint3d ends[2];double elev=0;
    {AcDbObjectPointer<TVS_Pipe> q(duct,AcDb::kForRead);if(q.openStatus()!=Acad::eOk||!plainDuct(q.object()))return false;
     ends[0]=q->FirstPoint;ends[1]=q->LastPoint;elev=q->Elev;}
    for(size_t k=0;k<all.size();++k)for(int side=0;side<=2;side+=2)for(int e=0;e<2;++e){
        const AcGePoint3d& ma=side==0?all[k].maA:all[k].maB;const double was=side==0?all[k].elevA:all[k].elevB;
        if(!nearXY(ends[e],ma,tol)||std::fabs(elev-was)<=tol)continue;
        if(declined.count(std::make_pair(static_cast<int>(k),side*2+e)))continue;
        wchar_t prompt[220];swprintf_s(prompt,L"\nKota kanala uz etaz promijenjena za %+.0f mm. Prilagoditi etaz? [Da/Ne] <Da>: ",(elev-was)*f);
        acedInitGet(0,L"Da Ne");ACHAR kw[16]=L"";const int rc=acedGetKword(prompt,kw);
        if(rc==RTCAN||(rc==RTNORM&&_wcsicmp(kw,L"Ne")==0)){acutPrintf(L"\nEtaz nije mijenjan.");declined.insert(std::make_pair(static_cast<int>(k),side*2+e));continue;}
        if(refit(all[k],side,elev,duct,e,f)){assemblies(db,all);return true;}
        declined.insert(std::make_pair(static_cast<int>(k),side*2+e));
    }
    return false;
}
// TVS_ETAZ_KOTA: settle the queued ducts.
inline void run(){
    posted()=false;
    std::vector<AcDbObjectId> work;work.swap(pending());
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db||work.empty())return;
    const double f=tvsUnits::factor(db),tol=1.0/f;
    std::vector<Assembly> all;if(!assemblies(db,all)||all.empty())return;
    for(size_t i=0;i<work.size();++i){
        std::set<std::pair<int,int> > declined;int guard=0;
        while(step(db,work[i],all,declined,f,tol)&&++guard<8)declined.clear();   // both ends may join an etaz
    }
    pending().clear();   // own edits of this command do not queue a new round
}
}
#endif

#pragma once
// Prelaz i kanali dijele dimenziju na spoju. When the size of a duct changes (Svojstva, double
// click, Properties), the transition at that duct end takes the new size on that end; when a
// transition end size changes, the duct at that end takes it (and so on through the next duct
// to the transition at its other end). Other ends never change. Changes made by TVS commands
// (drawing, TVS_MATCH, moves ...) are left to those commands. Settled by TVS_DIM_PRATI, run
// automatically after the edit; the transition's own height alignment then works as before
// (TVS_PRELAZ_KOTA asks which side stays if the outlet moved).
// Whole run (plan 14, phase 1): when a duct changed, TVS_DIM_PRATI offers to give the new size
// to the whole run of the old size up to the next prelaz / free end: ducts and plan elbows
// (new radius, the duct ends follow the elbow ports along their axes). It stops at an etaz,
// obilazak, T komad, vertical or round elbow it cannot recompute, or an element of another
// size, and says where.
#include "TVSMoveCommand.h"
#include "TransAlignCommand.h"
#include "TeeCommand.h"
#include "MleaderController.h"
#include "../TVS_Ventilation_DBX/SpiroCatalog.h"
#include "TeePort.h"
#include "../geometry/SizeFollowRules.h"
#include "RunDialog.h"
#include "TeeTypeDialog.h"
#ifdef TVS_OFFSET_INTEGRATION
#include "OffsetRefit.h"
#endif
#if defined(TVS_OFFSET_INTEGRATION)&&defined(TVS_BYPASS_PREVIEW)
#define TVS_SIZE_BYPASS 1
#include "BypassPreviewCommand.h"
#include "Settings.h"
#endif
#include "acdocman.h"
#include "aced.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
namespace tvsSizeFollow {
struct Sizes {double a[2],b[2];Sizes(){a[0]=a[1]=b[0]=b[1]=0;}};
inline bool& applying(){static bool b=false;return b;}
inline CString& command(){static CString c;return c;}
inline std::map<AcDbObjectId,Sizes>& opened(){static std::map<AcDbObjectId,Sizes> m;return m;}
// changed element -> ends whose size changed (bit 0 = first, bit 1 = second; ducts: 3)
inline std::map<AcDbObjectId,int>& pending(){static std::map<AcDbObjectId,int> m;return m;}
inline bool& posted(){static bool b=false;return b;}
// TVS elements modified in the command (any command, Properties, grips): their TVS labels get
// their text again in TVS_DIM_PRATI (plan 15; TVS_LEAD_AUTO turns it off).
inline std::set<AcDbObjectId>& touched(){static std::set<AcDbObjectId> s;return s;}
inline bool autoLabels(){ACHAR v[16]=L"";return !(acedGetEnv(L"TVS_LEAD_AUTO",v,16)==RTNORM&&!_wcsicmp(v,L"0"));}
// size of a user-edited duct before the edit (whole run: elements of that size follow)
inline std::map<AcDbObjectId,Sizes>& previous(){static std::map<AcDbObjectId,Sizes> m;return m;}
inline bool plainDuct(const AcDbObject* o){
    if(!o||o->isA()!=TVS_Pipe::desc())return false;
#ifdef TVS_OFFSET_INTEGRATION
    if(tvsOffsetData::marked(o))return false;
#endif
    return true;
}
inline bool sizes(const AcDbObject* o,Sizes& s){
    if(plainDuct(o)){const TVS_Pipe* p=TVS_Pipe::cast(o);s.a[0]=s.a[1]=p->SizeA;s.b[0]=s.b[1]=p->SizeB;return true;}
    if(const TVS_TRANS* t=TVS_TRANS::cast(o)){s.a[0]=t->SizeAp1;s.b[0]=t->SizeBp1;s.a[1]=t->SizeAp2;s.b[1]=t->SizeBp2;return true;}
    return false;
}
// User edits only: Properties palette (no command), double click / Svojstva (TVS_Change).
inline bool userEdit(){
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;
    if(doc&&doc->isQuiescent())return true;
    const CString& c=command();
    return c.IsEmpty()||!c.CompareNoCase(L"TVS_CHANGE")||!c.CompareNoCase(L"DBLCLKEDIT")||!c.CompareNoCase(L"PROPERTIES");
}
inline unsigned long long& postedAt(){static unsigned long long t=0;return t;}
// Edits waiting in this drawing (the state is shared by all open drawings; each settles its own).
inline bool waiting(AcDbDatabase* db){
    for(std::map<AcDbObjectId,int>::const_iterator it=pending().begin();it!=pending().end();++it)if(it->first.database()==db)return true;
    return false;
}
inline void post(){
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(!doc)return;
    if(posted()&&tvsSpiro::nowMs()-postedAt()<5000)return;   // a posted run that never came (drawing closed) does not block for ever
    const bool sizesWait=waiting(doc->database());
    if(!sizesWait&&(touched().empty()||!autoLabels()))return;
    if(!sizesWait){   // labels only: not into a running LISP / script (their input stream); later
        resbuf rb={};if(acedGetVar(L"CMDACTIVE",&rb)==RTNORM&&rb.restype==RTSHORT&&(rb.resval.rint&(4|32)))return;}
    posted()=true;postedAt()=tvsSpiro::nowMs();acDocManager->sendStringToExecute(doc,L"_TVS_DIM_PRATI\n",false,true,false);
}
class DbReactor : public AcDbDatabaseReactor {
public:
    virtual void objectOpenedForModify(const AcDbDatabase*,const AcDbObject* o){
        if(applying()||o->isUndoing())return;Sizes s;if(sizes(o,s))opened()[o->objectId()]=s;
    }
    virtual void objectModified(const AcDbDatabase*,const AcDbObject* o){
        if(!o->isUndoing()&&TVS_Entity::cast(o)){touched().insert(o->objectId());
            if(!applying()){AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(doc&&doc->isQuiescent())post();}}
        std::map<AcDbObjectId,Sizes>::iterator it=opened().find(o->objectId());if(it==opened().end())return;
        const Sizes was=it->second;opened().erase(it);
        if(applying()||o->isUndoing()||!userEdit())return;
        Sizes now;if(!sizes(o,now))return;
        int ends=0;for(int k=0;k<2;++k)if(std::fabs(now.a[k]-was.a[k])>1e-9||std::fabs(now.b[k]-was.b[k])>1e-9)ends|=1<<k;
        if(!ends)return;
        pending()[o->objectId()]|=ends;
        if(plainDuct(o)&&!previous().count(o->objectId()))previous()[o->objectId()]=was;
        AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;
        if(doc&&doc->isQuiescent())post();
    }
    virtual void goodbye(const AcDbDatabase* db);
};
inline DbReactor& dbReactor(){static DbReactor r;return r;}
inline std::set<AcDbDatabase*>& attached(){static std::set<AcDbDatabase*> s;return s;}
template<class M> inline void forget(M& m,const AcDbDatabase* db){for(typename M::iterator it=m.begin();it!=m.end();){if(it->first.database()==db)m.erase(it++);else ++it;}}
inline void DbReactor::goodbye(const AcDbDatabase* db){
    attached().erase(const_cast<AcDbDatabase*>(db));
    forget(pending(),db);forget(previous(),db);forget(opened(),db);   // ids of a closing drawing
    for(std::set<AcDbObjectId>::iterator it=touched().begin();it!=touched().end();){if(it->database()==db)touched().erase(it++);else ++it;}
}
class EdReactor : public AcEditorReactor {
public:
    virtual void commandWillStart(const ACHAR* c){command()=c?c:L"";}
    virtual void commandEnded(const ACHAR* c){
        command().Empty();
        if(c&&(!_wcsicmp(c,L"U")||!_wcsicmp(c,L"UNDO")||!_wcsicmp(c,L"REDO")||!_wcsicmp(c,L"MREDO"))){pending().clear();previous().clear();touched().clear();return;}
        if(c&&_wcsicmp(c,L"TVS_DIM_PRATI"))post();
    }
    virtual void commandCancelled(const ACHAR*){command().Empty();}
    virtual void commandFailed(const ACHAR*){command().Empty();}
};
inline EdReactor& edReactor(){static EdReactor r;return r;}
inline void attach(AcDbDatabase* db){if(db&&attached().insert(db).second)db->addReactor(&dbReactor());}
inline void start(){
    if(acedEditor)acedEditor->addReactor(&edReactor());
    if(!acDocManager)return;AcApDocumentIterator* it=acDocManager->newAcApDocumentIterator();if(!it)return;
    for(;!it->done();it->step())if(it->document())attach(it->document()->database());
    delete it;
}
inline void stop(){
    if(acedEditor)acedEditor->removeReactor(&edReactor());
    for(std::set<AcDbDatabase*>::iterator i=attached().begin();i!=attached().end();++i)(*i)->removeReactor(&dbReactor());
    attached().clear();
}
// --- settle ---
inline bool nearXY(const tvsMove::Point& u,const tvsMove::Point& v,double tol){return std::hypot(u.x-v.x,u.y-v.y)<=tol;}
// --- whole run up to the next prelaz (plan 14, phases 1-2) ---
inline bool plainElbow(const AcDbObject* o){
    if(!o||o->isA()!=TVS_TAP::desc())return false;
#ifdef TVS_OFFSET_INTEGRATION
    if(tvsOffsetData::marked(o))return false;
#endif
    return true;
}
// Ports of a plan elbow with bend width A (same radius rule as tvsMove::describe / TVS_TAP).
inline bool elbowPortsFor(const TVS_TAP* p,double A,tvsMove::Point out[2]){
    const int type=static_cast<int>(p->ThisRound?p->RadiusTypeRound:p->RadiusTypeRect);
    const bool variable=p->ThisRound?type==RadiusTypeRound_RadiusVariable:type==RadiusTypeRect_RadiusVariable;
    const bool constant=p->ThisRound?type==RadiusTypeRound_RadiusConstant:type==RadiusTypeRect_RadiusConstant;
    const bool specific=p->ThisRound?type==RadiusTypeRound_RadiusSpecific:type==RadiusTypeRect_RadiusSpecific;
    if(p->ThisRound&&!(variable||constant))return false;
    double radius=p->Radius;
    if(variable)radius=A*p->RadiusVariableParameter;
    if(constant)radius=p->RadiusConst;
    if(specific)radius=tvsUnits::mm(A,p)<355?A:A/2;
    return tvsMoveGeometry::elbowPorts(tvsMove::point(p->MiddlePoint),tvsMove::Point(p->Startvector.x,p->Startvector.y,0),p->Swectangle,radius+A/2,out);
}
// T komad on the run. port: 0/1 main (run start / end), 2 branch. mode: 0 port only,
// 1 main both sides, run continues behind it, 2 main both sides + prelaz back to the old size
// behind it.
struct TeeStep {int node,port,mode;};
// Etaz on the run (index into the etaz list, entered at its first elbow side).
struct EtazStep {int index;bool atA;};
// Obilazak on the run: its first member (the TVS_BYPASS dialog rebuilds it at the end).
// dialogs: TK-O / TK-G the run arrived at (their dialog opens after the run is written); marks:
// boundaries for the preview; met*: such elements on the way, passed or not (dialog rows).
struct Run {std::vector<int> elbows,ducts;std::vector<TeeStep> tees,dialogs;std::vector<EtazStep> etazes;std::vector<AcDbObjectId> bypasses;std::vector<CString> stops;
    std::vector<std::pair<tvsMove::Point,CString> > marks;int metTee,metEtaz,metBypass;Run():metTee(0),metEtaz(0),metBypass(0){}
    bool empty()const{return elbows.empty()&&ducts.empty()&&tees.empty()&&dialogs.empty()&&etazes.empty()&&bypasses.empty();}};
#ifdef TVS_OFFSET_INTEGRATION
typedef std::vector<tvsOffsetRefit::Assembly> Etazes;
#else
struct NoEtaz {};typedef std::vector<NoEtaz> Etazes;
#endif
inline CString boundary(const AcDbObject* o){
    if(TVS_WYE::cast(o))return L"T komad";
#ifdef TVS_OFFSET_INTEGRATION
    if(tvsBypassData::marked(o))return L"obilazak";
    if(tvsOffsetData::marked(o))return L"etaž";
#endif
    if(TVS_TAP::cast(o))return L"koljeno gore/dolje ili okruglo koljeno";
    if(TVS_Pipe::cast(o))return L"kanal druge dimenzije";
    return L"drugi element";
}
inline bool ask(const wchar_t* prompt,const wchar_t* keywords,ACHAR* answer,const wchar_t* def){
    acedInitGet(0,keywords);answer[0]=0;const int rc=acedGetKword(prompt,answer,32);
    if(rc==RTNONE){wcscpy_s(answer,32,def);return true;}
    return rc==RTNORM;
}
// From the ends of duct `start` (both, or one side: ch.scope), through joined ducts, plan
// elbows, T komadi, etazi and obilasci of the old size; what happens at a T komad, etaz and
// obilazak is decided in the dialog (ch), nothing is asked here.
inline void walk(const std::vector<tvsMove::Node>& nodes,int start,const Sizes& old,double tol,const Etazes& etazes,Run& run,std::set<int>& seen,const tvsRunDialog::Choices& ch){
    const bool round=nodes[start].round;
    auto same=[&](double a,double b,bool r){return r==round&&std::fabs(a-old.a[0])<=1e-6&&(round||std::fabs(b-old.b[0])<=1e-6);};
    auto indexOf=[&](const AcDbObjectId& id){for(size_t k=0;k<nodes.size();++k)if(nodes[k].id==id)return static_cast<int>(k);return -1;};
    std::vector<std::pair<int,tvsMove::Point> > open;
    for(int side=0;side<2&&side<static_cast<int>(nodes[start].ports.size());++side){
        if((ch.scope==tvsRunDialog::Side1&&side!=0)||(ch.scope==tvsRunDialog::Side2&&side!=1))continue;
        open.push_back(std::make_pair(start,nodes[start].ports[side]));}
    for(size_t o=0;o<open.size();++o){
        int prev=open[o].first;tvsMove::Point at=open[o].second;
        auto stop=[&](const CString& why){run.stops.push_back(why);run.marks.push_back(std::make_pair(at,why));};
        for(int guard=0;guard<2000;++guard){
            int next=-1,count=0,port=-1;
            for(size_t k=0;k<nodes.size();++k){if(static_cast<int>(k)==prev)continue;
                for(size_t j=0;j<nodes[k].ports.size();++j)
                    if(nearXY(nodes[k].ports[j],at,tol)&&std::fabs(nodes[k].elevation-nodes[prev].elevation)<=tol){next=static_cast<int>(k);port=static_cast<int>(j);++count;break;}}
            if(count==0)break;                                            // free end
            if(count>1){stop(L"spoj sa više elemenata");break;}
            if(seen.count(next))break;                                    // closed loop
            AcDbObjectPointer<AcDbEntity> e(nodes[next].id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)break;
            if(TVS_TRANS::cast(e.object())){run.marks.push_back(std::make_pair(at,CString(L"prelaz")));break;}   // prelaz: the size follow below
            const tvsMove::Node& n=nodes[next];
            const bool duct=plainDuct(e.object())&&n.kind==1,elbow=plainElbow(e.object())&&n.kind==2;
            if(duct||elbow){
                if(!same(n.a,n.b,n.round)||n.ports.size()!=2){stop(L"element druge dimenzije");break;}
                seen.insert(next);(duct?run.ducts:run.elbows).push_back(next);
                const int farPort=std::hypot(n.ports[0].x-at.x,n.ports[0].y-at.y)>std::hypot(n.ports[1].x-at.x,n.ports[1].y-at.y)?0:1;
                prev=next;at=n.ports[farPort];continue;
            }
            if(const TVS_WYE* w=TVS_WYE::cast(e.object())){
                ++run.metTee;
                if(n.ports.size()!=3||port<0){stop(L"T komad");break;}
                tvsTeeData::Record r;tvsTeeData::read(w,r);
                const bool branch=port==2;
                const double pa=branch?w->SizeAotv:(port==1&&r.type==tvsTee::Branch&&r.second>0?r.second:w->SizeApr);
                const double pb=branch?w->SizeBotv:w->SizeBpr;const bool pr=branch?w->ThisRoundotv:w->ThisRoundpr;
                // TK-O / TK-G keep their sizes: the duct on the port gets a prelaz there (TeePort.h, after the run)
                if(r.type==tvsTee::Pants||r.type==tvsTee::Branch){
                    if(ch.tee==0){TeeStep t={next,port,3};run.dialogs.push_back(t);seen.insert(next);stop(L"T komad TK-O / TK-G (njegov dijalog)");break;}
                    stop(L"T komad TK-O / TK-G (ostaje; prelaz na priključku)");break;}
                if(!same(pa,pb,pr)){stop(L"T komad druge dimenzije");break;}
                if(r.type==tvsTee::Branch&&port==1&&std::fabs(pb-old.b[0])>1e-6){stop(L"TK-O");break;}
                seen.insert(next);TeeStep t={next,port,0};
                if(branch||r.type==tvsTee::Branch){run.tees.push_back(t);break;}   // ogranak / one side of TK-O: stops here
                e.close();
                if(ch.tee==1){seen.erase(next);stop(L"T komad (Stani)");break;}
                t.mode=ch.tee==2?2:1;run.tees.push_back(t);
                if(t.mode==2)break;
                prev=next;at=n.ports[port==0?1:0];continue;               // on through the other main side
            }
#ifdef TVS_SIZE_BYPASS
            if(tvsBypassData::marked(e.object())){
                tvsBypassData::Record r;++run.metBypass;
                if(tvsBypassData::assembly(e.object(),r,false)!=tvsOffsetData::Valid||(n.id!=r.members[0]&&n.id!=r.members[6])){stop(L"obilazak");break;}
                const bool atFirst=n.id==r.members[0];
                const int h=indexOf(r.members[3]),other=indexOf(atFirst?r.members[6]:r.members[0]);
                AcGePoint3d next3;
                {AcDbObjectPointer<TVS_Pipe> q(atFirst?r.outlet:r.inlet,AcDb::kForRead);if(q.openStatus()!=Acad::eOk||h<0||other<0){stop(L"obilazak");break;}
                 // the duct end at the obilazak on the other side
                 next3=atFirst?(r.reverse?q->LastPoint:q->FirstPoint):(r.reverse?q->FirstPoint:q->LastPoint);}
                if(!same(nodes[h].a,nodes[h].b,nodes[h].round)){stop(L"obilazak druge dimenzije");break;}
                e.close();
                if(!ch.bypass){stop(L"obilazak (nije preračunat)");break;}
                for(int m=0;m<7;++m){const int x=indexOf(r.members[m]);if(x>=0)seen.insert(x);}
                run.bypasses.push_back(r.members[0]);
                prev=other;at=tvsMove::point(next3);continue;
            }
#endif
#ifdef TVS_OFFSET_INTEGRATION
            if(tvsOffsetData::marked(e.object())&&!tvsBypassData::marked(e.object())){
                int found=-1;bool atA=true;++run.metEtaz;
                for(size_t k=0;k<etazes.size();++k){
                    if(etazes[k].a==n.id&&nearXY(tvsMove::point(etazes[k].maA),at,tol)){found=static_cast<int>(k);atA=true;break;}
                    if(etazes[k].b==n.id&&nearXY(tvsMove::point(etazes[k].maB),at,tol)){found=static_cast<int>(k);atA=false;break;}}
                if(found<0){stop(L"etaž");break;}
                const int pi=indexOf(etazes[found].p),other=indexOf(atA?etazes[found].b:etazes[found].a);
                if(pi<0||other<0||!same(nodes[pi].a,nodes[pi].b,nodes[pi].round)){stop(L"etaž druge dimenzije");break;}
                e.close();
                if(!ch.etaz){stop(L"etaž (nije preračunat)");break;}
                seen.insert(next);seen.insert(pi);seen.insert(other);EtazStep st={found,atA};run.etazes.push_back(st);
                prev=other;at=tvsMove::point(atA?etazes[found].maB:etazes[found].maA);continue;
            }
#endif
            stop(boundary(e.object()));break;
        }
    }
}
#ifdef TVS_OFFSET_INTEGRATION
// Plan of an etaz with a new section (same elevations, angle and inner radius; the first
// elbow keeps its middle): new outer ports.
inline bool etazPlan(const tvsOffsetRefit::Assembly& s,double A,double B,double f,tvsOffset::Result& g,tvsOffsetAdapter::Result& d){
    AcDbObjectPointer<TVS_TAP> a(s.a,AcDb::kForRead);AcDbObjectPointer<TVS_Pipe> p(s.p,AcDb::kForRead);
    if(a.openStatus()!=Acad::eOk||p.openStatus()!=Acad::eOk)return false;
    const bool round=p->ThisRound;const double bend=round?A:B;
    const tvsOffset::Input in((s.elevB-s.elevA)*f,A*f,bend*f,a->get_radius()*f,a->Swectangle,0);
    g=tvsOffset::solve(in);if(!g.hasGeometry())return false;
    const AcGeVector3d axis=p->LastPoint-p->FirstPoint;if(axis.length()<=0)return false;
    const AcGePoint3d m=tvsOffsetRefit::rectMiddle(a.object());
    tvsOffsetAdapter::Placement place(tvsOffset::WorldPoint(m.x,m.y,m.z),std::atan2(axis.y,axis.x),f,s.elevA*f);
    d=tvsOffsetAdapter::adapt(in,g,place);return d.ready();
}
// Rebuild the etaz with the new section, as tvsOffsetRefit::refit (one transaction, new
// assembly record, validated afterwards). The ducts at its ends are moved by the caller.
inline bool etazResize(const tvsOffsetRefit::Assembly& s,double A,double B,double f){
    tvsOffset::Result g;tvsOffsetAdapter::Result d;if(!etazPlan(s,A,B,f,g,d))return false;
    AcDbDatabase* db=s.a.database();if(!db)return false;AcDbObjectId newA;bool ok=false;
    {
        tvsOffsetProof::BuildTransaction tx(db);if(!tx.space)return false;
        tvsOffsetData::WriteScope scope;
        AcDbObject *oa=NULL,*op=NULL,*ob=NULL;
        if(actrTransactionManager->getObject(oa,s.a,AcDb::kForWrite)!=Acad::eOk||actrTransactionManager->getObject(op,s.p,AcDb::kForWrite)!=Acad::eOk
            ||actrTransactionManager->getObject(ob,s.b,AcDb::kForWrite)!=Acad::eOk)return false;
        TVS_TAP* a=TVS_TAP::cast(oa);TVS_TAP* b=TVS_TAP::cast(ob);TVS_Pipe* p=TVS_Pipe::cast(op);if(!a||!b||!p)return false;
        tvsOffsetService::Snapshot snap;snap.db=db;snap.physical.factor=f;snap.physical.a=A;
        snap.round=p->ThisRound;snap.physical.b=p->ThisRound?A:B;snap.typeRoundTap=a->TypeRoundTap;
        AcDbEntity* old[3]={a,p,b};AcDbObjectId ids[3];AcDbEntity* made[3]={NULL,NULL,NULL};
        for(int j=0;j<2;++j){
            std::unique_ptr<TVS_TAP> tap=tvsOffsetService::makeElbow(p,snap,d.elbows[j]);if(!tap)return false;
            TVS_TAP* raw=tap.release();if(!tx.append(raw,ids[j==0?0:2]))return false;made[j==0?0:2]=raw;
        }
        std::unique_ptr<TVS_Pipe> slope=tvsOffsetService::makePipe(p,snap,d.middle.first,d.middle.last,(d.middle.firstElevationMm+d.middle.lastElevationMm)/2/f);
        if(!slope)return false;{TVS_Pipe* raw=slope.release();if(!tx.append(raw,ids[1]))return false;made[1]=raw;}
        const CString guid=tvsOffsetData::newGuid();if(guid.IsEmpty())return false;
        for(int j=0;j<3;++j){
            tvsOffsetData::Record r;r.guid=guid;r.role=j;for(int k=0;k<3;++k)r.members[k]=ids[k];
            r.rise=j==1?g.axis[2].z-g.axis[1].z:0;
            if(tvsOffsetData::write(made[j],r)!=Acad::eOk)return false;
            if(resbuf* x=old[j]->xData(NULL)){made[j]->setXData(x);acutRelRb(x);}
        }
        for(int j=0;j<3;++j)if(old[j]->erase()!=Acad::eOk)return false;
        if(!tx.commit())return false;
        newA=ids[0];ok=true;
    }
    if(ok){AcDbObjectPointer<TVS_TAP> check(newA,AcDb::kForRead);
        if(check.openStatus()==Acad::eOk){const tvsOffsetData::Status st=tvsOffsetData::validate(check.object());
            if(st!=tvsOffsetData::Valid)acutPrintf(L"\nUPOZORENJE: provjera novog etaža: %ls.",tvsOffsetData::statusText(st));}}
    return ok;
}
#endif
// TVS transition right behind a T komad: the duct `ductId` (end `nearIsFirst` on the T komad
// port `at`) is shortened by the transition length; prelaz new size (A x B) -> its own size.
inline bool prelazBehind(const AcDbObjectId& ductId,bool nearIsFirst,const AcGePoint3d& at,double A,double B,double f,const wchar_t* where=L"Iza T komada"){
    AcGePoint3d N,E;double width=0,height=0,elev=0;bool oneD=false,round=false;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()!=Acad::eOk)return false;
     N=nearIsFirst?d->FirstPoint:d->LastPoint;E=nearIsFirst?d->LastPoint:d->FirstPoint;width=d->SizeA;height=d->SizeB;elev=d->Elev;oneD=d->This1D;round=d->ThisRound;}
    AcGeVector3d dir=E-N;dir.z=0;const double length=dir.length(),lt=tvsSpiro::drawingLength(NULL,A,round,width,round,tvsTeeFit::transitionMm()/f);   // spiro: po katalogu SOKO
    if(length<lt+50.0/f){acutPrintf(L"\n%ls: kanal je prekratak za prelaz %.0f mm; prilagodite ručno.",where,lt*f);return false;}
    dir.normalize();const AcGePoint3d start(at.x,at.y,N.z),end=start+dir*lt;
    {AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForWrite);
     if(d.openStatus()!=Acad::eOk){acutPrintf(L"\n%ls: kanal se ne može mijenjati (zaključan sloj?); prelaz nije umetnut.",where);return false;}
     if((nearIsFirst?d->put_FirstPoint(end):d->put_Lastpoint(end))!=Acad::eOk)return false;d->recordGraphicsModified();}
    double a1=A,b1=round?0:B,a2=width,b2=round?0:height,len=lt;AcGeVector3d v=dir*lt;AcGePoint3d p0=start;
    TVS_TRANS* t=TVS_TRANS::add_new(a1,b1,a2,b2,len,v,p0,round,round,1,oneD);
    if(t&&!t->objectId().isNull()){
        AcDbObjectPointer<TVS_TRANS> o(t->objectId(),AcDb::kForWrite);
        if(o.openStatus()==Acad::eOk){
            AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForRead);if(d.openStatus()==Acad::eOk)o->setPropertiesFrom(d.object());
            o->put_Elevation(elev);o->recordGraphicsModified();}
        acutPrintf(L"\n%ls umetnut prelaz na staru dimenziju, dužine %.0f mm.",where,lt*f);
        return true;
    }
    return false;
}
// Other elements with a port on the plan point `at` (not node `self`).
inline int othersAt(const std::vector<tvsMove::Node>& nodes,int self,const tvsMove::Point& at,double tol){
    int n=0;for(size_t k=0;k<nodes.size();++k){if(static_cast<int>(k)==self)continue;
        for(size_t j=0;j<nodes[k].ports.size();++j)if(nearXY(nodes[k].ports[j],at,tol)){++n;break;}}
    return n;
}
// Ends of the edited duct `start` (new size) lying directly on a duct / plan elbow of the old
// size, without a prelaz. insert: a prelaz new -> old is put there, in the neighbouring duct
// (the edited duct keeps its length) or, at an elbow, in the edited duct. Returns the number
// of such joints (insert: of the transitions made).
inline int joints(const std::vector<tvsMove::Node>& nodes,int start,const Sizes& old,double tol,double f,bool insert){
    const tvsMove::Node& s=nodes[start];double A=0,B=0;
    {AcDbObjectPointer<TVS_Pipe> p(s.id,AcDb::kForRead);if(p.openStatus()!=Acad::eOk)return 0;A=p->SizeA;B=p->SizeB;}
    int found=0;
    for(int side=0;side<2&&side<static_cast<int>(s.ports.size());++side){
        const tvsMove::Point at=s.ports[side];int next=-1,count=0;
        for(size_t k=0;k<nodes.size();++k){if(static_cast<int>(k)==start)continue;
            for(size_t j=0;j<nodes[k].ports.size();++j)
                if(nearXY(nodes[k].ports[j],at,tol)&&std::fabs(nodes[k].elevation-s.elevation)<=tol){next=static_cast<int>(k);++count;break;}}
        if(count!=1)continue;
        const tvsMove::Node& n=nodes[next];
        if((n.kind!=1&&n.kind!=2)||n.ports.size()!=2||n.round!=s.round||std::fabs(n.a-old.a[0])>1e-6||(!n.round&&std::fabs(n.b-old.b[0])>1e-6))continue;
        {AcDbObjectPointer<AcDbEntity> e(n.id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk||!(plainDuct(e.object())||plainElbow(e.object())))continue;}
        if(!insert){++found;continue;}
        const AcGePoint3d P=tvsMove::acad(at);
        if(n.kind==1){if(prelazBehind(n.id,nearXY(n.ports[0],at,tol),P,A,B,f,L"Na spoju"))++found;}
        else if(prelazBehind(s.id,side==0,P,old.a[0],old.b[0],f,L"Na spoju (uz koljeno)"))++found;
    }
    return found;
}
// Applies the new size of duct `start` to the run. false: nothing changed (reason printed).
inline bool applyRun(const std::vector<tvsMove::Node>& nodes,int start,const Run& run,const Etazes& etazes,double tol,std::vector<AcDbObjectId>& resized){
    double A=0,B=0;bool round=false;
    {AcDbObjectPointer<TVS_Pipe> p(nodes[start].id,AcDb::kForRead);if(p.openStatus()!=Acad::eOk)return false;A=p->SizeA;B=p->SizeB;round=p->ThisRound;}
    const double f=tvsUnits::factor(nodes[start].id.database());
    std::vector<std::pair<tvsMove::Point,tvsMove::Point> > moves;   // port: old -> new
    for(size_t k=0;k<run.elbows.size();++k){const tvsMove::Node& n=nodes[run.elbows[k]];
        AcDbObjectPointer<TVS_TAP> t(n.id,AcDb::kForRead);tvsMove::Point np[2];
        if(t.openStatus()!=Acad::eOk||!elbowPortsFor(t.object(),A,np)){acutPrintf(L"\nDionica nije promijenjena: koljeno se ne može preračunati.");return false;}
        for(int m=0;m<2;++m)moves.push_back(std::make_pair(n.ports[m],np[m]));
    }
#ifdef TVS_OFFSET_INTEGRATION
    for(size_t k=0;k<run.etazes.size();++k){const tvsOffsetRefit::Assembly& s=etazes[run.etazes[k].index];
        tvsOffset::Result g;tvsOffsetAdapter::Result d;
        if(!etazPlan(s,A,B,f,g,d)){
            acutPrintf(L"\nDionica nije promijenjena: etaž sa novom dimenzijom nije moguć (razlika kota premala za dva koljena?).");return false;}
        moves.push_back(std::make_pair(tvsMove::point(s.maA),tvsMove::Point(d.incomingLast.x,d.incomingLast.y,d.incomingLast.z)));
        moves.push_back(std::make_pair(tvsMove::point(s.maB),tvsMove::Point(d.outgoingFirst.x,d.outgoingFirst.y,d.outgoingFirst.z)));
    }
#endif
    // T komadi: new sizes on a copy of the record, ports before / after
    struct TeeChange {int node;tvsTeeFit::Ports before,after;};std::vector<TeeChange> tees;
    for(size_t k=0;k<run.tees.size();++k){const TeeStep& t=run.tees[k];
        AcDbObjectPointer<TVS_WYE> w(nodes[t.node].id,AcDb::kForRead);if(w.openStatus()!=Acad::eOk)return false;
        TeeChange c;c.node=t.node;c.before=tvsTeeFit::ports(w.object());if(!c.before.ok)return false;tees.push_back(c);}
    // ducts whose end is on a moving port (in the run or not) + the run ducts
    std::vector<int> ducts=run.ducts;ducts.push_back(start);
    std::set<int> inRun(ducts.begin(),ducts.end());
    for(size_t k=0;k<nodes.size();++k){if(nodes[k].kind!=1||inRun.count(static_cast<int>(k)))continue;
        for(size_t m=0;m<moves.size();++m)for(int j=0;j<2&&j<static_cast<int>(nodes[k].ports.size());++j)
            if(nearXY(nodes[k].ports[j],moves[m].first,tol)&&std::find(ducts.begin(),ducts.end(),static_cast<int>(k))==ducts.end())ducts.push_back(static_cast<int>(k));}
    auto newEnds=[&](int d,std::pair<tvsMove::Point,tvsMove::Point>& out){const tvsMove::Node& n=nodes[d];tvsMove::Point e[2]={n.ports[0],n.ports[1]};
        for(int j=0;j<2;++j)for(size_t m=0;m<moves.size();++m)if(nearXY(e[j],moves[m].first,tol)){e[j].x=moves[m].second.x;e[j].y=moves[m].second.y;break;}
        const tvsMove::Point v=n.ports[1]-n.ports[0],w=e[1]-e[0];out=std::make_pair(e[0],e[1]);
        return std::hypot(w.x,w.y)>=10*tol&&v.x*w.x+v.y*w.y>0;};
    std::vector<std::pair<tvsMove::Point,tvsMove::Point> > ends(ducts.size());
    for(size_t k=0;k<ducts.size();++k)if(!newEnds(ducts[k],ends[k])){acutPrintf(L"\nDionica nije promijenjena: kanal bi sa novim koljenima / etažom bio prekratak.");return false;}
    // write: etazi, elbows, T komadi, ducts
#ifdef TVS_OFFSET_INTEGRATION
    for(size_t k=0;k<run.etazes.size();++k)if(!etazResize(etazes[run.etazes[k].index],A,B,f)){acutPrintf(L"\nEtaž nije preračunat; dionica je promijenjena samo do njega.");return false;}
#endif
    for(size_t k=0;k<run.elbows.size();++k){
        AcDbObjectPointer<TVS_TAP> t(nodes[run.elbows[k]].id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        t->put_SizeA(A);if(!round)t->put_SizeB(B);t->recordGraphicsModified();
    }
    for(size_t k=0;k<run.tees.size();++k){const TeeStep& t=run.tees[k];
        AcDbObjectPointer<TVS_WYE> w(nodes[t.node].id,AcDb::kForWrite);if(w.openStatus()!=Acad::eOk)continue;
        tvsTeeData::Record r;const bool typed=tvsTeeData::read(w.object(),r);
        w->assertWriteEnabled();   // before the fields change (UNDO)
        if(t.port==2){w->SizeAotv=A;w->SizeBotv=round?0:B;
            if(typed&&r.type==tvsTee::Tap){r.l=tvsTeeCommand::tapLength(A,f);w->LengthPl=r.l;}
            else if(typed&&r.type==tvsTee::Branch)w->LengthPl=tvsTee::lengthPl(r.l,tvsTee::defaultRadius(A),A);}
        else if(typed&&r.type==tvsTee::Branch){
            if(t.port==0){if(!(r.second>0))r.second=w->SizeApr;w->SizeApr=A;w->SizeBpr=round?0:B;}   // C keeps its width
            else r.second=A;}
        else{w->SizeApr=A;w->SizeBpr=round?0:B;}
        if(typed)tvsTeeData::write(w.object(),r);
        w->recordGraphicsModified();
        tees[k].after=tvsTeeFit::ports(w.object());
    }
    // T komad ports that moved: ducts on them follow (before the run ducts are written)
    for(size_t k=0;k<tees.size();++k)if(tees[k].after.ok)for(int m=0;m<3;++m){
        const tvsMove::Point from=tvsMove::point(tees[k].before.p[m]),to=tvsMove::point(tees[k].after.p[m]);
        if(std::hypot(from.x-to.x,from.y-to.y)<=1e-9)continue;
        for(size_t j=0;j<ducts.size();++j){std::pair<tvsMove::Point,tvsMove::Point>& e=ends[j];
            if(nearXY(e.first,from,tol))e.first=tvsMove::Point(to.x,to.y,e.first.z);
            if(nearXY(e.second,from,tol))e.second=tvsMove::Point(to.x,to.y,e.second.z);}
        for(size_t q=0;q<nodes.size();++q){if(nodes[q].kind!=1||std::find(ducts.begin(),ducts.end(),static_cast<int>(q))!=ducts.end())continue;
            for(int j=0;j<2&&j<static_cast<int>(nodes[q].ports.size());++j)if(nearXY(nodes[q].ports[j],from,tol)){
                AcDbObjectPointer<TVS_Pipe> p(nodes[q].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk||!plainDuct(p.object()))continue;
                AcGePoint3d at=j==0?p->FirstPoint:p->LastPoint;at.x=to.x;at.y=to.y;
                if(j==0)p->put_FirstPoint(at);else p->put_Lastpoint(at);p->recordGraphicsModified();}}
    }
    for(size_t k=0;k<ducts.size();++k){
        AcDbObjectPointer<TVS_Pipe> p(nodes[ducts[k]].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk||!plainDuct(p.object()))continue;
        p->put_FirstPoint(AcGePoint3d(ends[k].first.x,ends[k].first.y,p->FirstPoint.z));
        p->put_Lastpoint(AcGePoint3d(ends[k].second.x,ends[k].second.y,p->LastPoint.z));
        if(inRun.count(ducts[k])&&ducts[k]!=start){p->put_SizeA(A);p->put_SizeB(B);resized.push_back(nodes[ducts[k]].id);}
        p->recordGraphicsModified();
    }
#ifdef TVS_SIZE_BYPASS
    // obilasci last: the TVS_BYPASS dialog with the new size on the (already resized) inlet/outlet
    for(size_t k=0;k<run.bypasses.size();++k){
        TVSBypassRadius radius;radius.mode=globalRadiusTypeRect;radius.roundMode=globalRadiusTypeRound;radius.typeRoundTap=globalTypeRoundTap;
        radius.k=globalTapRadiusVariableParameter;radius.constant=globalTapRadiusConst;
        AcDbObjectId outlet;
        if(tvsBypassResize(run.bypasses[k],radius,outlet)){if(!outlet.isNull())resized.push_back(outlet);}
        else{bool stays=false;{AcDbObjectPointer<AcDbObject> m(run.bypasses[k],AcDb::kForRead);stays=m.openStatus()==Acad::eOk;}
            if(stays)acutPrintf(L"\nObilazak nije preračunat: kanali uz njega imaju novu dimenziju, a obilazak staru – uskladite ga (TVS_BYPASS) ili vratite izmjenu (U).");}
    }
#endif
    // prelaz back to the old size behind a T komad (option Prelaz)
    for(size_t k=0;k<run.tees.size();++k){const TeeStep& t=run.tees[k];if(t.mode!=2||!tees[k].after.ok)continue;
        const int other=t.port==0?1:0;const AcGePoint3d at=tees[k].after.p[other];
        for(size_t q=0;q<nodes.size();++q){if(nodes[q].kind!=1)continue;
            AcDbObjectPointer<TVS_Pipe> p(nodes[q].id,AcDb::kForRead);if(p.openStatus()!=Acad::eOk||!plainDuct(p.object()))continue;
            const bool first=tvsTeeFit::planDistance(p->FirstPoint,at)<=tol,last=tvsTeeFit::planDistance(p->LastPoint,at)<=tol;
            if(!first&&!last)continue;p.close();prelazBehind(nodes[q].id,first,at,A,B,f);break;}
    }
    return true;
}
// TVS labels of the touched elements (current space) get their text again. Returns the count.
inline int refreshLabels(AcDbDatabase* db){
    std::set<AcDbObjectId> ids;ids.swap(touched());
    if(!db||ids.empty()||!autoLabels())return 0;
    std::vector<AcDbObjectId> labels;MleaderController ml;
    {AcDbObjectPointer<AcDbBlockTableRecord> space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return 0;
     AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return 0;
     for(;!it->done();it->step()){AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id.objectClass()!=AcDbMLeader::desc())continue;
         AcDbObjectId element;if(ml.getObjectId(id,element)&&ids.count(element))labels.push_back(id);}
     delete it;}
    int n=0;for(size_t k=0;k<labels.size();++k)if(ml.recalculateMleader(labels[k]))++n;
    return n;
}
// Spiro prelaz (oba kraja okrugla) poslije promjene precnika: duzina po katalogu SOKO. The end
// `end` (0 = FirstPoint) sits on the duct `ductId`; that end moves along the transition and the
// duct end with it, so the rest of the run stays where it is. Transition open for write.
inline bool spiroLength(TVS_TRANS* t,int end,const AcDbObjectId& ductId,double f){
    if(!t||!tvsSpiro::applies(t->database(),t->SizeAp1,t->ThisRoundp1,t->SizeAp2,t->ThisRoundp2,t->getDuctType()==DuctTypeFlex))return false;
    AcGeVector3d dir=t->VectTr;dir.z=0;const double now=dir.length();if(now<1e-12)return false;dir.normalize();
    const double want=tvsSpiro::drawingLength(t->database(),t->SizeAp1,true,t->SizeAp2,true,now),delta=want-now;
    if(std::fabs(delta)<0.5/f)return false;
    const AcGeVector3d shift=end==0?-dir*delta:dir*delta;
    const AcGePoint3d at=end==0?t->FirstPoint:t->FirstPoint+t->VectTr;   // axis point on the duct side (plan)
    AcDbObjectPointer<TVS_Pipe> d(ductId,AcDb::kForWrite);if(d.openStatus()!=Acad::eOk)return false;
    const bool first=d->FirstPoint.distanceTo(AcGePoint3d(at.x,at.y,d->FirstPoint.z))<=d->LastPoint.distanceTo(AcGePoint3d(at.x,at.y,d->LastPoint.z));
    const AcGePoint3d nearEnd=first?d->FirstPoint:d->LastPoint,other=first?d->LastPoint:d->FirstPoint;
    const AcGePoint3d moved=nearEnd+shift;AcGeVector3d rest=other-moved;rest.z=0;
    if(rest.length()<50.0/f||rest.dotProduct(other-nearEnd)<=0){
        acutPrintf(L"\nSpiro prelaz: kanal je prekratak za katalošku dužinu %.0f mm; dužina prelaza nije promijenjena.",want*f);return false;}
    if((first?d->put_FirstPoint(moved):d->put_Lastpoint(moved))!=Acad::eOk)return false;d->recordGraphicsModified();
    t->assertWriteEnabled();
    if(end==0)t->FirstPoint+=shift;else t->LastPoint+=shift;
    t->put_Length(want);
    acutPrintf(L"\nSpiro prelaz: dužina po katalogu %.0f mm.",want*f);
    return true;
}
// Dialog resources (set by the command: resource.h is not known here).
struct DialogIds {UINT run,tee;};
// Elements the walk does not enter (the T komad whose dialog resized the duct) and the scope
// used when the run dialog is cancelled (from a T komad: prelaz on the joint, as before).
inline std::set<AcDbObjectId>& excluded(){static std::set<AcDbObjectId> s;return s;}
inline int& cancelScope(){static int c=tvsRunDialog::Only;return c;}
inline int& dialogsShown(){static int n=0;return n;}
inline DialogIds& ids(){static DialogIds d={0,0};return d;}
inline std::wstring shortMark(const CString& why){
    if(why.Find(L"T komad")==0||why==L"TK-O")return L"T komad";
    if(why.Find(L"etaž")==0)return L"etaž";if(why.Find(L"obilazak")==0)return L"obilazak";
    if(why==L"prelaz")return L"prelaz";if(why.Find(L"spoj")==0)return L"spoj";
    if(why.Find(L"druge dimenzije")>=0)return L"druga dim.";
    return L"";
}
// Preview of the dialog: the edited ducts, what the runs change, the T komadi with their own
// dialog and the network around them (plan, true widths), the boundaries and the counts.
inline void preview(const std::vector<tvsMove::Node>& nodes,const std::vector<int>& starts,const std::vector<std::pair<int,Run> >& runs,const Etazes& etazes,tvsRunDialog::Preview& pv){
    std::map<int,int> state;
    auto indexOf=[&](const AcDbObjectId& id){for(size_t k=0;k<nodes.size();++k)if(nodes[k].id==id)return static_cast<int>(k);return -1;};
    auto set=[&](int k,int st){if(k>=0&&!state.count(k))state[k]=st;};
    for(size_t k=0;k<starts.size();++k)set(starts[k],tvsRunSketch::Selected);
    for(size_t k=0;k<runs.size();++k){const Run& r=runs[k].second;
        pv.ducts+=static_cast<int>(r.ducts.size());pv.elbows+=static_cast<int>(r.elbows.size());pv.tees+=static_cast<int>(r.tees.size());
        pv.etazes+=static_cast<int>(r.etazes.size());pv.bypasses+=static_cast<int>(r.bypasses.size());pv.teeDialogs+=static_cast<int>(r.dialogs.size());
        pv.metTee+=r.metTee;pv.metEtaz+=r.metEtaz;pv.metBypass+=r.metBypass;
        for(size_t j=0;j<r.dialogs.size();++j)set(r.dialogs[j].node,tvsRunSketch::TeeDialog);
        for(size_t j=0;j<r.ducts.size();++j)set(r.ducts[j],tvsRunSketch::Changes);
        for(size_t j=0;j<r.elbows.size();++j)set(r.elbows[j],tvsRunSketch::Changes);
        for(size_t j=0;j<r.tees.size();++j)set(r.tees[j].node,tvsRunSketch::Changes);
#ifdef TVS_OFFSET_INTEGRATION
        for(size_t j=0;j<r.etazes.size();++j){const tvsOffsetRefit::Assembly& e=etazes[r.etazes[j].index];
            set(indexOf(e.a),tvsRunSketch::Changes);set(indexOf(e.p),tvsRunSketch::Changes);set(indexOf(e.b),tvsRunSketch::Changes);}
#else
        (void)etazes;
#endif
        for(size_t j=0;j<r.bypasses.size();++j)set(indexOf(r.bypasses[j]),tvsRunSketch::Changes);
        for(size_t j=0;j<r.stops.size();++j){const std::wstring t=static_cast<const wchar_t*>(r.stops[j]);
            if(std::find(pv.stops.begin(),pv.stops.end(),t)==pv.stops.end())pv.stops.push_back(t);}
        for(size_t j=0;j<r.marks.size();++j){tvsRunSketch::Mark m;m.at=tvsTeeSketch::Pt(r.marks[j].first.x,r.marks[j].first.y);m.text=shortMark(r.marks[j].second);
            bool dup=false;for(size_t q=0;q<pv.sketch.marks.size();++q)if(std::hypot(pv.sketch.marks[q].at.x-m.at.x,pv.sketch.marks[q].at.y-m.at.y)<1e-6)dup=true;
            if(!dup)pv.sketch.marks.push_back(m);}
    }
    if(state.empty())return;
    // window: the elements of the run and a margin round them; the rest of the network inside it is context
    double x0=1e300,y0=1e300,x1=-1e300,y1=-1e300;
    for(std::map<int,int>::const_iterator it=state.begin();it!=state.end();++it){const tvsMove::Node& n=nodes[it->first];
        for(size_t j=0;j<n.ports.size();++j){if(n.ports[j].x<x0)x0=n.ports[j].x;if(n.ports[j].x>x1)x1=n.ports[j].x;if(n.ports[j].y<y0)y0=n.ports[j].y;if(n.ports[j].y>y1)y1=n.ports[j].y;}}
    const double pad=0.25*std::hypot(x1-x0,y1-y0)+1e-9;x0-=pad;x1+=pad;y0-=pad;y1+=pad;
    auto clip=[&](tvsTeeSketch::Pt& q){if(q.x<x0)q.x=x0;if(q.x>x1)q.x=x1;if(q.y<y0)q.y=y0;if(q.y>y1)q.y=y1;};
    for(size_t k=0;k<nodes.size();++k){const tvsMove::Node& n=nodes[k];const std::map<int,int>::const_iterator st=state.find(static_cast<int>(k));
        if(n.ports.size()<2)continue;
        if(st==state.end()){bool inside=false;for(size_t j=0;j<n.ports.size();++j)if(n.ports[j].x>=x0&&n.ports[j].x<=x1&&n.ports[j].y>=y0&&n.ports[j].y<=y1)inside=true;if(!inside)continue;}
        tvsRunSketch::Element e;e.state=st==state.end()?tvsRunSketch::Context:st->second;
        e.kind=n.kind==2&&n.axes.size()>=2?2:n.kind==4&&n.ports.size()>=3?4:1;
        const int count=e.kind==4?3:2;
        for(int j=0;j<count;++j){e.p[j]=tvsTeeSketch::Pt(n.ports[j].x,n.ports[j].y);e.w[j]=static_cast<int>(n.sections.size())>j?n.sections[j].a:n.a;}
        if(e.kind==2)for(int j=0;j<2;++j)e.dir[j]=tvsTeeSketch::Pt(n.axes[j].x,n.axes[j].y);
        if(e.kind==1&&st==state.end()){clip(e.p[0]);clip(e.p[1]);}   // long ducts leaving the window
        pv.sketch.items.push_back(e);
    }
    if(starts.size()==1){const tvsMove::Node& n=nodes[starts[0]];   // sides of the edited duct (Obuhvat: strana 1 / 2)
        for(int j=0;j<2;++j){tvsRunSketch::Mark m;m.at=tvsTeeSketch::Pt(n.ports[j].x,n.ports[j].y);m.text=j==0?L"1":L"2";m.below=true;pv.sketch.marks.push_back(m);}}
}
inline void settle(AcDbDatabase* db,std::map<AcDbObjectId,int>& work);
inline void run(UINT runDialog=0,UINT teeDialog=0){
    if(runDialog)ids().run=runDialog;if(teeDialog)ids().tee=teeDialog;
    posted()=false;
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();
    // this drawing only: edits waiting in another open drawing stay for it
    std::map<AcDbObjectId,int> work;
    for(std::map<AcDbObjectId,int>::iterator it=pending().begin();it!=pending().end();){
        if(it->first.database()==db){work.insert(*it);pending().erase(it++);}else ++it;}
    if(db&&!work.empty())settle(db,work);
    if(db)refreshLabels(db);   // after the sizes: their own changes are in the set too
    touched().clear();
}
inline void settle(AcDbDatabase* db,std::map<AcDbObjectId,int>& work){
    const double f=tvsUnits::factor(db),tol=1.0/f;
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;
    if(!tvsMove::snapshot(db,nodes,labels)){acutPrintf(L"\nTVS_DIM_PRATI: mreža se ne može pročitati (objekat otvoren u drugoj komandi?); dimenzije na spojevima nisu prilagođene.");return;}
    bool dirty=false;   // geometry changed since the snapshot: read again before the next step
    auto fresh=[&](){if(!dirty)return true;dirty=false;nodes.clear();labels.setLogicalLength(0);return tvsMove::snapshot(db,nodes,labels);};
    auto index=[&](const AcDbObjectId& id){for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==id)return static_cast<int>(i);return -1;};
        const bool wasApplying=applying();applying()=true;   // also when nested: TVS_TK_TIP opened from a run
    int ducts=0,transitions=0;
    std::vector<std::pair<AcDbObjectId,int> > queue(work.begin(),work.end());std::set<AcDbObjectId> done;
    {   // whole run up to the next prelaz: one dialog for all edited ducts (RunDialog.h)
        std::map<AcDbObjectId,Sizes> old;   // of the ducts settled now (other drawings keep theirs)
        for(std::map<AcDbObjectId,int>::const_iterator w=work.begin();w!=work.end();++w){
            std::map<AcDbObjectId,Sizes>::iterator p=previous().find(w->first);if(p!=previous().end()){old.insert(*p);previous().erase(p);}}
        Etazes etazes;
#ifdef TVS_OFFSET_INTEGRATION
        tvsOffsetRefit::assemblies(db,etazes);
#endif
        std::vector<int> starts;   // edited plain ducts with a known old size
        for(std::map<AcDbObjectId,int>::const_iterator w=work.begin();w!=work.end();++w){const int i=index(w->first);
            if(old.count(w->first)&&i>=0&&nodes[i].kind==1&&nodes[i].ports.size()==2)starts.push_back(i);}
        std::vector<std::pair<int,Run> > runs;
        auto collect=[&](const tvsRunDialog::Choices& c){
            runs.clear();std::set<int> seen(starts.begin(),starts.end());
            for(size_t k=0;k<nodes.size();++k)if(excluded().count(nodes[k].id))seen.insert(static_cast<int>(k));
            for(size_t k=0;k<starts.size();++k){Run r;walk(nodes,starts[k],old[nodes[starts[k]].id],tol,etazes,r,seen,c);runs.push_back(std::make_pair(starts[k],r));}};
        auto anything=[&](){for(size_t k=0;k<runs.size();++k)if(!runs[k].second.empty())return true;return false;};
        tvsRunDialog::Choices ch;collect(ch);
        if(anything()){
            auto survey=[&](const tvsRunDialog::Choices& c,tvsRunDialog::Preview& pv){
                const bool whole=c.scope!=tvsRunDialog::Only&&c.scope!=tvsRunDialog::OnlyPrelaz;
                if(whole)collect(c);else{runs.clear();for(size_t k=0;k<starts.size();++k)pv.joints+=joints(nodes,starts[k],old[nodes[starts[k]].id],tol,f,false);}
                preview(nodes,starts,runs,etazes,pv);};
            CString banner;
            if(!starts.empty()){const tvsMove::Node& n=nodes[starts[0]];const Sizes& o=old[n.id];
                if(n.round)banner.Format(L"Kanal  Ø%.0f  →  Ø%.0f mm",o.a[0]*f,n.a*f);else banner.Format(L"Kanal  %.0f × %.0f  →  %.0f × %.0f mm",o.a[0]*f,o.b[0]*f,n.a*f,n.b*f);
                if(starts.size()>1){CString more;more.Format(L"   (+ %d)",static_cast<int>(starts.size())-1);banner+=more;}}
            if(ids().run)++dialogsShown();
            if(ids().run&&!tvsRunDialog::show(survey,banner,ch,ids().run)){ch=tvsRunDialog::Choices();ch.scope=cancelScope();   // Otkaži: the edited duct alone
                acutPrintf(L"\nDionica nije mijenjana; izabrani kanal zadržava novu dimenziju.");}
            if(ch.scope==tvsRunDialog::Only||ch.scope==tvsRunDialog::OnlyPrelaz){
                const bool insert=ch.scope==tvsRunDialog::OnlyPrelaz;int n=0;
                for(size_t k=0;k<starts.size();++k)n+=joints(nodes,starts[k],old[nodes[starts[k]].id],tol,f,insert);
                if(insert){if(n)dirty=true;else acutPrintf(L"\nPrelaz na spoju nije umetnut (spoj sa više elemenata, T komad ili element druge dimenzije).");}
                else if(n)acutPrintf(L"\nUPOZORENJE: na %d spoj(eva) kanal nove dimenzije naliježe na element stare dimenzije bez prelaza (obuhvat „kanal + prelaz na spoju“ ga umeće).",n);
            }else{
                collect(ch);int changed=0;
                struct Arrival {AcDbObjectId tee;int port;tvsTeePort::Section in;};std::vector<Arrival> arrivals;
                for(size_t k=0;k<runs.size();++k){const Run& r=runs[k].second;if(r.empty())continue;std::vector<AcDbObjectId> resized;
                    if(!applyRun(nodes,runs[k].first,r,etazes,tol,resized))continue;
                    changed+=static_cast<int>(r.elbows.size()+r.tees.size()+r.etazes.size()+resized.size());
                    for(size_t j=0;j<resized.size();++j)if(done.insert(resized[j]).second)queue.push_back(std::make_pair(resized[j],3));
                    AcDbObjectPointer<TVS_Pipe> p(nodes[runs[k].first].id,AcDb::kForRead);if(p.openStatus()!=Acad::eOk)continue;
                    for(size_t j=0;j<r.dialogs.size();++j){Arrival a;a.tee=nodes[r.dialogs[j].node].id;a.port=r.dialogs[j].port;
                        a.in.a=p->SizeA;a.in.round=p->ThisRound;a.in.b=p->ThisRound?0:p->SizeB;arrivals.push_back(a);}}
                if(changed){acutPrintf(L"\nDionica promijenjena: elemenata %d.",changed);dirty=true;}
                // TK-O / TK-G on the run: their own dialog, with the new size on the port the run arrived at
                for(size_t k=0;k<arrivals.size();++k){
                    if(tvsTeeDialog::openArriving(arrivals[k].tee,ids().tee,arrivals[k].port,arrivals[k].in))dirty=true;
                    else acutPrintf(L"\nT komad nije mijenjan: na priključku ostaje prelaz na njegovu dimenziju.");}
            }
        }
    }
    bool shapeNote=false;
    for(size_t q=0;q<queue.size();++q){
        if(q>=5000){acutPrintf(L"\nTVS_DIM_PRATI: prekinuto poslije 5000 elemenata; ostatak mreže nije prilagođen.");break;}
        if(!fresh())break;
        const AcDbObjectId id=queue[q].first;const int ends=queue[q].second;const int i=index(id);if(i<0)continue;
        AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)continue;
        if(TVS_Pipe::cast(e.object())){
            // duct -> transitions at its ends
            if(!plainDuct(e.object()))continue;const TVS_Pipe* p=TVS_Pipe::cast(e.object());
            const double A=p->SizeA,B=p->SizeB,level=p->Elev;const bool round=p->ThisRound;e.close();
            for(size_t k=0;k<nodes.size();++k){if(nodes[k].id==id||nodes[k].ports.size()!=2)continue;
                int touching=0;   // ends of node k at an end of the duct (cheap test before opening)
                for(int end=0;end<2;++end)for(int j=0;j<2;++j)if(nearXY(nodes[i].ports[j],nodes[k].ports[end],tol))touching|=1<<end;
                if(!touching)continue;
                {AcDbObjectPointer<AcDbEntity> probe(nodes[k].id,AcDb::kForRead);if(probe.openStatus()!=Acad::eOk||!TVS_TRANS::cast(probe.object()))continue;}
                AcDbObjectPointer<TVS_TRANS> t(nodes[k].id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
                for(int end=0;end<2;++end){
                    if(!(touching&(1<<end)))continue;
                    const bool tRound=end==0?t->ThisRoundp1:t->ThisRoundp2;
                    double& ta=end==0?t->SizeAp1:t->SizeAp2;double& tb=end==0?t->SizeBp1:t->SizeBp2;
                    // several elements on this plan point (ducts one above the other): the one that meets in height
                    if(!tvsFollowRules::joined(othersAt(nodes,i,nodes[k].ports[end],tol),level,tvsFollowRules::height(A,B,round),
                        t->Elev+(end==0?0:tvsTransAlignCmd::endOffset(t.object())),tvsFollowRules::height(ta,tb,tRound),tol))continue;
                    if(tRound!=round){if(!shapeNote){shapeNote=true;acutPrintf(L"\nPrelaz uz izmijenjeni kanal ima drugi oblik na tom kraju (okrugli / pravougaoni): nije mijenjan, prilagodite ga u Svojstvima.");}continue;}
                    if(std::fabs(ta-A)<=1e-9&&std::fabs(tb-B)<=1e-9)continue;
                    const tvsTransAlignCmd::Before was=tvsTransAlignCmd::before(t.object());
                    t->assertWriteEnabled();ta=A;tb=B;
                    tvsTransAlignCmd::afterEdit(t.object(),was,-1,-1);   // outlet moved: TVS_PRELAZ_KOTA asks
                    if(spiroLength(t.object(),end,id,f))dirty=true;      // spiro: duzina po katalogu, kanal skraceni / produzeni
                    t->recordGraphicsModified();++transitions;
                }
            }
        }else if(TVS_TRANS* t=TVS_TRANS::cast(e.object())){
            // transition end -> duct at that end (then on through that duct)
            if(nodes[i].ports.size()!=2)continue;
            double A[2]={t->SizeAp1,t->SizeAp2},B[2]={t->SizeBp1,t->SizeBp2};bool R[2]={t->ThisRoundp1,t->ThisRoundp2};
            const double level[2]={t->Elev,t->Elev+tvsTransAlignCmd::endOffset(t)};e.close();
            for(int end=0;end<2;++end){if(!(ends&(1<<end)))continue;
                for(size_t k=0;k<nodes.size();++k){if(nodes[k].kind!=1||nodes[k].id==id)continue;
                    bool touch=false;for(int j=0;j<2&&j<static_cast<int>(nodes[k].ports.size());++j)if(nearXY(nodes[k].ports[j],nodes[i].ports[end],tol))touch=true;
                    if(!touch)continue;
                    AcDbObjectPointer<TVS_Pipe> p(nodes[k].id,AcDb::kForWrite);
                    if(p.openStatus()!=Acad::eOk){acutPrintf(L"\nKanal uz prelaz se ne može mijenjati (zaključan sloj?): dimenzija nije prenesena.");continue;}
                    if(!plainDuct(p.object())||p->ThisRound!=R[end])continue;
                    if(std::fabs(p->SizeA-A[end])<=1e-9&&std::fabs(p->SizeB-B[end])<=1e-9)continue;
                    if(!tvsFollowRules::joined(othersAt(nodes,i,nodes[i].ports[end],tol),level[end],tvsFollowRules::height(A[end],B[end],R[end]),
                        p->Elev,tvsFollowRules::height(p->SizeA,p->SizeB,p->ThisRound),tol))continue;
                    p->put_SizeA(A[end]);p->put_SizeB(B[end]);p->recordGraphicsModified();++ducts;
                    if(done.insert(nodes[k].id).second)queue.push_back(std::make_pair(nodes[k].id,3));   // its other end
                }
            }
        }
    }
    {   // ducts on a TK-O / TK-G port: the T komad stays, a prelaz is inserted / adjusted at the port
        std::set<AcDbObjectId> changed(done.begin(),done.end());
        for(std::map<AcDbObjectId,int>::const_iterator w=work.begin();w!=work.end();++w)changed.insert(w->first);
        for(std::set<AcDbObjectId>::const_iterator c=changed.begin();c!=changed.end();++c){
            AcDbObjectPointer<AcDbEntity> e(*c,AcDb::kForRead);if(e.openStatus()!=Acad::eOk||!plainDuct(e.object()))continue;
            e.close();tvsTeePort::afterDuct(db,*c);}
    }
        applying()=wasApplying;
    if(ducts||transitions)acutPrintf(L"\nDimenzije na spojevima: prilagodjeno prelaza %d, kanala %d.",transitions,ducts);
}
// TVS_TK_TIP gave the duct on a port another size (same shape): the run of the old size beyond it
// gets the run dialog (the T komad itself is left out). Cancel = prelaz on the far joint.
// A prelaz at the far end takes the new size in settle; false when no dialog was shown.
inline bool followFromTee(const AcDbObjectId& duct,const tvsTeePort::Section& was,const AcDbObjectId& tee){
    AcDbDatabase* db=duct.database();if(!db||!ids().run)return false;
    {AcDbObjectPointer<TVS_Pipe> p(duct,AcDb::kForRead);if(p.openStatus()!=Acad::eOk||!plainDuct(p.object()))return false;}
    Sizes old;old.a[0]=old.a[1]=was.a;old.b[0]=old.b[1]=was.round?0:was.b;
    previous()[duct]=old;std::map<AcDbObjectId,int> work;work[duct]=3;
    const bool had=excluded().count(tee)>0;excluded().insert(tee);const int scope=cancelScope();cancelScope()=tvsRunDialog::OnlyPrelaz;
        const int shown=dialogsShown();settle(db,work);
    cancelScope()=scope;if(!had)excluded().erase(tee);previous().erase(duct);
    return dialogsShown()>shown;   // no run beyond the duct: the caller puts a prelaz on its far end (as before)
}
inline void enableTeeFollow(UINT runDialog,UINT teeDialog){ids().run=runDialog;ids().tee=teeDialog;tvsTeeDialog::followHook()=&followFromTee;}
}

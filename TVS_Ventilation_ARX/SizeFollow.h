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
inline void post(){
    if(posted()||pending().empty())return;
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(!doc)return;
    posted()=true;acDocManager->sendStringToExecute(doc,L"_TVS_DIM_PRATI\n",false,true,false);
}
class DbReactor : public AcDbDatabaseReactor {
public:
    virtual void objectOpenedForModify(const AcDbDatabase*,const AcDbObject* o){
        if(applying()||o->isUndoing())return;Sizes s;if(sizes(o,s))opened()[o->objectId()]=s;
    }
    virtual void objectModified(const AcDbDatabase*,const AcDbObject* o){
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
inline void DbReactor::goodbye(const AcDbDatabase* db){attached().erase(const_cast<AcDbDatabase*>(db));}
class EdReactor : public AcEditorReactor {
public:
    virtual void commandWillStart(const ACHAR* c){command()=c?c:L"";}
    virtual void commandEnded(const ACHAR* c){
        command().Empty();
        if(c&&(!_wcsicmp(c,L"U")||!_wcsicmp(c,L"UNDO")||!_wcsicmp(c,L"REDO")||!_wcsicmp(c,L"MREDO"))){pending().clear();previous().clear();return;}
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
// --- whole run up to the next prelaz (plan 14, phase 1) ---
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
struct Run {std::vector<int> elbows,ducts;std::vector<CString> stops;};
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
// From both ends of duct `start`, through joined ducts and plan elbows of the old size.
inline void walk(const std::vector<tvsMove::Node>& nodes,int start,const Sizes& old,double tol,Run& run,std::set<int>& seen){
    const bool round=nodes[start].round;
    auto same=[&](const tvsMove::Node& n){return n.round==round&&std::fabs(n.a-old.a[0])<=1e-6&&(round||std::fabs(n.b-old.b[0])<=1e-6);};
    for(int side=0;side<2&&side<static_cast<int>(nodes[start].ports.size());++side){
        int prev=start;tvsMove::Point at=nodes[start].ports[side];
        for(int guard=0;guard<2000;++guard){
            int next=-1,count=0;
            for(size_t k=0;k<nodes.size();++k){if(static_cast<int>(k)==prev)continue;
                for(size_t j=0;j<nodes[k].ports.size();++j)
                    if(nearXY(nodes[k].ports[j],at,tol)&&std::fabs(nodes[k].elevation-nodes[prev].elevation)<=tol){next=static_cast<int>(k);++count;break;}}
            if(count==0)break;                                            // free end
            if(count>1){run.stops.push_back(L"spoj sa više elemenata");break;}
            if(seen.count(next))break;                                    // closed loop
            AcDbObjectPointer<AcDbEntity> e(nodes[next].id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)break;
            if(TVS_TRANS::cast(e.object()))break;                         // prelaz: the size follow below
            const tvsMove::Node& n=nodes[next];
            const bool duct=plainDuct(e.object())&&n.kind==1,elbow=plainElbow(e.object())&&n.kind==2;
            if((duct||elbow)&&same(n)&&n.ports.size()==2){
                seen.insert(next);(duct?run.ducts:run.elbows).push_back(next);
                const int far=std::hypot(n.ports[0].x-at.x,n.ports[0].y-at.y)>std::hypot(n.ports[1].x-at.x,n.ports[1].y-at.y)?0:1;
                prev=next;at=n.ports[far];continue;
            }
            run.stops.push_back(duct||elbow?CString(L"element druge dimenzije"):boundary(e.object()));break;
        }
    }
}
// Applies the new size of duct `start` to the run. false: nothing changed (reason printed).
inline bool applyRun(const std::vector<tvsMove::Node>& nodes,int start,const Run& run,double tol,std::vector<AcDbObjectId>& resized){
    double A=0,B=0;bool round=false;
    {AcDbObjectPointer<TVS_Pipe> p(nodes[start].id,AcDb::kForRead);if(p.openStatus()!=Acad::eOk)return false;A=p->SizeA;B=p->SizeB;round=p->ThisRound;}
    std::vector<std::pair<tvsMove::Point,tvsMove::Point> > moves;   // elbow port: old -> new
    for(size_t k=0;k<run.elbows.size();++k){const tvsMove::Node& n=nodes[run.elbows[k]];
        AcDbObjectPointer<TVS_TAP> t(n.id,AcDb::kForRead);tvsMove::Point np[2];
        if(t.openStatus()!=Acad::eOk||!elbowPortsFor(t.object(),A,np)){acutPrintf(L"\nDionica nije promijenjena: koljeno se ne može preračunati.");return false;}
        for(int m=0;m<2;++m)moves.push_back(std::make_pair(n.ports[m],np[m]));
    }
    std::vector<int> ducts=run.ducts;ducts.push_back(start);
    std::vector<std::pair<tvsMove::Point,tvsMove::Point> > ends(ducts.size());
    for(size_t k=0;k<ducts.size();++k){const tvsMove::Node& n=nodes[ducts[k]];tvsMove::Point e[2]={n.ports[0],n.ports[1]};
        for(int j=0;j<2;++j)for(size_t m=0;m<moves.size();++m)if(nearXY(e[j],moves[m].first,tol)){e[j].x=moves[m].second.x;e[j].y=moves[m].second.y;break;}
        const tvsMove::Point v=n.ports[1]-n.ports[0],w=e[1]-e[0];
        if(std::hypot(w.x,w.y)<10*tol||v.x*w.x+v.y*w.y<=0){acutPrintf(L"\nDionica nije promijenjena: kanal bi sa novim koljenima bio prekratak.");return false;}
        ends[k]=std::make_pair(e[0],e[1]);
    }
    for(size_t k=0;k<run.elbows.size();++k){
        AcDbObjectPointer<TVS_TAP> t(nodes[run.elbows[k]].id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        t->put_SizeA(A);if(!round)t->put_SizeB(B);t->recordGraphicsModified();
    }
    for(size_t k=0;k<ducts.size();++k){
        AcDbObjectPointer<TVS_Pipe> p(nodes[ducts[k]].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk)continue;
        p->put_FirstPoint(AcGePoint3d(ends[k].first.x,ends[k].first.y,p->FirstPoint.z));
        p->put_Lastpoint(AcGePoint3d(ends[k].second.x,ends[k].second.y,p->LastPoint.z));
        if(ducts[k]!=start){p->put_SizeA(A);p->put_SizeB(B);resized.push_back(nodes[ducts[k]].id);}
        p->recordGraphicsModified();
    }
    return true;
}
inline void run(){
    posted()=false;
    std::map<AcDbObjectId,int> work;work.swap(pending());
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db||work.empty())return;
    const double f=tvsUnits::factor(db),tol=1.0/f;
    std::vector<tvsMove::Node> nodes;AcDbObjectIdArray labels;if(!tvsMove::snapshot(db,nodes,labels))return;
    auto index=[&](const AcDbObjectId& id){for(size_t i=0;i<nodes.size();++i)if(nodes[i].id==id)return static_cast<int>(i);return -1;};
    applying()=true;
    int ducts=0,transitions=0;
    std::vector<std::pair<AcDbObjectId,int> > queue(work.begin(),work.end());std::set<AcDbObjectId> done;
    {   // whole run up to the next prelaz: ask once for all edited ducts
        std::map<AcDbObjectId,Sizes> old;old.swap(previous());
        std::vector<std::pair<int,Run> > runs;std::set<int> seen;int elbows=0,others=0;std::vector<CString> stops;
        for(std::map<AcDbObjectId,int>::const_iterator w=work.begin();w!=work.end();++w){
            std::map<AcDbObjectId,Sizes>::const_iterator o=old.find(w->first);const int i=index(w->first);
            if(o==old.end()||i<0||nodes[i].kind!=1||nodes[i].ports.size()!=2||seen.count(i))continue;
            seen.insert(i);Run r;walk(nodes,i,o->second,tol,r,seen);
            for(size_t k=0;k<r.stops.size();++k)if(std::find(stops.begin(),stops.end(),r.stops[k])==stops.end())stops.push_back(r.stops[k]);
            if(r.elbows.empty()&&r.ducts.empty())continue;
            elbows+=static_cast<int>(r.elbows.size());others+=static_cast<int>(r.ducts.size());runs.push_back(std::make_pair(i,r));
        }
        if(!runs.empty()){
            if(!stops.empty()){CString list;for(size_t k=0;k<stops.size();++k){if(k)list+=L", ";list+=stops[k];}
                acutPrintf(L"\nDionica staje na: %ls (faza 1: ti elementi se ne mijenjaju).",list.GetString());}
            wchar_t prompt[200];swprintf_s(prompt,L"\nPromijeniti cijelu dionicu do prelaza (koljena %d, kanala %d)? [Da/Ne] <Da>: ",elbows,others);
            acedInitGet(0,L"Da Ne");ACHAR kw[16]=L"";const int rc=acedGetKword(prompt,kw);
            if(rc==RTNONE||(rc==RTNORM&&_wcsicmp(kw,L"Da")==0)){
                int changed=0;
                for(size_t k=0;k<runs.size();++k){std::vector<AcDbObjectId> resized;
                    if(!applyRun(nodes,runs[k].first,runs[k].second,tol,resized))continue;
                    changed+=static_cast<int>(runs[k].second.elbows.size()+resized.size());
                    for(size_t j=0;j<resized.size();++j)if(done.insert(resized[j]).second)queue.push_back(std::make_pair(resized[j],3));}
                if(changed)acutPrintf(L"\nDionica promijenjena: elemenata %d.",changed);
            }
        }
    }
    for(size_t q=0;q<queue.size()&&q<500;++q){
        const AcDbObjectId id=queue[q].first;const int ends=queue[q].second;const int i=index(id);if(i<0)continue;
        AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)continue;
        if(TVS_Pipe::cast(e.object())){
            // duct -> transitions at its ends
            if(!plainDuct(e.object()))continue;const TVS_Pipe* p=TVS_Pipe::cast(e.object());
            const double A=p->SizeA,B=p->SizeB;const bool round=p->ThisRound;e.close();
            for(size_t k=0;k<nodes.size();++k){if(nodes[k].id==id||nodes[k].ports.size()!=2)continue;
                int touching=0;   // ends of node k at an end of the duct (cheap test before opening)
                for(int end=0;end<2;++end)for(int j=0;j<2;++j)if(nearXY(nodes[i].ports[j],nodes[k].ports[end],tol))touching|=1<<end;
                if(!touching)continue;
                {AcDbObjectPointer<AcDbEntity> probe(nodes[k].id,AcDb::kForRead);if(probe.openStatus()!=Acad::eOk||!TVS_TRANS::cast(probe.object()))continue;}
                AcDbObjectPointer<TVS_TRANS> t(nodes[k].id,AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
                for(int end=0;end<2;++end){
                    if(!(touching&(1<<end)))continue;
                    const bool tRound=end==0?t->ThisRoundp1:t->ThisRoundp2;if(tRound!=round)continue;
                    double& ta=end==0?t->SizeAp1:t->SizeAp2;double& tb=end==0?t->SizeBp1:t->SizeBp2;
                    if(std::fabs(ta-A)<=1e-9&&std::fabs(tb-B)<=1e-9)continue;
                    const tvsTransAlignCmd::Before was=tvsTransAlignCmd::before(t.object());
                    t->assertWriteEnabled();ta=A;tb=B;
                    tvsTransAlignCmd::afterEdit(t.object(),was,-1,-1);   // outlet moved: TVS_PRELAZ_KOTA asks
                    t->recordGraphicsModified();++transitions;
                }
            }
        }else if(TVS_TRANS* t=TVS_TRANS::cast(e.object())){
            // transition end -> duct at that end (then on through that duct)
            if(nodes[i].ports.size()!=2)continue;
            double A[2]={t->SizeAp1,t->SizeAp2},B[2]={t->SizeBp1,t->SizeBp2};bool R[2]={t->ThisRoundp1,t->ThisRoundp2};e.close();
            for(int end=0;end<2;++end){if(!(ends&(1<<end)))continue;
                for(size_t k=0;k<nodes.size();++k){if(nodes[k].kind!=1||nodes[k].id==id)continue;
                    bool touch=false;for(int j=0;j<2&&j<static_cast<int>(nodes[k].ports.size());++j)if(nearXY(nodes[k].ports[j],nodes[i].ports[end],tol))touch=true;
                    if(!touch)continue;
                    AcDbObjectPointer<TVS_Pipe> p(nodes[k].id,AcDb::kForWrite);if(p.openStatus()!=Acad::eOk||!plainDuct(p.object())||p->ThisRound!=R[end])continue;
                    if(std::fabs(p->SizeA-A[end])<=1e-9&&std::fabs(p->SizeB-B[end])<=1e-9)continue;
                    p->put_SizeA(A[end]);p->put_SizeB(B[end]);p->recordGraphicsModified();++ducts;
                    if(done.insert(nodes[k].id).second)queue.push_back(std::make_pair(nodes[k].id,3));   // its other end
                }
            }
        }
    }
    applying()=false;pending().clear();
    if(ducts||transitions)acutPrintf(L"\nDimenzije na spojevima: prilagodjeno prelaza %d, kanala %d.",transitions,ducts);
}
}

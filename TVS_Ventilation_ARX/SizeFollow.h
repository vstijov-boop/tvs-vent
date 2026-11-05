#pragma once
// Prelaz i kanali dijele dimenziju na spoju. When the size of a duct changes (Svojstva, double
// click, Properties), the transition at that duct end takes the new size on that end; when a
// transition end size changes, the duct at that end takes it (and so on through the next duct
// to the transition at its other end). Other ends never change. Changes made by TVS commands
// (drawing, TVS_MATCH, moves ...) are left to those commands. Settled by TVS_DIM_PRATI, run
// automatically after the edit; the transition's own height alignment then works as before
// (TVS_PRELAZ_KOTA asks which side stays if the outlet moved).
#include "TVSMoveCommand.h"
#include "TransAlignCommand.h"
#include "acdocman.h"
#include "aced.h"
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
        if(c&&(!_wcsicmp(c,L"U")||!_wcsicmp(c,L"UNDO")||!_wcsicmp(c,L"REDO")||!_wcsicmp(c,L"MREDO"))){pending().clear();return;}
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

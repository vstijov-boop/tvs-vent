#pragma once
#ifdef TVS_OFFSET_INTEGRATION
// Whole-assembly MOVE/ROTATE ("cijeli sklop ili ništa"). AutoCAD transforms each entity on
// its own, so a member cannot see the rest of the selection. The selection of the running
// command (ssget result via AcEdSSGetFilter::endSSGet, plus pickfirst/grip set at command
// start) is expanded here, while nothing is open for write: members of every V1 offset
// triple or bypass whose members are ALL selected are permitted in the DBX guard. Partial
// selections, scale and mirror stay refused; one summary line at command end.
#include "acssgetfilter.h"
#include "acdocman.h"
#include "dbobjptr.h"
#include "OffsetAssemblyData.h"
#include "BypassAssemblyData.h"
#include <set>
#include <vector>
namespace tvsAssemblyMove {
inline std::set<AcDbObjectId>& selected(){static std::set<AcDbObjectId> s;return s;}
inline int& partial(){static int n=0;return n;}
inline void permitComplete(){
    tvsOffsetData::clearAssemblyTransforms();partial()=0;std::set<AcDbObjectId> done;
    for(const AcDbObjectId& id:selected()){
        if(done.count(id))continue;
        std::vector<AcDbObjectId> need;
        {AcDbObjectPointer<AcDbObject> o(id,AcDb::kForRead);if(o.openStatus()!=Acad::eOk||!tvsOffsetData::marked(o.object()))continue;
         AcDbObjectId all[7];
         if(tvsBypassData::members(o.object(),all))need.assign(all,all+7);
         else{tvsOffsetData::Record r;if(tvsOffsetData::read(o.object(),r)==tvsOffsetData::Valid)need.assign(r.members,r.members+3);}}
        if(need.empty())continue;
        bool complete=true;for(const AcDbObjectId& n:need){done.insert(n);if(!selected().count(n))complete=false;}
        if(complete){for(const AcDbObjectId& n:need)tvsOffsetData::permitAssemblyTransform(n);}else ++partial();
    }
}
inline void add(const AcDbObjectIdArray& ids){for(int i=0;i<ids.length();++i)selected().insert(ids[i]);}
inline void addSet(resbuf* rb){
    for(resbuf* p=rb;p;p=p->rbnext){if(p->restype!=RTPICKS)continue;Adesk::Int32 n=0;if(acedSSLength(p->resval.rlname,&n)!=RTNORM)continue;
        for(Adesk::Int32 i=0;i<n;++i){ads_name e;AcDbObjectId id;if(acedSSName(p->resval.rlname,i,e)==RTNORM&&acdbGetObjectId(id,e)==Acad::eOk)selected().insert(id);}}
}
inline void reset(){selected().clear();tvsOffsetData::clearAssemblyTransforms();partial()=0;}
// Command start: noun-verb (pickfirst) and grip editing use the current sets.
inline void begin(){
    reset();resbuf* grip=NULL;resbuf* pick=NULL;
    if(acedSSGetFirst(&grip,&pick)==RTNORM){addSet(grip);addSet(pick);}
    if(grip)acutRelRb(grip);if(pick)acutRelRb(pick);
    permitComplete();
}
inline void end(){
    const int refused=tvsOffsetData::takeTransformRefusals();
    if(refused>0)acutPrintf(L"\nTVS: %d clan(ova) obilaska/offseta nije pomjereno. Sklop se pomjera samo cijeli (svi clanovi u izboru), MOVE/ROTATE oko Z; SCALE/MIRROR i djelimicni izbor nisu dozvoljeni.",refused);
    reset();
}
class SelectionFilter : public AcEdSSGetFilter {
public:
    virtual void endSSGet(Acad::PromptStatus status,int,AcEdSelectionSetService&,const AcDbObjectIdArray& ids) override{
        if(status!=Acad::eNormal)return;selected().clear();add(ids);
        resbuf* grip=NULL;resbuf* pick=NULL;if(acedSSGetFirst(&grip,&pick)==RTNORM){addSet(grip);}if(grip)acutRelRb(grip);if(pick)acutRelRb(pick);
        permitComplete();
    }
};
inline SelectionFilter& filter(){static SelectionFilter f;return f;}
inline std::set<AcApDocument*>& attached(){static std::set<AcApDocument*> d;return d;}
inline void attach(AcApDocument* doc){if(doc&&attached().insert(doc).second)addSSgetFilterInputContextReactor(doc,&filter());}
inline void detach(AcApDocument* doc){if(doc&&attached().erase(doc))removeSSgetFilterInputContextReactor(doc,&filter());}
inline void attachAll(bool on){
    if(!acDocManager)return;AcApDocumentIterator* it=acDocManager->newAcApDocumentIterator();if(!it)return;
    for(;!it->done();it->step())on?attach(it->document()):detach(it->document());delete it;
}
}
#endif

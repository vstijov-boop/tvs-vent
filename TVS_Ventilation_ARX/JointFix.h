#pragma once
// Joints the network commands refuse (TVS_STRETCH / TVS_MOVE / TVS_ALIGN: "Presjeci ili elevacije
// na spoju ne odgovaraju") that have an obvious repair, offered instead of a bare refusal:
//  - plain duct and the other element with the same section, another axis level: the duct takes
//    the level of the other element (Izjednačiti kotu?),
//  - duct on a port of a TK-O / TK-G with another size: the prelaz at the port (tvsTeePort::ensure,
//    as TVS_TK_TIP / TVS_DIM_PRATI do).
// And after TVS_DRAW / TVS_PIPE: a new duct ending on a TK-O / TK-G port gets the prelaz too.
#include "TVSMoveCommand.h"
#include "TeePort.h"
#include <cmath>
namespace tvsJointFix {
inline bool ask(const wchar_t* prompt){
    acedInitGet(0,L"Da Ne");ACHAR kw[16]=L"";const int rc=acedGetKword(prompt,kw,16);
    return rc==RTNONE||(rc==RTNORM&&_wcsicmp(kw,L"Da")==0);
}
inline bool plainDuct(const AcDbObjectId& id){
    AcDbObjectPointer<TVS_Pipe> p(id,AcDb::kForRead);return p.openStatus()==Acad::eOk&&p->DuctType==DuctTypeStill;
}
inline bool fix(const std::vector<tvsMove::Node>& nodes,const std::vector<tvsMoveNetwork::Body>& bodies,int a,int pa,int b,int pb,const ACHAR* command){
    if(a<0||b<0||static_cast<size_t>(a)>=nodes.size()||static_cast<size_t>(b)>=nodes.size())return false;
    // the duct side d (a plain duct), the other side o
    int d=-1,pd=-1,o=-1,po=-1;
    if(nodes[a].kind==1&&plainDuct(nodes[a].id)){d=a;pd=pa;o=b;po=pb;}
    else if(nodes[b].kind==1&&plainDuct(nodes[b].id)){d=b;pd=pb;o=a;po=pa;}
    if(d<0||static_cast<size_t>(pd)>=bodies[d].sections.size()||static_cast<size_t>(po)>=bodies[o].sections.size())return false;
    const tvsMoveNetwork::Section& sd=bodies[d].sections[pd];const tvsMoveNetwork::Section& so=bodies[o].sections[po];
    const double f=tvsUnits::factor(acdbHostApplicationServices()->workingDatabase()),tol=0.1/f;
    const bool sameSection=sd.round==so.round&&std::fabs(sd.a-so.a)<=tol&&(sd.round||std::fabs(sd.b-so.b)<=tol);
    bool changed=false;
    if(std::fabs(sd.elevation-so.elevation)>tol){
        wchar_t prompt[220];swprintf_s(prompt,L"\nIzjednačiti kotu kanala (osa %.1f mm) sa susjednim elementom (osa %.1f mm)? [Da/Ne] <Da>: ",sd.elevation*f,so.elevation*f);
        if(!ask(prompt))return false;
        AcDbObjectPointer<TVS_Pipe> p(nodes[d].id,AcDb::kForWrite);
        if(p.openStatus()!=Acad::eOk||p->put_Elevation(so.elevation)!=Acad::eOk){acutPrintf(L"\nKota kanala se ne može promijeniti (zaključan sloj?)."); return false;}
        p->recordGraphicsModified();changed=true;
        acutPrintf(L"\nKota kanala: osa %.1f mm. Provjerite drugi kraj kanala.",so.elevation*f);
    }
    if(!sameSection){
        AcDbObjectPointer<TVS_WYE> w(nodes[o].id,AcDb::kForRead);
        if(w.openStatus()==Acad::eOk&&tvsTeePort::keeps(w.object())){
            int k=-1;const tvsMove::Point at=bodies[o].ports[po];
            for(int j=0;j<3;++j){tvsTeePort::Port P;if(tvsTeePort::port(w.object(),j,P)&&std::hypot(P.p.x-at.x,P.p.y-at.y)<=2.0/f)k=j;}
            w.close();
            if(k>=0&&ask(L"\nKanal druge dimenzije na priključku T komada: umetnuti prelaz? [Da/Ne] <Da>: ")){
                if(tvsTeePort::ensure(nodes[o].id,k))changed=true;
            }
        }else if(!changed)acutPrintf(L"\nSpoj kanala druge dimenzije: umetnite prelaz ili izjednačite dimenzije u Svojstvima.");
    }
    if(changed)acutPrintf(L"\nSpoj je popravljen. Ponovite %ls.",command);
    return changed;
}
// TVS_DRAW / TVS_PIPE: ducts made by the command (handle above the seed at its start) ending on a
// TK-O / TK-G port of another size get the prelaz there (destructor: every exit of the command).
inline unsigned long long value(const AcDbHandle& h){return (static_cast<unsigned long long>(h.high())<<32)|h.low();}
struct AfterDrawing {
    AcDbDatabase* db;unsigned long long seed;
    AfterDrawing():db(acdbHostApplicationServices()->workingDatabase()),seed(0){if(db)seed=value(db->handseed());}
    ~AfterDrawing(){
        if(!db)return;std::vector<AcDbObjectId> made;
        {AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForRead);AcDbBlockTableRecordIterator* it=NULL;
         if(space.openStatus()!=Acad::eOk||space->newIterator(it)!=Acad::eOk)return;
         for(;!it->done();it->step()){AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id.objectClass()!=TVS_Pipe::desc())continue;
             if(value(id.handle())>=seed)made.push_back(id);}
         delete it;}
        for(size_t k=0;k<made.size();++k)tvsTeePort::afterDuct(db,made[k]);
    }
};
inline void install(){tvsMove::jointFixHook()=&fix;}
}

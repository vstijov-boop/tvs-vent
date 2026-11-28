#pragma once
// TVS_POPRAVI (korisnik 24.11.): pregled crteža rađenog starijim buildom i popravka zapisa koje nove
// funkcije drugačije čitaju. Prvo izvještaj, onda „Popraviti? [Da/Ne]“; sve u jednoj komandi (jedan U).
// Popravlja (bez pomjeranja geometrije):
//  1. štucna: polje „second“ koje nije baza konusa tog ogranka (npr. širina prolaza C ostala od TK-O)
//     -> 0 (cilindrična štucna, kao što ju je stari build crtao);
//  2. zapis T komada sa nevažećim brojevima (NaN, negativno) -> očišćen;
//  3. konektori koji pokazuju na obrisan / nepostojeći / ne-TVS objekat -> prazni;
//  4. povezana oprema bez zapamćene tačke povezivanja (stariji build): tačka se upiše kad priključci
//     leže na bloku; inače se navede (blok pomjeren poslije povezivanja -> TVS_OPREMA).
//  5. ravni T komad (TK0) bočne rešetke iz starijeg builda: ogranak T komada je kanal koji na drugom
//     kraju ima priključak rešetke / difuzora -> zapis TVS_TEE_OPEN_V1 (bez poprečnih linija na
//     krajevima, kao TVS_RESETKA od 24.11.).
//  6. zapis krstastog komada / XK (TVS_TEE_X1, 28.11.) koji se ne može nacrtati – nevažeći brojevi,
//     T komad promijenjen u TK-O / TK-G / štucnu, prvi ogranak suziti ispod drugog -> uklonjen (ostaje T komad).
// Samo navodi (izabrani na kraju): elementi sa nevažećom geometrijom (NaN, dimenzija <= 0).
#include "TVSMoveCommand.h"
#include "EquipmentStore.h"
#include "CheckCommand.h"
#include "TeeNeighbours.h"
#include "../TVS_Ventilation_DBX/TeeOpenEnds.h"
#include "../TVS_Ventilation_DBX/TeeData.h"
#include "../TVS_Ventilation_DBX/TeeCross.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../geometry/TeeShape.h"
#include "../geometry/EquipmentFlow.h"
#include <cmath>
#include <vector>
namespace tvsRepair {
inline bool bad(double v){return !std::isfinite(v);}
inline bool badPoint(const AcGePoint3d& p){return bad(p.x)||bad(p.y)||bad(p.z);}
struct Found {
    std::vector<AcDbObjectId> tapSecond,teeRecord,connector,anchor,detached,geometry,openEnds,cross;
    int connectors;Found():connectors(0){}
    bool any()const{return !tapSecond.empty()||!teeRecord.empty()||!connector.empty()||!anchor.empty()||!openEnds.empty()||!cross.empty();}
};
// A connector target that is not a live TVS element.
inline bool deadTarget(const AcDbObjectId& id){
    if(id.isNull())return false;
    if(!id.isValid()||id.isErased())return true;
    AcDbObjectPointer<TVS_Entity> e(id,AcDb::kForRead);
    return e.openStatus()!=Acad::eOk&&e.openStatus()!=Acad::eWasOpenForWrite&&e.openStatus()!=Acad::eWasOpenForRead;
}
inline bool teeRecordBad(const tvsTeeData::Record& r){
    return r.type<0||r.type>4||bad(r.l)||bad(r.second)||bad(r.radius2)||bad(r.neck)||r.l<0||r.radius2<0;
}
// Ports of an old equipment record lie on the block (its extents + 1 mm): it was not moved since.
inline bool portsOnBlock(const AcDbBlockReference* b,const tvsEquipment::Record& r,double f){
    AcDbExtents x;if(b->getGeomExtents(x)!=Acad::eOk)return false;
    const double s=tvsEquipmentFlow::unitScale(r.mmPerUnit,f),tol=1.0/f;
    for(int k=0;k<r.ports&&k<2;++k){const double px=r.port[k].x*s,py=r.port[k].y*s;
        if(px<x.minPoint().x-tol||px>x.maxPoint().x+tol||py<x.minPoint().y-tol||py>x.maxPoint().y+tol)return false;}
    return r.ports>0;
}
inline void scan(AcDbDatabase* db,Found& out){
    const double f=tvsUnits::factor(db);
    AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return;
    struct Tk0 {AcDbObjectId id;AcGePoint3d branch;};std::vector<Tk0> tk0;   // TK0 without TVS_TEE_OPEN_V1
    std::vector<std::pair<AcGePoint3d,AcGePoint3d> > ducts;std::vector<AcGePoint3d> terminals;
    for(;!it->done();it->step()){
        AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
        const AcDbObjectId id=e->objectId();
        if(TVS_Entity* t=TVS_Entity::cast(e)){
            for(size_t c=0;c<t->connectors.size();++c)if(deadTarget(t->connectors[c].connectionID)){++out.connectors;
                if(out.connector.empty()||out.connector.back()!=id)out.connector.push_back(id);}
        }
        if(TVS_WYE* w=TVS_WYE::cast(e)){
            if(tvsTeeCross::present(w)){AcGePoint3d xp;AcGeVector3d xd;double xa,xb;bool xr;if(!w->crossPort(xp,xd,xa,xb,xr))out.cross.push_back(id);}
            tvsTeeData::Record r;
            if(tvsTeeData::read(w,r)){
                if(teeRecordBad(r))out.teeRecord.push_back(id);
                else if(r.type==tvsTee::Tap&&r.second>0&&!(w->ThisRoundotv&&tvsTee::isCone(r.second,w->SizeAotv,r.l)))out.tapSecond.push_back(id);
            }
            else if(!w->This1D&&!tvsTeeOpenEnds::read(w)){const tvsTeeFit::Ports q=tvsTeeFit::ports(w);if(q.ok){Tk0 t={id,q.p[2]};tk0.push_back(t);}}
            if(badPoint(w->Basepoint)||bad(w->SizeApr)||bad(w->SizeAotv)||!(w->SizeApr>0)||!(w->SizeAotv>0))out.geometry.push_back(id);
        }
        else if(TVS_Pipe* p=TVS_Pipe::cast(e)){
            if(badPoint(p->FirstPoint)||badPoint(p->LastPoint)||bad(p->SizeA)||!(p->SizeA>0))out.geometry.push_back(id);
            else ducts.push_back(std::make_pair(p->FirstPoint,p->LastPoint));
        }
        else if(TVS_TAP* a=TVS_TAP::cast(e)){
            if(badPoint(a->MiddlePoint)||bad(a->SizeA)||!(a->SizeA>0)||bad(a->Swectangle))out.geometry.push_back(id);
        }
        else if(AcDbBlockReference* b=AcDbBlockReference::cast(e)){
            tvsEquipment::Record r;
            const bool linked=tvsEquipmentStore::read(b,r);
            if(linked&&!r.anchored){if(portsOnBlock(b,r,f))out.anchor.push_back(id);else out.detached.push_back(id);}
            if(linked&&r.terminal()&&r.mount!=tvsEquipment::Bottom){const double s=tvsEquipmentFlow::unitScale(r.mmPerUnit,f);
                for(int k=0;k<(r.twin?2:1);++k)terminals.push_back(AcGePoint3d(r.port[k].x*s,r.port[k].y*s,0));}
        }
        e->close();
    }
    delete it;
    // TK0 whose branch is a duct ending at a grille / diffuser port: a side grille (TVS_RESETKA)
    const double tol=1.0/f;
    auto terminalAt=[&](const AcGePoint3d& q){for(size_t k=0;k<terminals.size();++k)if(tvsTeeFit::planDistance(terminals[k],q)<=tol)return true;return false;};
    for(size_t k=0;k<tk0.size();++k)for(size_t d=0;d<ducts.size();++d){
        const AcGePoint3d& a=ducts[d].first;const AcGePoint3d& b=ducts[d].second;
        if((tvsTeeFit::planDistance(a,tk0[k].branch)<=tol&&terminalAt(b))||(tvsTeeFit::planDistance(b,tk0[k].branch)<=tol&&terminalAt(a))){out.openEnds.push_back(tk0[k].id);break;}}
}
inline int fix(const Found& w){
    int n=0;
    for(size_t k=0;k<w.tapSecond.size();++k){AcDbObjectPointer<TVS_WYE> t(w.tapSecond[k],AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        tvsTeeData::Record r;if(!tvsTeeData::read(t.object(),r))continue;r.second=0;
        if(tvsTeeData::write(t.object(),r)==Acad::eOk){t->recordGraphicsModified();++n;}}
    for(size_t k=0;k<w.teeRecord.size();++k){AcDbObjectPointer<TVS_WYE> t(w.teeRecord[k],AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        tvsTeeData::Record r;if(!tvsTeeData::read(t.object(),r))continue;
        if(r.type<0||r.type>4)r.type=0;
        if(bad(r.l)||r.l<0)r.l=0;if(bad(r.second))r.second=0;if(bad(r.radius2)||r.radius2<0)r.radius2=0;if(bad(r.neck))r.neck=0;
        if(tvsTeeData::write(t.object(),r)==Acad::eOk){t->recordGraphicsModified();++n;}}
    for(size_t k=0;k<w.connector.size();++k){AcDbObjectPointer<TVS_Entity> t(w.connector[k],AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        std::vector<AcDbObjectId> dead;
        for(size_t c=0;c<t->connectors.size();++c)if(deadTarget(t->connectors[c].connectionID))dead.push_back(t->connectors[c].connectionID);
        if(dead.empty())continue;
        t->assertWriteEnabled();
        for(size_t c=0;c<t->connectors.size();++c)for(size_t d=0;d<dead.size();++d)
            if(t->connectors[c].connectionID==dead[d])t->connectors[c].connectionID=AcDbObjectId::kNull;
        ++n;}
    for(size_t k=0;k<w.anchor.size();++k){AcDbObjectPointer<AcDbBlockReference> b(w.anchor[k],AcDb::kForWrite);if(b.openStatus()!=Acad::eOk)continue;
        tvsEquipment::Record r;if(!tvsEquipmentStore::read(b.object(),r))continue;tvsEquipmentStore::anchor(b.object(),r);
        if(tvsEquipmentStore::write(b.object(),r)==Acad::eOk)++n;}
    for(size_t k=0;k<w.openEnds.size();++k){AcDbObjectPointer<TVS_WYE> t(w.openEnds[k],AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        if(tvsTeeOpenEnds::write(t.object(),true)==Acad::eOk)++n;}
    for(size_t k=0;k<w.cross.size();++k){AcDbObjectPointer<TVS_WYE> t(w.cross[k],AcDb::kForWrite);if(t.openStatus()!=Acad::eOk)continue;
        if(tvsTeeCross::write(t.object(),NULL)==Acad::eOk)++n;}
    return n;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    Found w;scan(db,w);
    acutPrintf(L"\nTVS_POPRAVI: pregled crteža (trenutni prostor).");
    acutPrintf(L"\n  štucne sa zaostalim podatkom (nisu konusne): %d",static_cast<int>(w.tapSecond.size()));
    acutPrintf(L"\n  zapisi T komada sa nevažećim brojevima: %d",static_cast<int>(w.teeRecord.size()));
    acutPrintf(L"\n  konektori prema obrisanom / nepostojećem objektu: %d (elemenata %d)",w.connectors,static_cast<int>(w.connector.size()));
    acutPrintf(L"\n  oprema povezana starijim buildom (upisuje se tačka povezivanja): %d",static_cast<int>(w.anchor.size()));
    acutPrintf(L"\n  ravni T komadi bočnih rešetaka (crtaju se bez poprečnih linija na krajevima): %d",static_cast<int>(w.openEnds.size()));
    acutPrintf(L"\n  zapisi krstastog komada / XK koji se ne mogu nacrtati (uklanjaju se, ostaje T komad): %d",static_cast<int>(w.cross.size()));
    if(!w.detached.empty())acutPrintf(L"\n  oprema pomjerena poslije povezivanja (ponovo TVS_OPREMA): %d",static_cast<int>(w.detached.size()));
    if(!w.geometry.empty())acutPrintf(L"\n  elementi sa nevažećom geometrijom (NaN / dimenzija <= 0) – samo navedeni, izabrani na kraju: %d",static_cast<int>(w.geometry.size()));
    if(w.any()){
        acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";
        const int rc=acedGetKword(L"\nPopraviti? [Da/Ne] <Da>: ",kw,8);
        if(rc==RTNONE||(rc==RTNORM&&_wcsicmp(kw,L"Da")==0)){const int n=fix(w);acutPrintf(L"\nTVS_POPRAVI: popravljeno elemenata %d (U vraća sve).",n);}
        else if(rc!=RTNORM)return;
    }else acutPrintf(L"\nTVS_POPRAVI: nema zapisa za popravku.");
    std::vector<AcDbObjectId> pick=w.geometry;pick.insert(pick.end(),w.detached.begin(),w.detached.end());
    if(!pick.empty())tvsCheck::select(pick);
    acutPrintf(L"\nSljedeće: TVS_PROVJERA (spojevi, kote, prelazi) i TVS_KOLJENA_UGAO (zaokruženi uglovi koljena).");
}
}

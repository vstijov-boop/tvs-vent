#pragma once
// TVS_OPREMA: obični AutoCAD blokovi koji su već nacrtani na kanalu (regulator protoka, PP klapna,
// CAV / VAV, prigušivač = u liniji, 2 priključka; rešetka, difuzor, kapa, žaluzina = terminal na
// slobodnom kraju, 1 priključak) se povezuju u TVS mrežu NA MJESTU - blok se nikad ne pomjera.
// Geometrija i odluka: geometry/EquipmentLink.h (slučaj 1 kanali već naliježu na blok, slučaj 2 krajevi
// kanala se skrate / produže do lica bloka, slučaj 3 kontinualan kanal se presiječe za dužinu bloka).
// Zapis: tvsEquipmentStore::write (EquipmentStore.h, XRecord TVS_OPREMA_V1 na bloku); vrsta, model
// gubitka, opis i dužina iz podrazumijevanog zapisa za naziv bloka (TVS_OPREMA_TIPOVI) kad postoji.
// Jedan blok = jedna transakcija (tvsAction::Transaction) uz provjeru prije upisa (zaključan sloj,
// etaž / obilazak): odbijen blok ne mijenja ništa.
// TVS_OPREMA_ODVEZI: zapis se briše, kanali ostaju kakvi jesu.
// Terminal po baznoj tački (crteži korisnika 23.11.): bazna tačka bloka na slobodnom kraju kanala / flexa /
// kratkog nastavka ili na slobodnom vertikalnom izlazu koljena gore / dolje = priključak, bez skraćivanja
// i bez gabarita bloka. Rešetka (vrsta terminal) preko kontinualnog kanala = rešetka na dnu kanala (mount
// Bottom, kanal se ne siječe); blok bez podrazumijevanog zapisa preko kanala: pitanje Dno / Linija jednom po
// nazivu bloka (pamti se kao podrazumijevana vrsta). Na kraju: blokovi bez podešene vrste -> TVS_OPREMA_SVOJSTVA.
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "ActionTransaction.h"
#include "BuildTransaction.h"
#include "EquipmentStore.h"
#include "EquipmentDialog.h"
#include "FanCommand.h"
#include "resource.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../geometry/EquipmentLink.h"
#include "../geometry/RoundElbow.h"
#include <dbdynblk.h>
#include <dbents.h>
#include <cmath>
#include <map>
#include <functional>
#include <set>
#include <string>
#include <vector>
namespace tvsEquipmentLinkCmd {
namespace lk=tvsEquipmentLink;
inline double& gapMm(){static double v=150;return v;}   // largest gap / overlap closed (Tolerancija)
const double axisTolMm=25,minPieceMm=50,tolMm=1;
// reasons of this command after the geometric ones (lk::Error)
enum Extra {NotPlan=lk::errorCount,NoGeometry,Assembly,Locked,WriteFailed,NotInRow,NotLeaning,reasonCount};
inline const wchar_t* reasonText(int e){
    switch(e){
    case NotPlan:return L"blok nije u osnovi (3D rotacija)";
    case NoGeometry:return L"blok nema geometriju";
    case Assembly:return L"kanal je dio etaža / obilaska";
    case Locked:return L"kanal ili blok je na zaključanom sloju";
    case WriteFailed:return L"upis nije uspio";
    case NotLeaning:return L"blok ne naliježe na susjedni povezani blok (razmak / preklop) – primaknite ga ili povežite cijeli niz zajedno";
    case NotInRow:return L"blokovi koji se dodiruju nisu jedan za drugim na osi kanala (niz opreme)";
    default:return lk::reasonText(e);
    }
}
inline std::wstring handleText(const AcDbObjectId& id){
    const AcDbHandle h=id.handle();wchar_t s[40];
    if(h.high())swprintf_s(s,L"%X%08X",h.high(),h.low());else swprintf_s(s,L"%X",h.low());
    return s;
}
inline std::wstring blockName(const AcDbObjectId& refId,const AcDbObjectId& btrId){
    std::wstring name;
    {AcDbBlockTableRecordPointer btr(btrId,AcDb::kForRead);
     if(btr.openStatus()==Acad::eOk){const ACHAR* n=NULL;if(btr->getName(n)==Acad::eOk&&n)name=n;}}
    AcDbDynBlockReference dyn(refId);
    if(dyn.isDynamicBlock()){AcDbBlockTableRecordPointer btr(dyn.dynamicBlockTableRecord(),AcDb::kForRead);
        if(btr.openStatus()==Acad::eOk){const ACHAR* n=NULL;if(btr->getName(n)==Acad::eOk&&n)name=n;}}
    return name;
}
inline bool inAssembly(const AcDbObjectId& id){
#ifdef TVS_OFFSET_INTEGRATION
    AcDbObjectPointer<AcDbEntity> e(id,AcDb::kForRead);if(e.openStatus()!=Acad::eOk)return true;
    return tvsOffsetData::marked(e.object())||tvsBypassData::marked(e.object());
#else
    (void)id;return false;
#endif
}
// Footprint in plan: bounding box of the block definition (attribute definitions left out) in block
// coordinates, its 4 corners mapped to WCS by blockTransform - a rotated block keeps its own box.
inline bool footprint(const AcDbBlockReference* ref,std::vector<lk::Pt>& out){
    out.clear();AcDbExtents ext;bool any=false;
    {AcDbBlockTableRecordPointer btr(ref->blockTableRecord(),AcDb::kForRead);
     if(btr.openStatus()==Acad::eOk){AcDbBlockTableRecordIterator* it=NULL;
        if(btr->newIterator(it)==Acad::eOk){
            for(;!it->done();it->step()){AcDbEntity* e=NULL;if(it->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
                if(!AcDbAttributeDefinition::cast(e)&&e->visibility()==AcDb::kVisible){
                    AcDbExtents x;if(e->getGeomExtents(x)==Acad::eOk){ext.addExt(x);any=true;}}
                e->close();}
            delete it;}}}
    if(any){
        const AcGeMatrix3d m=ref->blockTransform();const AcGePoint3d a=ext.minPoint(),b=ext.maxPoint();
        const AcGePoint3d c[4]={AcGePoint3d(a.x,a.y,a.z),AcGePoint3d(b.x,a.y,a.z),AcGePoint3d(b.x,b.y,a.z),AcGePoint3d(a.x,b.y,a.z)};
        for(int k=0;k<4;++k){AcGePoint3d w=c[k];w.transformBy(m);out.push_back(lk::Pt(w.x,w.y));}
        return true;
    }
    AcDbExtents w;if(ref->getGeomExtents(w)!=Acad::eOk)return false;   // fallback: WCS box
    const AcGePoint3d a=w.minPoint(),b=w.maxPoint();
    out.push_back(lk::Pt(a.x,a.y));out.push_back(lk::Pt(b.x,a.y));out.push_back(lk::Pt(b.x,b.y));out.push_back(lk::Pt(a.x,b.y));
    return true;
}
// The network of the current space: TVS elements (tvsMove::snapshot) and the ports of linked blocks.
// a port of a linked block; dir out of the block, size (mm) and level: a neighbour block can lean on it
struct EquipPort {AcDbObjectId id;lk::Pt at;double dx,dy,a,b,level;bool round;EquipPort():dx(1),dy(0),a(0),b(0),level(0),round(false){}};
struct Scene {std::vector<tvsMove::Node> nodes;std::vector<EquipPort> equipment;};
inline void remember(Scene& s,const AcDbObjectId& id,const tvsEquipment::Record& r){
    for(int k=0;k<r.ports&&k<2;++k){EquipPort p;p.id=id;p.at=lk::Pt(r.port[k].x,r.port[k].y);p.dx=r.port[k].dx;p.dy=r.port[k].dy;
        p.a=r.a;p.b=r.round?0:r.b;p.round=r.round;p.level=r.port[k].z;s.equipment.push_back(p);}
}
inline bool readScene(AcDbDatabase* db,Scene& s){
    AcDbObjectIdArray labels;if(!tvsMove::snapshot(db,s.nodes,labels))return false;
    AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return false;
    for(;!it->done();it->step()){AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id.objectClass()!=AcDbBlockReference::desc())continue;
        tvsEquipment::Record r;if(!tvsEquipmentStore::read(id,r))continue;
        {AcDbObjectPointer<AcDbBlockReference> b(id,AcDb::kForRead);if(b.openStatus()!=Acad::eOk||tvsEquipmentStore::detached(b.object(),r))continue;}   // moved / copied: its old ports are not taken
        remember(s,id,r);}
    delete it;return true;
}
inline bool isDuct(const tvsMove::Node& n){return n.ports.size()==2&&(n.kind==1||n.flex)&&n.id.objectClass()==TVS_Pipe::desc();}
inline double segmentDistance(const lk::Pt& p,const lk::Pt& a,const lk::Pt& b){
    const lk::Pt v=lk::sub(b,a);const double l=lk::dot(v,v);double t=l>0?lk::dot(lk::sub(p,a),v)/l:0;
    if(t<0)t=0;if(t>1)t=1;return lk::length(lk::sub(p,lk::add(a,lk::mul(v,t))));
}
// Ducts near the block for EquipmentLink.h, with what touches their ends; index[k] = node of duct k.
inline void ductsNear(const Scene& s,const AcDbObjectId& self,const std::vector<lk::Pt>& block,double reach,double tol,double f,
                      std::vector<lk::Duct>& out,std::vector<int>& index){
    out.clear();index.clear();
    lk::Pt c;for(size_t k=0;k<block.size();++k)c=lk::add(c,block[k]);c=lk::mul(c,1.0/block.size());
    double radius=0;for(size_t k=0;k<block.size();++k){const double r=lk::length(lk::sub(block[k],c));if(r>radius)radius=r;}
    for(size_t i=0;i<s.nodes.size();++i){const tvsMove::Node& n=s.nodes[i];if(!isDuct(n))continue;
        const lk::Pt a(n.ports[0].x,n.ports[0].y),b(n.ports[1].x,n.ports[1].y);
        if(segmentDistance(c,a,b)>radius+reach)continue;
        lk::Duct d;d.p0=a;d.p1=b;d.round=n.round;d.flex=n.flex;d.level=n.elevation;
        d.a=n.a*f;d.b=n.round?0:n.b*f;
        out.push_back(d);index.push_back(static_cast<int>(i));}
    for(size_t k=0;k<out.size();++k)for(int e=0;e<2;++e){
        const lk::Pt p=e==0?out[k].p0:out[k].p1;
        for(size_t j=0;j<s.nodes.size();++j){if(static_cast<int>(j)==index[k])continue;const tvsMove::Node& n=s.nodes[j];
            for(size_t q=0;q<n.ports.size();++q)if(lk::length(lk::sub(p,lk::Pt(n.ports[q].x,n.ports[q].y)))<=tol){
                ++out[k].others[e];
                for(size_t m=0;m<index.size();++m)if(index[m]==static_cast<int>(j))out[k].partner[e]=static_cast<int>(m);
                break;}}
        for(size_t q=0;q<s.equipment.size();++q)
            if(s.equipment[q].id!=self&&lk::length(lk::sub(p,s.equipment[q].at))<=tol)++out[k].others[e];
    }
    // a linked block next to this one (user 2026-11-27: FV linked first, then the fan): its port facing
    // this block is a "duct" lying inside that block (index -1, never trimmed / cut: only Touching);
    // the real ducts behind that block are not candidates
    std::vector<lk::Duct> lean;std::vector<AcDbObjectId> behind;
    for(size_t q=0;q<s.equipment.size();++q){const EquipPort& e=s.equipment[q];if(e.id==self||!(e.a>0))continue;
        const lk::Pt dir(e.dx,e.dy);if(lk::length(dir)<1e-9||lk::length(lk::sub(e.at,c))>radius+reach)continue;
        if(lk::dot(dir,lk::sub(c,e.at))<=0)continue;   // its port must face this block
        lk::Duct d;d.p0=e.at;d.p1=lk::sub(e.at,lk::mul(dir,(1000.0/f)/lk::length(dir)));d.round=e.round;d.flex=false;d.level=e.level;d.a=e.a;d.b=e.round?0:e.b;
        for(size_t j=0;j<s.nodes.size();++j)for(size_t m=0;m<s.nodes[j].ports.size();++m)
            if(lk::length(lk::sub(e.at,lk::Pt(s.nodes[j].ports[m].x,s.nodes[j].ports[m].y)))<=tol){++d.others[0];break;}
        for(size_t m=0;m<s.equipment.size();++m)if(m!=q&&s.equipment[m].id!=self&&s.equipment[m].id!=e.id&&lk::length(lk::sub(e.at,s.equipment[m].at))<=tol)++d.others[0];
        d.others[1]=1;lean.push_back(d);behind.push_back(e.id);}
    if(lean.empty())return;
    for(size_t k=out.size();k-->0;){bool hidden=false;
        for(int e=0;e<2&&!hidden;++e){const lk::Pt p=e==0?out[k].p0:out[k].p1;
            for(size_t q=0;q<s.equipment.size()&&!hidden;++q){bool lean2=false;for(size_t b=0;b<behind.size();++b)if(behind[b]==s.equipment[q].id)lean2=true;
                if(lean2&&lk::length(lk::sub(p,s.equipment[q].at))<=tol)hidden=true;}}
        if(hidden){out.erase(out.begin()+k);index.erase(index.begin()+k);}}
    for(size_t k=0;k<out.size();++k)for(int e=0;e<2;++e)if(out[k].partner[e]>=0)out[k].partner[e]=-1;   // indices changed; partners recomputed below
    for(size_t k=0;k<out.size();++k)for(int e=0;e<2;++e){const lk::Pt p=e==0?out[k].p0:out[k].p1;
        for(size_t m=0;m<out.size();++m)if(m!=k&&(lk::length(lk::sub(p,out[m].p0))<=tol||lk::length(lk::sub(p,out[m].p1))<=tol))out[k].partner[e]=static_cast<int>(m);}
    for(size_t k=0;k<lean.size();++k){out.push_back(lean[k]);index.push_back(-1);}
}
// a link that would trim / cut the leaning "duct" of a neighbour block (index -1): the faces must touch
inline bool leansWrong(const lk::Result& r,const std::vector<int>& index){
    for(size_t k=0;k<r.edits.size();++k)if(r.edits[k].duct>=0&&r.edits[k].duct<static_cast<int>(index.size())&&index[r.edits[k].duct]<0)return true;
    return r.linkCase==lk::Cut&&r.cutDuct>=0&&r.cutDuct<static_cast<int>(index.size())&&index[r.cutDuct]<0;
}
// Free vertical outlets of elbows Gore / Dolje (TVS_TAP Form_Up / Form_Down, 90 deg) within `reach` of
// the base point: outlet centre in plan as OffsetService::describe / RoundElbow.h compute it (rectangular:
// MiddlePoint; round constant / variable radius: tvsRoundElbow::ports; other round elbows: the drawn MC),
// level Elev -/+ rc (rc = R + B/2, round R + A/2). Free: no other element port, no other linked block there.
inline void elbowOutlets(const Scene& s,const AcDbObjectId& self,const lk::Pt& base,double reach,double tol,double f,
                         std::vector<lk::Outlet>& out){
    out.clear();const double halfPi=1.57079632679489661923;
    for(size_t i=0;i<s.nodes.size();++i){const tvsMove::Node& n=s.nodes[i];
        if(n.id.objectClass()!=TVS_TAP::desc())continue;
        lk::Outlet w;
        {AcDbObjectPointer<TVS_TAP> tap(n.id,AcDb::kForRead);if(tap.openStatus()!=Acad::eOk)continue;
         const int form=tap->get_Form();
         if((form!=Form_Up&&form!=Form_Down)||tap->DuctType!=DuctTypeStill)continue;
         if(std::fabs(tap->Swectangle-halfPi)>0.02||(tap->Normvector-AcGeVector3d(0,0,1)).length()>1e-6)continue;
         const bool round=tap->ThisRound;
         const double rc=tap->get_radius()+(round?tap->SizeA:tap->SizeB)/2;
         if(!(rc>0))continue;
         AcGePoint3d mc=tap->MiddlePoint;
         if(round){
             if(tap->RadiusTypeRound==RadiusTypeRound_RadiusConstant||tap->RadiusTypeRound==RadiusTypeRound_RadiusVariable){
                 const AcGeVector3d st=tap->Startvector;tvsRoundElbow::P ma,m;
                 tvsRoundElbow::ports(tvsRoundElbow::P(tap->MiddlePoint.x,tap->MiddlePoint.y),st.x,st.y,tap->Swectangle,rc,ma,m);
                 mc.x=m.x;mc.y=m.y;}
             else mc=tap->MC;}
         w.at=lk::Pt(mc.x,mc.y);w.level=tap->Elev+(form==Form_Up?1:-1)*rc*(1-std::cos(tap->Swectangle));
         w.a=tap->SizeA*f;w.b=round?0:tap->SizeB*f;w.round=round;}
        if(lk::length(lk::sub(w.at,base))>reach)continue;
        bool taken=false;
        for(size_t j=0;j<s.nodes.size()&&!taken;++j){if(j==i)continue;
            for(size_t q=0;q<s.nodes[j].ports.size();++q)
                if(lk::length(lk::sub(w.at,lk::Pt(s.nodes[j].ports[q].x,s.nodes[j].ports[q].y)))<=tol){taken=true;break;}}
        for(size_t q=0;q<s.equipment.size()&&!taken;++q)
            if(s.equipment[q].id!=self&&lk::length(lk::sub(w.at,s.equipment[q].at))<=tol)taken=true;
        if(!taken)out.push_back(w);
    }
}
struct Totals {
    int inl,term,ends,cuts;int count[reasonCount];int skippedLinked,offDuct,xref;int bottom;
    std::vector<AcDbObjectId> refused;
    std::vector<AcDbObjectId> unset;std::set<std::wstring> unsetNames;   // linked, block name without a default
    Totals():inl(0),term(0),ends(0),cuts(0),skippedLinked(0),offDuct(0),xref(0),bottom(0){for(int k=0;k<reasonCount;++k)count[k]=0;}
};
inline void refuse(Totals& t,const AcDbObjectId& id,const std::wstring& name,int reason,double detailMm,bool quiet){
    if(quiet&&reason==lk::NoDuct){++t.offDuct;return;}
    ++t.count[reason];t.refused.push_back(id);
    acutPrintf(L"\n  blok %ls [%ls]: %ls",name.c_str(),handleText(id).c_str(),reasonText(reason));
    if(detailMm>0&&(reason==lk::GapTooLarge||reason==lk::TooShort||reason==lk::OffAxis))acutPrintf(L" (%.0f mm)",detailMm);
    acutPrintf(L".");
}
// A new duct piece made by the cut: properties of the duct as TVS_CUT gives them (size, shape,
// level, flow, flex, wipeout, layer, colour, linetype, TVS xdata), fresh connectors.
inline TVS_Pipe* newPiece(TVS_Pipe* p,const AcGePoint3d& first){
    TVS_Pipe* n=new TVS_Pipe();tvsOffsetProof::initialize(n);n->ElevUp=0;n->ElevDown=0;
    TVS_Connector a,b;a.connectionID=AcDbObjectId::kNull;b.connectionID=AcDbObjectId::kNull;
    a.cTypeCurrent=conPipeStart;b.cTypeCurrent=conPipeEnd;a.cTypeAnother=b.cTypeAnother=0;
    a.point=b.point=AcGePoint3d::kOrigin;n->connectors.clear();n->connectors.push_back(a);n->connectors.push_back(b);
    n->setDatabaseDefaults(p->database());
    n->FirstPoint=first;n->LastPoint=p->LastPoint;
    n->SizeA=p->SizeA;n->SizeB=p->SizeB;n->ThisRound=p->ThisRound;n->This1D=p->This1D;n->DuctType=p->DuctType;
    n->Flow=p->Flow;n->Wipeout=p->Wipeout;n->WipeoutLength=p->WipeoutLength;n->Grani=p->Grani;
    n->Elev=p->Elev;n->ElevUp=p->ElevUp;n->ElevDown=p->ElevDown;
    n->setPropertiesFrom(p);n->setLineWeight(p->lineWeight());
    return n;
}
// One block: geometry decided (r), writes ducts + record in one transaction. false: nothing changed.
inline bool applyRow(const std::vector<AcDbObjectId>& blockIds,const lk::Result& r,const std::vector<int>& index,
                     Scene& s,const std::vector<tvsEquipment::Record>& recs,AcDbObjectId& pieceId);
inline bool apply(const AcDbObjectId& blockId,const lk::Result& r,const std::vector<int>& index,
                  Scene& s,const tvsEquipment::Record& rec,AcDbObjectId& pieceId){
    return applyRow(std::vector<AcDbObjectId>(1,blockId),r,index,s,std::vector<tvsEquipment::Record>(1,rec),pieceId);
}
// One block, or a row of blocks touching one another (niz opreme): the duct edits of r, every
// block its record, in one transaction. false: nothing changed.
inline bool applyRow(const std::vector<AcDbObjectId>& blockIds,const lk::Result& r,const std::vector<int>& index,
                     Scene& s,const std::vector<tvsEquipment::Record>& recs,AcDbObjectId& pieceId){
    tvsAction::Transaction tx;if(!tx.good())return false;
    for(size_t k=0;k<r.edits.size();++k){const lk::EndEdit& e=r.edits[k];
        TVS_Pipe* p=TVS_Pipe::cast(tx.open(s.nodes[index[e.duct]].id,AcDb::kForWrite));if(!p)return false;
        const AcGePoint3d to(e.to.x,e.to.y,e.end==0?p->FirstPoint.z:p->LastPoint.z);
        if((e.end==0?p->put_FirstPoint(to):p->put_Lastpoint(to))!=Acad::eOk)return false;
        p->recordGraphicsModified();}
    AcGePoint3d keep,first,last;
    if(r.linkCase==lk::Cut){
        TVS_Pipe* p=TVS_Pipe::cast(tx.open(s.nodes[index[r.cutDuct]].id,AcDb::kForWrite));if(!p)return false;
        AcDbBlockTableRecord* space=AcDbBlockTableRecord::cast(tx.open(p->ownerId(),AcDb::kForWrite));if(!space)return false;
        keep=AcGePoint3d(r.cutKeep.x,r.cutKeep.y,p->LastPoint.z);first=AcGePoint3d(r.cutNew.x,r.cutNew.y,p->LastPoint.z);last=p->LastPoint;
        TVS_Pipe* n=newPiece(p,first);
        if(!tx.append(space,n,pieceId))return false;
        if(resbuf* x=p->xData(NULL)){const Acad::ErrorStatus es=n->setXData(x);acutRelRb(x);if(es!=Acad::eOk)return false;}   // TVS svojstva, oznake
        n->recordGraphicsModified();
        if(p->put_Lastpoint(keep)!=Acad::eOk)return false;
        p->recordGraphicsModified();
    }
    for(size_t k=0;k<blockIds.size()&&k<recs.size();++k){
        AcDbObject* b=tx.open(blockIds[k],AcDb::kForWrite);
        tvsEquipment::Record anchored=recs[k];if(b)tvsEquipmentStore::anchor(b,anchored);   // where it was linked (moved / copied later = odvojen)
        if(!b||tvsEquipmentStore::write(b,anchored)!=Acad::eOk)return false;}
    if(!tx.commit())return false;
    // the scene follows (next blocks of this command)
    for(size_t k=0;k<r.edits.size();++k){const lk::EndEdit& e=r.edits[k];tvsMove::Node& n=s.nodes[index[e.duct]];
        n.ports[e.end].x=e.to.x;n.ports[e.end].y=e.to.y;}
    if(r.linkCase==lk::Cut){tvsMove::Node piece=s.nodes[index[r.cutDuct]];
        s.nodes[index[r.cutDuct]].ports[1]=tvsMove::point(keep);
        piece.id=pieceId;piece.ports[0]=tvsMove::point(first);piece.ports[1]=tvsMove::point(last);s.nodes.push_back(piece);}
    for(size_t m=0;m<blockIds.size()&&m<recs.size();++m)
        remember(s,blockIds[m],recs[m]);
    return true;
}
inline void forget(Scene& s,const AcDbObjectId& id){
    for(size_t k=s.equipment.size();k-->0;)if(s.equipment[k].id==id)s.equipment.erase(s.equipment.begin()+k);
}
// Block references of the current space (preselected, picked, or all of them with Sve).
inline bool choose(AcDbDatabase* db,std::vector<AcDbObjectId>& out,bool& all,const wchar_t* cmd,bool offerAll){
    out.clear();all=false;
    resbuf* filter=acutBuildList(RTDXF0,L"INSERT",RTNONE);ads_name ss;bool got=false;
    if(acedSSGet(L"_I",NULL,NULL,filter,ss)==RTNORM){got=true;acedSSSetFirst(NULL,NULL);}
    while(!got&&offerAll){
        acedInitGet(0,L"Izaberi Sve Tolerancija");ACHAR kw[32]=L"";
        wchar_t prompt[200];swprintf_s(prompt,L"\n%ls: blokovi opreme [Izaberi/Sve/Tolerancija] <Izaberi>: ",cmd);
        const int rc=acedGetKword(prompt,kw,32);
        if(rc!=RTNORM&&rc!=RTNONE){acutRelRb(filter);return false;}
        if(rc==RTNORM&&_wcsicmp(kw,L"Tolerancija")==0){
            double v=gapMm();wchar_t q[160];swprintf_s(q,L"\nNajveći razmak / preklop kanala i bloka koji se zatvara [mm] <%.0f>: ",v);
            acedInitGet(RSG_NONEG|RSG_NOZERO,NULL);const int r=acedGetReal(q,&v);
            if(r==RTNORM)gapMm()=v;else if(r!=RTNONE){acutRelRb(filter);return false;}
            continue;}
        if(rc==RTNORM&&_wcsicmp(kw,L"Sve")==0){all=true;break;}
        break;}
    if(all){
        AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForRead);AcDbBlockTableRecordIterator* it=NULL;
        if(space.openStatus()==Acad::eOk&&space->newIterator(it)==Acad::eOk){
            for(;!it->done();it->step()){AcDbObjectId id;if(it->getEntityId(id)==Acad::eOk&&id.objectClass()==AcDbBlockReference::desc())out.push_back(id);}
            delete it;}
        acutRelRb(filter);return true;
    }
    if(!got){acutPrintf(L"\nIzaberite blokove opreme: ");if(acedSSGet(NULL,NULL,NULL,filter,ss)!=RTNORM){acutRelRb(filter);return false;}}
    acutRelRb(filter);
    Adesk::Int32 n=0;acedSSLength(ss,&n);
    for(Adesk::Int32 i=0;i<n;++i){ads_name e;AcDbObjectId id;
        if(acedSSName(ss,i,e)==RTNORM&&acdbGetObjectId(id,e)==Acad::eOk&&id.objectClass()==AcDbBlockReference::desc())out.push_back(id);}
    acedSSFree(ss);return true;
}
inline void execute(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    const double f=tvsUnits::factor(db);
    std::vector<AcDbObjectId> blocks;bool all=false;
    if(!choose(db,blocks,all,L"TVS_OPREMA",true))return;
    if(blocks.empty()){acutPrintf(L"\nTVS_OPREMA: nema izabranih blokova.");return;}
    Scene scene;if(!readScene(db,scene)){acutPrintf(L"\nTVS_OPREMA: crtež se ne može pročitati (objekat otvoren u drugoj komandi?).");return;}
    // already linked: again only when asked
    int linked=0;for(size_t k=0;k<blocks.size();++k){AcDbObjectPointer<AcDbBlockReference> b(blocks[k],AcDb::kForRead);tvsEquipment::Record r;
        if(b.openStatus()==Acad::eOk&&tvsEquipmentStore::read(b.object(),r)&&!tvsEquipmentStore::detached(b.object(),r))++linked;}   // odvojeni (moved / copied) are linked again without asking
    bool relink=false;
    if(linked){
        wchar_t prompt[160];swprintf_s(prompt,L"\nTVS_OPREMA: već povezanih blokova: %d. Povezati ih ponovo [Da/Ne] <Ne>: ",linked);
        acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";const int rc=acedGetKword(prompt,kw,8);
        if(rc!=RTNORM&&rc!=RTNONE)return;
        relink=rc==RTNORM&&_wcsicmp(kw,L"Da")==0;
    }
    Totals t;
    lk::Options base;base.tol=tolMm/f;base.axisTol=axisTolMm/f;base.gap=gapMm()/f;base.minPiece=minPieceMm/f;base.bottom=true;
    std::map<std::wstring,int> answered;   // Dno / Linija per block name (when writeDefault did not keep it)
    bool stop=false;
    std::vector<AcDbObjectId> linkedNow;   // fans among them: usis / potis asked at the end
    // ventilator: a relinked fan keeps its suction side on the port at the same place
    auto keepSuction=[&](const tvsEquipment::Record& old,tvsEquipment::Record& rec){
        rec.suction=-1;if(rec.kind!=tvsEquipment::Fan||old.suction<0||old.ports!=2||rec.ports!=2)return;
        const tvsEquipment::Port& o=old.port[old.suction];const double so=old.mmPerUnit>0?old.mmPerUnit:1,sn=rec.mmPerUnit>0?rec.mmPerUnit:1;
        double best=-1;for(int k=0;k<2;++k){const double d=std::hypot(rec.port[k].x*sn-o.x*so,rec.port[k].y*sn-o.y*so);if(best<0||d<best){best=d;rec.suction=k;}}};
    // niz opreme (stavka 27, user 2026-11-27: kanal - FV - ventilator - FV - kanal): selected blocks whose
    // footprints touch, none a terminal by its kind, are linked together as one row (lk::chain)
    std::vector<std::vector<lk::Pt> > rowFoot(blocks.size());std::vector<int> rowOf(blocks.size(),-1);
    {std::vector<int> parent(blocks.size());for(size_t k=0;k<blocks.size();++k)parent[k]=static_cast<int>(k);
     std::function<int(int)> root=[&](int x){while(parent[x]!=x){parent[x]=parent[parent[x]];x=parent[x];}return x;};
     for(size_t k=0;k<blocks.size();++k){AcDbObjectPointer<AcDbBlockReference> ref(blocks[k],AcDb::kForRead);if(ref.openStatus()!=Acad::eOk||ref->ownerId()!=db->currentSpaceId())continue;
        if(std::fabs(std::fabs(ref->normal().z)-1)>1e-6)continue;
        {AcDbBlockTableRecordPointer btr(ref->blockTableRecord(),AcDb::kForRead);if(btr.openStatus()!=Acad::eOk||btr->isFromExternalReference()||btr->isLayout())continue;}
        tvsEquipment::Record old;const bool was=tvsEquipmentStore::read(ref.object(),old);
        if(was&&!relink&&!tvsEquipmentStore::detached(ref.object(),old))continue;
        const std::wstring name=blockName(blocks[k],ref->blockTableRecord());tvsEquipment::Record def;
        const int kind=was?old.kind:tvsEquipmentStore::readDefault(db,name.c_str(),def)?def.kind:-1;
        if(kind>=0&&tvsEquipment::terminalKind(kind))continue;
        if(!footprint(ref.object(),rowFoot[k]))rowFoot[k].clear();}
     for(size_t i=0;i<blocks.size();++i)for(size_t j=i+1;j<blocks.size();++j)
        if(rowFoot[i].size()>=3&&rowFoot[j].size()>=3&&lk::touching(rowFoot[i],rowFoot[j],base.tol))parent[root(static_cast<int>(i))]=root(static_cast<int>(j));
     std::map<int,int> size;for(size_t k=0;k<blocks.size();++k)if(rowFoot[k].size()>=3)++size[root(static_cast<int>(k))];
     for(size_t k=0;k<blocks.size();++k)if(rowFoot[k].size()>=3&&size[root(static_cast<int>(k))]>1)rowOf[k]=root(static_cast<int>(k));}
    auto linkRow=[&](int row){
        std::vector<AcDbObjectId> ids;std::vector<std::wstring> names;std::vector<tvsEquipment::Record> recs,olds;std::vector<char> was,known;
        std::vector<std::vector<lk::Pt> > feet;std::vector<lk::Pt> all;
        for(size_t k=0;k<blocks.size();++k){if(rowOf[k]!=row)continue;
            AcDbObjectPointer<AcDbBlockReference> ref(blocks[k],AcDb::kForRead);if(ref.openStatus()!=Acad::eOk)return;
            const std::wstring name=blockName(blocks[k],ref->blockTableRecord());tvsEquipment::Record old,def;
            const bool w=tvsEquipmentStore::read(ref.object(),old),d=tvsEquipmentStore::readDefault(db,name.c_str(),def);
            tvsEquipment::Record rec=w?old:d?def:tvsEquipment::Record();if(!w&&!d)rec.kind=tvsEquipment::Other;
            ids.push_back(blocks[k]);names.push_back(name);recs.push_back(rec);olds.push_back(old);was.push_back(w);known.push_back(w||d);
            feet.push_back(rowFoot[k]);all.insert(all.end(),rowFoot[k].begin(),rowFoot[k].end());}
        for(size_t m=0;m<ids.size();++m)if(was[m])forget(scene,ids[m]);
        auto back=[&](){for(size_t m=0;m<ids.size();++m)if(was[m])remember(scene,ids[m],olds[m]);};
        lk::Options o=base;o.expect=lk::Inline;o.bottom=false;
        std::vector<lk::Duct> ducts;std::vector<int> index;
        ductsNear(scene,ids[0],all,4*o.gap+o.tol,o.tol,f,ducts,index);
        const lk::Result r=lk::link(all,ducts,o);
        if(r.error!=lk::Ok||r.ports!=2||r.twin){for(size_t m=0;m<ids.size();++m)refuse(t,ids[m],names[m],r.error!=lk::Ok?r.error:static_cast<int>(lk::MissingSide),r.detail*f,false);back();return;}
        if(leansWrong(r,index)){for(size_t m=0;m<ids.size();++m)refuse(t,ids[m],names[m],NotLeaning,0,false);back();return;}
        std::vector<lk::ChainPart> parts;
        if(!lk::chain(feet,r,o.tol,parts)){for(size_t m=0;m<ids.size();++m)refuse(t,ids[m],names[m],NotInRow,0,false);back();return;}
        std::vector<AcDbObjectId> touched=ids;
        for(size_t e=0;e<r.edits.size();++e)touched.push_back(scene.nodes[index[r.edits[e].duct]].id);
        if(r.linkCase==lk::Cut)touched.push_back(scene.nodes[index[r.cutDuct]].id);
        bool assembly=false;for(size_t e=ids.size();e<touched.size();++e)if(inAssembly(touched[e]))assembly=true;
        if(assembly||!tvsAction::writable(touched)){for(size_t m=0;m<ids.size();++m)refuse(t,ids[m],names[m],assembly?Assembly:Locked,0,false);back();return;}
        for(size_t m=0;m<ids.size();++m){tvsEquipment::Record& rec=recs[m];
            rec.version=1;rec.ports=2;rec.twin=false;rec.mmPerUnit=f;rec.round=r.round;rec.a=r.a;rec.b=r.round?0:r.b;rec.lengthMm=parts[m].length*f;
            rec.port[0]=parts[m].port[0];rec.port[1]=parts[m].port[1];rec.mount=tvsEquipment::AtEnd;keepSuction(olds[m],rec);}
        AcDbObjectId piece;
        if(!applyRow(ids,r,index,scene,recs,piece)){for(size_t m=0;m<ids.size();++m)refuse(t,ids[m],names[m],WriteFailed,0,false);back();return;}
        t.inl+=static_cast<int>(ids.size());t.ends+=static_cast<int>(r.edits.size());if(r.linkCase==lk::Cut)++t.cuts;
        std::wstring list;for(size_t m=0;m<names.size();++m){if(m)list+=L" – ";list+=names[m];}
        acutPrintf(L"\n  niz opreme povezan zajedno: %ls.",list.c_str());
        for(size_t m=0;m<ids.size();++m){linkedNow.push_back(ids[m]);if(!known[m]){t.unset.push_back(ids[m]);t.unsetNames.insert(names[m]);}}};
    std::set<int> rowDone;
    for(size_t k=0;k<blocks.size()&&!stop;++k){
        const AcDbObjectId id=blocks[k];
        if(rowOf[k]>=0){if(rowDone.insert(rowOf[k]).second)linkRow(rowOf[k]);continue;}
        std::vector<lk::Pt> block;lk::Pt at;std::wstring name;tvsEquipment::Record old;bool wasLinked=false;int reason=lk::Ok;
        {AcDbObjectPointer<AcDbBlockReference> ref(id,AcDb::kForRead);if(ref.openStatus()!=Acad::eOk)continue;
         if(ref->ownerId()!=db->currentSpaceId())continue;
         {AcDbBlockTableRecordPointer btr(ref->blockTableRecord(),AcDb::kForRead);
          if(btr.openStatus()!=Acad::eOk)continue;
          if(btr->isFromExternalReference()||btr->isLayout()){++t.xref;continue;}}
         name=blockName(id,ref->blockTableRecord());
         wasLinked=tvsEquipmentStore::read(ref.object(),old);
         if(wasLinked&&!relink&&!tvsEquipmentStore::detached(ref.object(),old)){++t.skippedLinked;continue;}
         at=lk::Pt(ref->position().x,ref->position().y);   // insertion point (WCS)
         if(std::fabs(std::fabs(ref->normal().z)-1)>1e-6)reason=NotPlan;
         else if(!footprint(ref.object(),block))reason=NoGeometry;}
        if(reason!=lk::Ok){refuse(t,id,name,reason,0,false);continue;}
        // the record: kept from an earlier link, else the default for the block name, else new
        tvsEquipment::Record def;bool hasDefault=tvsEquipmentStore::readDefault(db,name.c_str(),def);
        if(!hasDefault){std::map<std::wstring,int>::const_iterator a=answered.find(name);
            if(a!=answered.end()){def=tvsEquipment::Record();def.kind=a->second;def.ports=a->second==tvsEquipment::Grille?1:2;hasDefault=true;}}
        bool userKind=wasLinked||hasDefault;   // kind not only guessed by this command
        tvsEquipment::Record rec=wasLinked?old:hasDefault?def:tvsEquipment::Record();
        bool known=wasLinked||hasDefault;
        lk::Options o=base;
        if(known)o.expect=tvsEquipment::terminalKind(rec.kind)?lk::Terminal:rec.kind!=tvsEquipment::Other?lk::Inline:lk::Any;
        if(hasDefault&&def.lengthMm>0&&o.expect!=lk::Terminal)o.fixedLength=def.lengthMm/f;
        if(wasLinked)forget(scene,id);
        std::vector<lk::Duct> ducts;std::vector<int> index;
        ductsNear(scene,id,block,4*o.gap+o.tol+(o.fixedLength>0?o.fixedLength:0),o.tol,f,ducts,index);
        std::vector<lk::Outlet> outlets;elbowOutlets(scene,id,at,10*o.tol,o.tol,f,outlets);
        lk::Result r=lk::link(block,at,ducts,outlets,o);
        if(!known&&r.error==lk::Ok&&r.linkCase==lk::Cut){
            // a block of a name without a default over a continuous duct: grille in the bottom face or inline
            wchar_t prompt[300];swprintf_s(prompt,L"\nBlok %ls preko kanala: [Dno/Linija] <Linija>: ",name.c_str());
            acedInitGet(0,L"Dno Linija");ACHAR kw[16]=L"";const int rc=acedGetKword(prompt,kw,16);
            if(rc!=RTNORM&&rc!=RTNONE){stop=true;
                if(wasLinked)remember(scene,id,old);
                break;}
            const bool dno=rc==RTNORM&&_wcsicmp(kw,L"Dno")==0;
            tvsEquipment::Record d;d.kind=dno?tvsEquipment::Grille:tvsEquipment::Other;d.ports=dno?1:2;
            if(dno)d.mount=tvsEquipment::Bottom;
            answered[name]=d.kind;
            if(tvsEquipmentStore::writeDefault(db,name.c_str(),d)!=Acad::eOk)
                acutPrintf(L"\n  podrazumijevana vrsta bloka %ls nije sačuvana (važi do kraja komande).",name.c_str());
            def=d;hasDefault=true;known=true;userKind=true;rec=d;
            if(dno){o.expect=lk::Terminal;r=lk::link(block,at,ducts,outlets,o);}
        }
        if(r.error!=lk::Ok){
            refuse(t,id,name,r.error,r.detail*f,all);
            if(wasLinked)remember(scene,id,old);
            continue;}
        if(leansWrong(r,index)){refuse(t,id,name,NotLeaning,0,false);if(wasLinked)remember(scene,id,old);continue;}
        // preflight: every element written can be written
        std::vector<AcDbObjectId> touched(1,id);
        for(size_t e=0;e<r.edits.size();++e)touched.push_back(scene.nodes[index[r.edits[e].duct]].id);
        if(r.linkCase==lk::Cut)touched.push_back(scene.nodes[index[r.cutDuct]].id);
        bool assembly=false;for(size_t e=1;e<touched.size();++e)if(inAssembly(touched[e]))assembly=true;
        if(assembly||!tvsAction::writable(touched)){
            refuse(t,id,name,assembly?Assembly:Locked,0,false);
            if(wasLinked)remember(scene,id,old);
            continue;}
        if(!known)rec.kind=r.twin?tvsEquipment::Diffuser:r.ports==1?tvsEquipment::Grille:tvsEquipment::Other;
        rec.twin=r.twin;   // linijski difuzor: 2 priključka na kutiji (Equipment.h)
        rec.version=1;rec.ports=r.ports;rec.mmPerUnit=f;rec.round=r.round;rec.a=r.a;rec.b=r.round?0:r.b;
        rec.lengthMm=r.ports==2&&!r.twin?r.length*f:0;
        for(int p=0;p<2;++p)rec.port[p]=p<r.ports?r.port[p]:tvsEquipment::Port();
        rec.mount=r.ports==1?r.mount:tvsEquipment::AtEnd;
        keepSuction(old,rec);
        bool neckGuessed=false;
        if(rec.mount==tvsEquipment::Bottom){   // neck: default of the name, else kept from an earlier link, else the footprint
            if(hasDefault&&def.neckA>0){rec.neckA=def.neckA;rec.neckB=def.neckB;rec.neckRound=def.neckRound;}
            else if(!(wasLinked&&old.neckA>0)){double w=0,h=0;lk::footprintMm(block,f,w,h);rec.neckA=w;rec.neckB=h;rec.neckRound=false;neckGuessed=w>0;}
        }
        AcDbObjectId piece;
        if(!apply(id,r,index,scene,rec,piece)){
            refuse(t,id,name,WriteFailed,0,false);
            if(wasLinked)remember(scene,id,old);
            continue;}
        if(r.ports==2&&!r.twin)++t.inl;else ++t.term;
        linkedNow.push_back(id);
        if(r.twin)acutPrintf(L"\n  blok %ls [%ls]: 2 priključka na kutiji (linijski difuzor) – protok Q/2 po priključku, pad pritiska difuzora za ukupni Q na obje putanje.",
            name.c_str(),handleText(id).c_str());
        t.ends+=static_cast<int>(r.edits.size());if(r.linkCase==lk::Cut)++t.cuts;
        if(rec.mount==tvsEquipment::Bottom&&r.ports==1)++t.bottom;
        if(neckGuessed)acutPrintf(L"\n  blok %ls [%ls]: rešetka na dnu kanala, priključak %.0f×%.0f mm uzet iz gabarita bloka – provjeriti u TVS_OPREMA_SVOJSTVA.",
            name.c_str(),handleText(id).c_str(),rec.neckA,rec.neckB);
        if(!userKind){t.unset.push_back(id);t.unsetNames.insert(name);}
    }
    acutPrintf(L"\nTVS_OPREMA: povezano %d (u liniji %d, terminala %d); krajeva kanala skraćeno / produženo %d, presječeno kanala %d.",
        t.inl+t.term,t.inl,t.term,t.ends,t.cuts);
    if(t.bottom)acutPrintf(L"\n  rešetki na dnu kanala (kanal nije presječen): %d.",t.bottom);
    if(t.skippedLinked)acutPrintf(L"\n  već povezanih (preskočeno): %d.",t.skippedLinked);
    if(t.offDuct)acutPrintf(L"\n  blokova van kanala (preskočeno): %d.",t.offDuct);
    if(t.xref)acutPrintf(L"\n  xref / layout blokova (preskočeno): %d.",t.xref);
    if(!t.refused.empty()){
        acutPrintf(L"\n  odbijeno %d:",static_cast<int>(t.refused.size()));
        for(int k=1;k<reasonCount;++k)if(t.count[k])acutPrintf(L"\n    %d × %ls",t.count[k],reasonText(k));
        tvsCheck::select(t.refused);   // odbijeni blokovi ostaju izabrani
    }
    if(stop)acutPrintf(L"\n  prekinuto: ostali blokovi nisu obrađeni.");
    // blocks whose name has no default: the kind is only guessed (Rešetka / Ostala oprema)
    if(!t.unset.empty()&&!stop){
        std::wstring names;for(std::set<std::wstring>::const_iterator n=t.unsetNames.begin();n!=t.unsetNames.end();++n){if(!names.empty())names+=L", ";names+=*n;}
        acutPrintf(L"\n  vrsta nije podešena za %d blokova (naziv bez podrazumijevanih svojstava: %ls) – pretpostavljeno Rešetka (terminal) / Ostala oprema (u liniji).",
            static_cast<int>(t.unset.size()),names.c_str());
        acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";
        const int rc=acedGetKword(L"\nPodesiti vrstu sada? [Da/Ne] <Da>: ",kw,8);
        if(rc==RTNONE||(rc==RTNORM&&_wcsicmp(kw,L"Da")==0))tvsEquipmentDialog::run(IDD_EQUIPMENT,t.unset);
        else if(rc==RTNORM)acutPrintf(L"\n  vrsta se može podesiti kasnije: TVS_OPREMA_SVOJSTVA („Primijeni na sve blokove ovog naziva“).");
    }
    // ventilatori: usis / potis by hand with the temporary labels (no attributes in the blocks)
    if(!stop)for(size_t k=0;k<linkedNow.size();++k){tvsEquipment::Record r;
        if(tvsFan::inlineRecord(linkedNow[k],r)&&tvsFan::isFan(r)&&r.suction<0)tvsFan::chooseSide(linkedNow[k]);}
}
// TVS_OPREMA_ODVEZI: the record goes, the block and the ducts stay as they are.
inline void unlink(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    std::vector<AcDbObjectId> blocks;bool all=false;
    if(!choose(db,blocks,all,L"TVS_OPREMA_ODVEZI",false)||blocks.empty())return;
    int removed=0,locked=0;
    for(size_t k=0;k<blocks.size();++k){
        tvsEquipment::Record r;if(!tvsEquipmentStore::read(blocks[k],r))continue;
        if(!tvsAction::writable(blocks[k])){++locked;continue;}
        AcDbObjectPointer<AcDbBlockReference> ref(blocks[k],AcDb::kForWrite);
        if(ref.openStatus()!=Acad::eOk||tvsEquipmentStore::remove(ref.object())!=Acad::eOk){++locked;continue;}
        ref->recordGraphicsModified();++removed;
    }
    acutPrintf(L"\nTVS_OPREMA_ODVEZI: odvezano blokova %d; kanali nisu mijenjani.",removed);
    if(locked)acutPrintf(L" Nije odvezano (zaključan sloj): %d.",locked);
}
}

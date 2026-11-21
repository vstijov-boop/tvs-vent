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
#include "TVSMoveCommand.h"
#include "CheckCommand.h"
#include "ActionTransaction.h"
#include "BuildTransaction.h"
#include "EquipmentStore.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include "../geometry/EquipmentLink.h"
#include <dbdynblk.h>
#include <dbents.h>
#include <cmath>
#include <string>
#include <vector>
namespace tvsEquipmentLinkCmd {
namespace lk=tvsEquipmentLink;
inline double& gapMm(){static double v=150;return v;}   // largest gap / overlap closed (Tolerancija)
const double axisTolMm=25,minPieceMm=50,tolMm=1;
// reasons of this command after the geometric ones (lk::Error)
enum Extra {NotPlan=lk::errorCount,NoGeometry,Assembly,Locked,WriteFailed,reasonCount};
inline const wchar_t* reasonText(int e){
    switch(e){
    case NotPlan:return L"blok nije u osnovi (3D rotacija)";
    case NoGeometry:return L"blok nema geometriju";
    case Assembly:return L"kanal je dio etaža / obilaska";
    case Locked:return L"kanal ili blok je na zaključanom sloju";
    case WriteFailed:return L"upis nije uspio";
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
struct EquipPort {AcDbObjectId id;lk::Pt at;};
struct Scene {std::vector<tvsMove::Node> nodes;std::vector<EquipPort> equipment;};
inline bool readScene(AcDbDatabase* db,Scene& s){
    AcDbObjectIdArray labels;if(!tvsMove::snapshot(db,s.nodes,labels))return false;
    AcDbBlockTableRecordPointer space(db->currentSpaceId(),AcDb::kForRead);if(space.openStatus()!=Acad::eOk)return false;
    AcDbBlockTableRecordIterator* it=NULL;if(space->newIterator(it)!=Acad::eOk)return false;
    for(;!it->done();it->step()){AcDbObjectId id;if(it->getEntityId(id)!=Acad::eOk||id.objectClass()!=AcDbBlockReference::desc())continue;
        tvsEquipment::Record r;if(!tvsEquipmentStore::read(id,r))continue;
        for(int k=0;k<r.ports&&k<2;++k){EquipPort p;p.id=id;p.at=lk::Pt(r.port[k].x,r.port[k].y);s.equipment.push_back(p);}}
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
}
struct Totals {
    int inl,term,ends,cuts;int count[reasonCount];int skippedLinked,offDuct,xref;
    std::vector<AcDbObjectId> refused;
    Totals():inl(0),term(0),ends(0),cuts(0),skippedLinked(0),offDuct(0),xref(0){for(int k=0;k<reasonCount;++k)count[k]=0;}
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
inline bool apply(const AcDbObjectId& blockId,const lk::Result& r,const std::vector<int>& index,
                  Scene& s,const tvsEquipment::Record& rec,AcDbObjectId& pieceId){
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
    AcDbObject* b=tx.open(blockId,AcDb::kForWrite);if(!b||tvsEquipmentStore::write(b,rec)!=Acad::eOk)return false;
    if(!tx.commit())return false;
    // the scene follows (next blocks of this command)
    for(size_t k=0;k<r.edits.size();++k){const lk::EndEdit& e=r.edits[k];tvsMove::Node& n=s.nodes[index[e.duct]];
        n.ports[e.end].x=e.to.x;n.ports[e.end].y=e.to.y;}
    if(r.linkCase==lk::Cut){tvsMove::Node piece=s.nodes[index[r.cutDuct]];
        s.nodes[index[r.cutDuct]].ports[1]=tvsMove::point(keep);
        piece.id=pieceId;piece.ports[0]=tvsMove::point(first);piece.ports[1]=tvsMove::point(last);s.nodes.push_back(piece);}
    for(int k=0;k<rec.ports&&k<2;++k){EquipPort p;p.id=blockId;p.at=lk::Pt(rec.port[k].x,rec.port[k].y);s.equipment.push_back(p);}
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
    int linked=0;for(size_t k=0;k<blocks.size();++k){tvsEquipment::Record r;if(tvsEquipmentStore::read(blocks[k],r))++linked;}
    bool relink=false;
    if(linked){
        wchar_t prompt[160];swprintf_s(prompt,L"\nTVS_OPREMA: već povezanih blokova: %d. Povezati ih ponovo [Da/Ne] <Ne>: ",linked);
        acedInitGet(0,L"Da Ne");ACHAR kw[8]=L"";const int rc=acedGetKword(prompt,kw,8);
        if(rc!=RTNORM&&rc!=RTNONE)return;
        relink=rc==RTNORM&&_wcsicmp(kw,L"Da")==0;
    }
    Totals t;
    lk::Options base;base.tol=tolMm/f;base.axisTol=axisTolMm/f;base.gap=gapMm()/f;base.minPiece=minPieceMm/f;
    for(size_t k=0;k<blocks.size();++k){
        const AcDbObjectId id=blocks[k];
        std::vector<lk::Pt> block;std::wstring name;tvsEquipment::Record old;bool wasLinked=false;int reason=lk::Ok;
        {AcDbObjectPointer<AcDbBlockReference> ref(id,AcDb::kForRead);if(ref.openStatus()!=Acad::eOk)continue;
         if(ref->ownerId()!=db->currentSpaceId())continue;
         {AcDbBlockTableRecordPointer btr(ref->blockTableRecord(),AcDb::kForRead);
          if(btr.openStatus()!=Acad::eOk)continue;
          if(btr->isFromExternalReference()||btr->isLayout()){++t.xref;continue;}}
         name=blockName(id,ref->blockTableRecord());
         wasLinked=tvsEquipmentStore::read(ref.object(),old);
         if(wasLinked&&!relink){++t.skippedLinked;continue;}
         if(std::fabs(std::fabs(ref->normal().z)-1)>1e-6)reason=NotPlan;
         else if(!footprint(ref.object(),block))reason=NoGeometry;}
        if(reason!=lk::Ok){refuse(t,id,name,reason,0,false);continue;}
        // the record: kept from an earlier link, else the default for the block name, else new
        tvsEquipment::Record def;const bool hasDefault=tvsEquipmentStore::readDefault(db,name.c_str(),def);
        tvsEquipment::Record rec=wasLinked?old:hasDefault?def:tvsEquipment::Record();
        const bool known=wasLinked||hasDefault;
        lk::Options o=base;
        if(known)o.expect=tvsEquipment::terminalKind(rec.kind)?lk::Terminal:rec.kind!=tvsEquipment::Other?lk::Inline:lk::Any;
        if(hasDefault&&def.lengthMm>0&&o.expect!=lk::Terminal)o.fixedLength=def.lengthMm/f;
        if(wasLinked)forget(scene,id);
        std::vector<lk::Duct> ducts;std::vector<int> index;
        ductsNear(scene,id,block,4*o.gap+o.tol+(o.fixedLength>0?o.fixedLength:0),o.tol,f,ducts,index);
        const lk::Result r=lk::link(block,ducts,o);
        if(r.error!=lk::Ok){
            refuse(t,id,name,r.error,r.detail*f,all);
            if(wasLinked){for(int p=0;p<old.ports&&p<2;++p){EquipPort e;e.id=id;e.at=lk::Pt(old.port[p].x,old.port[p].y);scene.equipment.push_back(e);}}
            continue;}
        // preflight: every element written can be written
        std::vector<AcDbObjectId> touched(1,id);
        for(size_t e=0;e<r.edits.size();++e)touched.push_back(scene.nodes[index[r.edits[e].duct]].id);
        if(r.linkCase==lk::Cut)touched.push_back(scene.nodes[index[r.cutDuct]].id);
        bool assembly=false;for(size_t e=1;e<touched.size();++e)if(inAssembly(touched[e]))assembly=true;
        if(assembly||!tvsAction::writable(touched)){
            refuse(t,id,name,assembly?Assembly:Locked,0,false);
            if(wasLinked){for(int p=0;p<old.ports&&p<2;++p){EquipPort e;e.id=id;e.at=lk::Pt(old.port[p].x,old.port[p].y);scene.equipment.push_back(e);}}
            continue;}
        if(!known)rec.kind=r.ports==1?tvsEquipment::Grille:tvsEquipment::Other;
        rec.version=1;rec.ports=r.ports;rec.mmPerUnit=f;rec.round=r.round;rec.a=r.a;rec.b=r.round?0:r.b;
        rec.lengthMm=r.ports==2?r.length*f:0;
        for(int p=0;p<2;++p)rec.port[p]=p<r.ports?r.port[p]:tvsEquipment::Port();
        AcDbObjectId piece;
        if(!apply(id,r,index,scene,rec,piece)){
            refuse(t,id,name,WriteFailed,0,false);
            if(wasLinked){for(int p=0;p<old.ports&&p<2;++p){EquipPort e;e.id=id;e.at=lk::Pt(old.port[p].x,old.port[p].y);scene.equipment.push_back(e);}}
            continue;}
        if(r.ports==2)++t.inl;else ++t.term;
        t.ends+=static_cast<int>(r.edits.size());if(r.linkCase==lk::Cut)++t.cuts;
    }
    acutPrintf(L"\nTVS_OPREMA: povezano %d (u liniji %d, terminala %d); krajeva kanala skraćeno / produženo %d, presječeno kanala %d.",
        t.inl+t.term,t.inl,t.term,t.ends,t.cuts);
    if(t.skippedLinked)acutPrintf(L"\n  već povezanih (preskočeno): %d.",t.skippedLinked);
    if(t.offDuct)acutPrintf(L"\n  blokova van kanala (preskočeno): %d.",t.offDuct);
    if(t.xref)acutPrintf(L"\n  xref / layout blokova (preskočeno): %d.",t.xref);
    if(!t.refused.empty()){
        acutPrintf(L"\n  odbijeno %d:",static_cast<int>(t.refused.size()));
        for(int k=1;k<reasonCount;++k)if(t.count[k])acutPrintf(L"\n    %d × %ls",t.count[k],reasonText(k));
        tvsCheck::select(t.refused);   // odbijeni blokovi ostaju izabrani
    }
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

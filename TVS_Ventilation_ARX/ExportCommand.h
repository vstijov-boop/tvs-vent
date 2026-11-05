#pragma once
// TVS_IZVOZ: copy of the current drawing for users without TVS_VENT. Every TVS element is
// replaced by what it draws (lines, arcs, polylines ... the same as TVS_ERASE), in a new DWG;
// the open drawing is not changed. The copy is a full database copy (handles kept), the
// geometry comes from the elements of the open drawing (their drawn shape), with layer and
// linetype taken over by name.
#include "TVS_Pipe.h"
#include "TVS_TAP.h"
#include "TVS_WYE.h"
#include "TVS_TRANS.h"
#include "TVSMoveCommand.h"
#include "dbsymtb.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include <memory>
#include <vector>
#include <map>
namespace tvsExport {
inline AcDbObjectId byName(AcDbSymbolTable* table,const ACHAR* name){
    AcDbObjectId id;if(table&&name&&table->getAt(name,id)!=Acad::eOk)id.setNull();return id;
}
// Moves an exploded (non-resident) entity of `from` into `to`: same layer / linetype names,
// colour, lineweight, linetype scale, transparency.
inline void retarget(AcDbEntity* e,AcDbDatabase* to,AcDbLayerTable* layers,AcDbLinetypeTable* types){
    ACHAR* layer=e->layer();ACHAR* linetype=e->linetype();
    const AcCmColor color=e->color();const AcDb::LineWeight weight=e->lineWeight();
    const double scale=e->linetypeScale();const AcCmTransparency transparency=e->transparency();
    e->setDatabaseDefaults(to);
    const AcDbObjectId l=byName(layers,layer);if(!l.isNull())e->setLayer(l);
    const AcDbObjectId t=byName(types,linetype);if(!t.isNull())e->setLinetype(t);
    e->setColor(color);e->setLineWeight(weight);e->setLinetypeScale(scale);e->setTransparency(transparency);
    if(layer)acutDelString(layer);if(linetype)acutDelString(linetype);
}
// Identity of a TVS element independent of handles: class, ports and elevation.
inline bool signature(AcDbEntity* e,CString& out){
    tvsMove::Node n;if(!TVS_Entity::cast(e))return false;tvsMove::describe(e,n);
    out=e->isA()->name();CString v;
    // elbow ports may be values cached at drawing time: an elbow is known by its middle point
    if(!TVS_TAP::cast(e))for(size_t j=0;j<n.ports.size();++j){v.Format(L"|%.3f,%.3f,%.3f",n.ports[j].x,n.ports[j].y,n.ports[j].z);out+=v;}
    v.Format(L"|%.3f|%.3f|%.3f",n.elevation,n.a,n.b);out+=v;
    if(TVS_TAP* t=TVS_TAP::cast(e)){v.Format(L"|%.3f,%.3f",t->MiddlePoint.x,t->MiddlePoint.y);out+=v;}
    return true;
}
inline void run(){
    AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db)return;
    // file name
    CString def=L"bez_TVS";{const ACHAR* name=NULL;if(db->getFilename(name)==Acad::eOk&&name){CString n(name);
        const int slash=n.ReverseFind(L'\\');const int dot=n.ReverseFind(L'.');
        if(dot>slash)n=n.Left(dot);def=n+L"_bez_TVS";}}
    resbuf* rb=acutNewRb(RTSTR);
    if(acedGetFileD(L"Izvoz crteza bez TVS objekata",def,L"dwg",1,rb)!=RTNORM||!rb->resval.rstring){acutRelRb(rb);return;}
    CString path(rb->resval.rstring);acutRelRb(rb);
    // every TVS element must have been drawn once (its drawn shape is what is exported)
    acedCommandS(RTSTR,L"_.REGENALL",RTNONE);
    AcDbDatabase* raw=NULL;Acad::ErrorStatus wb;
    {
#ifdef TVS_OFFSET_INTEGRATION
        tvsOffsetData::WriteScope scope;   // etaz / obilazak are copied too (they are exploded in the copy)
#endif
        wb=db->wblock(raw);
    }
    if(wb!=Acad::eOk||!raw){acutPrintf(L"\nKopija crteza nije napravljena (%ls).",acadErrorStatusText(wb));return;}
    std::unique_ptr<AcDbDatabase> copy(raw);
    int replaced=0,empty=0,missing=0;
    // originals by signature (used when the copy did not keep the handle)
    std::map<CString,std::vector<AcDbObjectId> > bySignature;
    {AcDbBlockTable* t=NULL;if(db->getBlockTable(t,AcDb::kForRead)==Acad::eOk){AcDbObjectPointer<AcDbBlockTable> bt;bt.acquire(t);
        AcDbBlockTableIterator* bi=NULL;bt->newIterator(bi);std::unique_ptr<AcDbBlockTableIterator> it(bi);
        for(;it&&!it->done();it->step()){AcDbObjectId r;if(it->getRecordId(r)!=Acad::eOk)continue;
            AcDbObjectPointer<AcDbBlockTableRecord> btr(r,AcDb::kForRead);if(btr.openStatus()!=Acad::eOk)continue;
            AcDbBlockTableRecordIterator* ei=NULL;if(btr->newIterator(ei)!=Acad::eOk)continue;std::unique_ptr<AcDbBlockTableRecordIterator> eit(ei);
            for(;!eit->done();eit->step()){AcDbEntity* e=NULL;if(eit->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
                CString key;if(signature(e,key))bySignature[key].push_back(e->objectId());e->close();}}}}
    {
        AcDbObjectPointer<AcDbLayerTable> layers;AcDbObjectPointer<AcDbLinetypeTable> types;
        {AcDbLayerTable* l=NULL;copy->getLayerTable(l,AcDb::kForRead);layers.acquire(l);}
        {AcDbLinetypeTable* t=NULL;copy->getLinetypeTable(t,AcDb::kForRead);types.acquire(t);}
        AcDbBlockTable* bt=NULL;if(copy->getBlockTable(bt,AcDb::kForRead)!=Acad::eOk){acutPrintf(L"\nKopija nije citljiva.");return;}
        AcDbObjectPointer<AcDbBlockTable> blocks;blocks.acquire(bt);
        AcDbBlockTableIterator* bi=NULL;blocks->newIterator(bi);std::unique_ptr<AcDbBlockTableIterator> bit(bi);
        for(;bit&&!bit->done();bit->step()){
            AcDbObjectId btrId;if(bit->getRecordId(btrId)!=Acad::eOk)continue;
            AcDbObjectPointer<AcDbBlockTableRecord> btr(btrId,AcDb::kForWrite);if(btr.openStatus()!=Acad::eOk||btr->isFromExternalReference())continue;
            std::vector<AcDbObjectId> tvs;std::vector<CString> keys;
            {AcDbBlockTableRecordIterator* ei=NULL;if(btr->newIterator(ei)!=Acad::eOk)continue;std::unique_ptr<AcDbBlockTableRecordIterator> eit(ei);
             for(;!eit->done();eit->step()){AcDbEntity* e=NULL;if(eit->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
                 CString key;if(signature(e,key)){tvs.push_back(e->objectId());keys.push_back(key);}e->close();}}
            for(size_t i=0;i<tvs.size();++i){
                // the same element in the open drawing: same handle and class, else same signature
                AcDbObjectId original;bool found=false;
                if(db->getAcDbObjectId(original,false,tvs[i].handle())==Acad::eOk&&!original.isNull()){
                    AcDbObjectPointer<AcDbEntity> o(original,AcDb::kForRead);CString key;
                    found=o.openStatus()==Acad::eOk&&signature(o.object(),key)&&key==keys[i];}
                if(!found){std::map<CString,std::vector<AcDbObjectId> >::iterator m=bySignature.find(keys[i]);
                    if(m!=bySignature.end()&&!m->second.empty()){original=m->second.back();m->second.pop_back();found=true;}}
                if(!found){++missing;continue;}
                AcDbVoidPtrArray parts;
                {AcDbObjectPointer<AcDbEntity> o(original,AcDb::kForRead);if(o.openStatus()!=Acad::eOk||!TVS_Entity::cast(o.object()))continue;
                 if(o->explode(parts)!=Acad::eOk)parts.removeAll();}
                if(parts.isEmpty()){++empty;continue;}   // never drawn (e.g. frozen layer): left as it is
                for(int k=0;k<parts.length();++k){AcDbEntity* part=static_cast<AcDbEntity*>(parts[k]);
                    retarget(part,copy.get(),layers.object(),types.object());
                    AcDbObjectId id;if(btr->appendAcDbEntity(id,part)==Acad::eOk)part->close();else delete part;}
                AcDbObjectPointer<AcDbEntity> victim(tvs[i],AcDb::kForWrite);if(victim.openStatus()==Acad::eOk){victim->erase();++replaced;}
            }
        }
    }
    // TVS assembly records (obilazak parent records point at the replaced elements) are not
    // needed without TVS_VENT: removed from the copy so it saves clean.
    {AcDbDictionary* raw=NULL;if(copy->getNamedObjectsDictionary(raw,AcDb::kForWrite)==Acad::eOk){
        AcDbObjectPointer<AcDbDictionary> nod;nod.acquire(raw);std::vector<CString> keys;
        std::unique_ptr<AcDbDictionaryIterator> it(nod->newIterator());
        for(;it&&!it->done();it->next())if(!_wcsnicmp(it->name(),L"TVS_",4))keys.push_back(it->name());
        for(size_t k=0;k<keys.size();++k){AcDbObjectId id;if(nod->getAt(keys[k],id)==Acad::eOk){   // erasing removes the entry
            AcDbObjectPointer<AcDbObject> o(id,AcDb::kForWrite);if(o.openStatus()==Acad::eOk)o->erase();}}}}
    const Acad::ErrorStatus es=copy->saveAs(path);
    if(es!=Acad::eOk){acutPrintf(L"\nIzvoz nije sacuvan (%ls).",acadErrorStatusText(es));return;}
    acutPrintf(L"\nTVS_IZVOZ: %d TVS elemenata zamijenjeno crtezom; sacuvano u %ls.",replaced,path.GetString());
    if(missing)acutPrintf(L"\n%d TVS elemenata nije povezano sa originalom i ostali su TVS objekti.",missing);
    if(empty)acutPrintf(L"\n%d elemenata nije prikazano (npr. zamrznut sloj) i ostali su TVS objekti; odmrznite slojeve i ponovite izvoz.",empty);
    acutPrintf(L"\nOtvoreni crtez nije mijenjan.");
}
}

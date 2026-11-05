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
#include "dbsymtb.h"
#include "dbobjptr.h"
#include <memory>
#include <vector>
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
    AcDbDatabase* raw=NULL;if(db->wblock(raw)!=Acad::eOk||!raw){acutPrintf(L"\nKopija crteza nije napravljena.");return;}
    std::unique_ptr<AcDbDatabase> copy(raw);
    int replaced=0,empty=0;
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
            std::vector<AcDbObjectId> tvs;
            {AcDbBlockTableRecordIterator* ei=NULL;if(btr->newIterator(ei)!=Acad::eOk)continue;std::unique_ptr<AcDbBlockTableRecordIterator> eit(ei);
             for(;!eit->done();eit->step()){AcDbEntity* e=NULL;if(eit->getEntity(e,AcDb::kForRead)!=Acad::eOk)continue;
                 if(TVS_Entity::cast(e))tvs.push_back(e->objectId());e->close();}}
            for(size_t i=0;i<tvs.size();++i){
                // the same element in the open drawing (full copy keeps handles)
                AcDbObjectId original;if(db->getAcDbObjectId(original,false,tvs[i].handle())!=Acad::eOk)continue;
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
    const Acad::ErrorStatus es=copy->saveAs(path);
    if(es!=Acad::eOk){acutPrintf(L"\nIzvoz nije sacuvan (%ls).",acadErrorStatusText(es));return;}
    acutPrintf(L"\nTVS_IZVOZ: %d TVS elemenata zamijenjeno crtezom; sacuvano u %ls.",replaced,path.GetString());
    if(empty)acutPrintf(L"\n%d elemenata nije prikazano (npr. zamrznut sloj) i ostali su TVS objekti; odmrznite slojeve i ponovite izvoz.",empty);
    acutPrintf(L"\nOtvoreni crtez nije mijenjan.");
}
}

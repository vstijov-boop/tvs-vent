#pragma once
// Storage of geometry/Equipment.h records (oprema na mrezi):
//  - on a linked block reference: extension dictionary, XRecord TVS_OPREMA_V1
//    (1 "TVS_OPREMA/1", 1 <tvsEquipment::format(record)>);
//  - per block NAME (defaults for every block of that name: kind, loss model, length, note):
//    named objects dictionary TVS_OPREMA_TIPOVI, one XRecord per block name, same layout.
// Pattern of TVS_Ventilation_DBX/FlexCuffData.h: write() needs the object open for write and
// uses the active transaction when there is one.
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "acutads.h"
#include "dbtrans.h"
#include <string>
#include "../geometry/Equipment.h"
namespace tvsEquipmentStore {
inline const ACHAR* key(){return L"TVS_OPREMA_V1";}
inline const ACHAR* typesKey(){return L"TVS_OPREMA_TIPOVI";}
inline const ACHAR* tag(){return L"TVS_OPREMA/1";}
inline bool fromXrecord(const AcDbObjectId& id,tvsEquipment::Record& r){
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    bool ok=data->restype==1&&data->resval.rstring&&wcscmp(data->resval.rstring,tag())==0&&data->rbnext&&data->rbnext->restype==1&&data->rbnext->resval.rstring;
    if(ok)ok=tvsEquipment::parse(data->rbnext->resval.rstring,r);
    acutRelRb(data);return ok;
}
inline Acad::ErrorStatus toXrecord(AcDbDictionary* dict,const ACHAR* name,const tvsEquipment::Record& r,AcDbTransactionManager* tm,bool managed){
    AcDbObjectId id;AcDbXrecord* rec=NULL;Acad::ErrorStatus es=Acad::eOk;AcDbObject* raw=NULL;
    if(dict->getAt(name,id)==Acad::eOk){
        if(managed){es=tm->getObject(raw,id,AcDb::kForWrite);rec=AcDbXrecord::cast(raw);}
        else es=acdbOpenObject(rec,id,AcDb::kForWrite);
    }else{rec=new AcDbXrecord();es=dict->setAt(name,rec,id);
        if(es!=Acad::eOk){delete rec;rec=NULL;}else if(managed)es=tm->addNewlyCreatedDBRObject(rec,true);}
    if(es==Acad::eOk&&rec){const std::wstring text=tvsEquipment::format(r);
        resbuf n[2]={};n[0].restype=1;n[0].resval.rstring=const_cast<ACHAR*>(tag());
        n[1].restype=1;n[1].resval.rstring=const_cast<ACHAR*>(text.c_str());n[0].rbnext=&n[1];n[1].rbnext=NULL;
        es=rec->setFromRbChain(n[0]);}
    if(!managed&&rec)rec->close();
    return es;
}
// ---- on the block reference ----
inline bool read(const AcDbObject* o,tvsEquipment::Record& r){
    if(!o)return false;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
    return fromXrecord(id,r);
}
inline bool read(const AcDbObjectId& id,tvsEquipment::Record& r){
    AcDbObjectPointer<AcDbObject> o(id,AcDb::kForRead);return o.openStatus()==Acad::eOk&&read(o.object(),r);
}
inline bool linked(const AcDbObject* o){tvsEquipment::Record r;return read(o,r);}
inline Acad::ErrorStatus write(AcDbObject* o,const tvsEquipment::Record& r){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;
    Acad::ErrorStatus es=o->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
    AcDbTransactionManager* tm=o->database()->transactionManager();const bool managed=tm&&tm->numActiveTransactions()>0;
    AcDbObject* raw=NULL;AcDbDictionary* dict=NULL;
    if(managed){es=tm->getObject(raw,o->extensionDictionary(),AcDb::kForWrite);dict=AcDbDictionary::cast(raw);}
    else es=acdbOpenObject(dict,o->extensionDictionary(),AcDb::kForWrite);
    if(es!=Acad::eOk||!dict)return es==Acad::eOk?Acad::eNotThatKindOfClass:es;
    es=toXrecord(dict,key(),r,tm,managed);
    if(!managed)dict->close();
    if(es==Acad::eOk)if(AcDbEntity* e=AcDbEntity::cast(o))e->recordGraphicsModified();
    return es;
}
// Anchor (geometry/Equipment.h): the block's insertion point and rotation now - only the link
// commands set it, right before writing the record (the properties dialog / TVS_PROTOK keep it).
inline void anchor(const AcDbObject* o,tvsEquipment::Record& r){
    const AcDbBlockReference* b=AcDbBlockReference::cast(o);if(!b)return;
    r.anchored=true;r.ax=b->position().x;r.ay=b->position().y;r.arot=b->rotation();
}
inline double anchorTol(){return 1e-4;}   // drawing units
inline bool detached(const AcDbObject* o,const tvsEquipment::Record& r){
    const AcDbBlockReference* b=AcDbBlockReference::cast(o);
    return b&&tvsEquipment::detached(r,b->position().x,b->position().y,b->rotation(),anchorTol());
}
// Unlink: the record is removed, the block stays as it is.
inline Acad::ErrorStatus remove(AcDbObject* o){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return Acad::eOk;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForWrite);if(dict.openStatus()!=Acad::eOk)return dict.openStatus();
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return Acad::eOk;
    AcDbObjectPointer<AcDbObject> rec(id,AcDb::kForWrite);if(rec.openStatus()!=Acad::eOk)return rec.openStatus();
    return rec->erase();
}
// ---- defaults per block name (named objects dictionary) ----
inline bool readDefault(AcDbDatabase* db,const ACHAR* blockName,tvsEquipment::Record& r){
    if(!db||!blockName||!*blockName)return false;
    AcDbObjectPointer<AcDbDictionary> nod(db->namedObjectsDictionaryId(),AcDb::kForRead);if(nod.openStatus()!=Acad::eOk)return false;
    AcDbObjectId typesId;if(nod->getAt(typesKey(),typesId)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbDictionary> types(typesId,AcDb::kForRead);if(types.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(types->getAt(blockName,id)!=Acad::eOk)return false;
    return fromXrecord(id,r);
}
inline Acad::ErrorStatus writeDefault(AcDbDatabase* db,const ACHAR* blockName,const tvsEquipment::Record& r){
    if(!db||!blockName||!*blockName)return Acad::eInvalidInput;
    AcDbObjectPointer<AcDbDictionary> nod(db->namedObjectsDictionaryId(),AcDb::kForWrite);if(nod.openStatus()!=Acad::eOk)return nod.openStatus();
    AcDbObjectId typesId;AcDbDictionary* types=NULL;Acad::ErrorStatus es=Acad::eOk;
    if(nod->getAt(typesKey(),typesId)==Acad::eOk)es=acdbOpenObject(types,typesId,AcDb::kForWrite);
    else{types=new AcDbDictionary();es=nod->setAt(typesKey(),types,typesId);if(es!=Acad::eOk){delete types;types=NULL;}}
    if(es!=Acad::eOk||!types)return es==Acad::eOk?Acad::eNullObjectPointer:es;
    es=toXrecord(types,blockName,r,NULL,false);
    types->close();return es;
}
}

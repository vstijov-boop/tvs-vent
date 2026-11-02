#pragma once
// Flex hose cuffs (manzetne): one flag per piece end, filed with the element in its extension
// dictionary (TVS_FLEX_CUFF_V1: 1 "TVS_FLEX_CUFF/1", 70 start, 70 end). Display/specification
// only: geometry, length and connections never change. Set by TVS_FLEX_MANZETNE or by hand.
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "acutads.h"
#include "dbtrans.h"
namespace tvsFlexCuff {
inline const ACHAR* key(){return L"TVS_FLEX_CUFF_V1";}
inline double lengthMm(){return 40;}   // fixed cuff length (user decision)
inline double extraMm(){return 5;}     // clamp: wider than D by this on each side
inline bool read(const AcDbObject* o,bool& start,bool& end){
    start=end=false;if(!o)return false;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    bool ok=data->restype==1&&data->rbnext&&data->rbnext->restype==70&&data->rbnext->rbnext&&data->rbnext->rbnext->restype==70;
    if(ok){start=data->rbnext->resval.rint!=0;end=data->rbnext->rbnext->resval.rint!=0;}
    acutRelRb(data);return ok;
}
// Object must be open for write. Uses the active transaction when there is one.
inline Acad::ErrorStatus write(AcDbObject* o,bool start,bool end){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;
    Acad::ErrorStatus es=o->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
    AcDbTransactionManager* tm=o->database()->transactionManager();const bool managed=tm&&tm->numActiveTransactions()>0;
    AcDbObject* raw=NULL;AcDbDictionary* dict=NULL;
    if(managed){es=tm->getObject(raw,o->extensionDictionary(),AcDb::kForWrite);dict=AcDbDictionary::cast(raw);}
    else es=acdbOpenObject(dict,o->extensionDictionary(),AcDb::kForWrite);
    if(es!=Acad::eOk||!dict)return es==Acad::eOk?Acad::eNotThatKindOfClass:es;
    AcDbObjectId id;AcDbXrecord* rec=NULL;
    if(dict->getAt(key(),id)==Acad::eOk){
        if(managed){raw=NULL;es=tm->getObject(raw,id,AcDb::kForWrite);rec=AcDbXrecord::cast(raw);}
        else es=acdbOpenObject(rec,id,AcDb::kForWrite);
    }else{rec=new AcDbXrecord();es=dict->setAt(key(),rec,id);
        if(es!=Acad::eOk){delete rec;rec=NULL;}else if(managed)es=tm->addNewlyCreatedDBRObject(rec,true);}
    if(es==Acad::eOk&&rec){resbuf n[3]={};n[0].restype=1;n[0].resval.rstring=const_cast<ACHAR*>(L"TVS_FLEX_CUFF/1");
        n[1].restype=70;n[1].resval.rint=start?1:0;n[2].restype=70;n[2].resval.rint=end?1:0;n[0].rbnext=&n[1];n[1].rbnext=&n[2];n[2].rbnext=NULL;
        es=rec->setFromRbChain(n[0]);}
    if(!managed){if(rec)rec->close();dict->close();}
    if(es==Acad::eOk)if(AcDbEntity* e=AcDbEntity::cast(o))e->recordGraphicsModified();
    return es;
}
}

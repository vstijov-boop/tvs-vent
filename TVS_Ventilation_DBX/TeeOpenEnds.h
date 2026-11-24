#pragma once
// Ravni T komad (TK0) without the cross lines at its two run ends (user decision 2026-11-24): a
// grille connected to the side of a duct with TVS_RESETKA / TK0 is drawn like a stub on a
// continuous duct. Only the drawing changes; the T komad stays a normal element (ports,
// specification, pressure drop). Filed in the element's extension dictionary:
// TVS_TEE_OPEN_V1 = (1 "TVS_TEE_OPEN/1"). Ducts and other TK0 have no record and are unchanged.
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "dbtrans.h"
#include "acutads.h"
namespace tvsTeeOpenEnds {
inline const ACHAR* key(){return L"TVS_TEE_OPEN_V1";}
inline bool read(const AcDbObject* o){
    if(!o)return false;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;return dict->getAt(key(),id)==Acad::eOk&&!id.isNull();
}
// Object open for write; uses the active transaction when there is one. on = false removes the record.
inline Acad::ErrorStatus write(AcDbObject* o,bool on){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;
    if(!on&&o->extensionDictionary().isNull())return Acad::eOk;
    Acad::ErrorStatus es=o->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
    AcDbTransactionManager* tm=o->database()->transactionManager();const bool managed=tm&&tm->numActiveTransactions()>0;
    AcDbObject* raw=NULL;AcDbDictionary* dict=NULL;
    if(managed){es=tm->getObject(raw,o->extensionDictionary(),AcDb::kForWrite);dict=AcDbDictionary::cast(raw);}
    else es=acdbOpenObject(dict,o->extensionDictionary(),AcDb::kForWrite);
    if(es!=Acad::eOk||!dict)return es==Acad::eOk?Acad::eNotThatKindOfClass:es;
    AcDbObjectId id;const bool exists=dict->getAt(key(),id)==Acad::eOk;
    if(!on){if(exists)es=dict->remove(key());}
    else if(!exists){AcDbXrecord* rec=new AcDbXrecord();es=dict->setAt(key(),rec,id);
        if(es!=Acad::eOk){delete rec;rec=NULL;}else if(managed)es=tm->addNewlyCreatedDBRObject(rec,true);
        if(es==Acad::eOk&&rec){resbuf n={};n.restype=1;n.resval.rstring=const_cast<ACHAR*>(L"TVS_TEE_OPEN/1");n.rbnext=NULL;es=rec->setFromRbChain(n);}
        if(!managed&&rec)rec->close();}
    if(!managed)dict->close();
    if(es==Acad::eOk)if(AcDbEntity* e=AcDbEntity::cast(o))e->recordGraphicsModified();
    return es;
}
}

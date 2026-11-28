#pragma once
// Krstasti komad / XK (plan 26, user 2026-11-28): a second branch opposite the first one on a
// TVS_WYE - the plain T komad (TK0) becomes the krstasti komad (Soko "X", e.g. 200/100/100),
// the kosa racva TK (round, 45 deg) the XK (Soko "Spiro XK", two 45 deg branches on opposite
// sides). Filed apart from TVS_TEE_V1 so every older reader still sees a normal T komad:
// TVS_TEE_X1 = (1 "TVS_TEE_X/1")(40 width / diameter)(40 height, 0 round)(70 round), drawing units.
// The second branch is the mirror of the first across the main axis (same place along the main,
// TK0: perpendicular, XK: the same 45 deg leaning downstream), not wider than the first one (the
// run stays as it is). TVS port order: 0, 1 the run, 2 the first branch, 3 the second branch.
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "dbtrans.h"
#include "acutads.h"
namespace tvsTeeCross {
inline const ACHAR* key(){return L"TVS_TEE_X1";}
struct Record {double a,b;bool round;Record():a(0),b(0),round(true){}};
inline bool read(const AcDbObject* o,Record& r){
    r=Record();if(!o)return false;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    const short types[]={1,40,40,70};resbuf* p=data;bool ok=true;
    for(int j=0;j<4;++j){if(!p||p->restype!=types[j]){ok=false;break;}
        if(j==1)r.a=p->resval.rreal;if(j==2)r.b=p->resval.rreal;if(j==3)r.round=p->resval.rint!=0;p=p->rbnext;}
    acutRelRb(data);
    if(!ok||!(r.a>0)||(!r.round&&!(r.b>0))){r=Record();return false;}
    return true;
}
// The record is there (valid or not) - TVS_POPRAVI.
inline bool present(const AcDbObject* o){
    if(!o)return false;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    return dict->has(key());
}
// Object open for write; uses the active transaction when there is one. r = NULL removes the record.
inline Acad::ErrorStatus write(AcDbObject* o,const Record* r){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;
    if(!r&&o->extensionDictionary().isNull())return Acad::eOk;
    Acad::ErrorStatus es=o->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
    AcDbTransactionManager* tm=o->database()->transactionManager();const bool managed=tm&&tm->numActiveTransactions()>0;
    AcDbObject* raw=NULL;AcDbDictionary* dict=NULL;
    if(managed){es=tm->getObject(raw,o->extensionDictionary(),AcDb::kForWrite);dict=AcDbDictionary::cast(raw);}
    else es=acdbOpenObject(dict,o->extensionDictionary(),AcDb::kForWrite);
    if(es!=Acad::eOk||!dict)return es==Acad::eOk?Acad::eNotThatKindOfClass:es;
    AcDbObjectId id;const bool exists=dict->getAt(key(),id)==Acad::eOk;
    if(!r){if(exists){es=dict->remove(key());   // and erase the xrecord (no orphan left in the drawing)
            if(es==Acad::eOk){AcDbObject* gone=NULL;
                if(managed){if(tm->getObject(gone,id,AcDb::kForWrite)==Acad::eOk&&gone)gone->erase();}
                else if(acdbOpenObject(gone,id,AcDb::kForWrite)==Acad::eOk){gone->erase();gone->close();}}}}
    else{AcDbXrecord* rec=NULL;
        if(exists){if(managed){raw=NULL;es=tm->getObject(raw,id,AcDb::kForWrite);rec=AcDbXrecord::cast(raw);}else es=acdbOpenObject(rec,id,AcDb::kForWrite);}
        else{rec=new AcDbXrecord();es=dict->setAt(key(),rec,id);if(es!=Acad::eOk){delete rec;rec=NULL;}else if(managed)es=tm->addNewlyCreatedDBRObject(rec,true);}
        if(es==Acad::eOk&&rec){resbuf n[4]={};n[0].restype=1;n[0].resval.rstring=const_cast<ACHAR*>(L"TVS_TEE_X/1");
            n[1].restype=40;n[1].resval.rreal=r->a;n[2].restype=40;n[2].resval.rreal=r->round?0:r->b;
            n[3].restype=70;n[3].resval.rint=static_cast<short>(r->round?1:0);
            for(int j=0;j<3;++j)n[j].rbnext=&n[j+1];n[3].rbnext=NULL;es=rec->setFromRbChain(n[0]);}
        if(!managed&&rec)rec->close();}
    if(!managed)dict->close();
    if(es==Acad::eOk)if(AcDbEntity* e=AcDbEntity::cast(o))e->recordGraphicsModified();
    return es;
}
}

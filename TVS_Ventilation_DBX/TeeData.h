#pragma once
// T komad type of a TVS_WYE (TK-O "sa ogrankom", TK-G "gacasti"), filed in the element's
// extension dictionary: TVS_TEE_V1 = (1 "TVS_TEE/1")(70 type)(40 collar l)(40 second width)
// (40 second radius) [(40 vrat)], drawing units. No record = existing straight TVS_WYE (TK0).
// vrat (TK-G, optional, older records have none): axis of branch 2 minus axis of branch 1, away
// from the inlet; > 0: branch 2 has that much more straight inlet wall (e2), < 0: branch 1 (e1).
// The first branch radius is not stored: R1 = LengthPl - l + w1, so ports, Properties
// "Length" and the drawing always agree (TVS_WYE port model, geometry/TeeShape.h).
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "dbtrans.h"
#include "acutads.h"
#include "../geometry/TeeShape.h"
namespace tvsTeeData {
inline const ACHAR* key(){return L"TVS_TEE_V1";}
inline double collarMm(){return 25;}   // ravni prikljucak, as in the block
inline double flangeMm(){return 25;}   // flange tick beyond the duct edge
inline double tapMinimumMm(){return 75;} // stucna SR5-3: L = 0.25 w, at least 75 mm
struct Record {int type;double l,second,radius2,neck;Record():type(0),l(0),second(0),radius2(0),neck(0){}};
inline bool read(const AcDbObject* o,Record& r){
    r=Record();if(!o)return false;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return false;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return false;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return false;
    const short types[]={1,70,40,40,40};resbuf* p=data;bool ok=true;
    for(int j=0;j<5;++j){if(!p||p->restype!=types[j]){ok=false;break;}
        if(j==1)r.type=p->resval.rint;if(j==2)r.l=p->resval.rreal;if(j==3)r.second=p->resval.rreal;if(j==4)r.radius2=p->resval.rreal;p=p->rbnext;}
    if(ok&&p&&p->restype==40)r.neck=p->resval.rreal;
    acutRelRb(data);
    if(!ok||(r.type!=tvsTee::Branch&&r.type!=tvsTee::Pants&&r.type!=tvsTee::Tap)||!(r.l>0)){r=Record();return false;}
    return true;
}
// Object open for write; uses the active transaction when there is one. type 0 removes the record.
inline Acad::ErrorStatus write(AcDbObject* o,const Record& r){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;
    Acad::ErrorStatus es=o->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
    AcDbTransactionManager* tm=o->database()->transactionManager();const bool managed=tm&&tm->numActiveTransactions()>0;
    AcDbObject* raw=NULL;AcDbDictionary* dict=NULL;
    if(managed){es=tm->getObject(raw,o->extensionDictionary(),AcDb::kForWrite);dict=AcDbDictionary::cast(raw);}
    else es=acdbOpenObject(dict,o->extensionDictionary(),AcDb::kForWrite);
    if(es!=Acad::eOk||!dict)return es==Acad::eOk?Acad::eNotThatKindOfClass:es;
    AcDbObjectId id;const bool exists=dict->getAt(key(),id)==Acad::eOk;
    if(r.type==0){if(exists)es=dict->remove(key());}
    else{AcDbXrecord* rec=NULL;
        if(exists){if(managed){raw=NULL;es=tm->getObject(raw,id,AcDb::kForWrite);rec=AcDbXrecord::cast(raw);}else es=acdbOpenObject(rec,id,AcDb::kForWrite);}
        else{rec=new AcDbXrecord();es=dict->setAt(key(),rec,id);if(es!=Acad::eOk){delete rec;rec=NULL;}else if(managed)es=tm->addNewlyCreatedDBRObject(rec,true);}
        if(es==Acad::eOk&&rec){resbuf n[6]={};n[0].restype=1;n[0].resval.rstring=const_cast<ACHAR*>(L"TVS_TEE/1");
            n[1].restype=70;n[1].resval.rint=static_cast<short>(r.type);n[2].restype=40;n[2].resval.rreal=r.l;n[3].restype=40;n[3].resval.rreal=r.second;n[4].restype=40;n[4].resval.rreal=r.radius2;
            n[5].restype=40;n[5].resval.rreal=r.neck;const int last=r.type==tvsTee::Pants&&r.neck!=0?5:4;   // no vrat: the old 5 items
            for(int j=0;j<last;++j)n[j].rbnext=&n[j+1];n[last].rbnext=NULL;es=rec->setFromRbChain(n[0]);}
        if(!managed&&rec)rec->close();}
    if(!managed)dict->close();
    if(es==Acad::eOk)if(AcDbEntity* e=AcDbEntity::cast(o))e->recordGraphicsModified();
    return es;
}
}

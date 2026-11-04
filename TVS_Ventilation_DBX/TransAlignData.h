#pragma once
// Poravnanje prelaza po visini (TVS_TRANS): 0 = osa (symmetric in height, as before),
// 1 = ravno uz gornju ivicu, 2 = ravno uz donju ivicu. Independent of the plan type
// (TransType 1 simetricno, 2 / 3 ravno uz jednu stranu). Filed in the element's extension
// dictionary: TVS_TRANS_VERT_V1 = (1 "TVS_TRANS_VERT/1")(70 align); no record = osa.
// Elev of the transition is the axis at its start (FirstPoint, SizeAp1); the axis at the end is
// Elev + endOffset().
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbobjptr.h"
#include "dbtrans.h"
#include "acutads.h"
namespace tvsTransAlign {
enum Align {Axis=0,Top=1,Bottom=2};
inline const ACHAR* key(){return L"TVS_TRANS_VERT_V1";}
inline const wchar_t* name(int a){return a==Top?L"Gore":a==Bottom?L"Dolje":L"Osa";}
inline double height(double a,double b){return b>0?b:a;}   // round: diameter
// Axis at the end relative to the axis at the start.
inline double endOffset(int align,double A1,double B1,double A2,double B2){
    const double d=(height(A1,B1)-height(A2,B2))/2;
    return align==Top?d:align==Bottom?-d:0;
}
inline int read(const AcDbObject* o){
    if(!o)return Axis;const AcDbObjectId dictId=o->extensionDictionary();if(dictId.isNull())return Axis;
    AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return Axis;
    AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return Axis;
    AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);if(rec.openStatus()!=Acad::eOk)return Axis;
    resbuf* data=NULL;if(rec->rbChain(&data)!=Acad::eOk||!data)return Axis;
    int a=Axis;if(data->restype==1&&data->rbnext&&data->rbnext->restype==70)a=data->rbnext->resval.rint;
    acutRelRb(data);return a==Top||a==Bottom?a:Axis;
}
// Object open for write.
inline Acad::ErrorStatus write(AcDbObject* o,int align){
    if(!o||!o->isWriteEnabled())return Acad::eInvalidInput;
    Acad::ErrorStatus es=o->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
    AcDbDictionary* dict=NULL;es=acdbOpenObject(dict,o->extensionDictionary(),AcDb::kForWrite);if(es!=Acad::eOk||!dict)return es;
    AcDbObjectId id;AcDbXrecord* rec=NULL;
    if(dict->getAt(key(),id)==Acad::eOk){es=acdbOpenObject(rec,id,AcDb::kForWrite);if(es!=Acad::eOk)rec=NULL;}
    else{rec=new AcDbXrecord();es=dict->setAt(key(),rec,id);if(es!=Acad::eOk){delete rec;rec=NULL;}}
    if(rec){resbuf* data=acutBuildList(1,L"TVS_TRANS_VERT/1",70,align,RTNONE);es=rec->setFromRbChain(*data);acutRelRb(data);rec->close();}
    dict->close();return es;
}
}

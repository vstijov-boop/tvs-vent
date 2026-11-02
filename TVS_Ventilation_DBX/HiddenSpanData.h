#pragma once
// Bypass-only hidden edges (GFX-01, variant 1). The lower straight duct of a bypass
// crossing (obstacle for "above", H member for "below") carries TVS_HIDDEN_BY_V1 in its
// extension dictionary: one entry per upper duct = soft pointer + plan axis, width and axis
// level at write time. Drawing uses the live upper duct when it can be read, the stored
// values otherwise, skips erased uppers and requires lower axis < upper axis, so a moved,
// copied or deleted piece falls back to the normal solid edges.
#include "dbxrecrd.h"
#include "dbdict.h"
#include "dbapserv.h"
#include "dbobjptr.h"
#include "acutads.h"
#include <vector>
namespace tvsHiddenSpan {
inline const ACHAR* key(){return L"TVS_HIDDEN_BY_V1";}
struct Entry {AcDbObjectId upper;AcGePoint3d first,last;double width,elevation;Entry():width(0),elevation(0){}};
inline bool read(const AcDbObject* object,std::vector<Entry>& out){
 out.clear();if(!object)return false;AcDbObjectId dictId=object->extensionDictionary();if(dictId.isNull())return false;
 AcDbObjectPointer<AcDbDictionary> dict(dictId,AcDb::kForRead);if(dict.openStatus()!=Acad::eOk)return false;
 AcDbObjectId id;if(dict->getAt(key(),id)!=Acad::eOk)return false;
 AcDbObjectPointer<AcDbXrecord> record(id,AcDb::kForRead);if(record.openStatus()!=Acad::eOk)return false;
 resbuf* data=NULL;if(record->rbChain(&data)!=Acad::eOk||!data)return false;
 // (1 "TVS_HIDDEN_BY/1") then per entry: 330 upper, 10 first, 11 last, 40 width, 41 elevation
 bool ok=data->restype==1;resbuf* p=data->rbnext;
 while(ok&&p){Entry e;const short types[]={330,10,11,40,41};
  for(int j=0;j<5;++j){if(!p||p->restype!=types[j]){ok=false;break;}
   if(j==0)ok=acdbGetObjectId(e.upper,p->resval.rlname)==Acad::eOk;
   if(j==1)e.first.set(p->resval.rpoint[0],p->resval.rpoint[1],p->resval.rpoint[2]);if(j==2)e.last.set(p->resval.rpoint[0],p->resval.rpoint[1],p->resval.rpoint[2]);
   if(j==3)e.width=p->resval.rreal;if(j==4)e.elevation=p->resval.rreal;p=p->rbnext;}
  if(ok)out.push_back(e);}
 acutRelRb(data);if(!ok)out.clear();return !out.empty();
}
// Appends one entry; inside an active transaction the dictionary/record go through it.
inline Acad::ErrorStatus append(AcDbObject* object,const Entry& e){
 if(!object||!object->isWriteEnabled()||e.upper.isNull())return Acad::eInvalidInput;
 std::vector<Entry> entries;read(object,entries);entries.push_back(e);
 Acad::ErrorStatus es=object->createExtensionDictionary();if(es!=Acad::eOk&&es!=Acad::eAlreadyInDb)return es;
 AcDbTransactionManager* manager=object->database()->transactionManager();
 const bool managed=manager&&manager->numActiveTransactions()>0;AcDbObject* raw=NULL;AcDbDictionary* dict=NULL;
 if(managed){es=manager->getObject(raw,object->extensionDictionary(),AcDb::kForWrite);dict=AcDbDictionary::cast(raw);}
 else es=acdbOpenObject(dict,object->extensionDictionary(),AcDb::kForWrite);
 if(es!=Acad::eOk||!dict)return es==Acad::eOk?Acad::eNotThatKindOfClass:es;
 AcDbObjectId id;AcDbXrecord* record=NULL;
 if(dict->getAt(key(),id)==Acad::eOk){
  if(managed){raw=NULL;es=manager->getObject(raw,id,AcDb::kForWrite);record=AcDbXrecord::cast(raw);}
  else es=acdbOpenObject(record,id,AcDb::kForWrite);
 }else{record=new AcDbXrecord();es=dict->setAt(key(),record,id);
  if(es!=Acad::eOk){delete record;record=NULL;}else if(managed)es=manager->addNewlyCreatedDBRObject(record,true);}
 if(es==Acad::eOk&&record){
  std::vector<resbuf> nodes(1+5*entries.size());for(auto& n:nodes){n=resbuf();}
  nodes[0].restype=1;nodes[0].resval.rstring=const_cast<ACHAR*>(L"TVS_HIDDEN_BY/1");
  for(size_t i=0;i<entries.size()&&es==Acad::eOk;++i){resbuf* n=&nodes[1+5*i];const Entry& x=entries[i];
   n[0].restype=330;es=acdbGetAdsName(n[0].resval.rlname,x.upper);
   n[1].restype=10;n[1].resval.rpoint[0]=x.first.x;n[1].resval.rpoint[1]=x.first.y;n[1].resval.rpoint[2]=x.first.z;
   n[2].restype=11;n[2].resval.rpoint[0]=x.last.x;n[2].resval.rpoint[1]=x.last.y;n[2].resval.rpoint[2]=x.last.z;
   n[3].restype=40;n[3].resval.rreal=x.width;n[4].restype=41;n[4].resval.rreal=x.elevation;}
  for(size_t j=0;j+1<nodes.size();++j)nodes[j].rbnext=&nodes[j+1];nodes.back().rbnext=NULL;
  if(es==Acad::eOk){record->setXlateReferences(true);es=record->setFromRbChain(nodes[0]);}
 }
 if(!managed){if(record)record->close();dict->close();}
 return es;
}
}

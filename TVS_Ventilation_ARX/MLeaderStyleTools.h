#pragma once
// Multileader styles for TVS labels: list the drawing's styles, find one by name, and copy a
// style (with its text style) from another DWG via wblockCloneObjects (existing names kept).
#include "dbmleaderstyle.h"
#include "dbdict.h"
#include "dbsymtb.h"
#include "dbobjptr.h"
#include "dbidmap.h"
#include <vector>
#include <memory>
namespace tvsMLeaderStyle {
inline bool names(AcDbDatabase* db,std::vector<CString>& out){
    out.clear();if(!db)return false;AcDbDictionary* raw=NULL;if(db->getMLeaderStyleDictionary(raw,AcDb::kForRead)!=Acad::eOk||!raw)return false;
    AcDbObjectPointer<AcDbDictionary> dict;dict.acquire(raw);
    std::unique_ptr<AcDbDictionaryIterator> it(dict->newIterator());if(!it)return false;
    for(;!it->done();it->next())out.push_back(CString(it->name()));
    return true;
}
inline AcDbObjectId find(AcDbDatabase* db,const CString& name){
    AcDbObjectId id;if(!db||name.IsEmpty())return id;AcDbDictionary* raw=NULL;
    if(db->getMLeaderStyleDictionary(raw,AcDb::kForRead)!=Acad::eOk||!raw)return id;
    AcDbObjectPointer<AcDbDictionary> dict;dict.acquire(raw);if(dict->getAt(name.GetString(),id)!=Acad::eOk)id.setNull();return id;
}
// Styles of a DWG on disk (read-only side database).
inline bool namesInFile(const CString& path,std::vector<CString>& out){
    out.clear();std::unique_ptr<AcDbDatabase> side(new AcDbDatabase(false,true));
    if(side->readDwgFile(path.GetString(),AcDbDatabase::kForReadAndAllShare)!=Acad::eOk)return false;
    return names(side.get(),out);
}
inline Acad::ErrorStatus import(AcDbDatabase* dest,const CString& path,const CString& name,AcDbObjectId& result){
    result=find(dest,name);if(!result.isNull())return Acad::eOk;
    std::unique_ptr<AcDbDatabase> side(new AcDbDatabase(false,true));
    Acad::ErrorStatus es=side->readDwgFile(path.GetString(),AcDbDatabase::kForReadAndAllShare);if(es!=Acad::eOk)return es;
    const AcDbObjectId source=find(side.get(),name);if(source.isNull())return Acad::eKeyNotFound;
    AcDbObjectId owner=dest->mleaderStyleDictionaryId();if(owner.isNull())return Acad::eNullObjectId;
    AcDbObjectIdArray ids;ids.append(source);AcDbIdMapping map;
    es=side->wblockCloneObjects(ids,owner,map,AcDb::kDrcIgnore);if(es!=Acad::eOk)return es;
    result=find(dest,name);return result.isNull()?Acad::eKeyNotFound:Acad::eOk;
}
// Text height (paper mm as defined in the style) and text style name, for the dialog info.
inline bool info(const AcDbObjectId& id,double& height,CString& textStyle){
    height=0;textStyle.Empty();AcDbObjectPointer<AcDbMLeaderStyle> s(id,AcDb::kForRead);if(s.openStatus()!=Acad::eOk)return false;
    height=s->textHeight();AcDbObjectPointer<AcDbTextStyleTableRecord> t(s->textStyleId(),AcDb::kForRead);
    if(t.openStatus()==Acad::eOk){const ACHAR* n=NULL;if(t->getName(n)==Acad::eOk&&n)textStyle=n;}
    return true;
}
}

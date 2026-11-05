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
#include <shlobj.h>
#include "acestext.h"
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
// Style shipped with TVS (TVS.bundle\Contents\Resources\TVS_Oznake.dwg). The bundle's
// Resources folder comes first, so a copy elsewhere on the support path (e.g. next to the
// ARX from a downloaded package) cannot shadow it; otherwise the AutoCAD support path.
inline const wchar_t* bundledStyle(){return L"TVS_MleaderStyle";}
inline CString bundledSource(){
    wchar_t base[MAX_PATH]={0};
    if(SUCCEEDED(SHGetFolderPathW(NULL,CSIDL_COMMON_APPDATA,NULL,0,base))){
        CString p(base);p+=L"\\Autodesk\\ApplicationPlugins\\TVS.bundle\\Contents\\Resources\\TVS_Oznake.dwg";
        if(GetFileAttributesW(p)!=INVALID_FILE_ATTRIBUTES)return p;}
    ACHAR found[MAX_PATH*2]={0};
    if(acedFindFile(L"TVS_Oznake.dwg",found)==RTNORM&&found[0])return CString(found);
    return CString();
}
// One message per style/source and session when the style cannot be taken from the DWG.
inline void report(const CString& name,const CString& source,const wchar_t* why){
    static std::vector<CString> told;CString key=name+L"|"+source;
    for(const CString& k:told)if(k==key)return;told.push_back(key);
    acutPrintf(L"\nTVS oznake: stil \"%s\" nije učitan iz \"%s\": %s",name.GetString(),source.IsEmpty()?L"TVS_Oznake.dwg":source.GetString(),why);
}
// Copies `name` from `source` when missing. A freshly copied style becomes the drawing's
// current multileader style only while the drawing still uses "Standard", so a user's own
// choice is never overwritten. Returns the style id (null when unavailable).
inline AcDbObjectId ensure(AcDbDatabase* db,const CString& name,const CString& source){
    AcDbObjectId id=find(db,name);if(!id.isNull()||!db||name.IsEmpty())return id;
    if(source.IsEmpty()){report(name,source,L"fajl nije pronađen (Resources ni putanja podrške)");return id;}
    if(GetFileAttributesW(source)==INVALID_FILE_ATTRIBUTES){report(name,source,L"fajl ne postoji");return id;}
    const Acad::ErrorStatus es=import(db,source,name,id);
    if(es!=Acad::eOk){report(name,source,es==Acad::eKeyNotFound?L"stil ne postoji u fajlu":acadErrorStatusText(es));return AcDbObjectId();}
    const AcDbObjectId current=db->mleaderstyle();
    const bool standard=current.isNull()||current==find(db,L"Standard");
    if(standard)db->setMLeaderstyle(id);
    return id;
}
// The shipped TVS_MleaderStyle is always made available, whatever style the user chose
// (an empty/other saved choice must not stop it); then the chosen style from its own DWG.
inline void ensureAll(AcDbDatabase* db,const CString& name,const CString& source){
    if(!db)return;
    ensure(db,bundledStyle(),bundledSource());
    if(!name.IsEmpty()&&name!=bundledStyle())ensure(db,name,source.IsEmpty()?bundledSource():source);
}
// Drawing open / ARX load / label dialog: same for the current document.
inline void ensureCurrentDocument(const CString& name,const CString& source){
    AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(!doc||!doc->database())return;
    const bool locked=acDocManager->lockDocument(doc,AcAp::kWrite)==Acad::eOk;
    ensureAll(doc->database(),name,source);
    if(locked)acDocManager->unlockDocument(doc);
}
}

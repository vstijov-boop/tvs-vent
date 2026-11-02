#pragma once
// TVS draws hidden/centre lines with the linetypes tvs_hidden and tvs_centerline
// (TVS_Entity::setHideProperty/setCenterProperty). The installed TVS loads them from
// TVS_LineType.lin via TVS_EnterPoint.lsp; when the ARX is loaded without it the names are
// missing and the lines fall back to Continuous. Create the missing ones here with the same
// pattern as TVS_LineType.lin (mm), scaled to the drawing units. Existing records are never changed.
#include "dbsymtb.h"
#include "acdocman.h"
#include "../TVS_Ventilation_DBX/DrawingUnits.h"
#include <cmath>
namespace tvsLinetypes {
struct Pattern {const wchar_t* name;const wchar_t* text;int count;double dash[4];};
inline bool ensure(AcDbDatabase* db){
 if(!db)return false;bool created=false;
 static const Pattern patterns[]={
  {L"tvs_hidden",L"TVS_hidden __ __ __ __ __ __ __ __",2,{100,-50,0,0}},
  {L"tvs_centerline",L"TVS_CenterLine ____ _ ____ _ ____ _ ____",4,{200,-40,40,-40}}};
 const double factor=tvsUnits::factor(db);
 AcDbLinetypeTable* table=NULL;if(db->getLinetypeTable(table,AcDb::kForRead)!=Acad::eOk||!table)return false;
 for(const Pattern& p:patterns){
  if(table->has(p.name))continue;
  if(!table->isWriteEnabled()&&table->upgradeOpen()!=Acad::eOk)break;
  AcDbLinetypeTableRecord* lt=new AcDbLinetypeTableRecord();double length=0;
  bool ok=lt->setName(p.name)==Acad::eOk&&lt->setAsciiDescription(p.text)==Acad::eOk;
  lt->setNumDashes(p.count);
  for(int i=0;i<p.count;++i){lt->setDashLengthAt(i,p.dash[i]/factor);length+=std::fabs(p.dash[i])/factor;}
  lt->setPatternLength(length);lt->setIsScaledToFit(false);
  if(ok&&table->add(lt)==Acad::eOk){lt->close();created=true;}else delete lt;
 }
 table->close();return created;
}
// Current document (application context: lock it for the write). TVS entities already drawn
// cached Continuous, so a REGEN is queued only when a linetype was actually created.
inline void ensureCurrent(){
 AcApDocument* doc=acDocManager?acDocManager->curDocument():NULL;if(!doc||!doc->database())return;
 const bool locked=acDocManager->lockDocument(doc,AcAp::kWrite)==Acad::eOk;
 const bool created=ensure(doc->database());
 if(locked)acDocManager->unlockDocument(doc);
 if(created)acDocManager->sendStringToExecute(doc,L"_.REGEN\n",false,true,false);
}
}

#pragma once
#include "../geometry/bypass/SideDetail.h"
#include "dbdim.h"
#include "dbmtext.h"
#include "dbxrecrd.h"
#include "dbtrans.h"
#include "actrans.h"
#include <set>
#include <map>
#include <stdexcept>
namespace tvsSideDetail {
using namespace tvsBypassPreview;
static const wchar_t* recordKey(){return L"TVS_BYPASS_SIDE_DETAIL_V1";}
static void require(Acad::ErrorStatus s){if(s!=Acad::eOk)throw std::runtime_error("DWG detail API error");}
static std::wstring widen(const std::string& s){return std::wstring(s.begin(),s.end());} // detail contract uses ASCII
struct Identity {std::wstring label,guid;};
static Identity identity(AcDbBlockReference* ref){
 Identity result;if(ref->extensionDictionary().isNull())return result;
 AcDbObjectPointer<AcDbDictionary> dict(ref->extensionDictionary(),AcDb::kForRead);require(dict.openStatus());
 AcDbObjectId id;Acad::ErrorStatus s=dict->getAt(recordKey(),id);if(s==Acad::eKeyNotFound)return result;require(s);
 AcDbObjectPointer<AcDbXrecord> rec(id,AcDb::kForRead);require(rec.openStatus());resbuf* data=NULL;require(rec->rbChain(&data));
 int i=0;for(resbuf* r=data;r;r=r->rbnext,++i)if(r->restype==1&&r->resval.rstring){if(i==1)result.guid=r->resval.rstring;if(i==2)result.label=r->resval.rstring;}
 if(data)acutRelRb(data);if(result.guid.empty()||result.label.empty())throw std::runtime_error("Malformed detail identity");return result;
}
struct Inventory {std::vector<std::string> labels;bool duplicated;Inventory():duplicated(false){}};
static Inventory inventory(AcDbDatabase* db){
 Inventory out;std::set<std::wstring> guids,labels;
 AcDbBlockTable* raw=NULL;require(db->getBlockTable(raw,AcDb::kForRead));AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
 AcDbBlockTableIterator* iter=NULL;require(table->newIterator(iter));std::unique_ptr<AcDbBlockTableIterator> it(iter);
 for(;!it->done();it->step()){
  AcDbBlockTableRecord* block=NULL;require(it->getRecord(block,AcDb::kForRead));AcDbObjectPointer<AcDbBlockTableRecord> b;b.acquire(block);
  if(b->isFromExternalReference())continue;
  AcDbBlockTableRecordIterator* entities=NULL;require(b->newIterator(entities));std::unique_ptr<AcDbBlockTableRecordIterator> e(entities);
  for(;!e->done();e->step()){
   AcDbEntity* entity=NULL;require(e->getEntity(entity,AcDb::kForRead));AcDbObjectPointer<AcDbEntity> ent;ent.acquire(entity);
   AcDbBlockReference* ref=AcDbBlockReference::cast(entity);if(!ref)continue;Identity value=identity(ref);if(value.guid.empty())continue;
   std::string label(value.label.begin(),value.label.end());std::wstring key=widen(labelKey(label));
   if(!guids.insert(value.guid).second||!labels.insert(key).second)out.duplicated=true;
   out.labels.push_back(label);
  }
 }
 return out;
}
struct Transaction {
 AcTransactionManager* manager;bool active;
 Transaction():manager(actrTransactionManager),active(false){if(!manager||!manager->startTransaction())throw std::runtime_error("Cannot start transaction");active=true;}
 ~Transaction(){if(active)manager->abortTransaction();}
 void commit(){require(manager->endTransaction());active=false;}
};
// Newly database-resident objects are closed immediately; the enclosing transaction still owns rollback.
template<class T> static AcDbObjectId append(AcDbBlockTableRecord* owner,std::unique_ptr<T> item){
 AcDbObjectId id;require(owner->appendAcDbEntity(id,item.get()));T* p=item.release();require(p->close());return id;
}
static AcGePoint3d point(Vertex p,double factor){return AcGePoint3d(p.s/factor,p.z/factor,0);}
static std::unique_ptr<AcDbPolyline> polyline(const DetailPath& path,double factor){
 std::unique_ptr<AcDbPolyline> p(new AcDbPolyline());p->setDatabaseDefaults();
 for(size_t i=0;i<path.points.size();++i)require(p->addVertexAt(static_cast<unsigned>(i),AcGePoint2d(path.points[i].s/factor,path.points[i].z/factor),path.bulges[i]));
 p->setClosed(path.closed);require(p->setColorIndex(path.member==7?1:path.axis?2:path.insulation?8:path.member==9?4:3));
 return p;
}
static std::unique_ptr<AcDbMText> note(const DetailNote& n,const SideDetail& d){
 std::unique_ptr<AcDbMText> p(new AcDbMText());p->setDatabaseDefaults();require(p->setLocation(point(n.at,d.factor)));
 p->setTextHeight(d.textHeight/d.factor);require(p->setWidth(0));p->setContents(widen(n.text).c_str());return p;
}
class PositionPreview {
 AcGiTransientManager* manager;AcDbIntArray viewports;std::vector<AcDbEntity*> entities;
 void add(std::unique_ptr<AcDbEntity> p,const AcGeMatrix3d& transform){require(p->transformBy(transform));
  if(!manager->addTransient(p.get(),kAcGiDirectShortTerm,129,viewports))throw std::runtime_error("Detail positioning preview failed");entities.push_back(p.release());}
public:
 PositionPreview():manager(acgiGetTransientManager()){}
 ~PositionPreview(){for(auto p:entities){manager->eraseTransient(p,viewports);delete p;}}
 void show(const SideDetail& d,const AcGePoint3d& position){
  if(!manager)throw std::runtime_error("No transient manager");
  entities.reserve(d.paths.size()+d.notes.size()+2*d.dimensions.size());
  AcGeMatrix3d transform=AcGeMatrix3d::translation(position.asVector());
  for(const auto& p:d.paths)add(polyline(p,d.factor),transform);
  for(const auto& n:d.notes)add(note(n,d),transform);
  // Positioning preview only: rails represent the future native dimensions, no DWG writes.
  for(const auto& q:d.dimensions){DetailPath p;p.member=9;p.insulation=false;p.axis=false;p.closed=false;
   Vertex a(q.a.s,q.rail.z),b(q.b.s,q.rail.z);if(q.a.s==q.b.s){a=Vertex(q.rail.s,q.a.z);b=Vertex(q.rail.s,q.b.z);}
   if(q.a.z!=q.b.z&&q.a.s!=q.b.s){double dx=q.b.s-q.a.s,dy=q.b.z-q.a.z,len=std::hypot(dx,dy);double t=((q.rail.s-q.a.s)*(-dy)+(q.rail.z-q.a.z)*dx)/(len*len);a=Vertex(q.a.s-dy*t,q.a.z+dx*t);b=Vertex(q.b.s-dy*t,q.b.z+dx*t);}
   p.points={q.a,a,b,q.b};p.bulges={0,0,0,0};add(polyline(p,d.factor),transform);
   add(note(DetailNote{q.rail,q.prefix+displayNumber(std::hypot(q.b.s-q.a.s,q.b.z-q.a.z))+" mm"},d),transform);
  }
 }
};
static void record(AcDbBlockReference* ref,const std::vector<std::wstring>& strings){
 require(ref->createExtensionDictionary());AcDbObjectPointer<AcDbDictionary> dict(ref->extensionDictionary(),AcDb::kForWrite);require(dict.openStatus());
 resbuf* head=NULL;resbuf* tail=NULL;
 try{for(const auto& value:strings){resbuf* r=acutNewRb(1);if(!r)throw std::bad_alloc();r->rbnext=NULL;
   if(!head)head=r;else tail->rbnext=r;tail=r;acutNewString(value.c_str(),r->resval.rstring);if(!r->resval.rstring)throw std::bad_alloc();}
  std::unique_ptr<AcDbXrecord> rec(new AcDbXrecord());require(rec->setFromRbChain(*head));AcDbObjectId id;require(dict->setAt(recordKey(),rec.get(),id));AcDbXrecord* stored=rec.release();require(stored->close());
 }catch(...){if(head)acutRelRb(head);throw;}if(head)acutRelRb(head);
}
static std::wstring scalar(const wchar_t* key,double value){std::wostringstream out;out.imbue(std::locale::classic());out<<key<<L"="<<std::setprecision(17)<<value;return out.str();}
// DEV-only fault injection: process environment TVS_SIDE_DETAIL_FAIL=1/2/3/4; clear after tests.
static void failpoint(int stage){wchar_t value[32]=L"";if(GetEnvironmentVariableW(L"TVS_SIDE_DETAIL_FAIL",value,32)&&_wtoi(value)==stage)throw std::runtime_error("Injected detail write failure");}
static void write(AcDbDatabase* db,const SideDetail& d,const View& view,const Options& options,const std::string& label,bool detailed,const AcGePoint3d& position,const std::wstring& guid,const std::wstring& routeHandle,const std::wstring& obstacleHandle){
 Inventory current=inventory(db);if(current.duplicated)throw std::runtime_error("Duplicate copied detail identity");
 for(const auto& used:current.labels)if(labelKey(used)==labelKey(label))throw std::runtime_error("Duplicate detail label");
 Transaction transaction;
 std::wstring token=guid;for(wchar_t& c:token)if(c==L'{'||c==L'}'||c==L'-')c=L'_';
 const std::wstring name=L"TVS_SIDE_"+token;
 AcDbObjectId layerId,textId,styleId,blockId;
 {
  AcDbLayerTable* raw=NULL;require(db->getLayerTable(raw,AcDb::kForWrite));AcDbObjectPointer<AcDbLayerTable> table;table.acquire(raw);
  std::unique_ptr<AcDbLayerTableRecord> layer(new AcDbLayerTableRecord());require(layer->setName(name.c_str()));require(table->add(layerId,layer.get()));require(layer.release()->close());
 }
 {
  AcDbTextStyleTable* raw=NULL;require(db->getTextStyleTable(raw,AcDb::kForWrite));AcDbObjectPointer<AcDbTextStyleTable> table;table.acquire(raw);
  std::unique_ptr<AcDbTextStyleTableRecord> style(new AcDbTextStyleTableRecord());require(style->setName(name.c_str()));require(style->setFileName(L"txt.shx"));require(style->setTextSize(0));require(style->setXScale(1));require(table->add(textId,style.get()));require(style.release()->close());
 }
 {
  AcDbDimStyleTable* raw=NULL;require(db->getDimStyleTable(raw,AcDb::kForWrite));AcDbObjectPointer<AcDbDimStyleTable> table;table.acquire(raw);
  std::unique_ptr<AcDbDimStyleTableRecord> style(new AcDbDimStyleTableRecord());require(style->setName(name.c_str()));
  require(style->setDimtxsty(textId));require(style->setDimtxt(d.textHeight/d.factor));require(style->setDimasz(d.arrowSize/d.factor));
  require(style->setDimscale(1));require(style->setDimlfac(d.factor));require(style->setDimlunit(2));require(style->setDimdec(2));
  require(style->setDimgap(d.textHeight*.4/d.factor));require(style->setDimexo(d.textHeight*.4/d.factor));require(style->setDimexe(d.textHeight*.6/d.factor));
  require(table->add(styleId,style.get()));require(style.release()->close());
 }
 failpoint(1);
 {
  AcDbBlockTable* raw=NULL;require(db->getBlockTable(raw,AcDb::kForWrite));AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);
  std::unique_ptr<AcDbBlockTableRecord> block(new AcDbBlockTableRecord());require(block->setName(name.c_str()));require(table->add(blockId,block.get()));require(block.release()->close());
 }
 failpoint(2);
 {
  AcDbObjectPointer<AcDbBlockTableRecord> block(blockId,AcDb::kForWrite);require(block.openStatus());
  for(const auto& path:d.paths){auto p=polyline(path,d.factor);require(p->setLayer(layerId));append(block.object(),std::move(p));}
  for(const auto& n:d.notes){auto p=note(n,d);require(p->setTextStyle(textId));require(p->setLayer(layerId));append(block.object(),std::move(p));}
  for(const auto& q:d.dimensions){std::wstring text=widen(q.prefix)+L"<> mm";
   std::unique_ptr<AcDbAlignedDimension> p(new AcDbAlignedDimension(point(q.a,d.factor),point(q.b,d.factor),point(q.rail,d.factor),text.c_str(),styleId));
   p->setDatabaseDefaults(db);require(p->setDimensionStyle(styleId));require(p->setLayer(layerId));
   require(p->setTextPosition(point(q.rail,d.factor)));p->useSetTextPosition();
   AcDbObjectId id=append(block.object(),std::move(p));AcDbObjectPointer<AcDbAlignedDimension> dimension(id,AcDb::kForWrite);require(dimension.openStatus());require(dimension->recomputeDimBlock(true));
  }
 }
 failpoint(3);
 {
  AcDbBlockTable* raw=NULL;require(db->getBlockTable(raw,AcDb::kForRead));AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);AcDbObjectId model;require(table->getAt(ACDB_MODEL_SPACE,model));
  AcDbObjectPointer<AcDbBlockTableRecord> space(model,AcDb::kForWrite);require(space.openStatus());
  std::unique_ptr<AcDbBlockReference> ref(new AcDbBlockReference(position,blockId));ref->setDatabaseDefaults(db);require(ref->setLayer(layerId));AcDbObjectId id=append(space.object(),std::move(ref));
  AcDbObjectPointer<AcDbBlockReference> inserted(id,AcDb::kForWrite);require(inserted.openStatus());
  std::vector<std::wstring> data={L"TVS_SIDE_DETAIL/1",guid,widen(label),L"STATIC_SNAPSHOT",L"SOURCE=checked-bypass-preview;writer=side-detail-v1",L"COPY=shared-snapshot;no-independent-identity",L"routeHandle="+routeHandle,L"obstacleHandle="+obstacleHandle,L"orientation=local+s/right;physical+Z/up;block=WCS_XY",L"dimensions=nonassociative;display=mm",scalar(L"factorMmPerDwg",d.factor),scalar(L"textMm",d.textHeight),scalar(L"arrowMm",d.arrowSize),scalar(L"detailed",double(detailed))};
  const double values[]={view.frame.width,view.frame.height,view.frame.z0,view.frame.obstacleWidth,view.frame.obstacleHeight,view.frame.obstacleZ,view.frame.ux,view.frame.uy,view.frame.origin.x,view.frame.origin.y,view.frame.origin.z,view.D,view.C,view.W,view.s0,view.zPass,view.gapLo,view.gapHi,options.ri,options.insulation,options.obstacleInsulation,options.clearance,options.reserve,double(options.angle),double(options.above),options.left,options.right,options.floor,options.ceiling,double(options.useFloor),double(options.useCeiling)};
  data.push_back(L"numericOrder=width,height,z0,obsWidth,obsHeight,obsZ,ux,uy,originX,originY,originZ,D,H,W,s0,zPass,gapLo,gapHi,Ri,iso,obsIso,clearance,reserve,angle,above,left,right,floor,ceiling,useFloor,useCeiling");
  for(double value:values)data.push_back(scalar(L"value",value));for(const auto& p:view.axis){data.push_back(scalar(L"axisS",p.s));data.push_back(scalar(L"axisZ",p.z));}
  data.push_back(scalar(L"witnessBodyS",view.witnessBody.s));data.push_back(scalar(L"witnessBodyZ",view.witnessBody.z));data.push_back(scalar(L"witnessObstacleS",view.witnessObstacle.s));data.push_back(scalar(L"witnessObstacleZ",view.witnessObstacle.z));
  record(inserted.object(),data);
 }
 failpoint(4);
 transaction.commit();
}
static std::wstring handle(const AcDbObjectId& id){AcDbHandle h=id.handle();wchar_t s[32];h.getIntoAsciiBuffer(s);return s;}
static void insert(AcDbDatabase* db,const View& v,const Options& o,const AcDbObjectId& route,const AcDbObjectId& obstacle){
 if(!v.ready||!db->tilemode())return;
 Inventory used=inventory(db);if(used.duplicated){acutPrintf(L"\nOtkrivene COPY/duple oznake ili GUID-i detalja. Kopije nisu nezavisni detalji. Uklonite duplikate prije novog umetanja.");return;}
 std::string label=nextDetailLabel(used.labels);ACHAR input[512]=L"";
 std::wstring prompt=L"\nOznaka detalja (A-Z, 0-9, -, _, max32) <"+widen(label)+L">: ";
 int code=acedGetString(0,prompt.c_str(),input);if(code!=RTNORM&&code!=RTNONE)return;if(code==RTNORM&&input[0]){label.clear();for(const wchar_t* c=input;*c;++c){if(*c>127){acutPrintf(L"\nOznaka mora biti ASCII.");return;}label.push_back(char(*c));}}
 if(!detailLabelValid(label)){acutPrintf(L"\nNevazeca oznaka detalja.");return;}for(const auto& s:used.labels)if(labelKey(s)==labelKey(label)){acutPrintf(L"\nOznaka vec postoji.");return;}
 acedInitGet(0,L"Osnovno Detaljno");ACHAR mode[32]=L"";code=acedGetKword(L"\nKotiranje [Osnovno/Detaljno] <Osnovno>: ",mode);if(code!=RTNORM&&code!=RTNONE)return;bool detailed=_wcsicmp(mode,L"Detaljno")==0;
 double text=3.5,arrow=3.5;acedInitGet(RSG_NONULL|RSG_NOZERO|RSG_NONEG,NULL);
 code=acedGetReal(L"\nVisina teksta u fizickim mm (0.5-100): ",&text);if(code!=RTNORM)return;
 acedInitGet(RSG_NONULL|RSG_NOZERO|RSG_NONEG,NULL);code=acedGetReal(L"\nVelicina strelice u fizickim mm (0.5-100): ",&arrow);if(code!=RTNORM)return;
 SideDetail d=sideDetail(v,o,label,detailed,text,arrow);
 GUID rawGuid; if(FAILED(CoCreateGuid(&rawGuid)))throw std::runtime_error("GUID failure");wchar_t guid[40];if(!StringFromGUID2(rawGuid,guid,40))throw std::runtime_error("GUID format failure");
 for(;;){ads_point ucs,wcs;if(acedGetPoint(NULL,L"\nTacka umetanja bocnog detalja (WCS XY prikaz): ",ucs)!=RTNORM)return;
  resbuf from={};from.restype=RTSHORT;from.resval.rint=1;resbuf to={};to.restype=RTSHORT;to.resval.rint=0;if(acedTrans(ucs,&from,&to,0,wcs)!=RTNORM)throw std::runtime_error("UCS conversion failed");
  AcGePoint3d position(wcs[0],wcs[1],wcs[2]);int confirm;
  {PositionPreview preview;preview.show(d,position);acedInitGet(0,L"Umetni Pozicija Otkazi");ACHAR answer[32]=L"";
   confirm=acedGetKword(L"\nPreview detalja: [Umetni/Pozicija/Otkazi] <Otkazi>: ",answer);
   if(confirm!=RTNORM)return;if(_wcsicmp(answer,L"Pozicija")==0)continue;if(_wcsicmp(answer,L"Umetni")!=0)return;}
  write(db,d,v,o,label,detailed,position,guid,handle(route),handle(obstacle));
  acutPrintf(L"\nUmetnut staticki detalj %ls. Standardne neasocijativne kote; bez trajnog TVS obilaska. COPY zadrzava identitet snimka.",widen(label).c_str());return;
 }
}
}


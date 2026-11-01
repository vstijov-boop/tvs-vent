#pragma once
// DEV writer fixture. It refuses non-empty Model space; never invoke on a user's design drawing.
namespace tvsSideDetail {
static void assertTest(bool value){if(!value)throw std::runtime_error("Native detail fixture assertion failed");}
static std::wstring newGuid(){GUID value;if(FAILED(CoCreateGuid(&value)))throw std::runtime_error("GUID failure");wchar_t s[40];assertTest(StringFromGUID2(value,s,40)!=0);return s;}
static bool fixtureEmpty(AcDbDatabase* db){
 AcDbBlockTable* raw=NULL;require(db->getBlockTable(raw,AcDb::kForRead));AcDbObjectPointer<AcDbBlockTable> table;table.acquire(raw);AcDbObjectId model;require(table->getAt(ACDB_MODEL_SPACE,model));
 AcDbObjectPointer<AcDbBlockTableRecord> space(model,AcDb::kForRead);require(space.openStatus());AcDbBlockTableRecordIterator* rawIterator=NULL;require(space->newIterator(rawIterator));std::unique_ptr<AcDbBlockTableRecordIterator> it(rawIterator);return it->done();
}
static bool noFixtureSymbols(AcDbDatabase* db,const std::wstring& guid){
 std::wstring token=guid;for(wchar_t& c:token)if(c==L'{'||c==L'}'||c==L'-')c=L'_';std::wstring name=L"TVS_SIDE_"+token;
 AcDbBlockTable* b=NULL;require(db->getBlockTable(b,AcDb::kForRead));bool absent=!b->has(name.c_str());b->close();
 AcDbLayerTable* l=NULL;require(db->getLayerTable(l,AcDb::kForRead));absent=absent&&!l->has(name.c_str());l->close();
 AcDbTextStyleTable* t=NULL;require(db->getTextStyleTable(t,AcDb::kForRead));absent=absent&&!t->has(name.c_str());t->close();
 AcDbDimStyleTable* d=NULL;require(db->getDimStyleTable(d,AcDb::kForRead));absent=absent&&!d->has(name.c_str());d->close();return absent;
}
static void verifyNative(AcDbObjectId id,const SideDetail& d,const std::string& label){
 AcDbObjectPointer<AcDbBlockReference> ref(id,AcDb::kForRead);require(ref.openStatus());Identity ident=identity(ref.object());assertTest(ident.label==widen(label)&&!ident.guid.empty());
 AcDbObjectPointer<AcDbBlockTableRecord> block(ref->blockTableRecord(),AcDb::kForRead);require(block.openStatus());
 AcDbBlockTableRecordIterator* raw=NULL;require(block->newIterator(raw));std::unique_ptr<AcDbBlockTableRecordIterator> it(raw);size_t count=0;
 for(;!it->done();it->step()){
  AcDbEntity* entity=NULL;require(it->getEntity(entity,AcDb::kForRead));AcDbObjectPointer<AcDbEntity> ent;ent.acquire(entity);
  AcDbAlignedDimension* dimension=AcDbAlignedDimension::cast(ent.object());if(!dimension)continue;
  assertTest(count<d.dimensions.size());double measured=0;require(dimension->measurement(measured));
  const auto& q=d.dimensions[count];double expected=std::hypot(q.a.s-q.b.s,q.a.z-q.b.z);
  assertTest(std::fabs(measured*d.factor-expected)<1e-6);assertTest(std::fabs(dimension->dimlfac()-d.factor)<1e-12);assertTest(!dimension->dimBlockId().isNull());++count;
 }
 assertTest(count==d.dimensions.size());
}
static void nativeFixture(){
 AcDbDatabase* db=acdbHostApplicationServices()->workingDatabase();if(!db||!db->tilemode()||!fixtureEmpty(db)){
  acutPrintf(L"\nFIXTURE REFUSED: use a NEW EMPTY disposable Model drawing.");return;}
 // These are synthetic checked fixtures, not a test of interactive selection of real TVS objects.
 Engine engine;unsigned count=0;AcDbObjectId first;SideDetail firstDetail;View firstView;Options firstOptions;
 for(double factor:{1.,10.,1000.})for(bool above:{false,true})for(int angle:{30,45,60})for(double iso:{0.,25.})for(bool detailed:{false,true}){
  Snapshot s;Segment* parts[]={&s.route,&s.obstacle};for(auto p:parts){p->rectangular=p->rigid=p->ordinary=true;p->elevation=12000/factor;}
  s.route.first=Point(-1600/factor,0,0);s.route.last=Point(1600/factor,0,0);s.route.a=400/factor;s.route.b=250/factor;
  s.obstacle.first=Point(0,-1200/factor,0);s.obstacle.last=Point(0,1200/factor,0);s.obstacle.a=s.obstacle.b=250/factor;
  Options o;o.confirmed=true;o.above=above;o.angle=angle;o.insulation=o.obstacleInsulation=iso;o.left=-1600;o.right=1600;
  View v=engine.calculate(s,factor,false,o,false);assertTest(v.ready);std::string label="TEST-"+std::to_string(++count);SideDetail d=sideDetail(v,o,label,detailed,3.5,3.5);
  AcDbObjectId id=write(db,d,v,o,label,detailed,AcGePoint3d(count*5000.0/factor,0,0),newGuid(),L"SYNTHETIC",L"SYNTHETIC");verifyNative(id,d,label);
  if(count==1){first=id;firstDetail=d;firstView=v;firstOptions=o;}
 }
 assertTest(inventory(db).labels.size()==count);
 for(int stage=1;stage<=4;++stage){std::wstring guid=newGuid();bool failed=false;injectedStage=stage;
  try{write(db,firstDetail,firstView,firstOptions,"ROLLBACK",false,AcGePoint3d::kOrigin,guid,L"SYNTHETIC",L"SYNTHETIC");}
  catch(...){failed=true;}injectedStage=0;assertTest(failed);assertTest(inventory(db).labels.size()==count);assertTest(noFixtureSymbols(db,guid));
 }
 bool duplicate=false;try{write(db,firstDetail,firstView,firstOptions,"test-1",false,AcGePoint3d::kOrigin,newGuid(),L"SYNTHETIC",L"SYNTHETIC");}catch(...){duplicate=true;}assertTest(duplicate);verifyNative(first,firstDetail,"TEST-1");
 acutPrintf(L"\nNATIVE_DETAIL_FIXTURE_PASS: %u checked fixtures, native dimension measurements/blocks/DIMLFAC, metadata, 4 rollback stages, duplicate label. Save/reopen/Undo/COPY/visual tests remain separate.",count);
}
}


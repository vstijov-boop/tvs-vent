"""Execute the real parent XRecord writer with SDK dictionary lifetime semantics.
These substitutes do not establish native AutoCAD filing/transaction success.
"""
import pathlib
import subprocess
import tempfile
import unittest
from test_offset_service import function
from bypass_compiler import compile_cpp, executable

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ParentWrite(unittest.TestCase):
    def test_reuse_existing_v1_extension_dictionaries(self):
        source = (ROOT/'TVS_Ventilation_DBX/BypassAssemblyData.cpp').read_text(encoding='utf-8-sig')
        header = (ROOT/'TVS_Ventilation_DBX/BypassAssemblyData.h').read_text(encoding='utf-8-sig')
        code = r'''
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
typedef wchar_t ACHAR;
typedef long ads_name[2];
namespace Acad {enum ErrorStatus {eOk,eInvalidInput,eAlreadyInDb,eFail};}
namespace AcDb {enum OpenMode {kForWrite};}
struct CString {
 std::wstring text;CString(){}CString(const wchar_t* t):text(t){}
 bool IsEmpty()const{return text.empty();}const wchar_t* GetString()const{return text.c_str();}
 operator const wchar_t*()const{return GetString();}
 friend CString operator+(const wchar_t* a,const CString& b){CString r;r.text=a+b.text;return r;}
};
struct AcDbObjectId {int id;AcDbObjectId(int v=0):id(v){}bool isNull()const{return id==0;}
 bool operator==(const AcDbObjectId& b)const{return id==b.id;} };
struct resbuf {short restype;resbuf* rbnext;struct Value {
 long rlong;short rint;double rreal;ads_name rlname;wchar_t* rstring;
 } resval;};
inline Acad::ErrorStatus acdbGetAdsName(ads_name out,AcDbObjectId id){
 if(id.isNull())return Acad::eInvalidInput;
 out[0]=17;out[1]=id.id;return Acad::eOk;
}
struct AcDbObject {
 AcDbObjectId id,ext;int creates;bool failCreate;
 AcDbObject():creates(0),failCreate(false){}virtual ~AcDbObject(){}
 AcDbObjectId extensionDictionary()const{return ext;}
 Acad::ErrorStatus createExtensionDictionary();
};
static std::map<int,AcDbObject*> objects;
static std::vector<std::unique_ptr<AcDbObject> > storage;
static int nextId=100;
static AcDbObjectId keep(AcDbObject* o,int id=0){
 if(!id)id=nextId++;
 o->id=id;objects[id]=o;storage.emplace_back(o);return o->id;
}
struct AcDbDictionary:AcDbObject {
 std::map<std::wstring,AcDbObjectId> entries;
 static AcDbDictionary* cast(AcDbObject* o){return dynamic_cast<AcDbDictionary*>(o);}
 bool has(const wchar_t* key)const{return entries.count(key)!=0;}
 Acad::ErrorStatus setAt(const wchar_t* key,AcDbObject* value,AcDbObjectId& resultId){
  if(has(key))return Acad::eInvalidInput;
  resultId=keep(value);entries[key]=resultId;return Acad::eOk;
 }
};
Acad::ErrorStatus AcDbObject::createExtensionDictionary(){
 ++creates;
 // Autodesk SDK contract: a pre-existing dictionary is eAlreadyInDb, not eOk.
 if(!ext.isNull())return Acad::eAlreadyInDb;
 if(failCreate)return Acad::eFail;
 ext=keep(new AcDbDictionary());return Acad::eOk;
}
struct Field {int type,id,integer;double value;std::wstring text;};
static bool failChain=false;
struct AcDbXrecord:AcDbObject {
 bool translated;std::vector<Field> fields;AcDbXrecord():translated(false){}
 void setXlateReferences(bool yes){translated=yes;}
 Acad::ErrorStatus setFromRbChain(const resbuf& head){
  if(failChain)return Acad::eFail;
  for(const resbuf* p=&head;p;p=p->rbnext){Field f={};f.type=p->restype;
   if(f.type==330)f.id=static_cast<int>(p->resval.rlname[1]);
   if(f.type==90)f.integer=static_cast<int>(p->resval.rlong);
   if(f.type==70)f.integer=p->resval.rint;
   if(f.type==40)f.value=p->resval.rreal;
   if(f.type==1)f.text=p->resval.rstring;
   fields.push_back(f);
  }
  return Acad::eOk;
 }
};
struct AcDbTransactionManager {
 int active,failOpen,adds,failAdd;
 AcDbTransactionManager():active(1),failOpen(0),adds(0),failAdd(0){}
 int numActiveTransactions()const{return active;}
 Acad::ErrorStatus getObject(AcDbObject*& out,AcDbObjectId id,AcDb::OpenMode mode){
  assert(mode==AcDb::kForWrite);out=NULL;
  if(id.id==failOpen)return Acad::eFail;
  auto it=objects.find(id.id);if(it==objects.end())return Acad::eInvalidInput;
  out=it->second;return Acad::eOk;
 }
 Acad::ErrorStatus addNewlyCreatedDBRObject(AcDbObject*,bool yes){
  assert(yes);return ++adds==failAdd?Acad::eFail:Acad::eOk;
 }
};
static AcDbTransactionManager manager;
struct AcDbDatabase {
 AcDbTransactionManager* transactionManager(){return &manager;}
 AcDbObjectId namedObjectsDictionaryId(){return 1;}
};
namespace tvsOffsetData {inline CString newGuid(){return CString(L"{new-test-guid}");}}
namespace tvsBypassData {
'''
        code += function(header, 'struct Record') + ';\n'
        code += function(source, 'static const wchar_t* memberKey(') + '\n'
        code += function(source, 'static int typeAt(') + '\n'
        code += function(source, 'static Acad::ErrorStatus newRecord(') + '\n'
        code += function(source, 'Acad::ErrorStatus write(') + '\n}\n'
        code += r'''
static AcDbDatabase db;
static void setup(tvsBypassData::Record& r,AcDbObject* members[7],bool hDictionary=false){
 objects.clear();storage.clear();nextId=100;manager=AcDbTransactionManager();failChain=false;
 keep(new AcDbDictionary(),1);r=tvsBypassData::Record();r.factor=1;r.inlet=50;r.outlet=51;r.obstacle=52;
 for(int j=0;j<7;++j){
  members[j]=new AcDbObject();r.members[j]=keep(members[j],10+j);
  if(j!=3||hDictionary){
   members[j]->ext=keep(new AcDbDictionary());
   auto d=AcDbDictionary::cast(objects[members[j]->ext.id]);
   // Model the existing V1 record; it must survive unchanged.
   d->entries[j==3?L"USER_RECORD":L"TVS_OFFSET_MEMBER"]=200+j;
  }
 }
}
static void originalsIntact(AcDbObject* members[7],bool hDictionary=false){
 for(int j=0;j<7;++j){if(j==3&&!hDictionary)continue;
  auto d=AcDbDictionary::cast(objects[members[j]->ext.id]);
  assert(d->entries.at(j==3?L"USER_RECORD":L"TVS_OFFSET_MEMBER")==200+j);
 }
}
int main(){
 int cases=0;
 for(bool existingH:{false,true}){
  tvsBypassData::Record r;AcDbObject* members[7];setup(r,members,existingH);AcDbObjectId parent;
  auto es=tvsBypassData::write(&db,r,members,parent);
  if(es!=Acad::eOk){std::cerr<<"existing V1 dictionaries: expected eOk, actual="<<es<<"\n";return 1;}
  auto record=dynamic_cast<AcDbXrecord*>(objects[parent.id]);
  assert(record&&record->translated&&record->fields.size()==27);
  assert(record->fields[4].id==10&&record->fields[5].id==13&&record->fields[6].id==14);
  assert(record->fields[7].id==50&&record->fields[8].id==51&&record->fields[9].id==52);
  assert(record->fields[23].id==0&&record->fields[24].id==0);
  for(int j=0;j<7;++j){
   assert(members[j]->creates==(!existingH&&j==3?1:0));
   auto d=AcDbDictionary::cast(objects[members[j]->ext.id]);
   auto link=dynamic_cast<AcDbXrecord*>(objects[d->entries.at(L"TVS_BYPASS_PARENT").id]);
   assert(link&&link->translated&&link->fields.size()==3&&link->fields[1].id==parent.id&&link->fields[2].integer==j);
  }
  originalsIntact(members,existingH);assert(manager.adds==8);++cases;
 }
 for(int fail=0;fail<6;++fail){
  tvsBypassData::Record r;AcDbObject* members[7];setup(r,members);AcDbObjectId parent;
  if(fail==0)members[3]->failCreate=true;
  if(fail==1)manager.failOpen=1;
  if(fail==2)failChain=true;
  if(fail==3)manager.failAdd=1;
  if(fail==4)manager.active=0;
  if(fail==5){auto d=AcDbDictionary::cast(objects[members[0]->ext.id]);d->entries[L"TVS_BYPASS_PARENT"]=777;}
  assert(tvsBypassData::write(&db,r,members,parent)!=Acad::eOk);
  originalsIntact(members);
  if(fail==5){auto d=AcDbDictionary::cast(objects[members[0]->ext.id]);assert(d->entries.at(L"TVS_BYPASS_PARENT")==777);}
  ++cases;
 }
 std::cout<<"PASS PARENT WRITE "<<cases<<" real writer cases: reuse 6 V1 dictionaries, optional existing H dictionary, retain records, parent+7 backlinks, propagate failures; SDK substitutes, not native filing/rollback\n";
}
'''
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)/'parent-write.cpp'
            path.write_text(code, encoding='utf-8')
            target = executable(pathlib.Path(directory)/'parent-write')
            compile_cpp(path, target)
            subprocess.run([str(target)], check=True)


if __name__ == '__main__':
    unittest.main()

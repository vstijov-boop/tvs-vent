"""Actual pinned/read-only reference reader with SDK storage substitutes, not AutoCAD runtime."""
import pathlib
import subprocess
import tempfile
import unittest
from test_offset_service import function
from bypass_compiler import compile_cpp, executable

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ParentScope(unittest.TestCase):
    def test_incomplete_parent_is_scoped_before_blocking(self):
        source = (ROOT/'TVS_Ventilation_DBX/BypassAssemblyData.cpp').read_text(encoding='utf-8-sig')
        header = (ROOT/'TVS_Ventilation_DBX/BypassAssemblyData.h').read_text(encoding='utf-8-sig')
        code = r'''
#include <cassert>
#include <cwchar>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
typedef long ads_name[2];
typedef std::wstring CString;
struct resbuf {short restype;resbuf* rbnext;struct Value {
 long rlong;short rint;double rreal;ads_name rlname;
 Value():rlong(0),rint(0),rreal(0){rlname[0]=rlname[1]=0;}
 } resval;resbuf():restype(0),rbnext(NULL){} };
namespace Acad {enum ErrorStatus {eOk,eInvalid};}
namespace AcDb {enum OpenMode {kForRead};}
struct AcDbDatabase;
struct AcDbObjectId {
 int value;AcDbObjectId(int v=0):value(v){}
 bool isNull()const{return value==0;}
 AcDbDatabase* database()const;
 bool operator==(const AcDbObjectId& o)const{return value==o.value;}
};
struct AcDbDatabase {AcDbObjectId namedObjectsDictionaryId(){return 999;}};
static AcDbDatabase db;
AcDbDatabase* AcDbObjectId::database()const{return value?&db:NULL;}
inline Acad::ErrorStatus acdbGetAdsName(ads_name out,AcDbObjectId id){
 out[0]=id.isNull()?0:17;out[1]=id.value;return Acad::eOk;
}
static std::vector<std::wstring> diagnostics;
template<class... A> void acutPrintf(const wchar_t* message,A...){diagnostics.push_back(message);}
#define _wcsnicmp scopeWcsCompare
inline int scopeWcsCompare(const wchar_t* a,const wchar_t* b,size_t n){return std::wcsncmp(a,b,n);}
static int releases=0,opens=0;
inline void acutRelRb(resbuf*){++releases;}
namespace tvsOffsetData {
enum Status {Legacy,Valid,Incomplete,Corrupt,UnsupportedVersion,GeometryMismatch};
inline const wchar_t* statusText(Status){return L"test-status";}
}
namespace tvsBypassData {
using namespace tvsOffsetData;
'''
        code += function(header, 'struct Record') + ';\n}\n'
        code += r'''
struct Fixture {
 std::wstring key;int id;tvsOffsetData::Status status;tvsBypassData::Record record;
 std::vector<resbuf> data;bool openFail,chainFail;
 Fixture():id(1),status(tvsOffsetData::Valid),data(27),openFail(false),chainFail(false){}
};
static std::map<int,Fixture> fixtures;
class AcDbDictionaryIterator {
 std::map<int,Fixture>::iterator it;
public:
 AcDbDictionaryIterator():it(fixtures.begin()){}
 bool done()const{return it==fixtures.end();}
 void next(){++it;}
 const wchar_t* name()const{return it->second.key.c_str();}
 AcDbObjectId objectId()const{return it->second.id;}
};
class AcDbDictionary {public:AcDbDictionaryIterator* newIterator(){return new AcDbDictionaryIterator();}};
class AcDbXrecord {
 AcDbObjectId id;
public:
 AcDbXrecord(AcDbObjectId v=0):id(v){}
 Acad::ErrorStatus rbChain(resbuf** out){
  Fixture& f=fixtures.at(id.value);if(f.chainFail){*out=NULL;return Acad::eInvalid;}
  for(size_t j=0;j<f.data.size();++j)f.data[j].rbnext=j+1<f.data.size()?&f.data[j+1]:NULL;
  *out=f.data.empty()?NULL:&f.data[0];return Acad::eOk;
 }
};
template<class T> class AcDbObjectPointer {
 T object;Acad::ErrorStatus status;
public:
 AcDbObjectPointer(AcDbObjectId id,AcDb::OpenMode mode):object(),status(Acad::eOk){
  assert(mode==AcDb::kForRead);++opens;(void)id;
 }
 Acad::ErrorStatus openStatus()const{return status;}
 T* operator->(){return &object;}
};
template<> class AcDbObjectPointer<AcDbXrecord> {
 AcDbXrecord object;Acad::ErrorStatus status;
public:
 AcDbObjectPointer(AcDbObjectId id,AcDb::OpenMode mode):object(id),status(Acad::eOk){
  assert(mode==AcDb::kForRead);++opens;if(fixtures.at(id.value).openFail)status=Acad::eInvalid;
 }
 Acad::ErrorStatus openStatus()const{return status;}
 AcDbXrecord* operator->(){return &object;}
};
namespace tvsBypassData {
static Status read(const AcDbObjectId& id,Record& out){
 const auto& f=fixtures.at(id.value);out=f.record;return f.status;
}
'''
        code += function(source, 'static int typeAt(') + '\n'
        code += function(source, 'static bool boundaryPinned(') + '\n'
        # Before the fix, use the unmodified production pinned() to prove the regression fails.
        if 'enum ReferenceRelation' in source:
            code += function(source, 'enum ReferenceRelation') + ';\n'
            code += function(source, 'static ReferenceRelation referenceRelation(') + '\n'
            code += function(source, 'static ReferenceRelation parentRelation(') + '\n'
        code += function(source, 'bool pinned(') + '\n}\n'
        code += r'''
static Fixture make(tvsOffsetData::Status status){
 Fixture f;f.key=L"TVS_BYPASS_fixture";f.status=status;f.record.inlet=10;f.record.outlet=11;
 f.record.members[0]=20;f.record.members[3]=23;f.record.members[4]=24;
 for(int j=0;j<27;++j)f.data[j].restype=static_cast<short>(tvsBypassData::typeAt(j));
 f.data[0].resval.rlong=1;
 for(int j=4;j<=9;++j){const int ids[]={20,23,24,10,11,30};acdbGetAdsName(f.data[j].resval.rlname,ids[j-4]);}
 return f;
}
static int cases=0;
static void expect(int source,bool first,bool blocked){
 const bool actual=tvsBypassData::pinned(source,first);++cases;
 if(actual!=blocked){std::cerr<<"case "<<cases<<" source="<<source<<" first="<<first<<" expected="<<blocked<<" actual="<<actual<<"\n";std::abort();}
}
int main(){
 using namespace tvsOffsetData;
 fixtures[1]=make(Valid);
 expect(90,false,false);expect(10,false,true);expect(11,true,true);expect(11,false,false);
 for(Status status:{Incomplete,Corrupt}){
  // Deleted anchor / unreadable V1 membership, but the parent's complete references remain readable.
  fixtures[1]=make(status);diagnostics.clear();
  expect(90,false,false);expect(90,true,false);
  expect(10,false,true);expect(11,true,true);
  expect(10,true,true);expect(11,false,true); // related invalid data: refuse either end
  expect(20,false,true); // positively referenced member: conservatively refused
  assert(!diagnostics.empty());
  acdbGetAdsName(fixtures[1].data[23].resval.rlname,80);
  fixtures[1].data[25].resval.rint=1;
  expect(80,true,true);expect(80,false,true);expect(90,false,false);
 }
 // Whole bypass deleted, all seven members erased: retained soft references must not block another pipe.
 fixtures[1]=make(Incomplete);expect(90,false,false);expect(90,true,false);
 // Purged references become null; an empty orphan is still unrelated, without deleting its record.
 for(int j=4;j<=9;++j)acdbGetAdsName(fixtures[1].data[j].resval.rlname,0);
 expect(90,false,false);assert(fixtures.size()==1&&fixtures[1].data.size()==27);
 // Multiple entries: an orphan first must not hide a later genuinely protected boundary.
 fixtures[2]=make(Valid);fixtures[2].id=2;expect(10,false,true);expect(90,false,false);
 fixtures.erase(2);
 fixtures[1]=make(Corrupt);fixtures[1].data[1].restype=40;
 // Malformed layout: positive reference survives, absence is not proof of independence.
 expect(10,false,true);diagnostics.clear();expect(90,false,true);
 assert(!diagnostics.empty()&&diagnostics.back().find(L"NEUTVRDJENA")!=std::wstring::npos);
 // Extra reference must not be silently ignored, including a reference to the selected source.
 fixtures[1]=make(Corrupt);fixtures[1].data.push_back(resbuf());fixtures[1].data.back().restype=330;
 acdbGetAdsName(fixtures[1].data.back().resval.rlname,90);expect(90,false,true);
 fixtures[1]=make(Corrupt);fixtures[1].openFail=true;diagnostics.clear();expect(90,false,true);
 assert(diagnostics.back().find(L"NEUTVRDJENA")!=std::wstring::npos);
 fixtures[1]=make(Corrupt);fixtures[1].chainFail=true;expect(90,false,true);
 fixtures[1]=make(UnsupportedVersion);fixtures[1].data[0].resval.rlong=2;expect(90,false,true);
 assert(opens>0&&releases>0&&fixtures.size()==1);
 std::cout<<"PASS PARENT SCOPE "<<cases<<" valid/incomplete/orphan/corrupt cases; actual read-only pinned/reference reader, SDK storage substitutes; no DWG writes or AutoCAD runtime\n";
}
'''
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)/'parent-scope.cpp'
            path.write_text(code, encoding='utf-8')
            target = executable(pathlib.Path(directory)/'parent-scope')
            compile_cpp(path, target)
            subprocess.run([str(target)], check=True)


if __name__ == '__main__':
    unittest.main()

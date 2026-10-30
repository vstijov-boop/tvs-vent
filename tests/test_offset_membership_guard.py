"""Run the actual membership-validation prefix against DB/API stubs.
Does not prove ObjectARX serialization, transaction, or clone runtime behavior.
"""
import pathlib
import subprocess
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class OffsetMembershipGuard(unittest.TestCase):
    def test_actual_reciprocal_membership_validation(self):
        code=(ROOT/'TVS_Ventilation_DBX/OffsetAssemblyData.cpp').read_text(encoding='utf-8-sig')
        start=code.index('Status validate(')
        end=code.index('    if(!geometry)return Valid;',start)+len('    if(!geometry)return Valid;')
        body=code[start:end]+'\n    return Valid;\n}\n'
        # Whitespace-only formatting of the extracted dense source for GCC -Werror.
        body=body.replace('if(q==Legacy)return Corrupt;if(q!=Valid)return q;',
                          'if(q==Legacy)return Corrupt;\n        if(q!=Valid)return q;')
        stub=r'''
#include <map>
#include <string>
#include <cassert>
#include <iostream>
namespace Acad {enum ErrorStatus {eOk,eNotThatKindOfClass};}
namespace AcDb {enum OpenMode {kForRead};}
struct AcDbDatabase {};
struct AcDbObjectId {
 int id;AcDbDatabase* db;bool erased;
 AcDbObjectId(int i=0,AcDbDatabase* d=0,bool e=false):id(i),db(d),erased(e){}
 bool isNull()const{return id==0;}bool isErased()const{return erased;}
 AcDbDatabase* database()const{return db;}
 bool operator==(const AcDbObjectId& b)const{return id==b.id&&db==b.db;}
 bool operator!=(const AcDbObjectId& b)const{return !(*this==b);}
};
struct AcDbObject {AcDbObjectId id;AcDbObject(int i,AcDbDatabase* d):id(i,d){}virtual ~AcDbObject(){}AcDbDatabase* database()const{return id.db;}};
struct TVS_TAP:AcDbObject {TVS_TAP(int i,AcDbDatabase* d):AcDbObject(i,d){}};
struct TVS_Pipe:AcDbObject {TVS_Pipe(int i,AcDbDatabase* d):AcDbObject(i,d){}};
std::map<int,AcDbObject*> objectsById;
template<class T> struct AcDbObjectPointer {
 T* p;AcDbObjectPointer(AcDbObjectId id,AcDb::OpenMode):p(dynamic_cast<T*>(objectsById[id.id])){}
 Acad::ErrorStatus openStatus()const{return p?Acad::eOk:Acad::eNotThatKindOfClass;}
 T* object()const{return p;}
};
enum Status {Legacy,Valid,Incomplete,Corrupt,UnsupportedVersion,GeometryMismatch};
struct Record {int role;std::string guid;AcDbObjectId members[3];Record():role(-1){}};
std::map<int,Record> records;std::map<int,Status> statuses;
Status read(const AcDbObject* o,Record& r){r=records[o->id.id];return statuses[o->id.id];}
void setup(AcDbObject* a,AcDbObject* p,AcDbObject* b){
 objectsById.clear();records.clear();statuses.clear();AcDbObject* members[]={a,p,b};
 for(int j=0;j<3;++j){objectsById[members[j]->id.id]=members[j];Record r;r.role=j;r.guid="original";
  for(int k=0;k<3;++k)r.members[k]=members[k]->id;
  records[members[j]->id.id]=r;statuses[members[j]->id.id]=Valid;}
}
'''
        cases=r'''
int main(){
 AcDbDatabase db,foreign;TVS_TAP a(1,&db),b(3,&db);TVS_Pipe p(2,&db);AcDbObject wrong(2,&db);
 setup(&a,&p,&b);assert(validate(&a,false)==Valid);assert(validate(&p,false)==Valid);assert(validate(&b,false)==Valid);
 statuses[2]=Legacy;assert(validate(&a,false)==Corrupt);assert(validate(&b,false)==Corrupt);
 statuses[2]=UnsupportedVersion;assert(validate(&a,false)==UnsupportedVersion);
 setup(&a,&p,&b);records[1].members[2]=AcDbObjectId();assert(validate(&a,false)==Incomplete);
 setup(&a,&p,&b);records[1].members[2].erased=true;assert(validate(&a,false)==Incomplete);
 setup(&a,&p,&b);records[1].members[2].db=&foreign;assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);records[1].members[2]=records[1].members[0];assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);objectsById[2]=&wrong;assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);objectsById[2]=0;assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);records[2].guid="copied";assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);records[2].role=0;assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);records[2].members[0]=b.id;assert(validate(&a,false)==Corrupt);
 setup(&a,&p,&b);statuses[1]=Legacy;assert(validate(&a,false)==Legacy);
 std::cout<<"PASS actual membership prefix: complete reciprocal tuple, null/erased Incomplete, missing record/wrong type/foreign DB/duplicate ID/broken GUID-role-reference Corrupt; SDK stubs only\n";
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp=pathlib.Path(directory)/'membership.cpp';binary=pathlib.Path(directory)/'membership'
            cpp.write_text(stub+body+cases)
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-pedantic',str(cpp),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
if __name__=='__main__':unittest.main(verbosity=2)

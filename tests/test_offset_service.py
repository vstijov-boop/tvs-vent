"""Execute actual service topology/current/write and drawing adapters with SDK substitutes.
No ObjectARX filing, real transactions, MFC, UCS or Undo/Redo runtime claim.
"""
import pathlib
import subprocess
import tempfile
import unittest
from bypass_compiler import compile_cpp, executable
ROOT = pathlib.Path(__file__).resolve().parents[1]

def function(text, signature):
    start = text.index(signature)
    opening = text.index('{', start)
    level = 1
    end = opening + 1
    while level:
        level += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

class OffsetServiceTests(unittest.TestCase):
    def test_actual_service_guards_topology_and_transaction(self):
        text = (ROOT/'TVS_Ventilation_ARX/OffsetService.h').read_text(encoding='utf-8-sig')
        extracted = '\n'.join(function(text, s) for s in [
            'inline bool topology(', 'inline bool current(', 'inline Outcome write('])
        # Layout-only formatting for the extracted existing compact C++ statements.
        extracted = extracted.replace(';if(', ';\n    if(')
        main = r'''
using namespace tvsOffsetService;
static void reset(){
 for(auto& o:objects)if(o.second!=&fixtureSource&&o.second!=&block)delete o.second;
 objects.clear();nodes.clear();tvsOffsetData::records.clear();fixtureSource=TVS_Pipe();fixtureSource.id=1;block.id=99;
 objects[1]=&fixtureSource;objects[99]=&block;
 live=Snapshot();live.physical.first=tvsOffset::WorldPoint(-2000,0,37);
 live.physical.last=tvsOffset::WorldPoint(0,0,37);live.physical.a=400;live.physical.b=200;
 live.physical.elevation=-400;live.physical.factor=1;live.fingerprint=L"original";live.network=L"network";
 failAppend=appendCalls=failRecord=recordCalls=commits=aborts=txStarts=0;
 failCommit=badCapture=false;failNext()=false;
}
static TVS_Pipe* neighbor(int end){
 TVS_Pipe* n=new TVS_Pipe();n->id=2;
 AcGePoint3d q=end==0?fixtureSource.FirstPoint:fixtureSource.LastPoint;
 n->FirstPoint=AcGePoint3d(q.x+(end==0?-500:500),q.y,q.z);n->LastPoint=q;
 objects[2]=n;tvsMove::Node node;
 node.ports.push_back(tvsMove::point(n->FirstPoint));node.ports.push_back(tvsMove::point(n->LastPoint));
 node.axes.push_back(tvsMoveGeometry::Point(1,0,0));node.axes.push_back(node.axes[0]);
 node.sections.push_back(tvsMoveNetwork::Section(400,200,false,-400));node.sections.push_back(node.sections[0]);
 nodes[2]=node;return n;
}
static void connect(TVS_Pipe* n,int opposite){
 fixtureSource.connectors.resize(2);
 for(int i=0;i<2;++i){fixtureSource.connectors[i].cTypeCurrent=i+1;fixtureSource.connectors[i].point=i==0?fixtureSource.FirstPoint:fixtureSource.LastPoint;}
 fixtureSource.connectors[opposite].connectionID=2;fixtureSource.connectors[opposite].cTypeAnother=2;
 TVS_Connector c;c.cTypeCurrent=2;c.cTypeAnother=opposite+1;c.connectionID=1;c.point=n->LastPoint;n->connectors.push_back(c);
}
int main(){
 std::cout<<std::unitbuf;
 int assertions=0;
 for(int e=0;e<2;++e){
  std::cout<<"topology end="<<e<<"\n";
  reset();live.physical.end=tvsOffsetOperation::End(e);std::wstring signature;
  assert(topology(live,&fixtureSource,signature));++assertions;
  TVS_Pipe* n=neighbor(1-e);connect(n,1-e);
  assert(topology(live,&fixtureSource,signature));++assertions;
  n->connectors[0].cTypeAnother=e+1;assert(!topology(live,&fixtureSource,signature));++assertions;
  n->connectors[0].cTypeAnother=2-e;
  fixtureSource.connectors[e].connectionID=2;assert(!topology(live,&fixtureSource,signature));++assertions;
  fixtureSource.connectors[e].connectionID=0;
  n->connectors[0].point.x+=1;assert(!topology(live,&fixtureSource,signature));++assertions;
  n->connectors[0].point.x-=1;
  n->connectors.clear();assert(!topology(live,&fixtureSource,signature));++assertions;
  fixtureSource.connectors.clear();assert(topology(live,&fixtureSource,signature));++assertions;
  nodes[2].sections[1].a=300;assert(!topology(live,&fixtureSource,signature));++assertions;
  nodes[2].sections[1].a=400;
  nodes[2].axes[1]=tvsMoveGeometry::Point(0,1,0);assert(!topology(live,&fixtureSource,signature));++assertions;
  reset();live.physical.end=tvsOffsetOperation::End(e);neighbor(e);
  assert(!topology(live,&fixtureSource,signature));++assertions;
 }
 reset();neighbor(0);TVS_Pipe* duplicate=new TVS_Pipe();duplicate->id=3;objects[3]=duplicate;nodes[3]=nodes[2];
 std::wstring signature;assert(!topology(live,&fixtureSource,signature));++assertions;
 reset();neighbor(0);nodes[2].ports[1]=tvsMoveGeometry::Point(-1000,0,37);
 assert(!topology(live,&fixtureSource,signature));++assertions;
 for(int e=0;e<2;++e)for(int failure=0;failure<11;++failure){
  std::cout<<"write end="<<e<<" failure="<<failure<<"\n";
  reset();live.physical.end=tvsOffsetOperation::End(e);Snapshot before=live;
  tvsOffsetOperation::Request request;request.elevation=0;
  const auto plan=tvsOffsetOperation::prepare(before.physical,request);assert(plan.status==tvsOffsetOperation::Ready);
  const AcGePoint3d first=fixtureSource.FirstPoint,last=fixtureSource.LastPoint;
  TVS_Connector preserved;preserved.cTypeCurrent=2-e;preserved.connectionID=9;preserved.cTypeAnother=1;preserved.point=e==0?last:first;
  fixtureSource.connectors.push_back(preserved);
  if(failure==1)live.fingerprint=L"changed attributes";
  if(failure==2)live.network=L"changed connector";
  if(failure==3)live.physical.factor=10;
  if(failure==4)fixtureSource.locked=true;
  if(failure==5)failAppend=1;
  if(failure==6)failAppend=4;
  if(failure==7)failRecord=2;
  if(failure==8)failNext()=true;
  if(failure==9)failCommit=true;
  if(failure==10)badCapture=true;
  Commit result;const Outcome outcome=write(before,request,plan,result);
  if(failure){
   assert(outcome==Failed);assert(objects.size()==2);assert(tvsOffsetData::records.empty());
   assert(fixtureSource.FirstPoint.distanceTo(first)==0&&fixtureSource.LastPoint.distanceTo(last)==0);
   assert(result.residentOutput==NULL&&commits==0);
   if(failure<5||failure==10)assert(txStarts==0);
  }else{
   assert(outcome==Applied&&commits==1&&aborts==0&&objects.size()==6&&tvsOffsetData::records.size()==3);
   assert((e==0?fixtureSource.LastPoint:fixtureSource.FirstPoint).distanceTo(e==0?last:first)==0);
   assert(result.residentOutput->Flow==12&&result.elevation==0&&result.angle==45);
   assert(result.residentOutput->FirstPoint.distanceTo(tvsOffsetProof::pt(plan.exitFirst))==0);
   assert(result.residentOutput->LastPoint.distanceTo(tvsOffsetProof::pt(plan.exitLast))==0);
   assert(std::fabs(tvsOffsetData::records[result.members[1].id].rise-282.842712474619)<1e-7);
   for(int i=0;i<3;++i){const auto& r=tvsOffsetData::records[result.members[i].id];assert(r.role==i);
    for(int j=0;j<3;++j)assert(r.members[j]==result.members[j]);}
  }
  assert(fixtureSource.connectors.size()==1&&fixtureSource.connectors[0].connectionID==preserved.connectionID);
  assert(fixtureSource.connectors[0].cTypeCurrent==preserved.cTypeCurrent&&fixtureSource.connectors[0].cTypeAnother==preserved.cTypeAnother);
  assert(fixtureSource.connectors[0].point.distanceTo(preserved.point)==0);++assertions;
 }
 reset();
 std::cout<<"PASS actual service: "<<assertions<<" topology/lifecycle scenarios, both ends, reciprocal/implicit contacts, bad/connected/ambiguous ports, source/order/connector preservation, stale attributes/network/units, locked layer, append/metadata/source/commit rollback; SDK substitutes only\n";
}
'''
        stub = (ROOT/'tests/offset_service_stubs.h').read_text()
        # Keep includes relative to the repository tests directory for generated files.
        stub = stub.replace('#include "../', '#include "' + ROOT.as_posix() + '/')
        source = stub + '\nnamespace tvsOffsetService {\n' + extracted + '\n}\n' + main
        with tempfile.TemporaryDirectory() as folder:
            cpp = pathlib.Path(folder)/'service.cpp'
            cpp.write_text(source, encoding='utf-8')
            target = pathlib.Path(folder)/'service'
            compile_cpp(cpp, target)
            subprocess.run([str(executable(target))], check=True)

    def test_both_actual_drawing_callers_preserve_outcomes(self):
        ui = (ROOT/'TVS_Ventilation_ARX/OffsetIntegrationCommand.h').read_text(encoding='utf-8-sig')
        entry = (ROOT/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_text(encoding='utf-8-sig')
        execute = function(ui, 'inline Outcome execute(')
        wrapper = function(entry, 'static tvsOffsetIntegration::Outcome changeZ(')
        needle = 'if(changeZ(pipi)==tvsOffsetIntegration::Applied)'
        callers = []
        remaining = entry
        while needle in remaining:
            body = function(remaining, needle)
            callers.append(body)
            remaining = remaining[remaining.index(needle) + len(body):]
        self.assertEqual(len(callers), 2, 'Both existing Z call sites must be exercised')
        prelude = r'''
#include <cassert>
#include <iostream>
#include <stdexcept>
struct AcGePoint3d {double x,y,z;AcGePoint3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}};
struct AcDbObjectId {int id;AcDbObjectId(int n=0):id(n){}};
struct TVS_Pipe {int id;AcGePoint3d FirstPoint,LastPoint;AcDbObjectId objectId(){return AcDbObjectId(id);}};
struct AcDbDatabase {};
struct Host {AcDbDatabase* workingDatabase(){return NULL;}};
static Host host;
static Host* acdbHostApplicationServices(){return &host;}
static double globalAxis=33,globalElevMid=-400,globalElev=-400;
static void acutPrintf(const wchar_t*,...){}
namespace tvsOffsetOperation {enum End {First,Last};}
namespace tvsOffsetService {
enum Outcome {Applied,NoChange,Cancelled,Failed};
struct Commit {AcDbObjectId output;TVS_Pipe* residentOutput;double elevation,angle,before;};
}
static int events=0;static bool throwHistory=false;
namespace tvsOffsetSession {inline void applied(AcDbDatabase*,AcDbObjectId a,AcDbObjectId b,double before){
 assert(a.id==1&&b.id==2&&before==-400);++events;if(throwHistory)throw std::runtime_error("test allocation failure");}}
namespace tvsOffsetIntegration {
using namespace tvsOffsetService;
static Outcome desired=Failed;static Commit fixture;
inline Outcome interact(AcDbObjectId id,tvsOffsetOperation::End end,double angle,Commit& out){
 assert(id.id==1&&end==tvsOffsetOperation::Last&&angle==33);out=fixture;return desired;}
'''
        source = prelude + execute + '\n}\n' + wrapper + '\n'
        for index, body in enumerate(callers):
            source += f'\nstatic void caller{index}(TVS_Pipe*& pipi,AcGePoint3d& A1,AcGePoint3d& A2,double* pt1){{' + body + '}\n'
        source += r'''
int main(){
 TVS_Pipe original;original.id=1;original.FirstPoint=AcGePoint3d(-2000,0,37);original.LastPoint=AcGePoint3d(0,0,37);
 TVS_Pipe output;output.id=2;output.FirstPoint=AcGePoint3d(500,0,37);output.LastPoint=AcGePoint3d(800,0,37);
 auto& commit=tvsOffsetIntegration::fixture;commit.output=2;commit.residentOutput=&output;commit.elevation=0;commit.angle=45;commit.before=-400;
 int cases=0;
 for(int which=0;which<2;++which)for(int status=0;status<4;++status)for(int fail=0;fail<2;++fail){
  globalAxis=33;globalElevMid=globalElev=-400;events=0;throwHistory=fail!=0;
  tvsOffsetIntegration::desired=tvsOffsetService::Outcome(status);
  TVS_Pipe* current=&original;AcGePoint3d a1(-1,-2,37),a2(0,0,37);double pt[3]={0,0,37};
  if(which==0)caller0(current,a1,a2,pt);else caller1(current,a1,a2,pt);
  if(status==tvsOffsetService::Applied){
   assert(current==&output&&globalElevMid==0&&globalElev==0&&globalAxis==45&&events==1);
   assert(a1.x==500&&a2.x==800&&pt[0]==800&&pt[1]==0&&pt[2]==37);
  }else{
   assert(current==&original&&globalElevMid==-400&&globalElev==-400&&globalAxis==33&&events==0);
   assert(a1.x==-1&&a1.y==-2&&a2.x==0&&pt[0]==0&&pt[1]==0&&pt[2]==37);
  }
  ++cases;
 }
 std::cout<<"PASS "<<cases<<" actual TVS_PIPE/drawStep3 Z caller cases: Applied/NoChange/Cancelled/Failed, continuation/globals, post-commit memory failure; no MFC/CAD runtime\n";
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp = pathlib.Path(folder)/'drawing.cpp'
            cpp.write_text(source, encoding='utf-8')
            target = pathlib.Path(folder)/'drawing'
            compile_cpp(cpp, target)
            subprocess.run([str(executable(target))], check=True)

    def test_actual_attribute_policy(self):
        text = (ROOT/'TVS_Ventilation_ARX/OffsetService.h').read_text(encoding='utf-8-sig')
        body = function(text, 'inline bool properties(')
        stub = r'''
#include <cassert>
#include <cwchar>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#define CTVSProperty L"TVS_Property"
namespace Acad {enum ErrorStatus {eOk,eInvalid};}
namespace AcDb {enum OpenMode {kForRead};}
struct AcDbObjectId {bool exists;bool isNull()const{return !exists;}};
struct resbuf {int restype;struct {int rint;double rreal;const wchar_t* rstring;} resval;resbuf* rbnext;
 resbuf(int type):restype(type),rbnext(NULL){resval.rint=0;resval.rreal=0;resval.rstring=L"";}};
static int allocated=0,released=0,dictionaryEntries=0;
static resbuf* node(int type){++allocated;return new resbuf(type);}
static void acutRelRb(resbuf* p){while(p){resbuf* next=p->rbnext;delete p;++released;p=next;}}
struct AcDbDictionary {int numEntries(){return dictionaryEntries;}};
template<class T>struct AcDbObjectPointer {T value;AcDbObjectPointer(AcDbObjectId,AcDb::OpenMode){}
 Acad::ErrorStatus openStatus(){return Acad::eOk;}T* operator->(){return &value;}};
struct TVS_Pipe {resbuf* data;bool dictionary;TVS_Pipe():data(NULL),dictionary(false){}
 AcDbObjectId extensionDictionary(){AcDbObjectId id={dictionary};return id;}resbuf* xData(void*){return data;}};
namespace tvsOffsetService {
struct Snapshot {std::wstring manufacturer,note;};
inline bool reject(const wchar_t*){return false;}
inline void putString(std::wostringstream& s,const std::wstring& v){s<<v.size()<<L':'<<v;}
}
'''
        main = r'''
using namespace tvsOffsetService;
static resbuf* app(const wchar_t* name=L"TVS_Property"){resbuf* n=node(1001);n->resval.rstring=name;return n;}
static void append(resbuf* head,int tag,const wchar_t* value){
 while(head->rbnext)head=head->rbnext;
 head->rbnext=node(1070);head=head->rbnext;head->resval.rint=tag;
 head->rbnext=node(1000);head->rbnext->resval.rstring=value;
}
static void number(resbuf* head,int tag,double value){
 while(head->rbnext)head=head->rbnext;
 head->rbnext=node(1070);head=head->rbnext;head->resval.rint=tag;
 head->rbnext=node(1040);head->rbnext->resval.rreal=value;
}
static bool run(resbuf* data,Snapshot& s){TVS_Pipe p;p.data=data;std::wostringstream out;return properties(&p,s,out);}
int main(){
 int cases=0;
 {Snapshot s;assert(run(NULL,s));++cases;}
 {Snapshot s;resbuf* data=app();append(data,0,L"UNIQUE-01");append(data,5,L"Manufacturer");append(data,9,L"Shared note");
  append(data,1,L"");number(data,7,0);number(data,8,0);assert(run(data,s));
  assert(s.manufacturer==L"Manufacturer"&&s.note==L"Shared note");++cases;}
 {Snapshot s;assert(!run(app(L"UnknownApplication"),s));++cases;}
 for(int tag=1;tag<=6;++tag)if(tag!=5){Snapshot s;resbuf* data=app();append(data,tag,L"custom override");assert(!run(data,s));++cases;}
 for(int tag=7;tag<=8;++tag){Snapshot s;resbuf* data=app();number(data,tag,123);assert(!run(data,s));++cases;}
 {Snapshot s;resbuf* data=app();append(data,10,L"unknown");assert(!run(data,s));++cases;}
 {Snapshot s;resbuf* data=app();append(data,5,L"A");append(data,5,L"B");assert(!run(data,s));++cases;}
 {Snapshot s;resbuf* data=app();data->rbnext=node(1070);assert(!run(data,s));++cases;}
 {Snapshot s;resbuf* data=app();number(data,5,0);assert(!run(data,s));++cases;}
 {Snapshot s;TVS_Pipe p;p.dictionary=true;dictionaryEntries=1;std::wostringstream out;assert(!properties(&p,s,out));++cases;}
 assert(allocated==released);
 std::cout<<"PASS "<<cases<<" actual read-only attribute-policy cases: shared manufacturer/note, unique position retained, ambiguous overrides/unknown apps/duplicates/dictionaries refused, buffers released; SDK substitutes only\n";
}
'''
        source = stub + '\nnamespace tvsOffsetService {\n' + body + '\n}\n' + main
        with tempfile.TemporaryDirectory() as folder:
            cpp = pathlib.Path(folder)/'attributes.cpp'
            cpp.write_text(source, encoding='utf-8')
            target = pathlib.Path(folder)/'attributes'
            compile_cpp(cpp, target)
            subprocess.run([str(executable(target))], check=True)

    def test_actual_shared_property_transfer(self):
        body = function((ROOT/'TVS_Ventilation_ARX/OffsetService.h').read_text(encoding='utf-8-sig'),
                        'inline bool appearance(')
        # Known SDK setPropertiesFrom semantics intentionally cover only five basic
        # properties, so loss of material/weight/transparency/plot style is detected.
        source = r'''
#include <cassert>
#include <iostream>
#include <string>
#define CTVSProperty L"TVS_Property"
namespace Acad {enum ErrorStatus {eOk,eInvalid};}
namespace AcDb {enum PlotStyleNameType {kByLayer,kById};}
struct AcDbObjectId {int id;AcDbObjectId(int value=0):id(value){}};
struct resbuf {std::wstring manufacturer,note;};
static resbuf* acutBuildList(int,const wchar_t*,int,int,int,const wchar_t* manufacturer,int,int,int,const wchar_t* note,int){
 resbuf* p=new resbuf;p->manufacturer=manufacturer;p->note=note;return p;}
static void acutRelRb(resbuf* p){delete p;}
static int failSetter=0;
struct TVS_Entity {
 int color,layer,linetype,visibility,weight,material,alpha,plot;double scale,Flow,WipeoutLength;
 int Direct;bool Grani,Wipeout,This1D;std::wstring position,manufacturer,note;
 TVS_Entity():color(0),layer(0),linetype(0),visibility(0),weight(0),material(0),alpha(0),plot(0),scale(0),Flow(0),WipeoutLength(0),Direct(0),Grani(false),Wipeout(false),This1D(false){}
 Acad::ErrorStatus setPropertiesFrom(const TVS_Entity* p){
  if(failSetter==1)return Acad::eInvalid;
  color=p->color;layer=p->layer;linetype=p->linetype;scale=p->scale;visibility=p->visibility;return Acad::eOk;}
 int lineWeight(){return weight;}AcDbObjectId materialId(){return AcDbObjectId(material);}int transparency(){return alpha;}
 AcDb::PlotStyleNameType getPlotStyleNameId(AcDbObjectId& id){id=plot;return AcDb::kById;}
 Acad::ErrorStatus setLineWeight(int value){weight=value;return failSetter==2?Acad::eInvalid:Acad::eOk;}
 Acad::ErrorStatus setMaterial(AcDbObjectId value){material=value.id;return failSetter==3?Acad::eInvalid:Acad::eOk;}
 Acad::ErrorStatus setTransparency(int value){alpha=value;return failSetter==4?Acad::eInvalid:Acad::eOk;}
 Acad::ErrorStatus setPlotStyleName(AcDb::PlotStyleNameType type,AcDbObjectId value){assert(type==AcDb::kById);plot=value.id;return failSetter==5?Acad::eInvalid:Acad::eOk;}
 Acad::ErrorStatus setXData(resbuf* p){manufacturer=p->manufacturer;note=p->note;return failSetter==6?Acad::eInvalid:Acad::eOk;}
};
struct TVS_Pipe:TVS_Entity {};
namespace tvsOffsetService {
struct Snapshot {std::wstring manufacturer,note;};
inline bool checked(Acad::ErrorStatus e){return e==Acad::eOk;}
''' + body + r'''
}
int main(){
 TVS_Pipe original;original.color=2;original.layer=3;original.linetype=4;original.visibility=1;original.scale=2.5;
 original.weight=50;original.material=17;original.alpha=100;original.plot=19;
 original.Flow=123;original.WipeoutLength=35;original.Grani=original.Wipeout=original.This1D=true;original.position=L"UNIQUE-ORIGINAL";
 tvsOffsetService::Snapshot snapshot;snapshot.manufacturer=L"Shared manufacturer";snapshot.note=L"Shared note";
 for(int failure=0;failure<=6;++failure){failSetter=failure;TVS_Entity target;
  const bool ok=tvsOffsetService::appearance(&original,&target,snapshot);
  if(failure){assert(!ok);continue;}
  assert(target.color==2&&target.layer==3&&target.linetype==4&&target.visibility==1&&target.scale==2.5);
  assert(target.weight==50&&target.material==17&&target.alpha==100&&target.plot==19);
  assert(target.Flow==123&&target.Direct==0&&target.WipeoutLength==35&&target.Grani&&target.Wipeout&&target.This1D);
  assert(target.position.empty()&&original.position==L"UNIQUE-ORIGINAL");
  assert(target.manufacturer==snapshot.manufacturer&&target.note==snapshot.note);
 }
 std::cout<<"PASS 7 actual property-transfer cases: display/material/plot/flow/shared metadata, no unique position clone, all setter failures propagated; SDK substitutes only\n";
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp = pathlib.Path(folder)/'transfer.cpp'
            cpp.write_text(source, encoding='utf-8')
            target = pathlib.Path(folder)/'transfer'
            compile_cpp(cpp, target)
            subprocess.run([str(executable(target))], check=True)

    def test_actual_transient_preview_cleanup(self):
        body = function((ROOT/'TVS_Ventilation_ARX/OffsetIntegrationCommand.h').read_text(encoding='utf-8-sig'),
                        'class Preview') + ';'
        prelude = r'''
#include <cassert>
#include <cstdarg>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
#include "OFFSET_HEADER"
struct AcGeVector3d {double x,y,z;AcGeVector3d(double a,double b,double c):x(a),y(b),z(c){}};
struct AcGePoint3d {double x,y,z;AcGePoint3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}
 AcGePoint3d operator+(const AcGeVector3d& v)const{return AcGePoint3d(x+v.x,y+v.y,z+v.z);}};
struct AcGePoint2d {double x,y;AcGePoint2d(double a,double b):x(a),y(b){}};
struct CString {void Format(const wchar_t*,...){}operator const wchar_t*()const{return L"test";}};
namespace Acad {enum ErrorStatus {eOk,eInvalid};}
static int liveEntities=0,addCalls=0,failAdd=0,throwAdd=0;
static bool nullManager=false;
struct AcDbEntity {AcDbEntity(){++liveEntities;}virtual ~AcDbEntity(){--liveEntities;}void setColorIndex(int){}};
struct AcDbPolyline:AcDbEntity {double z;AcDbPolyline():z(0){}
 Acad::ErrorStatus addVertexAt(int,const AcGePoint2d&){return Acad::eOk;}
 void setElevation(double value){z=value;}double elevation(){return z;}};
struct AcDbText:AcDbEntity {
 Acad::ErrorStatus setPosition(const AcGePoint3d&){return Acad::eOk;}
 Acad::ErrorStatus setHeight(double h){assert(h>0);return Acad::eOk;}
 Acad::ErrorStatus setTextString(const wchar_t*){return Acad::eOk;}};
struct AcDbIntArray {};
static const int kAcGiDirectShortTerm=0;
struct AcGiTransientManager {std::set<AcDbEntity*> entries;
 bool addTransient(AcDbEntity* e,int,int,AcDbIntArray&){++addCalls;if(addCalls==throwAdd)throw std::runtime_error("test");
  if(addCalls==failAdd)return false;
  entries.insert(e);return true;}
 bool eraseTransient(AcDbEntity* e,AcDbIntArray&){assert(entries.erase(e)==1);return true;}};
static AcGiTransientManager transientManager;
static AcGiTransientManager* acgiGetTransientManager(){return nullManager?NULL:&transientManager;}
namespace tvsOffsetProof {inline AcGePoint3d pt(const tvsOffset::WorldPoint& p){return AcGePoint3d(p.x,p.y,p.z);}}
namespace tvsOffsetService {struct Snapshot {tvsOffsetOperation::Snapshot physical;};}
namespace tvsOffsetIntegration {
'''
        main = r'''
}
int main(){
 tvsOffsetService::Snapshot snapshot;snapshot.physical.a=400;snapshot.physical.b=200;snapshot.physical.elevation=-400;
 snapshot.physical.factor=1;snapshot.physical.first=tvsOffset::WorldPoint(-2000,0,37);snapshot.physical.last=tvsOffset::WorldPoint(0,0,37);
 tvsOffsetOperation::Request request;request.elevation=0;
 const auto plan=tvsOffsetOperation::prepare(snapshot.physical,request);assert(plan.status==tvsOffsetOperation::Ready);
 for(int failure=0;failure<8;++failure){
  addCalls=0;failAdd=failure>=1&&failure<=5?failure:0;throwAdd=failure==6?3:0;nullManager=failure==7;
  try {tvsOffsetIntegration::Preview preview;const bool ok=preview.show(snapshot,plan);
   assert(ok==(failure==0));if(ok)assert(transientManager.entries.size()==5);
  }catch(const std::runtime_error&){assert(failure==6);}
  assert(liveEntities==0&&transientManager.entries.empty());
 }
 std::cout<<"PASS 8 actual preview cleanup cases: normal dismissal, each add failure, thrown manager error and missing manager; no database entities\n";
}
'''
        source = prelude.replace('OFFSET_HEADER', (ROOT/'geometry/OffsetOperation.h').as_posix()) + body + main
        with tempfile.TemporaryDirectory() as folder:
            cpp = pathlib.Path(folder)/'preview.cpp'
            cpp.write_text(source, encoding='utf-8')
            target = pathlib.Path(folder)/'preview'
            compile_cpp(cpp, target)
            subprocess.run([str(executable(target))], check=True)

if __name__ == '__main__':
    unittest.main(verbosity=2)

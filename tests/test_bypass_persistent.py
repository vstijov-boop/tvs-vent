"""Actual bypass writer with a real checked candidate and SDK transaction substitutes.
This is not an AutoCAD runtime/filing/Undo test.
"""
import pathlib, subprocess, tempfile, unittest
from test_offset_service import function
from bypass_compiler import compile_cpp, executable
ROOT=pathlib.Path(__file__).resolve().parents[1]
class PersistentBypass(unittest.TestCase):
    def test_checked_candidate_atomic_writer(self):
        offset=(ROOT/'TVS_Ventilation_ARX/OffsetService.h').read_text(encoding='utf-8-sig')
        bypass=(ROOT/'TVS_Ventilation_ARX/BypassService.h').read_text(encoding='utf-8-sig')
        data=(ROOT/'TVS_Ventilation_DBX/BypassAssemblyData.h').read_text(encoding='utf-8-sig')
        parent=(ROOT/'TVS_Ventilation_DBX/BypassAssemblyData.cpp').read_text(encoding='utf-8-sig')
        stub=(ROOT/'tests/offset_service_stubs.h').read_text().replace('#include "../','#include "'+ROOT.as_posix()+'/')
        code='#include <set>\n#include <cwchar>\n'+stub+'\n#define TVS_BYPASS_PREVIEW\n#include "'+(ROOT/'TVS_Ventilation_ARX/BypassNumericBridge.cpp').as_posix()+'"\n'
        code+='\nnamespace tvsOffsetService {\n'
        for name in ['inline bool current(', 'inline std::unique_ptr<TVS_TAP> makeElbow(', 'inline std::unique_ptr<TVS_Pipe> makePipe(']:
            code+=function(offset,name)+'\n'
        code+='}\nnamespace tvsBypassData {\n'+function(data,'struct Record')+';\n'
        code+=function(parent,'static bool boundaryPinned(')+'\n'
        code+=r'''
static std::map<int,Record> parents;static bool failParent=false,pinnedFlag=false;
inline bool pinned(AcDbObjectId,bool){return pinnedFlag;}
inline Acad::ErrorStatus write(AcDbDatabase*,Record& r,AcDbObject* members[7],AcDbObjectId& parent){
 if(failParent)return Acad::eInvalid;
 r.guid=tvsOffsetData::newGuid();parent=2000;
 for(int j=0;j<7;++j)assert(members[j]->objectId()==r.members[j]);
 parents[parent.id]=r;return Acad::eOk;
}
}
namespace tvsBypassService {
using tvsBypassPreview::View;using tvsBypassPreview::Options;
struct Snapshot {tvsOffsetService::Snapshot route;AcDbObjectId obstacle;std::wstring obstacleState;};
'''
        code+=function(bypass,'struct Commit')+';\n'
        code+=function(bypass,'inline int connector(')+'\n'
        fs=bypass.index('inline std::wstring& lastFailure(');code+=bypass[fs:bypass.index('inline bool inject(',fs)]+'\n'
        code+=r'''
static int failStage=0;
inline bool inject(int stage){return stage==failStage;}
inline bool obstacleState(AcDbObjectId id,std::wstring& value){
 TVS_Pipe* p=TVS_Pipe::cast(objects[id.id]);if(!p)return false;
 std::wostringstream out;out<<std::setprecision(17);
 tvsOffsetService::putPoint(out,p->FirstPoint);tvsOffsetService::putPoint(out,p->LastPoint);
 out<<p->SizeA<<L';'<<p->SizeB<<L';'<<p->Elev;value=out.str();return true;
}
'''
        code+=function(bypass,'inline bool current(')+'\n'+function(bypass,'inline bool write(')+'\n}\n'
        code+=r'''
static void clearMetadata(){tvsBypassData::parents.clear();}
static void reset(){
 for(auto& o:objects)if(o.second!=&fixtureSource&&o.second!=&block)delete o.second;
 objects.clear();nodes.clear();tvsOffsetData::records.clear();tvsBypassData::parents.clear();
 fixtureSource=TVS_Pipe();fixtureSource.id=1;block.id=99;objects[1]=&fixtureSource;objects[99]=&block;
 failAppend=appendCalls=failRecord=recordCalls=commits=aborts=txStarts=0;
 failCommit=false;tvsOffsetService::badCapture=false;tvsBypassData::failParent=tvsBypassData::pinnedFlag=false;
 tvsBypassService::failStage=0;rollbackHook=&clearMetadata;
}
static tvsBypassPreview::Snapshot setup(double f,double heading,bool reverse,tvsBypassService::Snapshot& s,bool linked=false){
 reset();
 const double c=std::cos(heading),n=std::sin(heading);
 auto point=[=](double x,double y){return AcGePoint3d((123+x*c-y*n)/f,(-87+x*n+y*c)/f,37/f);};
 fixtureSource.FirstPoint=point(-2000,0);fixtureSource.LastPoint=point(2000,0);
 fixtureSource.SizeA=400/f;fixtureSource.SizeB=200/f;fixtureSource.Elev=-400/f;fixtureSource.Flow=123;
 TVS_Pipe* obstacle=new TVS_Pipe();obstacle->id=2;objects[2]=obstacle;
 obstacle->FirstPoint=point(0,-1500);obstacle->LastPoint=point(0,1500);
 obstacle->SizeA=250/f;obstacle->SizeB=250/f;obstacle->Elev=-400/f;
 tvsOffsetService::live=tvsOffsetService::Snapshot();
 auto& p=tvsOffsetService::live.physical;p.first=tvsOffset::WorldPoint(fixtureSource.FirstPoint.x,fixtureSource.FirstPoint.y,fixtureSource.FirstPoint.z);
 p.last=tvsOffset::WorldPoint(fixtureSource.LastPoint.x,fixtureSource.LastPoint.y,fixtureSource.LastPoint.z);
 p.a=400/f;p.b=200/f;p.elevation=-400/f;p.factor=f;p.end=reverse?tvsOffsetOperation::First:tvsOffsetOperation::Last;
 tvsOffsetService::live.fingerprint=L"source";tvsOffsetService::live.network=L"network";
 s.route=tvsOffsetService::live;s.obstacle=2;tvsBypassService::obstacleState(2,s.obstacleState);
 fixtureSource.connectors.resize(2);
 for(int k=0;k<2;++k){fixtureSource.connectors[k].cTypeCurrent=k+1;fixtureSource.connectors[k].point=k==0?fixtureSource.FirstPoint:fixtureSource.LastPoint;}
 const int selected=reverse?0:1;const AcGePoint3d far=selected==0?fixtureSource.FirstPoint:fixtureSource.LastPoint;
 TVS_Pipe* neighbor=new TVS_Pipe();neighbor->id=3;objects[3]=neighbor;neighbor->FirstPoint=far;
 neighbor->LastPoint=far+AcGeVector3d((reverse?-1:1)*c*300/f,(reverse?-1:1)*n*300/f,0);
 // Geometric outer neighbor always touches the far end; an explicit reciprocal link only when requested.
 TVS_Connector back;back.connectionID=linked?1:0;back.cTypeCurrent=1;back.cTypeAnother=linked?selected+1:0;back.point=far;neighbor->connectors.push_back(back);
 if(linked){fixtureSource.connectors[selected].connectionID=3;fixtureSource.connectors[selected].cTypeAnother=1;}
 // Opposite external reference is retained verbatim (the boundary validator is tested separately).
 fixtureSource.connectors[1-selected].connectionID=44;fixtureSource.connectors[1-selected].cTypeAnother=2;
 tvsBypassPreview::Snapshot model;
 auto copy=[](TVS_Pipe* p,tvsBypassPreview::Segment& t){t.first=tvsBypassPreview::Point(p->FirstPoint.x,p->FirstPoint.y,p->FirstPoint.z);
 t.last=tvsBypassPreview::Point(p->LastPoint.x,p->LastPoint.y,p->LastPoint.z);t.a=p->SizeA;t.b=p->SizeB;t.elevation=p->Elev;t.rectangular=t.rigid=t.ordinary=true;};
 copy(&fixtureSource,model.route);copy(obstacle,model.obstacle);return model;
}
int main(){
 for(bool reverse:{false,true}){
  tvsBypassData::Record r;r.reverse=reverse;r.inlet=71;r.outlet=72;
  assert(tvsBypassData::boundaryPinned(r,71,reverse));
  assert(!tvsBypassData::boundaryPinned(r,71,!reverse));
  assert(tvsBypassData::boundaryPinned(r,72,!reverse));
  assert(!tvsBypassData::boundaryPinned(r,72,reverse));
  r.neighbors[0]=73;r.ports[0]=1;r.neighbors[1]=74;r.ports[1]=2;
  assert(tvsBypassData::boundaryPinned(r,71,!reverse));
  assert(tvsBypassData::boundaryPinned(r,72,reverse));
  assert(tvsBypassData::boundaryPinned(r,73,true));
  assert(!tvsBypassData::boundaryPinned(r,73,false));
  assert(tvsBypassData::boundaryPinned(r,74,false));
  assert(!tvsBypassData::boundaryPinned(r,74,true));
  assert(!tvsBypassData::boundaryPinned(r,75,false));
 }
 int cases=0,failures=0;
 for(double factor:{1.,10.,1000.})for(double heading:{0.,.6457718232379019})for(bool reverse:{false,true})
 for(bool above:{false,true})for(int angle:{30,45,60}){
  tvsBypassService::Snapshot s;auto model=setup(factor,heading,reverse,s);
  tvsBypassPreview::Engine engine;tvsBypassPreview::Options options;options.confirmed=true;options.above=above;options.angle=angle;
  auto frame=engine.frame(model,factor,reverse);options.left=frame.left;options.right=frame.right;
  const auto view=engine.calculate(model,factor,reverse,options,false);assert(view.ready);
  const AcGePoint3d fixed=reverse?fixtureSource.LastPoint:fixtureSource.FirstPoint,far=reverse?fixtureSource.FirstPoint:fixtureSource.LastPoint;
  TVS_Pipe* neighbor=TVS_Pipe::cast(objects[3]);const AcGePoint3d neighborFirst=neighbor->FirstPoint,neighborLast=neighbor->LastPoint;
  const std::vector<TVS_Connector> sourceBefore=fixtureSource.connectors,neighborBefore=neighbor->connectors;
  auto same=[](const std::vector<TVS_Connector>& a,const std::vector<TVS_Connector>& b){if(a.size()!=b.size())return false;
   for(size_t k=0;k<a.size();++k){if(a[k].connectionID!=b[k].connectionID||a[k].cTypeCurrent!=b[k].cTypeCurrent||a[k].cTypeAnother!=b[k].cTypeAnother||a[k].point.distanceTo(b[k].point)!=0)return false;}
   return true;};
  tvsBypassService::Commit out;assert(tvsBypassService::write(s,view,options,out));
  assert(commits==1&&aborts==0&&objects.size()==12&&tvsOffsetData::records.size()==6&&tvsBypassData::parents.size()==1);
  const auto parent=tvsBypassData::parents[out.parent.id];assert(parent.inlet==1&&parent.outlet==out.output&&parent.obstacle==2);
  assert(parent.neighbors[0]==44&&parent.neighbors[1].isNull()&&parent.ports[1]==0);
  assert((reverse?fixtureSource.LastPoint:fixtureSource.FirstPoint).distanceTo(fixed)==0);
  assert((reverse?out.resident->FirstPoint:out.resident->LastPoint).distanceTo(far)==0);
  assert(neighbor->FirstPoint.distanceTo(neighborFirst)==0&&neighbor->LastPoint.distanceTo(neighborLast)==0);
  // Connector contract: nothing non-filed changes, so UNDO/reopen cannot leave a stale link.
  assert(same(fixtureSource.connectors,sourceBefore)&&same(neighbor->connectors,neighborBefore));
  assert(out.resident->connectors.size()==2&&out.resident->connectors[0].connectionID.isNull()&&out.resident->connectors[1].connectionID.isNull());
  assert(tvsOffsetData::records[parent.members[0].id].guid.text!=tvsOffsetData::records[parent.members[4].id].guid.text);
  double total=0;
  for(int pair=0;pair<2;++pair){const int start=pair?4:0;TVS_Pipe* p=TVS_Pipe::cast(objects[parent.members[start+1].id]);
   const auto record=tvsOffsetData::records[p->id.id];const double projected=p->FirstPoint.distanceTo(p->LastPoint)*factor;
   const double physical=std::hypot(projected,record.rise);
   assert(std::fabs(physical-view.slopePhysical[pair])<1e-6);total+=physical;
   assert(p->Flow==123);
   for(int j=0;j<3;++j){const auto member=tvsOffsetData::records[parent.members[start+j].id];
    assert(member.role==j);for(int k=0;k<3;++k)assert(member.members[k]==parent.members[start+k]);}
  }
  TVS_Pipe* h=TVS_Pipe::cast(objects[parent.members[3].id]);
  assert(tvsOffsetData::records.count(h->id.id)==0&&std::fabs(h->FirstPoint.distanceTo(h->LastPoint)*factor-view.C)<1e-6);
  assert(out.elevation==-400/factor&&std::fabs(h->Elev*factor-view.zPass)<1e-6&&total>0);++cases;
 }
 std::set<std::wstring> reasons;
 for(int fail=1;fail<=11;++fail){
  tvsBypassService::Snapshot s;auto model=setup(1,0,false,s,fail==7);tvsBypassPreview::Engine engine;tvsBypassPreview::Options options;options.confirmed=true;
  options.left=-2000;options.right=2000;auto view=engine.calculate(model,1,false,options,false);assert(view.ready);
  TVS_Pipe* neighbor=TVS_Pipe::cast(objects[3]);const AcGePoint3d first=fixtureSource.FirstPoint,last=fixtureSource.LastPoint;
  if(fail<=3)tvsBypassService::failStage=fail;
  if(fail==4)failAppend=4;
  if(fail==5)failRecord=4;
  if(fail==6)tvsBypassData::failParent=true;
  // fail==7: explicitly linked outer end is refused (no relinking of non-filed connectors).
  if(fail==8)tvsOffsetService::live.fingerprint=L"changed";
  if(fail==9)TVS_Pipe::cast(objects[2])->Elev+=1;
  if(fail==10)tvsBypassData::pinnedFlag=true;
  if(fail==11)failCommit=true;
  tvsBypassService::Commit out;assert(!tvsBypassService::write(s,view,options,out));
  {const std::wstring why=tvsBypassService::lastFailure();assert(!why.empty()&&why[0]==L'[');reasons.insert(why);
   const wchar_t* phase=fail<=3?L"[DEV]":fail==4?L"[append]":fail<=6?L"[metapodaci]":fail==7?L"[konektori]":fail<=10?L"[snapshot]":L"[commit]";
   if(why.find(phase)!=0){std::wcerr<<L"case "<<fail<<L": "<<why<<L"\n";std::abort();}
   if((fail==4||fail==5||fail==6||fail==11)&&why.find(L"ErrorStatus")==std::wstring::npos)std::abort();}
  assert(commits==0&&objects.size()==4&&tvsOffsetData::records.empty()&&tvsBypassData::parents.empty());
  assert(fixtureSource.FirstPoint.distanceTo(first)==0&&fixtureSource.LastPoint.distanceTo(last)==0);
  assert(fixtureSource.connectors[1].connectionID==(fail==7?3:0)&&neighbor->connectors[0].connectionID==(fail==7?1:0)&&out.resident==NULL);++failures;
 } assert(reasons.size()==11); // every injected cause is reported distinctly

 reset();std::cout<<"PASS PERSISTENT BYPASS "<<cases<<" real checked candidates, same 7 native member DTOs, both ends, rotations/units/angles/sides, parent over 2 V1 triples+H, connectors unchanged (not filed), linked outer end refused, quantities; "<<failures<<" abort/stale/lock guards; SDK substitutes, not AutoCAD runtime\n";
}
'''
        # Avoid warning-only ambiguities in compact extracted existing code.
        code=code.replace('return false;members[role]=p;', 'return false;\n            members[role]=p;')
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/'persistent.cpp';cpp.write_text(code,encoding='utf-8')
            target=pathlib.Path(folder)/'persistent';compile_cpp(cpp,target)
            subprocess.run([str(executable(target))],check=True)
    def test_both_drawing_bypass_continuations(self):
        text=(ROOT/'TVS_Ventilation_ARX/acrxEntryPoint.cpp').read_text(encoding='utf-8-sig')
        needle='if(tvsBypassDuringDrawing(pipi->objectId(),result))'
        callers=[]
        while needle in text:
            body=function(text,needle);callers.append(body);text=text[text.index(needle)+len(body):]
        self.assertEqual(len(callers),2)
        code=r'''
#include <cassert>
#include <iostream>
struct AcDbObjectId {int value;};
struct AcGePoint3d {double x,y,z;AcGePoint3d(double a=0,double b=0,double c=0):x(a),y(b),z(c){}};
struct TVS_Pipe {int id;AcGePoint3d FirstPoint,LastPoint;AcDbObjectId objectId(){AcDbObjectId r={id};return r;}};
struct TVSBypassOutput {TVS_Pipe* pipe;double elevation,angle;};
static TVS_Pipe original,output;
static bool accept=false;
static double globalElevMid=-400,globalElev=-400,globalAxis=33;
static bool tvsBypassDuringDrawing(AcDbObjectId id,TVSBypassOutput& r){assert(id.value==1);r.pipe=&output;r.elevation=-400;r.angle=45;return accept;}
'''
        for i,body in enumerate(callers):
            code+=f'\nstatic void caller{i}(TVS_Pipe*& pipi,AcGePoint3d& A1,AcGePoint3d& A2,double* pt1){{TVSBypassOutput result;'+body+'}\n'
        code+=r'''
int main(){
 original.id=1;original.FirstPoint=AcGePoint3d(0,0,37);original.LastPoint=AcGePoint3d(4000,0,37);
 output.id=2;output.FirstPoint=AcGePoint3d(2700,0,37);output.LastPoint=original.LastPoint;
 for(int caller=0;caller<2;++caller)for(int success=0;success<2;++success){
  accept=success!=0;globalElevMid=globalElev=-400;globalAxis=33;
  TVS_Pipe* pipe=&original;AcGePoint3d a1(-1,-1,37),a2(4000,0,37);double point[3]={4000,0,37};
  if(caller)caller1(pipe,a1,a2,point);else caller0(pipe,a1,a2,point);
  assert(globalElevMid==-400&&globalElev==-400&&a2.x==4000&&point[0]==4000&&point[2]==37);
  if(success)assert(pipe==&output&&a1.x==2700&&globalAxis==45);
  else assert(pipe==&original&&a1.x==-1&&a1.y==-1&&globalAxis==33);
 }
 std::cout<<"PASS 4 actual drawing bypass caller cases: same service, source elevation/far endpoint preserved, success selects exit, Cancel/failure preserves continuation; no AutoCAD runtime\n";
}
'''
        with tempfile.TemporaryDirectory() as folder:
            cpp=pathlib.Path(folder)/'drawing-bypass.cpp';cpp.write_text(code,encoding='utf-8')
            target=pathlib.Path(folder)/'drawing-bypass';compile_cpp(cpp,target)
            subprocess.run([str(executable(target))],check=True)

if __name__=='__main__':unittest.main(verbosity=2)

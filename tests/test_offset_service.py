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

if __name__ == '__main__':
    unittest.main(verbosity=2)

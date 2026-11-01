// This translation unit alone is built /fp:strict, /GL-, without ARX/MFC PCH.
#ifdef TVS_BYPASS_PREVIEW
#include "../geometry/bypass/Generator.h"
#include "../geometry/bypass/PreviewModel.h"
#include <sstream>
#include <iomanip>
#include <memory>
namespace tvsBypassPreview {
struct State {std::vector<double> key;Revision revision;std::unique_ptr<tvsBypassSearch::Result> best;View view;};
Engine::Engine():state(new State){}
Engine::~Engine(){delete static_cast<State*>(state);}
void Engine::invalidate(){State& s=*static_cast<State*>(state);s.revision.invalidate();s.best.reset();s.view=View();}
bool Engine::ready()const{return static_cast<State*>(state)->revision.current();}
Frame Engine::frame(const Snapshot& s,double f,bool r)const{
 if(!tvsBypass::bounded::supported()){Frame bad;bad.reason="unsupported numeric environment";return bad;}
 tvsBypass::bounded::Environment env;if(!env.active)return Frame();return adapt(s,f,r);
}
static double middle(tvsBypass::I v){return v.lo+(v.hi-v.lo)/2;}
static Vertex at(const tvsBypass::Body& b,double u,double v){tvsBypass::V p=b.at(tvsBypass::I(u),tvsBypass::I(v));return Vertex(middle(p.s),middle(p.z));}
static void outline(const tvsBypass::Body& b,bool insulation,std::vector<Curve>& lines){
 Curve c;c.member=static_cast<int>(b.role);c.insulation=insulation;
 // Display tessellation only, never used for collision acceptance. Chord error <= .05 mm.
 unsigned n=b.kind==tvsBypass::Body::Sector?static_cast<unsigned>(std::ceil(std::sqrt(b.outer.hi*b.theta*b.theta/(8*.05)))):1;
 n=std::max(1u,std::min(262144u,n));
 for(unsigned j=0;j<=n;++j)c.points.push_back(at(b,double(j)/n,0));
 for(unsigned j=0;j<=n;++j)c.points.push_back(at(b,1-double(j)/n,1));
 c.points.push_back(c.points.front());lines.push_back(c);
}
static Curve rectangle(int member,bool iso,double l,double r,double bottom,double top){
 Curve c;c.member=member;c.insulation=iso;c.points={Vertex(l,bottom),Vertex(r,bottom),Vertex(r,top),Vertex(l,top),Vertex(l,bottom)};return c;
}
static bool integer(double v,double max){return finite(v)&&v>=1&&v<=max&&std::floor(v)==v;}
View Engine::calculate(const Snapshot& snapshot,double factor,bool reverse,const Options& o,bool improve){
 State& stateRef=*static_cast<State*>(state);View failed;failed.status="Nedostaju ili su nevažeći potvrđeni ulazi.";
 if(!tvsBypass::bounded::supported()){failed.status="Nepodržano numeričko okruženje: potrebno FE_TONEAREST, bez FTZ/DAZ.";return failed;}
 tvsBypass::bounded::Environment environment;if(!environment.active)return failed;
 std::vector<double> key={factor,double(reverse)};
 for(const Segment* segment:{&snapshot.route,&snapshot.obstacle}){
  const double v[]={segment->first.x,segment->first.y,segment->first.z,segment->last.x,segment->last.y,segment->last.z,
  segment->a,segment->b,segment->elevation,double(segment->rectangular),double(segment->rigid),double(segment->ordinary)};
  key.insert(key.end(),v,v+12);
 }
 const double policy[]={o.ri,o.insulation,o.obstacleInsulation,o.clearance,o.reserve,o.minSlope,o.minBridge,o.minProjection,o.left,o.right,
 o.floor,o.ceiling,o.timeMs,o.calls,o.cells,o.depthStep,o.bridgeStep,o.maxDepth,o.depthLevels,o.bridgeLevels,o.positions,o.seedSolves,o.proposals,
 o.inset,o.wTolerance,o.dTolerance,double(o.angle),double(o.above),double(o.useFloor),double(o.useCeiling),double(o.confirmed)};
 key.insert(key.end(),policy,policy+31);
 if(key!=stateRef.key){invalidate();stateRef.key=key;}
 Frame f=adapt(snapshot,factor,reverse);failed.frame=f;
 if(!f.valid){failed.details=f.reason;return failed;}
 if(!o.confirmed||!integer(o.calls,128)||!integer(o.cells,1000000)||!integer(o.seedSolves,128)||!integer(o.proposals,2048)||
 !integer(o.depthLevels,128)||!integer(o.bridgeLevels,16)||!integer(o.positions,16)||!finite(o.timeMs)||o.timeMs<1||o.timeMs>10000)return failed;
 tvsBypass::Input in(o.minSlope,o.minBridge,o.minProjection);
 in.width=f.width;in.height=f.height;in.ri=o.ri;in.z0=f.z0;in.angleDegrees=o.angle;in.above=o.above;
 in.obstacleS=0;in.obstacleZ=f.obstacleZ;in.obstacleWidth=f.obstacleWidth;in.obstacleHeight=f.obstacleHeight;
 in.obstacleTmin=f.tmin;in.obstacleTmax=f.tmax;in.insulation=o.insulation;in.obstacleInsulation=o.obstacleInsulation;in.clearance=o.clearance;
 in.windowLeft=o.left;in.windowRight=o.right;in.floor.active=o.useFloor;in.floor.mm=o.floor;in.ceiling.active=o.useCeiling;in.ceiling.mm=o.ceiling;
 tvsBypassSearch::Policy p(o.reserve,o.depthStep,o.bridgeStep);p.stopAtFirstFeasible=!improve;
 p.maxElapsedMs=o.timeMs;p.maxCheckerCalls=static_cast<unsigned>(o.calls);p.maxTotalCells=static_cast<std::uint64_t>(o.cells);
 p.maxSeedSolves=static_cast<unsigned>(o.seedSolves);p.maxProposals=static_cast<unsigned>(o.proposals);
 p.depthLevels=static_cast<unsigned>(o.depthLevels);p.bridgeLevels=static_cast<unsigned>(o.bridgeLevels);p.positionLevels=static_cast<unsigned>(o.positions);
 p.maximumSearchDepthMm=o.maxDepth;p.windowInsetMm=o.inset;p.improvementWToleranceMm=o.wTolerance;p.improvementDToleranceMm=o.dTolerance;
 tvsBypassSearch::Result found=tvsBypassSearch::generate(in,p);
 bool retained=improve&&stateRef.revision.current()&&stateRef.best&&(!found.hasCandidate||
 !tvsBypassSearch::better(found.candidate,found.checked,*stateRef.best,p));
 if(!retained){stateRef.best.reset();stateRef.revision.ready=false;
  if(found.hasCandidate)stateRef.best.reset(new tvsBypassSearch::Result(found));}
 View out;out.frame=f;out.elapsedMs=found.elapsedMs;out.calls=found.attempts;out.cells=found.processedCells;
 std::ostringstream text;text<<std::fixed<<std::setprecision(6);
 text<<"Zavrsetak: "<<found.termination<<"\r\nPozivi="<<found.attempts<<", celije="<<found.processedCells<<", vrijeme="<<found.elapsedMs<<" ms\r\n";
 if(!stateRef.best){
  switch(found.status){case tvsBypassSearch::NoCandidateFoundWithinBudget:out.status="Kandidat nije pronađen u zadatom budžetu.";break;
  case tvsBypassSearch::Unresolved:out.status="Numerička granica je nerazriješena — prolaz nije potvrđen.";break;
  case tvsBypassSearch::Unsupported:out.status="Nepodržani ulazi ili numeričko okruženje.";break;
  default:out.status="Nevažeći ulazi.";}
  out.details=text.str();stateRef.view=out;return out;
 }
 const tvsBypassSearch::Result& r=*stateRef.best;const tvsBypass::Result& checked=r.checked;
 out.ready=true;out.status="Izvodljiv kandidat — minimalni zahvat nije potvrđen.";
 out.D=r.D;out.C=r.C;out.s0=r.s0;out.W=middle(r.W);out.zPass=middle(r.passElevation);out.gapLo=r.clearance.lo;out.gapHi=r.clearance.hi;
 if(retained)text<<"Zadrzan prethodni potvrđeni kandidat za iste ulaze.\r\n";
 text<<"Kota ose prolaza="<<out.zPass<<" mm; D="<<out.D<<" mm\r\nC="<<out.C<<" mm; W="<<out.W<<" mm; s0="<<out.s0<<" mm\r\n";
 text<<std::setprecision(17)<<"Zazor ["<<r.clearance.lo<<", "<<r.clearance.hi<<"] mm\r\n"<<std::setprecision(6)<<"Obavezan="<<o.clearance<<"; projektna rezerva="<<o.reserve<<" mm\r\n";
 for(int j=0;j<2;++j)text<<"Nagib "<<j+1<<": Lfiz="<<checked.offsets[j].straightLength<<"; Lproj="<<checked.offsets[j].projectedLength<<" mm\r\n";
 text<<"Koljeno: Losa="<<checked.offsets[0].arcLength<<" mm (4 kom.); most="<<r.C<<" mm\r\n";
 text<<"Kriticni clan="<<static_cast<int>(checked.obstacle.witnessMember)<<"; donja ograda clan="<<static_cast<int>(checked.obstacle.lowerMember)<<"\r\n";
 text<<"Tijelo(s,z)=("<<middle(checked.obstacle.onBody.s)<<","<<middle(checked.obstacle.onBody.z)<<")\r\nPrepreka(s,z)=("<<middle(checked.obstacle.onObstacle.s)<<","<<middle(checked.obstacle.onObstacle.z)<<") mm\r\n";
 out.details=text.str();
 out.criticalRole=static_cast<int>(checked.obstacle.witnessMember);
 out.witnessBody=Vertex(middle(checked.obstacle.onBody.s),middle(checked.obstacle.onBody.z));
 out.witnessObstacle=Vertex(middle(checked.obstacle.onObstacle.s),middle(checked.obstacle.onObstacle.z));
 for(const auto& a:checked.axis)out.axis.push_back(Vertex(middle(a.s),middle(a.z)));
 for(int j=0;j<2;++j){out.slopePhysical[j]=checked.offsets[j].straightLength;out.slopeProjected[j]=checked.offsets[j].projectedLength;}
 for(const tvsBypass::Body& body:checked.members){
  outline(body,true,out.profile);
  tvsBypass::Body metal=body;
  if(metal.kind==tvsBypass::Body::Sector){metal.inner=metal.inner+tvsBypass::I(o.insulation);metal.outer=metal.outer-tvsBypass::I(o.insulation);}
  else metal.half=metal.half-tvsBypass::I(o.insulation);
  outline(metal,false,out.profile);
  tvsBypass::Box box=body.enclosure(tvsBypass::I(0,1),tvsBypass::I(0,1));
  out.plan.push_back(rectangle(static_cast<int>(body.role),true,box.s.lo,box.s.hi,-f.width/2-o.insulation,f.width/2+o.insulation));out.plan.back().kind=DiagnosticLine;
  tvsBypass::Box bare=metal.enclosure(tvsBypass::I(0,1),tvsBypass::I(0,1));
  out.plan.push_back(rectangle(static_cast<int>(body.role),false,bare.s.lo,bare.s.hi,-f.width/2,f.width/2));out.plan.back().kind=DiagnosticLine;
  tvsBypass::V ports[2];for(int end=0;end<2;++end){tvsBypass::port(body,end!=0,ports);
   for(int k=0;k<2;++k){Curve c;c.member=static_cast<int>(body.role);c.insulation=false;
    c.kind=JointSymbol;
    c.points.push_back(Vertex(middle(ports[k].s),-f.width/2-o.insulation));c.points.push_back(Vertex(middle(ports[k].s),f.width/2+o.insulation));out.plan.push_back(c);}}
 }
 out.profile.push_back(rectangle(7,false,-f.obstacleWidth/2,f.obstacleWidth/2,f.obstacleZ-f.obstacleHeight/2,f.obstacleZ+f.obstacleHeight/2));
 out.profile.push_back(rectangle(7,true,-f.obstacleWidth/2-o.obstacleInsulation,f.obstacleWidth/2+o.obstacleInsulation,f.obstacleZ-f.obstacleHeight/2-o.obstacleInsulation,f.obstacleZ+f.obstacleHeight/2+o.obstacleInsulation));
 out.plan.push_back(rectangle(7,true,-f.obstacleWidth/2-o.obstacleInsulation,f.obstacleWidth/2+o.obstacleInsulation,f.tmin,f.tmax));
 // The union footprint has constant width. Individual fitting envelopes are diagnostics.
 double left=out.plan.front().points.front().s,right=left;
 for(const auto& c:out.plan)if(c.member<7)for(const auto& vertex:c.points){left=std::min(left,vertex.s);right=std::max(right,vertex.s);}
 out.plan.push_back(rectangle(10,true,left,right,-f.width/2-o.insulation,f.width/2+o.insulation));
 out.plan.push_back(rectangle(10,false,out.axis.front().s,out.axis.back().s,-f.width/2,f.width/2));
 Curve planAxis;planAxis.member=8;planAxis.insulation=false;planAxis.kind=AxisLine;
 planAxis.points.push_back(Vertex(out.axis.front().s,0));planAxis.points.push_back(Vertex(out.axis.back().s,0));out.plan.push_back(planAxis);
 // Reference axes and witness distance, from the same Result.
 Curve axis;axis.member=8;axis.insulation=false;axis.kind=AxisLine;for(const auto& v:checked.axis)axis.points.push_back(Vertex(middle(v.s),middle(v.z)));out.profile.push_back(axis);
 Curve witness;witness.member=9;witness.insulation=false;witness.kind=DiagnosticLine;
 witness.points.push_back(Vertex(middle(checked.obstacle.onBody.s),middle(checked.obstacle.onBody.z)));
 witness.points.push_back(Vertex(middle(checked.obstacle.onObstacle.s),middle(checked.obstacle.onObstacle.z)));out.profile.push_back(witness);
 stateRef.revision.accept();stateRef.view=out;return out;
}
}
#endif

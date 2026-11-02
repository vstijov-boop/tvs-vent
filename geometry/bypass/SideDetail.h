#ifndef TVS_BYPASS_SIDE_DETAIL_H
#define TVS_BYPASS_SIDE_DETAIL_H
#include "PreviewPresentation.h"
#include <stdexcept>
#include <cctype>
namespace tvsBypassPreview {
struct DetailDimension {Vertex a,b,rail;std::string prefix;};
struct DetailNote {Vertex at;std::string text;};
struct SideDetail {
 std::vector<DetailPath> paths;std::vector<DetailDimension> dimensions;std::vector<DetailNote> notes;
 std::vector<DetailNote> levelTexts; // texts of the level marks (symbols are member-9 paths)
 double textHeight,arrowSize,factor,minS,maxS,minZ,maxZ;
};
inline bool detailLabelValid(const std::string& s){
 if(s.empty()||s.size()>32)return false;
 for(unsigned char c:s)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return false;
 return true;
}
inline std::string labelKey(std::string s){for(char& c:s)if(c>='a'&&c<='z')c=char(c-'a'+'A');return s;}
inline std::string nextDetailLabel(const std::vector<std::string>& used){
 for(unsigned n=1;n<1000000;++n){std::ostringstream a;a<<"OB-"<<std::setw(2)<<std::setfill('0')<<n;
  bool exists=false;for(const auto& s:used)if(labelKey(s)==a.str())exists=true;if(!exists)return a.str();}
 throw std::runtime_error("No free detail label");
}
inline DetailPath detailRectangle(double l,double r,double bottom,double top,bool insulation){
 DetailPath p;p.member=7;p.insulation=insulation;p.axis=false;p.closed=true;
 p.points={Vertex(l,bottom),Vertex(r,bottom),Vertex(r,top),Vertex(l,top)};p.bulges={0,0,0,0};return p;
}
// Obstacle section for the detail: exact bulge circle for a perpendicular round duct,
// rectangle or polygonal ellipse otherwise (same shape as the checked section).
inline DetailPath detailObstacle(const Frame& f,double insulation,bool iso){
 if(f.obstacleRound&&f.crossingSine()==1){const double r=f.obstacleWidth/2+insulation;
  DetailPath p;p.member=7;p.insulation=iso;p.axis=false;p.closed=true;
  p.points={Vertex(-r,f.obstacleZ),Vertex(r,f.obstacleZ)};p.bulges={1,1};return p;}
 DetailPath p;p.member=7;p.insulation=iso;p.axis=false;p.closed=true;
 for(const auto& q:obstacleSection(f,insulation)){p.points.push_back(Vertex(q.first,q.second));p.bulges.push_back(0);}
 return p;
}
// All internal coordinates/annotations are mm; only the final DWG adapter divides by factor.
inline SideDetail sideDetail(const View& v,const Options& o,const std::string& label,bool detailed,double textMm,double arrowMm){
 if(!v.ready||!v.frame.valid||v.axis.size()!=8||v.detailPaths.size()!=21||!detailLabelValid(label)||
 !(v.frame.factor==1||v.frame.factor==10||v.frame.factor==1000)||!finite(textMm)||!finite(arrowMm)||
 textMm<.5||textMm>100||arrowMm<.5||arrowMm>100)throw std::runtime_error("Invalid detail snapshot/options");
 SideDetail d;d.textHeight=textMm;d.arrowSize=arrowMm;d.factor=v.frame.factor;
 d.paths=v.detailPaths;
 d.paths.push_back(detailObstacle(v.frame,0,false));
 d.paths.push_back(detailObstacle(v.frame,o.obstacleInsulation,true));
 // Translate the absolute physical elevation datum to the local drawing origin. Geometry stays 1:1.
 d.minS=d.maxS=0;d.minZ=d.maxZ=0;
 for(auto& p:d.paths)for(auto& x:p.points){x.s-=v.axis.front().s;x.z-=v.frame.z0;
  d.minS=(std::min)(d.minS,x.s);d.maxS=(std::max)(d.maxS,x.s);d.minZ=(std::min)(d.minZ,x.z);d.maxZ=(std::max)(d.maxZ,x.z);}
 // A bulge circle has only two vertices: include the true obstacle extent (with insulation).
 for(const auto& q:obstacleSection(v.frame,o.obstacleInsulation)){const double s=q.first-v.axis.front().s,z=q.second-v.frame.z0;
  d.minS=(std::min)(d.minS,s);d.maxS=(std::max)(d.maxS,s);d.minZ=(std::min)(d.minZ,z);d.maxZ=(std::max)(d.maxZ,z);}
 auto local=[&](Vertex p){return Vertex(p.s-v.axis.front().s,p.z-v.frame.z0);};
 // Drawing-office layout: smaller dimensions next to the part, larger outside; texts never
 // on the parts; labels next to their piece without leaders; notes as a block BELOW.
 const double th=textMm,gapDim=(std::max)(3.5*th,2.5*arrowMm),half=v.frame.height/2+o.insulation;
 // Level marks (absolute elevations in drawing units, like TVS "Kota gornje/donje ivice"):
 // bare duct top/bottom at start, passage and end; obstacle top/bottom. Apex on the level,
 // triangle on the side of the text, shelf under/over the text; start marks left of the part,
 // end marks right of it, passage and obstacle marks on their piece.
 double levelMinS=d.minS,levelMaxS=d.maxS,levelMinZ=d.minZ;
 // On a piece the stem crosses its insulation so the shelf and text lie outside the part.
 auto level=[&](double sa,double z,double absolute,int hs,bool up,double from,double insulation){
  const int decimals=v.frame.factor==1000?3:v.frame.factor==10?1:0;const std::string text=displayNumber(absolute/v.frame.factor,decimals);
  const double u=up?1:-1,tri=.9*th,w=.55*th,textW=.9*th*double(text.size()),stem=(std::max)(tri,insulation+.4*th),
   shelf=sa+hs*(w+.3*th+textW);
  DetailPath p;p.member=9;p.insulation=false;p.axis=false;p.closed=false;p.points={Vertex(from,z),Vertex(sa+hs*w,z)};p.bulges={0,0};d.paths.push_back(p);
  p.closed=true;p.points={Vertex(sa,z),Vertex(sa-w,z+u*tri),Vertex(sa+w,z+u*tri)};p.bulges={0,0,0};d.paths.push_back(p);
  p.closed=false;p.points={Vertex(sa,z+u*tri),Vertex(sa,z+u*stem)};p.bulges={0,0};if(stem>tri)d.paths.push_back(p);
  p.points={Vertex(sa-hs*w,z+u*stem),Vertex(shelf,z+u*stem)};d.paths.push_back(p);
  const double x=hs>0?sa+w+.3*th:sa-w-.3*th-textW,top=up?z+stem+.35*th+th:z-stem-.35*th;
  d.levelTexts.push_back(DetailNote{Vertex(x,top),text});
  levelMinS=(std::min)(levelMinS,(std::min)(x,from));levelMaxS=(std::max)(levelMaxS,(std::max)(x+textW,shelf));levelMinZ=(std::min)(levelMinZ,top-th);
 };
 {const double bare=v.frame.height/2,zs=v.axis.front().z,ze=v.axis.back().z,lS=v.axis.back().s-v.axis.front().s,
   hLen=v.axis[4].s-v.axis[3].s,hAt=(v.axis[3].s+v.axis[4].s)/2-v.axis.front().s+.25*hLen;
  for(int k=0;k<2;++k){const double e=k?-bare:bare;const bool up=k==0;
   level(-2*th,zs+e-v.frame.z0,zs+e,-1,up,0,0);
   level(lS+2*th,ze+e-v.frame.z0,ze+e,1,up,lS,0);
   level(hAt,v.zPass+e-v.frame.z0,v.zPass+e,1,up,hAt-.55*th,o.insulation);}
  // Obstacle top/bottom on the obstacle, left of its centre with the text to the left (the
  // passage marks sit right of the centre, the gap witness at the centre); the triangle and
  // text are always outside the obstacle (above its top, below its bottom).
  double oMinS=1e300,oMaxS=-1e300,oMinZ=1e300,oMaxZ=-1e300;
  for(const auto& q:obstacleSection(v.frame,0)){const double t=q.first-v.axis.front().s;
   oMinS=(std::min)(oMinS,t);oMaxS=(std::max)(oMaxS,t);oMinZ=(std::min)(oMinZ,q.second);oMaxZ=(std::max)(oMaxZ,q.second);}
  if(v.frame.obstacleRound&&v.frame.crossingSine()==1){oMinZ=v.frame.obstacleZ-v.frame.obstacleWidth/2;oMaxZ=v.frame.obstacleZ+v.frame.obstacleWidth/2;}
  const double oc=(oMinS+oMaxS)/2,oAt=oc-.25*(oMaxS-oMinS);
  level(oAt,oMaxZ-v.frame.z0,oMaxZ,-1,true,oc,o.obstacleInsulation);
  level(oAt,oMinZ-v.frame.z0,oMinZ,-1,false,oc,o.obstacleInsulation);
 }
 auto dim=[&](Vertex a,Vertex b,Vertex rail,const char* prefix){if(std::hypot(a.s-b.s,a.z-b.z)>1e-8)d.dimensions.push_back(DetailDimension{a,b,rail,prefix});};
 const double rowH=levelMinZ-gapDim,rowW=rowH-gapDim;
 dim(local(v.axis.front()),local(v.axis.back()),Vertex(v.W/2,rowW),"W = ");
 dim(local(v.axis[3]),local(v.axis[4]),Vertex((v.axis[3].s+v.axis[4].s)/2-v.axis.front().s,rowH),"H = ");
 dim(Vertex(0,0),Vertex(0,v.zPass-v.frame.z0),Vertex(levelMinS-gapDim,(v.zPass-v.frame.z0)/2),"D = ");
 // Outward side of a piece: away from the obstacle centre (free space beside the bypass).
 const Vertex obstacleCentre(-v.axis.front().s,v.frame.obstacleZ-v.frame.z0);
 auto outward=[&](Vertex a,Vertex b,double& nx,double& nz){double dx=b.s-a.s,dz=b.z-a.z,len=std::hypot(dx,dz);
  nx=-dz/len;nz=dx/len;const Vertex m((a.s+b.s)/2,(a.z+b.z)/2);if((obstacleCentre.s-m.s)*nx+(obstacleCentre.z-m.z)*nz>0){nx=-nx;nz=-nz;}};
 if(detailed){
  // Physical slope lengths, aligned and just outside each slope.
  for(int j=0;j<2;++j){Vertex a=local(v.axis[j==0?1:5]),b=local(v.axis[j==0?2:6]);double nx,nz;outward(a,b,nx,nz);
   const double off=half+gapDim;dim(a,b,Vertex((a.s+b.s)/2+nx*off,(a.z+b.z)/2+nz*off),j==0?"S1 = ":"S2 = ");}
 }
 double row=rowW-2.5*th;auto note=[&](const std::string& s){d.notes.push_back(DetailNote{Vertex(d.minS,row),s});row-=1.7*th;};
 const double crossingDeg=std::asin((std::min)(1.0,v.frame.crossingSine()))*180/3.141592653589793;
 note(label+" - OBILAZAK KANALA, prolaz "+(o.above?"iznad":"ispod")+" prepreke");
 note((v.frame.routeRound?"Kanal fi "+displayNumber(v.frame.width):"Kanal A x B = "+displayNumber(v.frame.width)+" x "+displayNumber(v.frame.height))+" mm;  prepreka "+
  (v.frame.obstacleRound?"fi "+displayNumber(v.frame.obstacleWidth):displayNumber(v.frame.obstacleWidth)+" x "+displayNumber(v.frame.obstacleHeight))+" mm"+
  (v.frame.crossingSine()<1?";  ukrstanje "+displayNumber(crossingDeg)+" deg":""));
 note("Koljena K1-K4: "+displayNumber(double(o.angle),0)+" deg, Ri = "+displayNumber(o.ri)+" mm;  izolacija kanal / prepreka = "+displayNumber(o.insulation)+" / "+displayNumber(o.obstacleInsulation)+" mm");
 note("Kote ose: pocetak "+displayNumber(v.axis.front().z)+", prolaz "+displayNumber(v.zPass)+", kraj "+displayNumber(v.axis.back().z)+" mm (apsolutno)");
 note("Fizicke duzine: S1 = "+displayNumber(v.slopePhysical[0])+",  H = "+displayNumber(v.C)+",  S2 = "+displayNumber(v.slopePhysical[1])+" mm");
 note((exactGap(v)?"Najmanji razmak = "+displayGap(v):"Garantovana donja granica razmaka = "+guaranteedGap(v)+" (konzervativno)")
  +";  zahtjev "+displayNumber(o.clearance)+" + rezerva "+displayNumber(o.reserve)+" mm");
 if(!exactGap(v))note("Nacrtan je stvarni presjek prepreke u ravni trase; provjera koristi siri konzervativni gabarit.");
 note(std::string("Visinske kote: ivice kanala bez izolacije i prepreke, apsolutno, u jedinicama crteza (")+(v.frame.factor==1000?"m":v.frame.factor==10?"cm":"mm")+").");
 note("Staticki snimak, kote neasocijativne, mjere u mm.");
 if(detailed){
  DetailPath witness;witness.member=9;witness.insulation=false;witness.axis=false;witness.closed=false;
  witness.points={local(v.witnessBody),local(v.witnessObstacle)};witness.bulges={0,0};d.paths.push_back(witness);
  // Elbow labels beside each elbow, on its outward side, no leaders.
  const char* names[]={"K1","K2","K3","K4"};const int index[]={0,2,4,6};
  for(int j=0;j<4;++j){Vertex a=local(v.axis[index[j]]),b=local(v.axis[index[j]+1]);double nx,nz;outward(a,b,nx,nz);
   const double off=half+1.2*th;d.notes.push_back(DetailNote{Vertex((a.s+b.s)/2+nx*off-.6*th,(a.z+b.z)/2+nz*off+.5*th),names[j]});}
 }
 return d;
}
}
#endif



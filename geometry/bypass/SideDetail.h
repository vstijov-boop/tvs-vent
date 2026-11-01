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
// All internal coordinates/annotations are mm; only the final DWG adapter divides by factor.
inline SideDetail sideDetail(const View& v,const Options& o,const std::string& label,bool detailed,double textMm,double arrowMm){
 if(!v.ready||!v.frame.valid||v.axis.size()!=8||v.detailPaths.size()!=21||!detailLabelValid(label)||
 !(v.frame.factor==1||v.frame.factor==10||v.frame.factor==1000)||!finite(textMm)||!finite(arrowMm)||
 textMm<.5||textMm>100||arrowMm<.5||arrowMm>100)throw std::runtime_error("Invalid detail snapshot/options");
 SideDetail d;d.textHeight=textMm;d.arrowSize=arrowMm;d.factor=v.frame.factor;
 d.paths=v.detailPaths;
 d.paths.push_back(detailRectangle(-v.frame.obstacleWidth/2,v.frame.obstacleWidth/2,v.frame.obstacleZ-v.frame.obstacleHeight/2,v.frame.obstacleZ+v.frame.obstacleHeight/2,false));
 d.paths.push_back(detailRectangle(-v.frame.obstacleWidth/2-o.obstacleInsulation,v.frame.obstacleWidth/2+o.obstacleInsulation,v.frame.obstacleZ-v.frame.obstacleHeight/2-o.obstacleInsulation,v.frame.obstacleZ+v.frame.obstacleHeight/2+o.obstacleInsulation,true));
 // Translate the absolute physical elevation datum to the local drawing origin. Geometry stays 1:1.
 d.minS=d.maxS=v.axis.front().s;d.minZ=d.maxZ=0;
 for(auto& p:d.paths)for(auto& x:p.points){x.s-=v.axis.front().s;x.z-=v.frame.z0;
  d.minS=(std::min)(d.minS,x.s);d.maxS=(std::max)(d.maxS,x.s);d.minZ=(std::min)(d.minZ,x.z);d.maxZ=(std::max)(d.maxZ,x.z);}
 auto local=[&](Vertex p){return Vertex(p.s-v.axis.front().s,p.z-v.frame.z0);};
 const double pad=(std::max)(12*textMm,6*arrowMm),pitch=(std::max)(8*textMm,4*arrowMm);
 const double bottom=d.minZ-pad,top=d.maxZ+pad;
 auto dim=[&](Vertex a,Vertex b,Vertex rail,const char* prefix){if(std::hypot(a.s-b.s,a.z-b.z)>1e-8)d.dimensions.push_back(DetailDimension{a,b,rail,prefix});};
 dim(local(v.axis.front()),local(v.axis.back()),Vertex(v.W/2,bottom),"W = ");
 dim(local(v.axis[3]),local(v.axis[4]),Vertex((v.axis[3].s+v.axis[4].s)/2-v.axis.front().s,bottom-pitch),"H = ");
 dim(Vertex(0,0),Vertex(0,v.zPass-v.frame.z0),Vertex(d.minS-pad,(v.zPass-v.frame.z0)/2),"D = ");
 if(detailed){
  // Put each slope dimension outside the supporting line of the complete section bounds.
  for(int j=0;j<2;++j){Vertex a=local(v.axis[j==0?1:5]),b=local(v.axis[j==0?2:6]);
   double dx=b.s-a.s,dz=b.z-a.z,len=std::hypot(dx,dz),nx=-dz/len,nz=dx/len;
   if((j==0&&nx>0)||(j==1&&nx<0)){nx=-nx;nz=-nz;}
   Vertex mid((a.s+b.s)/2,(a.z+b.z)/2);double support=0;
   for(double x:{d.minS,d.maxS})for(double z:{d.minZ,d.maxZ})support=(std::max)(support,(x-mid.s)*nx+(z-mid.z)*nz);
   Vertex rail(mid.s+nx*(support+3*pad),mid.z+nz*(support+3*pad));
   dim(a,b,rail,j==0?"S1 = ":"S2 = ");
  }
 }
 double row=top+10*pitch;auto note=[&](const std::string& s){d.notes.push_back(DetailNote{Vertex(d.minS,row),s});row+=2.8*textMm;};
 note(label+" - Obilazak kanala, prolaz "+(o.above?"iznad":"ispod"));
 note("STATICKI SNIMAK; bez zive veze. Pogled: +s desno, fizicki +Z gore; papirni XY nije WCS Z.");
 note("Sve kote/oznake u mm; geometrija 1:1 u DWG "+std::string(d.factor==1?"mm":d.factor==10?"cm":"m")+". Kote nisu asocijativne.");
 note("Z pocetak = "+displayNumber(v.axis.front().z)+"; Z kraj = "+displayNumber(v.axis.back().z)+"; Z prolaz = "+displayNumber(v.zPass)+" mm (apsolutni datum)");
 note("Kanal A x B = "+displayNumber(v.frame.width)+" x "+displayNumber(v.frame.height)+"; prepreka = "+displayNumber(v.frame.obstacleWidth)+" x "+displayNumber(v.frame.obstacleHeight)+" mm");
 note("K1-K4: ugao = "+displayNumber(double(o.angle),0)+" deg; Ri = "+displayNumber(o.ri)+" mm; izolacije kanal/prepreka = "+displayNumber(o.insulation)+" / "+displayNumber(o.obstacleInsulation)+" mm");
 note("Komadi redom: K1 / S1 / K2 / H / K3 / S2 / K4");
 note("S1 = "+displayNumber(v.slopePhysical[0])+"; H = "+displayNumber(v.C)+"; S2 = "+displayNumber(v.slopePhysical[1])+" mm (fizicke duzine)");
 note("Kriticni geometrijski razmak = "+displayGap(v)+"; zahtjev = "+displayNumber(o.clearance)+"; rezerva = "+displayNumber(o.reserve)+" mm");
 double overlapLeft=(std::max)(v.axis[3].s,-v.frame.obstacleWidth/2-o.obstacleInsulation);
 double overlapRight=(std::min)(v.axis[4].s,v.frame.obstacleWidth/2+o.obstacleInsulation);
 const double gap=std::fabs(v.zPass-v.frame.obstacleZ)-v.frame.height/2-o.insulation-v.frame.obstacleHeight/2-o.obstacleInsulation;
 note(overlapLeft<=overlapRight?"Vertikalni zazor H/prepreka u horizontalnom preklopu = "+displayNumber(gap)+" mm (nije geometrijski minimum)":"Vertikalni zazor H/prepreka: nema horizontalnog preklopa; nije definisan.");
 if(detailed){
  DetailPath witness;witness.member=9;witness.insulation=false;witness.axis=false;witness.closed=false;
  witness.points={local(v.witnessBody),local(v.witnessObstacle)};witness.bulges={0,0};d.paths.push_back(witness);
  const char* names[]={"K1","S1","K2","H","K3","S2","K4"};
  // Each member gets a separate row; leaders run to checked centerline ports, never a guessed silhouette.
  for(int j=0;j<7;++j){Vertex a=local(v.axis[j]),b=local(v.axis[j+1]);
   Vertex endpoint((a.s+b.s)/2,(a.z+b.z)/2);Vertex tag(d.maxS+3*pad,top+j*pitch);
   d.notes.push_back(DetailNote{tag,names[j]});DetailPath leader;leader.member=9;leader.insulation=false;leader.axis=false;leader.closed=false;
   leader.points={endpoint,Vertex(tag.s-pad/2,tag.z),tag};leader.bulges={0,0,0};d.paths.push_back(leader);
  }
 }
 return d;
}
}
#endif


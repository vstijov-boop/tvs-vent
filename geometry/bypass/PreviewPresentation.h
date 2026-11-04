#ifndef TVS_BYPASS_PREVIEW_PRESENTATION_H
#define TVS_BYPASS_PREVIEW_PRESENTATION_H
#include "PreviewModel.h"
#include <sstream>
#include <iomanip>
#include <locale>
#include <limits>
#include <algorithm>
namespace tvsBypassPreview {
struct PixelRect {int left,top,right,bottom;};
struct SectionLayout {PixelRect area,plot;std::vector<PixelRect> labels;};
inline SectionLayout sectionLayout(double sx,double sy,bool dimensions,bool advanced=false){
 SectionLayout d;d.area=PixelRect{int((advanced?590:470)*sx),int(136*sy),int(1180*sx),int(546*sy)};
 d.plot=d.area;d.plot.left+=int(55*sx);d.plot.right-=int(20*sx);d.plot.top+=int(30*sy);d.plot.bottom-=int((dimensions?127:30)*sy);
 if(dimensions){int cw=(d.area.right-d.area.left)/2,h=int(18*sy),rowH=int(18*sy);
  d.labels.push_back(PixelRect{d.area.left,d.area.top,d.area.left+cw,d.area.top+h});
  d.labels.push_back(PixelRect{d.area.left+cw,d.area.top,d.area.right,d.area.top+h});
  int wy=d.plot.bottom+int(18*sy),cy=d.plot.bottom+int(42*sy);
  d.labels.push_back(PixelRect{d.plot.left,wy-h,d.plot.right,wy});
  d.labels.push_back(PixelRect{d.area.left,cy-h,d.area.right,cy});
  for(int j=0;j<12;++j){int colW=(d.area.right-d.area.left)/3,x=d.area.left+(j%3)*colW,y=d.area.bottom-4*rowH+(j/3)*rowH;
   d.labels.push_back(PixelRect{x,y,x+colW,y+rowH});}
 }
 return d;
}
inline bool overlap(PixelRect a,PixelRect b){return a.left<b.right&&b.left<a.right&&a.top<b.bottom&&b.top<a.bottom;}
inline std::string displayNumber(double x,int decimals=1){std::ostringstream s;s.imbue(std::locale::classic());s<<std::fixed<<std::setprecision(decimals)<<x;return s.str();}
inline std::string memberName(int n){const char* names[]={"prvo koljeno","prvi nagibni komad","drugo koljeno","srednji komad","treće koljeno","drugi nagibni komad","četvrto koljeno","ulazni susjed","izlazni susjed"};return n>=0&&n<9?names[n]:"nije dostupan";}
// Display-only interval enclosure at 0.01 mm, never fed back to the checker.
// Perpendicular crossing: the checked section distance IS the 3D distance (prismatic obstacle,
// tube/slab cut through its axis). Oblique: only sin(beta) x section distance to the conservative
// checking envelope is guaranteed; never present it as a measured minimum.
inline bool exactGap(const View& v){return v.frame.crossingSine()==1;}
inline std::string guaranteedGap(const View& v){
 return displayNumber(std::fmax(0.0,std::floor(std::nextafter(v.frame.crossingSine()*v.gapLo*100,-std::numeric_limits<double>::infinity()))/100),2)+" mm";}
inline std::string displayGap(const View& v){return "["+displayNumber(std::fmax(0.0,std::floor(std::nextafter(v.gapLo*100,-std::numeric_limits<double>::infinity()))/100),2)+", "+displayNumber(std::ceil(std::nextafter(v.gapHi*100,std::numeric_limits<double>::infinity()))/100,2)+"] mm";}
inline std::string summary(const View& v,const Options& o){
 if(!v.ready)return v.status+"\r\nOtvorite Napredno / dijagnostika za razlog i brojače.";
 return v.status+"\r\nOsa prolaza: Z = "+displayNumber(v.zPass)+" mm   |   Zahvat W = "+displayNumber(v.W)+" mm"+
 (exactGap(v)?"\r\nNajmanji razmak (presjek u ravni trase = prostorni): "+displayGap(v):
  "\r\nGarantovana DONJA granica prostornog razmaka: "+guaranteedGap(v)+" (sin x razmak do provjernog gabarita; nije izmjereni minimum)")+
 "\r\nZahtijevan razmak: "+displayNumber(o.clearance)+" mm   |   Dodatna rezerva: "+displayNumber(o.reserve)+" mm"+
 "\r\nKritični član: "+memberName(v.criticalRole)+". Razmak se mjeri između spoljnih gabarita, sa izolacijom.";
}
struct Caption {std::string text;int color;Caption(std::string s,int c=0):text(s),color(c){}};
inline std::vector<Caption> captions(const View& v,const Options& o){
 std::vector<Caption> a;if(!v.ready||v.axis.size()!=8)return a;
 a.push_back(Caption("Osa prolaza Z="+displayNumber(v.zPass)+" mm"));
 a.push_back(Caption("Prepreka: visina="+displayNumber(v.frame.obstacleHeight)+" mm",1));
 a.push_back(Caption("Kanal: visina="+displayNumber(v.frame.height)+" mm"));
 a.push_back(Caption("Prepreka vrh/dno: "+displayNumber(v.frame.obstacleZ+v.frame.obstacleHeight/2+o.obstacleInsulation)+" / "+displayNumber(v.frame.obstacleZ-v.frame.obstacleHeight/2-o.obstacleInsulation),1));
 a.push_back(Caption("Koljeno: "+displayNumber(double(o.angle),0)+"°; Ri="+displayNumber(o.ri)+" mm"));
 a.push_back(Caption("Prolaz vrh/dno: "+displayNumber(v.zPass+v.frame.height/2+o.insulation)+" / "+displayNumber(v.zPass-v.frame.height/2-o.insulation)));
 a.push_back(Caption("Nagib 1: L="+displayNumber(v.slopePhysical[0])+"; proj="+displayNumber(v.slopeProjected[0])+" mm"));
 a.push_back(Caption("Nagib 2: L="+displayNumber(v.slopePhysical[1])+"; proj="+displayNumber(v.slopeProjected[1])+" mm"));
 a.push_back(Caption("Zeleni spoj: geometrijski razmak",2));
 a.push_back(Caption("D je otklon ose; nije zazor.",2));
 a.push_back(Caption(o.useFloor?"Donja granica Z="+displayNumber(o.floor)+" mm":"Donja granica: neaktivna",3));
 a.push_back(Caption(o.useCeiling?"Gornja granica Z="+displayNumber(o.ceiling)+" mm":"Gornja granica: neaktivna",3));
 return a;
}
// Conservative diagnostic top projection; NO hidden-line/occlusion classification.
inline bool showCurve(const Curve& c,bool allEdges,bool axes){
 return c.kind==PhysicalOutline || (c.kind==AxisLine&&axes) || (allEdges&&(c.kind==JointSymbol||c.kind==DiagnosticLine));
}
inline bool sameSegment(Vertex a,Vertex b,Vertex c,Vertex d){
 return (a.s==c.s&&a.z==c.z&&b.s==d.s&&b.z==d.z)||(a.s==d.s&&a.z==d.z&&b.s==c.s&&b.z==c.z);
}
struct PlanLine {Vertex a,b;int kind,member;bool insulation;};
inline std::vector<PlanLine> planLines(const View& v,bool allEdges,bool axes){
 std::vector<PlanLine> out;
 // Physical edges first; exact coincident diagnostics never override a physical contour.
 for(int pass=0;pass<2;++pass)for(const auto& c:v.plan){
  if(!showCurve(c,allEdges,axes)||(c.kind==PhysicalOutline)!=(pass==0))continue;
  for(size_t j=1;j<c.points.size();++j){Vertex a=c.points[j-1],b=c.points[j];if(a.s==b.s&&a.z==b.z)continue;
   bool duplicate=false;for(const auto& l:out)if(sameSegment(a,b,l.a,l.b)){duplicate=true;break;}
   if(!duplicate)out.push_back(PlanLine{a,b,c.kind,c.member,c.insulation});
  }
 }
 std::stable_sort(out.begin(),out.end(),[](const PlanLine& a,const PlanLine& b){
  int pa=a.kind==PhysicalOutline?(a.insulation?1:2):0,pb=b.kind==PhysicalOutline?(b.insulation?1:2):0;return pa<pb;
 });
 return out;
}
}
#endif

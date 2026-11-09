// TVS_PRESJEK dialog preview as SVG (visual check): g++ -std=c++14 tests/section_sketch_svg.cpp -o s && ./s dir
#include "../geometry/SectionSketch.h"
#include <cstdio>
#include <string>
using namespace tvsSectionSketch;
static std::string hex(unsigned c){if(c==tvsTeeSketch::none)return "none";char b[8];std::snprintf(b,8,"#%06X",c&0xFFFFFF);return b;}
static void svg(const Input& in,const std::string& file){
    std::vector<Prim> p;build(in,p);double x0,y0,x1,y1;tvsTeeSketch::bounds(p,x0,y0,x1,y1);
    const double W=280,H=200,m=10,s=std::fmin((W-2*m)/(x1-x0),(H-2*m)/(y1-y0));
    const double ox=(W-(x1-x0)*s)/2-x0*s,oy=(H-(y1-y0)*s)/2-y0*s;
    FILE* f=std::fopen(file.c_str(),"w");if(!f)return;
    std::fprintf(f,"<svg xmlns='http://www.w3.org/2000/svg' width='%g' height='%g' font-family='Segoe UI,Arial' font-size='11'><rect width='%g' height='%g' fill='white' stroke='#d2deeb'/>",W,H,W,H);
    for(size_t i=0;i<p.size();++i){const Prim& q=p[i];auto X=[&](double x){return ox+x*s;};auto Y=[&](double y){return oy+y*s;};
        if(q.kind==tvsTeeSketch::Polygon||q.kind==tvsTeeSketch::Polyline){if(q.stroke==tvsTeeSketch::none&&q.kind==tvsTeeSketch::Polyline)continue;std::string pts;for(size_t j=0;j<q.pts.size();++j){char b[40];std::snprintf(b,40,"%.1f,%.1f ",X(q.pts[j].x),Y(q.pts[j].y));pts+=b;}
            std::fprintf(f,"<%s points='%s' fill='%s' stroke='%s' stroke-width='%g'%s/>",q.kind==tvsTeeSketch::Polygon?"polygon":"polyline",pts.c_str(),q.kind==tvsTeeSketch::Polygon?hex(q.fill).c_str():"none",hex(q.stroke).c_str(),q.width,q.dash?" stroke-dasharray='5 4'":"");}
        else if(q.kind==tvsTeeSketch::Circle)std::fprintf(f,"<circle cx='%.1f' cy='%.1f' r='%.1f' fill='%s' stroke='%s'/>",X(q.pts[0].x),Y(q.pts[0].y),q.scaled?q.r*s:q.r,hex(q.fill).c_str(),hex(q.stroke).c_str());
        else{std::string t(q.text.begin(),q.text.end());std::fprintf(f,"<text x='%.1f' y='%.1f' dy='4' fill='%s' text-anchor='%s'>%s</text>",X(q.pts[0].x)-(q.align==2?4:0),Y(q.pts[0].y),hex(q.fill).c_str(),q.align==2?"end":"start",t.c_str());}}
    std::fprintf(f,"</svg>");std::fclose(f);
}
int main(int argc,char** argv){
    const std::string dir=argc>1?argv[1]:".";
    Input a;a.L=24000;a.H=3200;a.floorM=0;a.ceilingM=3.2;
    a.boxes.push_back({11200,2700,600,300,false});a.boxes.push_back({12500,2750,250,250,true});a.boxes.push_back({13800,2300,400,200,false});
    svg(a,dir+"/1_poprecni.svg");
    a.sp=600;a.spM=0.6;svg(a,dir+"/2_poprecni_sp.svg");
    Input b;b.L=12000;b.lengthM=12;b.H=3200;b.floorM=0;b.ceilingM=3.2;b.sp=500;b.spM=0.5;
    std::vector<Pt> band={Pt(0,2750),Pt(5000,2750),Pt(5000,2450),Pt(0,2450)};b.bands.push_back(band);
    band={Pt(5000,2750),Pt(5600,2350),Pt(5600,2100),Pt(5000,2450)};b.bands.push_back(band);
    band={Pt(5600,2350),Pt(12000,2350),Pt(12000,2100),Pt(5600,2100)};b.bands.push_back(band);
    svg(b,dir+"/3_uzduzni_sp.svg");
    Input c;c.L=8000;c.H=3200;c.ceilingM=3.2;svg(c,dir+"/4_prazno.svg");
    return 0;
}

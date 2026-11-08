// Writes the TVS_TK_TIP sketch of many cases as SVG (visual check, no AutoCAD):
//   g++ -std=c++14 tests/tee_sketch_svg.cpp -o sketch && ./sketch out_dir
#include "../geometry/TeeSketch.h"
#include <cstdio>
#include <string>
using namespace tvsTeeSketch;
static std::string hex(unsigned c){if(c==none)return "none";char b[8];std::snprintf(b,8,"#%06X",c&0xFFFFFF);return b;}
static void svg(const Input& in,const std::string& file,const std::string& title){
    std::vector<Prim> p;build(in,p);double x0,y0,x1,y1;bounds(p,x0,y0,x1,y1);
    const double W=280,H=210,m=12,s0=std::fmin((W-2*m)/(x1-x0),(H-2*m)/(y1-y0)),s=s0>2.2?2.2:s0;
    const double ox=(W-(x1-x0)*s)/2-x0*s,oy=(H-(y1-y0)*s)/2-y0*s;
    FILE* f=std::fopen(file.c_str(),"w");if(!f)return;
    std::fprintf(f,"<svg xmlns='http://www.w3.org/2000/svg' width='%g' height='%g' font-family='Segoe UI,Arial' font-size='11'>",W,H+16);
    std::fprintf(f,"<rect width='%g' height='%g' fill='white' stroke='#d2deeb'/>",W,H);
    for(size_t i=0;i<p.size();++i){const Prim& q=p[i];
        auto X=[&](double x){return ox+x*s;};auto Y=[&](double y){return oy+y*s;};
        if(q.kind==Polygon||q.kind==Polyline){std::string pts;for(size_t j=0;j<q.pts.size();++j){char b[40];std::snprintf(b,40,"%.1f,%.1f ",X(q.pts[j].x),Y(q.pts[j].y));pts+=b;}
            std::fprintf(f,"<%s points='%s' fill='%s' stroke='%s' stroke-width='%g'%s/>",q.kind==Polygon?"polygon":"polyline",pts.c_str(),q.kind==Polygon?hex(q.fill).c_str():"none",hex(q.stroke).c_str(),q.width,q.dash?" stroke-dasharray='4 4'":"");}
        else if(q.kind==Circle)std::fprintf(f,"<circle cx='%.1f' cy='%.1f' r='%.1f' fill='%s' stroke='%s'/>",X(q.pts[0].x),Y(q.pts[0].y),q.r*(s>1.4?1.4:s),hex(q.fill).c_str(),hex(q.stroke).c_str());
        else{std::string t(q.text.begin(),q.text.end());std::fprintf(f,"<text x='%.1f' y='%.1f' fill='%s' text-anchor='%s'%s>%s</text>",X(q.pts[0].x),Y(q.pts[0].y),hex(q.fill).c_str(),q.align==2?"end":q.align==1?"middle":"start",q.bold?" font-weight='bold'":"",t.c_str());}
    }
    std::fprintf(f,"<text x='4' y='%g' font-size='10' fill='#555'>%s</text></svg>",H+12,title.c_str());std::fclose(f);
}
int main(int argc,char** argv){
    const std::string dir=argc>1?argv[1]:".";const char* types[4]={"TKO","TKG","Stucna","TK0"};int n=0;
    for(int t=0;t<4;++t)for(int sel=0;sel<2;++sel){if((t==Tap||t==Plain)&&sel==1)continue;
        for(int c=0;c<7;++c){Input in;in.type=t;in.selected=sel;in.side[0].used=true;in.side[1].used=t==Branch||t==Pants;
            in.side[1].hasDuct=true;in.side[0].hasDuct=true;Side& s=in.side[sel];
            std::string what;
            switch(c){case 0:what="bez";break;case 1:s.prelaz=true;what="prelaz";break;case 2:s.elbow=90;s.turn=Right;what="k90desno";break;
                case 3:s.elbow=45;s.turn=Left;s.prelaz=true;what="k45lijevo_prelaz";break;case 4:s.elbow=60;s.turn=Right;s.prelaz=true;s.order=1;what="prelaz_k60";break;
                case 5:s.elbow=90;s.turn=Up;s.hasDuct=false;what="gore";break;default:s.elbow=90;s.turn=Right;in.flipped=true;what="okrenut_k90desno";}
            char name[200];std::snprintf(name,200,"%s/%02d_%s_%s_%s.svg",dir.c_str(),n++,types[t],sel?"C":"B",what.c_str());
            svg(in,name,std::string(types[t])+(sel?" C ":" B ")+what);}}
    return 0;
}

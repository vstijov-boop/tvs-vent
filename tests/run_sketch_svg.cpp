// TVS_DIM_PRATI dialog preview as SVG (visual check): g++ -std=c++14 tests/run_sketch_svg.cpp -o s && ./s dir
#include "../geometry/RunSketch.h"
#include <cstdio>
#include <string>
using namespace tvsRunSketch;
static std::string hex(unsigned c){if(c==tvsTeeSketch::none)return "none";char b[8];std::snprintf(b,8,"#%06X",c&0xFFFFFF);return b;}
static void svg(const Input& in,const std::string& file){
    std::vector<Prim> p;build(in,p);double x0,y0,x1,y1;tvsTeeSketch::bounds(p,x0,y0,x1,y1);
    const double W=280,H=200,m=8,s=std::fmin((W-2*m)/(x1-x0),(H-2*m)/(y1-y0));
    const double ox=(W-(x1-x0)*s)/2-x0*s,oy=(H-(y1-y0)*s)/2-y0*s;
    FILE* f=std::fopen(file.c_str(),"w");if(!f)return;
    std::fprintf(f,"<svg xmlns='http://www.w3.org/2000/svg' width='%g' height='%g' font-family='Segoe UI,Arial' font-size='11'><rect width='%g' height='%g' fill='white' stroke='#d2deeb'/>",W,H,W,H);
    for(size_t i=0;i<p.size();++i){const Prim& q=p[i];auto X=[&](double x){return ox+x*s;};auto Y=[&](double y){return oy+y*s;};
        if(q.kind==tvsTeeSketch::Polygon||q.kind==tvsTeeSketch::Polyline){if(q.stroke==tvsTeeSketch::none&&q.kind==tvsTeeSketch::Polyline)continue;std::string pts;for(size_t j=0;j<q.pts.size();++j){char b[40];std::snprintf(b,40,"%.1f,%.1f ",X(q.pts[j].x),Y(q.pts[j].y));pts+=b;}
            std::fprintf(f,"<polygon points='%s' fill='%s' stroke='%s' stroke-width='%g'/>",pts.c_str(),hex(q.fill).c_str(),hex(q.stroke).c_str(),q.width);}
        else if(q.kind==tvsTeeSketch::Circle)std::fprintf(f,"<circle cx='%.1f' cy='%.1f' r='%.1f' fill='%s' stroke='%s'/>",X(q.pts[0].x),Y(q.pts[0].y),q.r,hex(q.fill).c_str(),hex(q.stroke).c_str());
        else{std::string t(q.text.begin(),q.text.end());std::fprintf(f,"<text x='%.1f' y='%.1f' dy='4' fill='%s' text-anchor='middle'>%s</text>",X(q.pts[0].x),Y(q.pts[0].y),hex(q.fill).c_str(),t.c_str());}}
    std::fprintf(f,"</svg>");std::fclose(f);
}
static Element duct(double ax,double ay,double bx,double by,double w0,double w1,int st){Element e;e.kind=1;e.p[0]=Pt(ax,ay);e.p[1]=Pt(bx,by);e.w[0]=w0;e.w[1]=w1;e.state=st;return e;}
int main(int argc,char** argv){
    const std::string dir=argc>1?argv[1]:".";
    // the user's case: duct 400 - prelaz - duct 350 (edited) - prelaz - TK-O (branch up) - duct - elbow up
    Input a;
    a.items.push_back(duct(0,0,3000,0,800,800,Context));a.items.push_back(duct(3000,0,3600,0,800,700,Context));
    a.items.push_back(duct(3600,0,8000,0,700,700,Selected));a.items.push_back(duct(8000,0,8600,0,700,600,Changes));
    {Element t;t.kind=4;t.p[0]=Pt(8600,0);t.p[1]=Pt(9900,0);t.p[2]=Pt(9250,900);t.w[0]=t.w[1]=600;t.w[2]=600;t.state=TeeDialog;a.items.push_back(t);}
    a.items.push_back(duct(9250,900,9250,3000,600,600,Context));
    a.items.push_back(duct(9900,0,13500,0,600,600,Context));
    {Element e;e.kind=2;e.p[0]=Pt(13500,0);e.p[1]=Pt(14400,900);e.dir[0]=Pt(1,0);e.dir[1]=Pt(0,-1);e.w[0]=e.w[1]=600;e.state=Context;a.items.push_back(e);}
    a.items.push_back(duct(14400,900,14400,3000,600,600,Context));
    {Mark m;m.at=Pt(3600,0);m.text=L"prelaz";a.marks.push_back(m);m.at=Pt(8600,0);m.text=L"T komad";a.marks.push_back(m);}
    svg(a,dir+"/run1.svg");
    // whole run with an elbow
    Input b;b.items.push_back(duct(0,0,20000,0,400,400,Selected));
    {Element e;e.kind=2;e.p[0]=Pt(20000,0);e.p[1]=Pt(20600,-600);e.dir[0]=Pt(1,0);e.dir[1]=Pt(0,1);e.w[0]=e.w[1]=400;e.state=Changes;b.items.push_back(e);}
    b.items.push_back(duct(20600,-600,20600,-9000,400,400,Changes));b.items.push_back(duct(20600,-9000,20600,-9500,400,300,Context));
    {Mark m;m.at=Pt(20600,-9000);m.text=L"prelaz";b.marks.push_back(m);}
    svg(b,dir+"/run2.svg");
    return 0;
}

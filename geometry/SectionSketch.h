#pragma once
// TVS_PRESJEK dialog preview: the section (poprecni: cut ducts as box with a cross / circle;
// uzduzni: the developed route as bands) between the floor and the floor above, with the
// optional spusteni plafon (dashed) and the levels in m. Pure geometry (y down, units as given),
// drawn by the dialog with the same renderer as the TVS_TK_TIP sketch (TeeSketch.h primitives).
#include "TeeSketch.h"
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
namespace tvsSectionSketch {
using tvsTeeSketch::Prim;using tvsTeeSketch::Pt;
struct Box {double s,z,w,h;bool round;};   // centre along the line, centre height above the floor
struct Input {
    double L,H,sp;double floorM,ceilingM,spM;std::vector<Box> boxes;std::vector<std::vector<Pt> > bands;   // bands: (s, height above floor)
    Input():L(0),H(0),sp(0),floorM(0),ceilingM(0),spM(0){}
};
inline std::wstring metres(double v){wchar_t s[32];std::swprintf(s,32,L"%.2f",v);std::wstring r(s);for(size_t i=0;i<r.size();++i)if(r[i]==L'.')r[i]=L',';return r;}
inline void build(const Input& in,std::vector<Prim>& out){
    out.clear();
    const double L=in.L>0?in.L:1,H=in.H>0?in.H:1,m=0.06*L+0.04*H;
    auto line=[&](double x0,double y0,double x1,double y1,unsigned c,bool dash,double w){Prim p;p.kind=tvsTeeSketch::Polyline;p.stroke=c;p.dash=dash;p.width=w;
        p.pts.push_back(Pt(x0,-y0));p.pts.push_back(Pt(x1,-y1));out.push_back(p);};
    auto text=[&](double x,double y,const std::wstring& t,unsigned c,int align,bool bold){Prim p;p.kind=tvsTeeSketch::Text;p.fill=c;p.text=t;p.align=align;p.bold=bold;
        p.pts.push_back(Pt(x,-y));out.push_back(p);};
    // floor and floor above, spusteni plafon dashed
    line(-m,0,L+m,0,0x8FA7BD,false,1.2);line(-m,H,L+m,H,0x8FA7BD,false,1.2);
    text(-m,0,metres(in.floorM),0x315A80,2,false);text(-m,H,metres(in.ceilingM),0x315A80,2,false);
    if(in.sp>0&&in.sp<H){line(-m,H-in.sp,L+m,H-in.sp,0x4788B9,true,1);text(L+m,H-in.sp+0.02*H,L"spušteni plafon "+metres(in.ceilingM-in.spM),0x2675AE,2,false);}
    // developed route (uzduzni)
    for(size_t i=0;i<in.bands.size();++i){if(in.bands[i].size()<3)continue;Prim p;p.kind=tvsTeeSketch::Polygon;p.fill=0xE3F0FC;p.stroke=0x0863A9;p.width=1.2;
        for(size_t j=0;j<in.bands[i].size();++j){p.pts.push_back(Pt(in.bands[i][j].x,-in.bands[i][j].y));}
        out.push_back(p);}
    // cut ducts (poprecni)
    for(size_t i=0;i<in.boxes.size();++i){const Box& b=in.boxes[i];
        if(b.round){Prim c;c.kind=tvsTeeSketch::Circle;c.pts.push_back(Pt(b.s,-b.z));c.r=b.w/2;c.scaled=true;c.fill=0xE3F0FC;c.stroke=0x0863A9;c.width=1.2;out.push_back(c);continue;}
        Prim r;r.kind=tvsTeeSketch::Polygon;r.fill=0xE3F0FC;r.stroke=0x0863A9;r.width=1.2;
        r.pts.push_back(Pt(b.s-b.w/2,-(b.z-b.h/2)));r.pts.push_back(Pt(b.s+b.w/2,-(b.z-b.h/2)));r.pts.push_back(Pt(b.s+b.w/2,-(b.z+b.h/2)));r.pts.push_back(Pt(b.s-b.w/2,-(b.z+b.h/2)));out.push_back(r);
        line(b.s-b.w/2,b.z-b.h/2,b.s+b.w/2,b.z+b.h/2,0x0863A9,false,1);line(b.s-b.w/2,b.z+b.h/2,b.s+b.w/2,b.z-b.h/2,0x0863A9,false,1);
    }
    text(L/2,-0.09*H,L"Kote u m",0x2675AE,1,false);
    // invisible frame so the fit keeps the margins
    Prim f;f.kind=tvsTeeSketch::Polyline;f.stroke=tvsTeeSketch::none;f.pts.push_back(Pt(-m*2.2,-(H+m)));f.pts.push_back(Pt(L+m,0.14*H));out.push_back(f);
}
}

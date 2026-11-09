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
    double L,H,sp;double floorM,ceilingM,spM,lengthM;std::vector<Box> boxes;std::vector<std::vector<Pt> > bands;   // bands: (s, height above floor)
    Input():L(0),H(0),sp(0),floorM(0),ceilingM(0),spM(0),lengthM(0){}
};
inline std::wstring metres(double v){wchar_t s[32];std::swprintf(s,32,L"%.2f",v);std::wstring r(s);for(size_t i=0;i<r.size();++i)if(r[i]==L'.')r[i]=L',';return r;}
inline void build(const Input& in,std::vector<Prim>& out){
    out.clear();
    const double L=in.L>0?in.L:1,H=in.H>0?in.H:1;
    // window along the line: poprecni = around the cut ducts (a long line would make them dots), uzduzni = whole route
    double x0=0,x1=L;
    if(!in.boxes.empty()){x0=1e300;x1=-1e300;
        for(size_t i=0;i<in.boxes.size();++i){const Box& b=in.boxes[i];if(b.s-b.w/2<x0)x0=b.s-b.w/2;if(b.s+b.w/2>x1)x1=b.s+b.w/2;}
        const double span=x1-x0,pad=(0.25*span>0.4*H?0.25*span:0.4*H);x0-=pad;x1+=pad;if(x0<0)x0=0;if(x1>L)x1=L;if(!(x1>x0)){x0=0;x1=L;}}
    const double W=x1-x0,m=0.05*W+0.03*H;
    auto line=[&](double ax,double ay,double bx,double by,unsigned c,bool dash,double w){Prim p;p.kind=tvsTeeSketch::Polyline;p.stroke=c;p.dash=dash;p.width=w;
        p.pts.push_back(Pt(ax,-ay));p.pts.push_back(Pt(bx,-by));out.push_back(p);};
    auto text=[&](double x,double y,const std::wstring& t,unsigned c,int align,bool bold){Prim p;p.kind=tvsTeeSketch::Text;p.fill=c;p.text=t;p.align=align;p.bold=bold;
        p.pts.push_back(Pt(x,-y));out.push_back(p);};
    // floor and floor above, spusteni plafon dashed
    line(x0-m,0,x1+m,0,0x8FA7BD,false,1.2);line(x0-m,H,x1+m,H,0x8FA7BD,false,1.2);
    text(x0-m,0,metres(in.floorM),0x315A80,2,false);text(x0-m,H,metres(in.ceilingM),0x315A80,2,false);
    if(in.sp>0&&in.sp<H){line(x0-m,H-in.sp,x1+m,H-in.sp,0x4788B9,true,1);text(x0-m,H-in.sp,L"SP "+metres(in.ceilingM-in.spM),0x2675AE,2,false);}
    // developed route (uzduzni), stations of its ends under the floor
    for(size_t i=0;i<in.bands.size();++i){if(in.bands[i].size()<3)continue;Prim p;p.kind=tvsTeeSketch::Polygon;p.fill=0xE3F0FC;p.stroke=0x0863A9;p.width=1.2;
        for(size_t j=0;j<in.bands[i].size();++j){p.pts.push_back(Pt(in.bands[i][j].x,-in.bands[i][j].y));}
        out.push_back(p);}
    if(!in.bands.empty()){text(0,-0.07*H,metres(0)+L" m",0x315A80,0,false);text(L,-0.07*H,metres(in.lengthM)+L" m",0x315A80,2,false);}
    // cut ducts (poprecni)
    for(size_t i=0;i<in.boxes.size();++i){const Box& b=in.boxes[i];
        if(b.round){Prim c;c.kind=tvsTeeSketch::Circle;c.pts.push_back(Pt(b.s,-b.z));c.r=b.w/2;c.scaled=true;c.fill=0xE3F0FC;c.stroke=0x0863A9;c.width=1.2;out.push_back(c);continue;}
        Prim r;r.kind=tvsTeeSketch::Polygon;r.fill=0xE3F0FC;r.stroke=0x0863A9;r.width=1.2;
        r.pts.push_back(Pt(b.s-b.w/2,-(b.z-b.h/2)));r.pts.push_back(Pt(b.s+b.w/2,-(b.z-b.h/2)));r.pts.push_back(Pt(b.s+b.w/2,-(b.z+b.h/2)));r.pts.push_back(Pt(b.s-b.w/2,-(b.z+b.h/2)));out.push_back(r);
        line(b.s-b.w/2,b.z-b.h/2,b.s+b.w/2,b.z+b.h/2,0x0863A9,false,1);line(b.s-b.w/2,b.z+b.h/2,b.s+b.w/2,b.z-b.h/2,0x0863A9,false,1);
    }
    if(in.boxes.empty()&&in.bands.empty())text((x0+x1)/2,H/2,L"Nema presječenih kanala",0xC53D3D,1,false);
    else if(in.bands.empty())text((x0+x1)/2,-0.08*H,L"Kote u m",0x2675AE,1,false);
    // invisible frame so the fit keeps the margins
    Prim f;f.kind=tvsTeeSketch::Polyline;f.stroke=tvsTeeSketch::none;f.pts.push_back(Pt(x0-m-0.22*W-0.1*H,-(H+0.05*H)));f.pts.push_back(Pt(x1+m,0.13*H));out.push_back(f);
}
}

#pragma once
// TVS_DIM_PRATI dialog "Promjena dimenzije dionice": plan of the run in true widths. Elements
// that change are blue, the edited duct dark blue, a T komad that opens its own dialog amber,
// the rest of the network around grey; short labels on the boundaries. Pure geometry (input in
// drawing coordinates, y up; output y down), drawn by SketchRender.h / tests/run_sketch_svg.cpp.
#include "TeeSketch.h"
#include <cmath>
#include <string>
#include <vector>
namespace tvsRunSketch {
using tvsTeeSketch::Prim;using tvsTeeSketch::Pt;
enum State {Context=0,Changes=1,Selected=2,TeeDialog=3};
struct Element {
    int kind;          // 1 duct / prelaz (p[0], p[1], widths w[0], w[1]), 2 plan elbow (dir = outward at the ports), 4 T komad (p[2], w[2] = branch)
    Pt p[3],dir[2];double w[3];int state;
    Element():kind(1),state(Context){w[0]=w[1]=w[2]=0;}
};
struct Mark {Pt at;std::wstring text;bool below;Mark():below(false){}};
struct Input {std::vector<Element> items;std::vector<Mark> marks;};
inline double len(const Pt& a,const Pt& b){return std::hypot(b.x-a.x,b.y-a.y);}
// Band round a centre line c with half widths h0 -> h1.
inline void band(const std::vector<Pt>& c,double h0,double h1,std::vector<Pt>& poly){
    poly.clear();const size_t n=c.size();if(n<2)return;std::vector<Pt> left(n),right(n);
    for(size_t i=0;i<n;++i){const Pt& a=c[i==0?0:i-1];const Pt& b=c[i+1<n?i+1:n-1];double dx=b.x-a.x,dy=b.y-a.y;const double l=std::hypot(dx,dy);
        if(l<1e-12){dx=1;dy=0;}else{dx/=l;dy/=l;}
        const double h=h0+(h1-h0)*(n>1?double(i)/(n-1):0);left[i]=Pt(c[i].x-dy*h,c[i].y+dx*h);right[i]=Pt(c[i].x+dy*h,c[i].y-dx*h);}
    for(size_t i=0;i<n;++i)poly.push_back(left[i]);for(size_t i=n;i-->0;)poly.push_back(right[i]);
}
inline void build(const Input& in,std::vector<Prim>& out){
    out.clear();if(in.items.empty())return;
    double x0=1e300,y0=1e300,x1=-1e300,y1=-1e300;
    for(size_t i=0;i<in.items.size();++i){const Element& e=in.items[i];const int n=e.kind==4?3:2;
        for(int k=0;k<n;++k){if(e.p[k].x<x0)x0=e.p[k].x;if(e.p[k].x>x1)x1=e.p[k].x;if(e.p[k].y<y0)y0=e.p[k].y;if(e.p[k].y>y1)y1=e.p[k].y;}}
    const double extent=std::hypot(x1-x0,y1-y0)>0?std::hypot(x1-x0,y1-y0):1,minHalf=0.007*extent;
    auto half=[&](double w){return w/2>minHalf?w/2:minHalf;};
    auto flip=[](std::vector<Pt>& q){for(size_t i=0;i<q.size();++i)q[i].y=-q[i].y;};
    auto add=[&](std::vector<Pt>& poly,int state){if(poly.size()<3)return;flip(poly);Prim p;p.kind=tvsTeeSketch::Polygon;p.pts=poly;p.width=state==Context?1:1.3;
        p.fill=state==Selected?0x1B6CA5:state==Changes?0xCFE4F8:state==TeeDialog?0xFFE9BF:0xF1F4F7;
        p.stroke=state==Selected?0x134E7D:state==Changes?0x1B6498:state==TeeDialog?0xC0811A:0xA7B7C9;out.push_back(p);};
    for(int pass=0;pass<4;++pass)for(size_t i=0;i<in.items.size();++i){const Element& e=in.items[i];   // context first, the edited duct last
        if(e.state!=(pass==0?Context:pass==1?Changes:pass==2?TeeDialog:Selected))continue;
        std::vector<Pt> c,poly;
        if(e.kind==2){
            // centre line: curve from p0 to p1 with the corner where the port directions meet
            const double d=e.dir[0].x*e.dir[1].y-e.dir[0].y*e.dir[1].x;Pt corner((e.p[0].x+e.p[1].x)/2,(e.p[0].y+e.p[1].y)/2);
            if(std::fabs(d)>1e-9){const double t=((e.p[1].x-e.p[0].x)*e.dir[1].y-(e.p[1].y-e.p[0].y)*e.dir[1].x)/d;corner=Pt(e.p[0].x+e.dir[0].x*t,e.p[0].y+e.dir[0].y*t);
                if(len(corner,e.p[0])>4*len(e.p[0],e.p[1]))corner=Pt((e.p[0].x+e.p[1].x)/2,(e.p[0].y+e.p[1].y)/2);}
            for(int j=0;j<=10;++j){const double t=j/10.0,u=1-t;c.push_back(Pt(u*u*e.p[0].x+2*u*t*corner.x+t*t*e.p[1].x,u*u*e.p[0].y+2*u*t*corner.y+t*t*e.p[1].y));}
            band(c,half(e.w[0]),half(e.w[1]>0?e.w[1]:e.w[0]),poly);add(poly,e.state);
        }else if(e.kind==4){
            // branch from the main axis to its port, then the main over it
            const double mx=e.p[1].x-e.p[0].x,my=e.p[1].y-e.p[0].y,l2=mx*mx+my*my;Pt foot=e.p[0];
            if(l2>1e-18){const double t=((e.p[2].x-e.p[0].x)*mx+(e.p[2].y-e.p[0].y)*my)/l2;foot=Pt(e.p[0].x+mx*t,e.p[0].y+my*t);}
            c.push_back(foot);c.push_back(e.p[2]);band(c,half(e.w[2]),half(e.w[2]),poly);add(poly,e.state);
            c.clear();c.push_back(e.p[0]);c.push_back(e.p[1]);band(c,half(e.w[0]),half(e.w[1]>0?e.w[1]:e.w[0]),poly);add(poly,e.state);
        }else{c.push_back(e.p[0]);c.push_back(e.p[1]);band(c,half(e.w[0]),half(e.w[1]>0?e.w[1]:e.w[0]),poly);add(poly,e.state);}
    }
    for(size_t i=0;i<in.marks.size();++i){const Mark& m=in.marks[i];
        Prim c;c.kind=tvsTeeSketch::Circle;c.pts.push_back(Pt(m.at.x,-m.at.y));c.r=3;c.fill=0xFFFFFF;c.stroke=0x1766A5;c.width=1.3;out.push_back(c);
        if(m.text.empty())continue;
        Prim t;t.kind=tvsTeeSketch::Text;t.fill=0x376B95;t.text=m.text;t.align=1;t.pts.push_back(Pt(m.at.x,-(m.at.y+(m.below?-0.075:0.075)*extent)));out.push_back(t);}
    // invisible frame: room for the labels
    Prim f;f.kind=tvsTeeSketch::Polyline;f.stroke=tvsTeeSketch::none;f.pts.push_back(Pt(x0-0.06*extent,-(y1+0.13*extent)));f.pts.push_back(Pt(x1+0.06*extent,-(y0-0.06*extent)));out.push_back(f);
}
}

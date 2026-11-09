#pragma once
// GDI renderer of the sketch primitives (geometry/TeeSketch.h) for the TVS dialogs: fits the
// bounding box into the frame (at most maxScale), dashes and markers in pixels, circles marked
// `scaled` in drawing units.
#include "../geometry/TeeSketch.h"
#include <cmath>
#include <vector>
namespace tvsSketchRender {
inline void draw(CDC* dc,const CRect& r,const std::vector<tvsTeeSketch::Prim>& p,CFont* normal,CFont* bold,double maxScale,double margin=10){
    dc->FillSolidRect(r,RGB(255,255,255));if(p.empty())return;
    double x0,y0,x1,y1;tvsTeeSketch::bounds(p,x0,y0,x1,y1);if(!(x1>x0)||!(y1>y0))return;
    const double fit=std::fmin((r.Width()-2*margin)/(x1-x0),(r.Height()-2*margin)/(y1-y0)),sc=fit>maxScale?maxScale:fit;
    const double ox=r.left+(r.Width()-(x1-x0)*sc)/2-x0*sc,oy=r.top+(r.Height()-(y1-y0)*sc)/2-y0*sc;
    auto P=[&](const tvsTeeSketch::Pt& q){return CPoint(static_cast<int>(ox+q.x*sc+0.5),static_cast<int>(oy+q.y*sc+0.5));};
    auto rgb=[](unsigned c){return RGB((c>>16)&255,(c>>8)&255,c&255);};
    const double px=sc>1?sc:1;   // line widths / markers grow with zoom-in only
    CFont* oldFont=dc->SelectObject(normal);dc->SetBkMode(TRANSPARENT);
    for(size_t i=0;i<p.size();++i){const tvsTeeSketch::Prim& q=p[i];
        if(q.kind==tvsTeeSketch::Text){CFont* f=dc->SelectObject(q.bold&&bold?bold:normal);dc->SetTextColor(rgb(q.fill));
            const CPoint a=P(q.pts[0]);const CSize s=dc->GetTextExtent(q.text.c_str());
            dc->TextOut(q.align==2?a.x-s.cx-4:q.align==1?a.x-s.cx/2:a.x,a.y-s.cy/2,q.text.c_str());dc->SelectObject(f);continue;}
        if(q.kind==tvsTeeSketch::Polyline&&q.stroke==tvsTeeSketch::none)continue;
        const int w=static_cast<int>(q.width*px+0.5);
        LOGBRUSH lb={BS_SOLID,rgb(q.stroke),0};DWORD dash[2]={static_cast<DWORD>(4*px+1),static_cast<DWORD>(4*px+1)};
        HPEN pen=q.stroke==tvsTeeSketch::none?static_cast<HPEN>(GetStockObject(NULL_PEN))
            :ExtCreatePen(PS_GEOMETRIC|(q.dash?PS_USERSTYLE:PS_SOLID)|PS_ENDCAP_FLAT|PS_JOIN_ROUND,w<1?1:w,&lb,q.dash?2:0,q.dash?dash:NULL);
        const bool filled=!(q.fill==tvsTeeSketch::none||q.kind==tvsTeeSketch::Polyline);
        HBRUSH brush=filled?CreateSolidBrush(rgb(q.fill)):static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        HGDIOBJ op=dc->SelectObject(pen),ob=dc->SelectObject(brush);
        std::vector<CPoint> pts;for(size_t j=0;j<q.pts.size();++j)pts.push_back(P(q.pts[j]));
        if(q.kind==tvsTeeSketch::Polygon&&pts.size()>2)dc->Polygon(&pts[0],static_cast<int>(pts.size()));
        else if(q.kind==tvsTeeSketch::Polyline&&pts.size()>1)dc->Polyline(&pts[0],static_cast<int>(pts.size()));
        else if(q.kind==tvsTeeSketch::Circle&&!pts.empty()){int rr=static_cast<int>((q.scaled?q.r*sc:q.r*(px>1.4?1.4:px))+0.5);if(rr<2)rr=2;
            dc->Ellipse(pts[0].x-rr,pts[0].y-rr,pts[0].x+rr+1,pts[0].y+rr+1);}
        dc->SelectObject(op);dc->SelectObject(ob);
        if(q.stroke!=tvsTeeSketch::none)DeleteObject(pen);if(filled)DeleteObject(brush);
    }
    dc->SelectObject(oldFont);
}
}
